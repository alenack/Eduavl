/**
 * EduAVL - Client Application Controller
 * Connects modern Web UI with the C-powered AVL Tree Backend
 */

// Global State
let currentStudents = [];
let currentTraversal = 'inorder';

// DOM Elements
const statCount = document.getElementById('stat-count');
const statHeight = document.getElementById('stat-height');
const statBalance = document.getElementById('stat-balance');
const rotationMsg = document.getElementById('rotation-msg');
const searchForm = document.getElementById('search-form');
const searchInput = document.getElementById('search-id-input');
const btnSearchClear = document.getElementById('btn-search-clear');
const searchResultContainer = document.getElementById('search-result-container');
const studentForm = document.getElementById('student-form');
const formSpinner = document.getElementById('form-spinner');
const btnSubmitStudent = document.getElementById('btn-submit-student');
const studentTableBody = document.getElementById('student-table-body');
const tableFilter = document.getElementById('table-filter');
const treeSvg = document.getElementById('tree-svg');
const treeEmptyState = document.getElementById('tree-empty-state');
const btnSeed = document.getElementById('btn-seed');
const btnClear = document.getElementById('btn-clear');
const traversalChips = document.getElementById('traversal-chips');
const traversalInfo = document.getElementById('traversal-info');
const traversalTabs = document.querySelectorAll('.tab-btn');
const toastContainer = document.getElementById('toast-container');

// Initialize App
document.addEventListener('DOMContentLoaded', () => {
  setupEventListeners();
  refreshAll();
});

// Setup UI Event Listeners
function setupEventListeners() {
  // Search Form Submit
  searchForm.addEventListener('submit', async (e) => {
    e.preventDefault();
    const id = parseInt(searchInput.value.trim(), 10);
    if (!id || id <= 0) return;
    await lookupStudent(id);
  });

  // Search input input event for clear button visibility
  searchInput.addEventListener('input', () => {
    btnSearchClear.style.display = searchInput.value ? 'block' : 'none';
  });

  btnSearchClear.addEventListener('click', () => {
    searchInput.value = '';
    btnSearchClear.style.display = 'none';
    searchResultContainer.innerHTML = '';
    searchInput.focus();
  });

  // Student Registry Form Submit
  studentForm.addEventListener('submit', async (e) => {
    e.preventDefault();
    await handleRegisterStudent();
  });

  // Table Filter Input
  tableFilter.addEventListener('input', (e) => {
    filterStudentTable(e.target.value);
  });

  // Traversal Tabs
  traversalTabs.forEach((tab) => {
    tab.addEventListener('click', () => {
      traversalTabs.forEach((t) => t.classList.remove('active'));
      tab.classList.add('active');
      currentTraversal = tab.dataset.traversal;
      updateTraversalInfoText(currentTraversal);
      loadTraversal(currentTraversal);
    });
  });

  // Action Buttons
  btnSeed.addEventListener('click', handleSeedData);
  btnClear.addEventListener('click', handleClearTree);
}

// Master Refresh Function
async function refreshAll() {
  await Promise.all([
    loadStats(),
    loadStudents(),
    loadTreeVisualization(),
    loadTraversal(currentTraversal)
  ]);
}

// Show Toast Alert
function showToast(message, type = 'success') {
  const toast = document.createElement('div');
  toast.className = `toast toast-${type}`;
  toast.innerHTML = `
    <span>${message}</span>
  `;
  toastContainer.appendChild(toast);

  setTimeout(() => {
    toast.style.opacity = '0';
    toast.style.transform = 'translateX(100%)';
    toast.style.transition = 'all 0.3s ease';
    setTimeout(() => toast.remove(), 300);
  }, 4000);
}

