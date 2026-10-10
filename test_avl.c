#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "avl.h"

static int g_tests_passed = 0;
static int g_tests_failed = 0;

#define TEST_ASSERT(expr, msg) do { \
    if (expr) { \
        g_tests_passed++; \
    } else { \
        g_tests_failed++; \
        printf("[FAIL] %s:%d: %s\n", __FILE__, __LINE__, msg); \
    } \
} while (0)

/* Helper to quickly create StudentData */
static StudentData make_student(int id, const char *name, const char *course) {
    StudentData s;
    s.student_id = id;
    strncpy(s.full_name, name, sizeof(s.full_name) - 1);
    s.full_name[sizeof(s.full_name) - 1] = '\0';
    strncpy(s.gender, "Unspecified", sizeof(s.gender) - 1);
    strncpy(s.dob, "2003-01-15", sizeof(s.dob) - 1);
    strncpy(s.address, "123 University Ave", sizeof(s.address) - 1);
    strncpy(s.contact, "student@univ.edu", sizeof(s.contact) - 1);
    strncpy(s.course, course, sizeof(s.course) - 1);
    s.course[sizeof(s.course) - 1] = '\0';
    return s;
}

/* Test 1: Single Right Rotation (LL Case) */
static void test_single_right_rotation(void) {
    printf("Running Test 1: Single Right Rotation (LL Case)...\n");
    AVLNode *root = NULL;
    AVLError status;
    char log[256] = {0};

    /* Insert 30, 20, 10 -> causes LL imbalance at 30, requiring single right rotation */
    StudentData s30 = make_student(30, "Student 30", "CS");
    StudentData s20 = make_student(20, "Student 20", "CS");
    StudentData s10 = make_student(10, "Student 10", "CS");

    root = avl_insert(root, &s30, &status, log, sizeof(log));
    TEST_ASSERT(status == AVL_OK, "Insert 30 should succeed");

    root = avl_insert(root, &s20, &status, log, sizeof(log));
    TEST_ASSERT(status == AVL_OK, "Insert 20 should succeed");

    root = avl_insert(root, &s10, &status, log, sizeof(log));
    TEST_ASSERT(status == AVL_OK, "Insert 10 should succeed");
    TEST_ASSERT(strstr(log, "Single Right") != NULL, "Log should mention Single Right Rotation");

    /* After right rotation: 20 becomes root, 10 is left child, 30 is right child */
    TEST_ASSERT(root != NULL && root->student_id == 20, "Root should be 20");
    TEST_ASSERT(root->left != NULL && root->left->student_id == 10, "Left child should be 10");
    TEST_ASSERT(root->right != NULL && root->right->student_id == 30, "Right child should be 30");
    TEST_ASSERT(root->height == 2, "Root height should be 2");
    TEST_ASSERT(avl_is_balanced(root), "Tree must be balanced");

    avl_free(root);
}

/* Test 2: Single Left Rotation (RR Case) */
static void test_single_left_rotation(void) {
    printf("Running Test 2: Single Left Rotation (RR Case)...\n");
    AVLNode *root = NULL;
    AVLError status;
    char log[256] = {0};

    /* Insert 10, 20, 30 -> causes RR imbalance at 10, requiring single left rotation */
    StudentData s10 = make_student(10, "Student 10", "CS");
    StudentData s20 = make_student(20, "Student 20", "CS");
    StudentData s30 = make_student(30, "Student 30", "CS");

    root = avl_insert(root, &s10, &status, log, sizeof(log));
    root = avl_insert(root, &s20, &status, log, sizeof(log));
    root = avl_insert(root, &s30, &status, log, sizeof(log));

    TEST_ASSERT(strstr(log, "Single Left") != NULL, "Log should mention Single Left Rotation");
    TEST_ASSERT(root != NULL && root->student_id == 20, "Root should be 20");
    TEST_ASSERT(root->left != NULL && root->left->student_id == 10, "Left child should be 10");
    TEST_ASSERT(root->right != NULL && root->right->student_id == 30, "Right child should be 30");
    TEST_ASSERT(avl_is_balanced(root), "Tree must be balanced");

    avl_free(root);
}