// 1. Fetch & Render Stats
async function loadStats() {
  try {
    const res = await fetch('/api/stats');
    if (!res.ok) return;
    const data = await res.json();

    statCount.textContent = data.node_count;
    statHeight.textContent = data.height;

    if (data.is_balanced) {
      statBalance.textContent = `Balanced (BF: ${data.balance_factor})`;
      statBalance.className = 'metric-val badge-balanced';
    } else {
      statBalance.textContent = `Unbalanced (BF: ${data.balance_factor})`;
      statBalance.className = 'metric-val badge-unbalanced';
    }

    if (data.last_rotation) {
      rotationMsg.textContent = data.last_rotation;
    }
  } catch (err) {
    console.error('Failed to load stats:', err);
  }
}

// 2. Fetch & Render In-Order Student Table
async function loadStudents() {
  try {
    const res = await fetch('/api/students');
    if (!res.ok) return;
    const students = await res.json();
    currentStudents = students;
    renderStudentTable(students);
  } catch (err) {
    console.error('Failed to load students:', err);
  }
}

function renderStudentTable(students) {
  studentTableBody.innerHTML = '';

  if (!students || students.length === 0) {
    studentTableBody.innerHTML = `
      <tr>
        <td colspan="7" style="text-align: center; padding: 2.5rem; color: var(--text-muted);">
          No student records found in the AVL tree. Use the form on the left or click "Seed Demo" to start.
        </td>
      </tr>
    `;
    return;
  }

  students.forEach((student) => {
    const tr = document.createElement('tr');
    tr.innerHTML = `
      <td><strong>#${student.student_id}</strong></td>
      <td><strong>${escapeHtml(student.full_name)}</strong></td>
      <td><span class="badge-course">${escapeHtml(student.course || 'General')}</span></td>
      <td>${escapeHtml(student.gender || '-')}</td>
      <td>${escapeHtml(student.contact || '-')}</td>
      <td>${escapeHtml(student.dob || '-')}</td>
      <td class="text-right">
        <button class="table-btn table-btn-delete" onclick="handleDeleteStudent(${student.student_id})">Delete</button>
      </td>
    `;
    studentTableBody.appendChild(tr);
  });
}

function filterStudentTable(query) {
  const q = query.toLowerCase().trim();
  if (!q) {
    renderStudentTable(currentStudents);
    return;
  }
  const filtered = currentStudents.filter((s) => {
    return (
      String(s.student_id).includes(q) ||
      (s.full_name && s.full_name.toLowerCase().includes(q)) ||
      (s.course && s.course.toLowerCase().includes(q))
    );
  });
  renderStudentTable(filtered);
}

// 3. Search / Lookup Student by ID
async function lookupStudent(studentId) {
  try {
    searchInput.value = studentId;
    btnSearchClear.style.display = 'block';

    const res = await fetch(`/api/students/${studentId}`);
    if (res.status === 404) {
      const err = await res.json();
      searchResultContainer.innerHTML = `
        <div class="candidate-card" style="border-left-color: var(--color-rose);">
          <p style="color: var(--color-rose); font-weight: 500;">${err.error || 'Student not found in AVL tree.'}</p>
        </div>
      `;
      return;
    }

    if (!res.ok) {
      showToast('Error searching student record.', 'error');
      return;
    }

    const s = await res.json();
    renderCandidateCard(s);
  } catch (err) {
    console.error('Error during search:', err);
    showToast('Failed to connect to AVL backend server.', 'error');
  }
}

function renderCandidateCard(student) {
  const initials = student.full_name
    ? student.full_name
        .split(' ')
        .map((n) => n[0])
        .join('')
        .substring(0, 2)
        .toUpperCase()
    : 'ST';

  searchResultContainer.innerHTML = `
    <div class="candidate-card">
      <div class="candidate-header">
        <div class="candidate-avatar-wrap">
          <div class="avatar-circle">${initials}</div>
          <div class="candidate-identity">
            <h3>${escapeHtml(student.full_name)} <span class="id-badge">ID #${student.student_id}</span></h3>
            <p>${escapeHtml(student.course || 'Student')}</p>
          </div>
        </div>
        <div>
          <button class="btn btn-outline-danger btn-sm" onclick="handleDeleteStudent(${student.student_id})">
            Delete Record
          </button>
        </div>
      </div>

      <div class="candidate-meta-grid">
        <div class="meta-item">
          <span class="meta-key">Gender</span>
          <span class="meta-val">${escapeHtml(student.gender || 'Not Specified')}</span>
        </div>
        <div class="meta-item">
          <span class="meta-key">Date of Birth</span>
          <span class="meta-val">${escapeHtml(student.dob || 'Not Recorded')}</span>
        </div>
        <div class="meta-item">
          <span class="meta-key">Contact / Email</span>
          <span class="meta-val">${escapeHtml(student.contact || 'None')}</span>
        </div>
        <div class="meta-item">
          <span class="meta-key">Tree Height</span>
          <span class="meta-val">${student.height}</span>
        </div>
        <div class="meta-item">
          <span class="meta-key">Balance Factor (BF)</span>
          <span class="meta-val" style="color: ${Math.abs(student.balance_factor) <= 1 ? 'var(--color-emerald)' : 'var(--color-rose)'}">
            ${student.balance_factor} (${student.balance_factor === 0 ? 'Equally Balanced' : student.balance_factor > 0 ? 'Left-Leaning' : 'Right-Leaning'})
          </span>
        </div>
        <div class="meta-item" style="grid-column: 1 / -1;">
          <span class="meta-key">Address</span>
          <span class="meta-val">${escapeHtml(student.address || 'Not Provided')}</span>
        </div>
      </div>
    </div>
  `;
}

// 4. Register Student (Insert into AVL Tree)
async function handleRegisterStudent() {
  const id = parseInt(document.getElementById('student-id').value.trim(), 10);
  const fullName = document.getElementById('full-name').value.trim();
  const gender = document.getElementById('gender').value;
  const dob = document.getElementById('dob').value;
  const course = document.getElementById('course').value;
  const contact = document.getElementById('contact').value.trim();
  const address = document.getElementById('address').value.trim();

  if (!id || id <= 0) {
    showToast('Please enter a valid positive Student ID.', 'warning');
    return;
  }
  if (!fullName) {
    showToast('Please enter student full name.', 'warning');
    return;
  }

  const payload = {
    student_id: id,
    full_name: fullName,
    gender: gender,
    dob: dob,
    course: course,
    contact: contact,
    address: address
  };

  btnSubmitStudent.disabled = true;
  formSpinner.style.display = 'inline-block';

  try {
    const res = await fetch('/api/students', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload)
    });

    const data = await res.json();

    if (res.status === 201) {
      showToast(`Student #${id} registered successfully!`, 'success');
      studentForm.reset();
      await refreshAll();
      lookupStudent(id);
    } else if (res.status === 409) {
      showToast(data.error || 'Duplicate Student ID! Record already exists in AVL tree.', 'error');
    } else {
      showToast(data.error || 'Failed to register student.', 'error');
    }
  } catch (err) {
    console.error('Error registering student:', err);
    showToast('Network error while registering student.', 'error');
  } finally {
    btnSubmitStudent.disabled = false;
    formSpinner.style.display = 'none';
  }
}

// 5. Delete Student
async function handleDeleteStudent(studentId) {
  if (!confirm(`Are you sure you want to delete student record #${studentId}? The AVL Tree will automatically rebalance.`)) {
    return;
  }

  try {
    const res = await fetch(`/api/students/${studentId}`, {
      method: 'DELETE'
    });

    const data = await res.json();
    if (res.ok) {
      showToast(`Record #${studentId} deleted. ${data.rotation || ''}`, 'success');
      searchResultContainer.innerHTML = '';
      if (searchInput.value == studentId) {
        searchInput.value = '';
        btnSearchClear.style.display = 'none';
      }
      await refreshAll();
    } else {
      showToast(data.error || 'Failed to delete student.', 'error');
    }
  } catch (err) {
    console.error('Error deleting student:', err);
    showToast('Failed to delete student record.', 'error');
  }
}