/* Test 3: Double Left-Right Rotation (LR Case) */
static void test_left_right_rotation(void) {
    printf("Running Test 3: Double Left-Right Rotation (LR Case)...\n");
    AVLNode *root = NULL;
    AVLError status;
    char log[256] = {0};

    /* Insert 30, 10, 20 -> LR imbalance at 30 */
    StudentData s30 = make_student(30, "Student 30", "CS");
    StudentData s10 = make_student(10, "Student 10", "CS");
    StudentData s20 = make_student(20, "Student 20", "CS");

    root = avl_insert(root, &s30, &status, log, sizeof(log));
    root = avl_insert(root, &s10, &status, log, sizeof(log));
    root = avl_insert(root, &s20, &status, log, sizeof(log));

    TEST_ASSERT(strstr(log, "Double Left-Right") != NULL, "Log should mention Double Left-Right Rotation");
    TEST_ASSERT(root != NULL && root->student_id == 20, "Root should be 20");
    TEST_ASSERT(root->left != NULL && root->left->student_id == 10, "Left child should be 10");
    TEST_ASSERT(root->right != NULL && root->right->student_id == 30, "Right child should be 30");
    TEST_ASSERT(avl_is_balanced(root), "Tree must be balanced");

    avl_free(root);
}

/* Test 4: Double Right-Left Rotation (RL Case) */
static void test_right_left_rotation(void) {
    printf("Running Test 4: Double Right-Left Rotation (RL Case)...\n");
    AVLNode *root = NULL;
    AVLError status;
    char log[256] = {0};

    /* Insert 10, 30, 20 -> RL imbalance at 10 */
    StudentData s10 = make_student(10, "Student 10", "CS");
    StudentData s30 = make_student(30, "Student 30", "CS");
    StudentData s20 = make_student(20, "Student 20", "CS");

    root = avl_insert(root, &s10, &status, log, sizeof(log));
    root = avl_insert(root, &s30, &status, log, sizeof(log));
    root = avl_insert(root, &s20, &status, log, sizeof(log));

    TEST_ASSERT(strstr(log, "Double Right-Left") != NULL, "Log should mention Double Right-Left Rotation");
    TEST_ASSERT(root != NULL && root->student_id == 20, "Root should be 20");
    TEST_ASSERT(root->left != NULL && root->left->student_id == 10, "Left child should be 10");
    TEST_ASSERT(root->right != NULL && root->right->student_id == 30, "Right child should be 30");
    TEST_ASSERT(avl_is_balanced(root), "Tree must be balanced");

    avl_free(root);
}

/* Test 5: Duplicate ID rejection */
static void test_duplicate_rejection(void) {
    printf("Running Test 5: Duplicate ID Rejection...\n");
    AVLNode *root = NULL;
    AVLError status;
    char log[256] = {0};

    StudentData s100 = make_student(100, "Alice Smith", "Physics");
    root = avl_insert(root, &s100, &status, log, sizeof(log));
    TEST_ASSERT(status == AVL_OK, "First insert of 100 should succeed");

    /* Attempt to insert duplicate ID 100 */
    StudentData s100_dup = make_student(100, "Bob Jones", "Chemistry");
    root = avl_insert(root, &s100_dup, &status, log, sizeof(log));
    TEST_ASSERT(status == AVL_ERR_DUPLICATE, "Duplicate insert should return AVL_ERR_DUPLICATE");
    TEST_ASSERT(avl_count(root) == 1, "Count should remain 1");

    avl_free(root);
}

/* Test 6: Search operation */
static void test_search(void) {
    printf("Running Test 6: Search Operation...\n");
    AVLNode *root = NULL;
    AVLError status;
    char log[256] = {0};

    int ids[] = {50, 20, 80, 10, 30, 70, 90};
    for (int i = 0; i < 7; i++) {
        StudentData s = make_student(ids[i], "Student", "Engineering");
        root = avl_insert(root, &s, &status, log, sizeof(log));
    }

    AVLNode *found = avl_search(root, 70);
    TEST_ASSERT(found != NULL && found->student_id == 70, "Search for 70 should find node");

    AVLNode *notFound = avl_search(root, 999);
    TEST_ASSERT(notFound == NULL, "Search for 999 should return NULL");

    avl_free(root);
}

/* Test 7: Deletion with leaf, 1 child, and 2 children with rebalancing */
static void test_deletion(void) {
    printf("Running Test 7: Deletion & Rebalancing...\n");
    AVLNode *root = NULL;
    AVLError status;
    char log[256] = {0};

    /* Build a tree with 7 nodes */
    int ids[] = {10, 20, 30, 40, 50, 25};
    for (int i = 0; i < 6; i++) {
        StudentData s = make_student(ids[i], "Test Student", "CS");
        root = avl_insert(root, &s, &status, log, sizeof(log));
        TEST_ASSERT(avl_is_balanced(root), "Tree must be balanced after each insert");
    }

    /* Delete non-existent node */
    root = avl_delete(root, 999, &status, log, sizeof(log));
    TEST_ASSERT(status == AVL_ERR_NOT_FOUND, "Deleting 999 should return AVL_ERR_NOT_FOUND");

    /* Delete a node with two children or internal node */
    root = avl_delete(root, 30, &status, log, sizeof(log));
    TEST_ASSERT(status == AVL_OK, "Deleting 30 should succeed");
    TEST_ASSERT(avl_search(root, 30) == NULL, "30 should no longer exist");
    TEST_ASSERT(avl_is_balanced(root), "Tree must be balanced after delete");

    /* Delete leaf node */
    root = avl_delete(root, 10, &status, log, sizeof(log));
    TEST_ASSERT(status == AVL_OK, "Deleting 10 should succeed");
    TEST_ASSERT(avl_is_balanced(root), "Tree must be balanced after delete");

    avl_free(root);
}

/* In-order validation accumulator */
typedef struct {
    int last_id;
    bool is_sorted;
    int count;
} InOrderCheck;

static void check_inorder_sorted(const AVLNode *node, void *user_data) {
    InOrderCheck *chk = (InOrderCheck*)user_data;
    if (chk->count > 0 && node->student_id <= chk->last_id) {
        chk->is_sorted = false;
    }
    chk->last_id = node->student_id;
    chk->count++;
}

/* Test 8: Traversals & In-order strictly ascending property */
static void test_traversals(void) {
    printf("Running Test 8: Traversals & In-Order Ascending Check...\n");
    AVLNode *root = NULL;
    AVLError status;
    char log[256] = {0};

    int ids[] = {45, 12, 89, 34, 67, 23, 78, 56, 91, 5};
    int n = sizeof(ids) / sizeof(ids[0]);

    for (int i = 0; i < n; i++) {
        StudentData s = make_student(ids[i], "Student", "Math");
        root = avl_insert(root, &s, &status, log, sizeof(log));
    }

    TEST_ASSERT(avl_count(root) == n, "Total count should match inserted elements");
    TEST_ASSERT(avl_is_balanced(root), "Tree must remain balanced");

    InOrderCheck chk = {0, true, 0};
    avl_inorder(root, check_inorder_sorted, &chk);
    TEST_ASSERT(chk.is_sorted, "In-Order traversal must be strictly ascending");
    TEST_ASSERT(chk.count == n, "In-Order visit count must equal n");

    /* Test JSON traversal array export */
    char json_buf[4096];
    avl_traversal_to_json(root, TRAVERSAL_INORDER, json_buf, sizeof(json_buf));
    TEST_ASSERT(json_buf[0] == '[' && json_buf[strlen(json_buf) - 1] == ']', "JSON array should start with [ and end with ]");
    TEST_ASSERT(strstr(json_buf, "\"student_id\":5") != NULL, "JSON should contain student 5");

    /* Test tree hierarchy JSON export */
    char tree_json[8192];
    avl_tree_to_json(root, tree_json, sizeof(tree_json));
    TEST_ASSERT(tree_json[0] == '{', "Tree JSON should start with {");
    TEST_ASSERT(strstr(tree_json, "\"balance_factor\":") != NULL, "Tree JSON should include balance factors");

    avl_free(root);
}

int main(void) {
    printf("=========================================\n");
    printf(" AVL Tree Backend C Automated Unit Tests \n");
    printf("=========================================\n\n");

    test_single_right_rotation();
    test_single_left_rotation();
    test_left_right_rotation();
    test_right_left_rotation();
    test_duplicate_rejection();
    test_search();
    test_deletion();
    test_traversals();

    printf("\n-----------------------------------------\n");
    printf("Test Results: %d Passed, %d Failed\n", g_tests_passed, g_tests_failed);
    printf("-----------------------------------------\n");

    if (g_tests_failed == 0) {
        printf("[SUCCESS] All AVL Tree Unit Tests Passed Perfectly!\n");
        return 0;
    } else {
        printf("[FAILURE] Some tests failed.\n");
        return 1;
    }
}