// 6. Tree Visualization (Dynamic SVG Node Map)
async function loadTreeVisualization() {
  try {
    const res = await fetch('/api/tree');
    if (!res.ok) return;
    const tree = await res.json();

    if (!tree || !tree.student_id) {
      treeSvg.innerHTML = '';
      treeEmptyState.style.display = 'flex';
      return;
    }

    treeEmptyState.style.display = 'none';
    renderSvgTree(tree);
  } catch (err) {
    console.error('Failed to load tree:', err);
  }
}

function renderSvgTree(root) {
  treeSvg.innerHTML = '';

  // Calculate layout bounds and node positions recursively
  const depth = getTreeDepth(root);
  const svgWidth = Math.max(860, Math.pow(2, depth) * 60);
  const svgHeight = Math.max(380, (depth + 1) * 90);

  treeSvg.setAttribute('viewBox', `0 0 ${svgWidth} ${svgHeight}`);
  treeSvg.setAttribute('preserveAspectRatio', 'xMidYMid meet');
  treeSvg.style.width = '100%';
  treeSvg.style.maxWidth = '100%';
  treeSvg.style.height = 'auto';

  const nodeRadius = 24;
  const levelHeight = 80;

  // First pass: assign coordinates
  function assignCoordinates(node, leftBound, rightBound, level) {
    if (!node) return;
    node.x = (leftBound + rightBound) / 2;
    node.y = 45 + level * levelHeight;

    assignCoordinates(node.left, leftBound, node.x, level + 1);
    assignCoordinates(node.right, node.x, rightBound, level + 1);
  }

  assignCoordinates(root, 20, svgWidth - 20, 0);

  // Second pass: Draw branch connector lines
  function drawBranches(node) {
    if (!node) return;

    if (node.left) {
      drawBranchLine(node.x, node.y, node.left.x, node.left.y);
      drawBranches(node.left);
    }
    if (node.right) {
      drawBranchLine(node.x, node.y, node.right.x, node.right.y);
      drawBranches(node.right);
    }
  }

  function drawBranchLine(x1, y1, x2, y2) {
    const line = document.createElementNS('http://www.w3.org/2000/svg', 'line');
    line.setAttribute('x1', x1);
    line.setAttribute('y1', y1);
    line.setAttribute('x2', x2);
    line.setAttribute('y2', y2);
    line.setAttribute('class', 'tree-branch');
    treeSvg.appendChild(line);
  }

  drawBranches(root);

  // Third pass: Draw node groups (circles, text, badges)
  function drawNodes(node) {
    if (!node) return;

    const g = document.createElementNS('http://www.w3.org/2000/svg', 'g');
    g.setAttribute('class', 'tree-node-group');
    g.setAttribute('transform', `translate(${node.x}, ${node.y})`);
    g.setAttribute('title', `Student ID: ${node.student_id}, Name: ${node.full_name}, BF: ${node.balance_factor}`);

    // Click on node triggers lookup
    g.addEventListener('click', () => {
      lookupStudent(node.student_id);
    });

    // Node circle
    const circle = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
    circle.setAttribute('r', nodeRadius);
    const isBalanced = node.balance_factor === 0;
    circle.setAttribute('class', `tree-node-circle ${isBalanced ? 'balanced' : 'mild'}`);
    g.appendChild(circle);

    // Node ID text
    const textId = document.createElementNS('http://www.w3.org/2000/svg', 'text');
    textId.setAttribute('class', 'node-text-id');
    textId.setAttribute('y', 0);
    textId.textContent = node.student_id;
    g.appendChild(textId);

    // Height & Balance Factor pill badge above circle
    const bfBadge = document.createElementNS('http://www.w3.org/2000/svg', 'text');
    bfBadge.setAttribute('class', 'node-badge-bf');
    bfBadge.setAttribute('y', -nodeRadius - 6);
    bfBadge.setAttribute('fill', isBalanced ? '#059669' : '#d97706');
    bfBadge.textContent = `h:${node.height} | BF:${node.balance_factor}`;
    g.appendChild(bfBadge);

    // Student First Name label below circle
    const textName = document.createElementNS('http://www.w3.org/2000/svg', 'text');
    textName.setAttribute('class', 'node-text-name');
    textName.setAttribute('y', nodeRadius + 14);
    const firstName = node.full_name ? node.full_name.split(' ')[0] : '';
    textName.textContent = firstName.length > 10 ? firstName.substring(0, 9) + '…' : firstName;
    g.appendChild(textName);

    treeSvg.appendChild(g);

    drawNodes(node.left);
    drawNodes(node.right);
  }

  drawNodes(root);
}

function getTreeDepth(node) {
  if (!node) return 0;
  return 1 + Math.max(getTreeDepth(node.left), getTreeDepth(node.right));
}

// 7. Traversals Explorer
async function loadTraversal(type) {
  try {
    const res = await fetch(`/api/traversal?type=${type}`);
    if (!res.ok) return;
    const list = await res.json();
    renderTraversalChips(list);
  } catch (err) {
    console.error('Failed to load traversal:', err);
  }
}

function updateTraversalInfoText(type) {
  if (type === 'inorder') {
    traversalInfo.innerHTML = `<strong>In-Order Traversal (L &rarr; Root &rarr; R):</strong> Visits nodes in strictly ascending numerical order of Student ID.`;
  } else if (type === 'preorder') {
    traversalInfo.innerHTML = `<strong>Pre-Order Traversal (Root &rarr; L &rarr; R):</strong> Visits parent root before subtrees. Useful for serializing and visualizing tree hierarchy.`;
  } else {
    traversalInfo.innerHTML = `<strong>Post-Order Traversal (L &rarr; R &rarr; Root):</strong> Visits children before root. Ideal for safe bottom-up memory deallocation and cleanup.`;
  }
}

function renderTraversalChips(list) {
  traversalChips.innerHTML = '';
  if (!list || list.length === 0) {
    traversalChips.innerHTML = `<span style="color: var(--text-muted); font-size: 0.85rem;">Tree is empty.</span>`;
    return;
  }

  list.forEach((node, idx) => {
    const chip = document.createElement('div');
    chip.className = 'traversal-chip';
    chip.innerHTML = `
      <span class="chip-step">${idx + 1}</span>
      <span class="chip-id">#${node.student_id}</span>
      <span class="chip-name">${escapeHtml(node.full_name)}</span>
    `;
    chip.addEventListener('click', () => lookupStudent(node.student_id));
    traversalChips.appendChild(chip);
  });
}

// 8. Seed Demo & Clear Tree Handlers
async function handleSeedData() {
  btnSeed.disabled = true;
  try {
    const res = await fetch('/api/seed', { method: 'POST' });
    if (res.ok) {
      showToast('Seeded realistic demo student records!', 'success');
      await refreshAll();
    }
  } catch (err) {
    showToast('Failed to seed records.', 'error');
  } finally {
    btnSeed.disabled = false;
  }
}

async function handleClearTree() {
  if (!confirm('Are you sure you want to clear all records from the AVL Tree? Memory will be deallocated using Post-Order traversal.')) {
    return;
  }

  btnClear.disabled = true;
  try {
    const res = await fetch('/api/clear', { method: 'POST' });
    if (res.ok) {
      showToast('All student records cleared from AVL tree.', 'success');
      searchResultContainer.innerHTML = '';
      await refreshAll();
    }
  } catch (err) {
    showToast('Failed to clear tree.', 'error');
  } finally {
    btnClear.disabled = false;
  }
}

// Utility: Escape HTML for security
function escapeHtml(str) {
  if (!str) return '';
  return str
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;')
    .replace(/'/g, '&#039;');
}
