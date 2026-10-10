#include "avl.h"

/* Helper to get max of two integers */
int avl_max(int a, int b) {
    return (a > b) ? a : b;
}

/* Get height of a node (0 for NULL) */
int avl_height(const AVLNode *node) {
    if (node == NULL) return 0;
    return node->height;
}

/* Calculate balance factor (Height(Left) - Height(Right)) */
int avl_balance_factor(const AVLNode *node) {
    if (node == NULL) return 0;
    return avl_height(node->left) - avl_height(node->right);
}

/* Safe string copy helper */
static void safe_copy(char *dest, const char *src, size_t max_len) {
    if (!dest || max_len == 0) return;
    dest[0] = '\0';
    if (!src) return;
    size_t i = 0;
    while (src[i] != '\0' && i < max_len - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

/* Allocate and initialize a new AVL tree node */
AVLNode* avl_create_node(const StudentData *data) {
    if (data == NULL) return NULL;

    AVLNode *node = (AVLNode*)malloc(sizeof(AVLNode));
    if (node == NULL) return NULL;

    node->student_id = data->student_id;
    safe_copy(node->full_name, data->full_name, sizeof(node->full_name));
    safe_copy(node->gender, data->gender, sizeof(node->gender));
    safe_copy(node->dob, data->dob, sizeof(node->dob));
    safe_copy(node->address, data->address, sizeof(node->address));
    safe_copy(node->contact, data->contact, sizeof(node->contact));
    safe_copy(node->course, data->course, sizeof(node->course));
    node->height = 1; /* New node is initially at height 1 (leaf) */
    node->left = NULL;
    node->right = NULL;

    return node;
}

/* Single Right Rotation (Clockwise) - LL Case */
static AVLNode* avl_rotate_right(AVLNode *y) {
    AVLNode *x = y->left;
    AVLNode *T2 = x->right;

    /* Perform rotation */
    x->right = y;
    y->left = T2;

    /* Update heights */
    y->height = 1 + avl_max(avl_height(y->left), avl_height(y->right));
    x->height = 1 + avl_max(avl_height(x->left), avl_height(x->right));

    return x;
}

/* Single Left Rotation (Counter-Clockwise) - RR Case */
static AVLNode* avl_rotate_left(AVLNode *x) {
    AVLNode *y = x->right;
    AVLNode *T2 = y->left;

    /* Perform rotation */
    y->left = x;
    x->right = T2;

    /* Update heights */
    x->height = 1 + avl_max(avl_height(x->left), avl_height(x->right));
    y->height = 1 + avl_max(avl_height(y->left), avl_height(y->right));

    return y;
}

/* Recursive insertion with AVL balancing and rotation tracking */
AVLNode* avl_insert(AVLNode *root, const StudentData *data, AVLError *status, char *rotation_log, size_t log_size) {
    if (data == NULL) {
        if (status) *status = AVL_ERR_INVALID;
        return root;
    }

    /* 1. Perform standard BST insertion */
    if (root == NULL) {
        AVLNode *new_node = avl_create_node(data);
        if (new_node == NULL) {
            if (status) *status = AVL_ERR_MEMORY;
            return NULL;
        }
        if (status) *status = AVL_OK;
        return new_node;
    }

    if (data->student_id < root->student_id) {
        root->left = avl_insert(root->left, data, status, rotation_log, log_size);
    } else if (data->student_id > root->student_id) {
        root->right = avl_insert(root->right, data, status, rotation_log, log_size);
    } else {
        /* Duplicate ID not allowed in AVL tree */
        if (status) *status = AVL_ERR_DUPLICATE;
        return root;
    }

    /* If child insert failed due to duplicate or memory, pass through */
    if (status && *status != AVL_OK) {
        return root;
    }

    /* 2. Update height of current ancestor node */
    root->height = 1 + avl_max(avl_height(root->left), avl_height(root->right));

    /* 3. Get the balance factor of this ancestor node */
    int balance = avl_balance_factor(root);

    /* 4. If this node becomes unbalanced, there are 4 cases: */

    /* Left-Left (LL) Case -> Single Right Rotation */
    if (balance > 1 && data->student_id < root->left->student_id) {
        if (rotation_log && log_size > 0) {
            snprintf(rotation_log, log_size, "Single Right (R / LL) Rotation performed at Node ID #%d (BF=%d)", root->student_id, balance);
        }
        return avl_rotate_right(root);
    }

    /* Right-Right (RR) Case -> Single Left Rotation */
    if (balance < -1 && data->student_id > root->right->student_id) {
        if (rotation_log && log_size > 0) {
            snprintf(rotation_log, log_size, "Single Left (L / RR) Rotation performed at Node ID #%d (BF=%d)", root->student_id, balance);
        }
        return avl_rotate_left(root);
    }

    /* Left-Right (LR) Case -> Double Rotation (Left on left child, then Right on root) */
    if (balance > 1 && data->student_id > root->left->student_id) {
        if (rotation_log && log_size > 0) {
            snprintf(rotation_log, log_size, "Double Left-Right (LR) Rotation performed at Node ID #%d (BF=%d)", root->student_id, balance);
        }
        root->left = avl_rotate_left(root->left);
        return avl_rotate_right(root);
    }

    /* Right-Left (RL) Case -> Double Rotation (Right on right child, then Left on root) */
    if (balance < -1 && data->student_id < root->right->student_id) {
        if (rotation_log && log_size > 0) {
            snprintf(rotation_log, log_size, "Double Right-Left (RL) Rotation performed at Node ID #%d (BF=%d)", root->student_id, balance);
        }
        root->right = avl_rotate_right(root->right);
        return avl_rotate_left(root);
    }

    /* Node is balanced */
    return root;
}

/* Helper to find node with minimum key value (in-order successor) */
static AVLNode* avl_min_value_node(AVLNode *node) {
    AVLNode *curr = node;
    while (curr && curr->left != NULL) {
        curr = curr->left;
    }
    return curr;
}

/* Copy student data fields between nodes */
static void avl_copy_student_data(AVLNode *dest, const AVLNode *src) {
    dest->student_id = src->student_id;
    safe_copy(dest->full_name, src->full_name, sizeof(dest->full_name));
    safe_copy(dest->gender, src->gender, sizeof(dest->gender));
    safe_copy(dest->dob, src->dob, sizeof(dest->dob));
    safe_copy(dest->address, src->address, sizeof(dest->address));
    safe_copy(dest->contact, src->contact, sizeof(dest->contact));
    safe_copy(dest->course, src->course, sizeof(dest->course));
}

/* Recursive deletion with AVL balancing */
AVLNode* avl_delete(AVLNode *root, int student_id, AVLError *status, char *rotation_log, size_t log_size) {
    if (root == NULL) {
        if (status) *status = AVL_ERR_NOT_FOUND;
        return NULL;
    }

    /* 1. Perform standard BST deletion */
    if (student_id < root->student_id) {
        root->left = avl_delete(root->left, student_id, status, rotation_log, log_size);
    } else if (student_id > root->student_id) {
        root->right = avl_delete(root->right, student_id, status, rotation_log, log_size);
    } else {
        /* Node with matching ID found */
        if (status) *status = AVL_OK;

        /* Node with only one child or no child */
        if (root->left == NULL || root->right == NULL) {
            AVLNode *temp = root->left ? root->left : root->right;

            if (temp == NULL) {
                /* No child case (Leaf node) */
                temp = root;
                root = NULL;
            } else {
                /* One child case */
                *root = *temp; /* Copy child contents */
            }
            free(temp);
        } else {
            /* Node with two children: get in-order successor (min in right subtree) */
            AVLNode *successor = avl_min_value_node(root->right);

            /* Copy successor's student data to this node */
            avl_copy_student_data(root, successor);

            /* Delete the successor node */
            AVLError sub_status = AVL_OK;
            root->right = avl_delete(root->right, successor->student_id, &sub_status, rotation_log, log_size);
        }
    }

    /* If the tree had only one node and it was deleted */
    if (root == NULL) {
        return NULL;
    }

    /* 2. Update height of current node */
    root->height = 1 + avl_max(avl_height(root->left), avl_height(root->right));

    /* 3. Get balance factor */
    int balance = avl_balance_factor(root);

    /* 4. Rebalance if unbalanced */

    /* Left-Left Case */
    if (balance > 1 && avl_balance_factor(root->left) >= 0) {
        if (rotation_log && log_size > 0) {
            snprintf(rotation_log, log_size, "Single Right (R / LL) Rotation on Delete at Node ID #%d (BF=%d)", root->student_id, balance);
        }
        return avl_rotate_right(root);
    }

    /* Left-Right Case */
    if (balance > 1 && avl_balance_factor(root->left) < 0) {
        if (rotation_log && log_size > 0) {
            snprintf(rotation_log, log_size, "Double Left-Right (LR) Rotation on Delete at Node ID #%d (BF=%d)", root->student_id, balance);
        }
        root->left = avl_rotate_left(root->left);
        return avl_rotate_right(root);
    }

    /* Right-Right Case */
    if (balance < -1 && avl_balance_factor(root->right) <= 0) {
        if (rotation_log && log_size > 0) {
            snprintf(rotation_log, log_size, "Single Left (L / RR) Rotation on Delete at Node ID #%d (BF=%d)", root->student_id, balance);
        }
        return avl_rotate_left(root);
    }

    /* Right-Left Case */
    if (balance < -1 && avl_balance_factor(root->right) > 0) {
        if (rotation_log && log_size > 0) {
            snprintf(rotation_log, log_size, "Double Right-Left (RL) Rotation on Delete at Node ID #%d (BF=%d)", root->student_id, balance);
        }
        root->right = avl_rotate_right(root->right);
        return avl_rotate_left(root);
    }

    return root;
}

/* Search for student by ID (O(log n)) */
AVLNode* avl_search(AVLNode *root, int student_id) {
    if (root == NULL || root->student_id == student_id) {
        return root;
    }
    if (student_id < root->student_id) {
        return avl_search(root->left, student_id);
    }
    return avl_search(root->right, student_id);
}

/* Count total nodes in AVL tree */
int avl_count(const AVLNode *root) {
    if (root == NULL) return 0;
    return 1 + avl_count(root->left) + avl_count(root->right);
}

/* Verify if AVL balancing property is maintained across the tree */
bool avl_is_balanced(const AVLNode *root) {
    if (root == NULL) return true;

    int bf = avl_balance_factor(root);
    if (bf < -1 || bf > 1) return false;

    return avl_is_balanced(root->left) && avl_is_balanced(root->right);
}

/* In-Order Traversal (Sorted ascending by Student ID) */
void avl_inorder(const AVLNode *root, AVLVisitCallback callback, void *user_data) {
    if (root == NULL || callback == NULL) return;
    avl_inorder(root->left, callback, user_data);
    callback(root, user_data);
    avl_inorder(root->right, callback, user_data);
}

/* Pre-Order Traversal (Root, Left, Right - Hierarchical View) */
void avl_preorder(const AVLNode *root, AVLVisitCallback callback, void *user_data) {
    if (root == NULL || callback == NULL) return;
    callback(root, user_data);
    avl_preorder(root->left, callback, user_data);
    avl_preorder(root->right, callback, user_data);
}

/* Post-Order Traversal (Left, Right, Root) */
void avl_postorder(const AVLNode *root, AVLVisitCallback callback, void *user_data) {
    if (root == NULL || callback == NULL) return;
    avl_postorder(root->left, callback, user_data);
    avl_postorder(root->right, callback, user_data);
    callback(root, user_data);
}

/* Free all nodes in tree using post-order cleanup */
void avl_free(AVLNode *root) {
    if (root == NULL) return;
    avl_free(root->left);
    avl_free(root->right);
    free(root);
}

/* JSON string escape helper */
static void json_escape(const char *src, char *dest, size_t max_len) {
    if (!src || !dest || max_len == 0) return;
    size_t d = 0;
    for (size_t s = 0; src[s] != '\0' && d + 2 < max_len; s++) {
        if (src[s] == '"' || src[s] == '\\') {
            dest[d++] = '\\';
            dest[d++] = src[s];
        } else if (src[s] == '\n') {
            dest[d++] = '\\';
            dest[d++] = 'n';
        } else if (src[s] == '\r') {
            dest[d++] = '\\';
            dest[d++] = 'r';
        } else if (src[s] == '\t') {
            dest[d++] = '\\';
            dest[d++] = 't';
        } else {
            dest[d++] = src[s];
        }
    }
    dest[d] = '\0';
}

/* Serialize a single student node to JSON */
void avl_node_to_json(const AVLNode *node, char *buffer, size_t max_len) {
    if (node == NULL || buffer == NULL || max_len == 0) return;

    char esc_name[128], esc_gender[32], esc_dob[32], esc_address[300], esc_contact[80], esc_course[80];
    json_escape(node->full_name, esc_name, sizeof(esc_name));
    json_escape(node->gender, esc_gender, sizeof(esc_gender));
    json_escape(node->dob, esc_dob, sizeof(esc_dob));
    json_escape(node->address, esc_address, sizeof(esc_address));
    json_escape(node->contact, esc_contact, sizeof(esc_contact));
    json_escape(node->course, esc_course, sizeof(esc_course));

    snprintf(buffer, max_len,
        "{\"student_id\":%d,\"full_name\":\"%s\",\"gender\":\"%s\",\"dob\":\"%s\","
        "\"address\":\"%s\",\"contact\":\"%s\",\"course\":\"%s\","
        "\"height\":%d,\"balance_factor\":%d}",
        node->student_id, esc_name, esc_gender, esc_dob,
        esc_address, esc_contact, esc_course,
        node->height, avl_balance_factor(node));
}

/* Recursive helper for tree JSON export */
static void append_tree_json(const AVLNode *node, char *buffer, size_t *offset, size_t max_len) {
    if (*offset >= max_len - 1) return;

    if (node == NULL) {
        int written = snprintf(buffer + *offset, max_len - *offset, "null");
        if (written > 0) *offset += (size_t)written;
        return;
    }

    char esc_name[128], esc_gender[32], esc_course[80];
    json_escape(node->full_name, esc_name, sizeof(esc_name));
    json_escape(node->gender, esc_gender, sizeof(esc_gender));
    json_escape(node->course, esc_course, sizeof(esc_course));

    int written = snprintf(buffer + *offset, max_len - *offset,
        "{\"student_id\":%d,\"full_name\":\"%s\",\"gender\":\"%s\",\"course\":\"%s\","
        "\"height\":%d,\"balance_factor\":%d,\"left\":",
        node->student_id, esc_name, esc_gender, esc_course,
        node->height, avl_balance_factor(node));
    if (written > 0) *offset += (size_t)written;

    append_tree_json(node->left, buffer, offset, max_len);

    written = snprintf(buffer + *offset, max_len - *offset, ",\"right\":");
    if (written > 0) *offset += (size_t)written;

    append_tree_json(node->right, buffer, offset, max_len);

    written = snprintf(buffer + *offset, max_len - *offset, "}");
    if (written > 0) *offset += (size_t)written;
}

/* Serialize entire AVL tree hierarchy to JSON */
void avl_tree_to_json(const AVLNode *root, char *buffer, size_t max_len) {
    if (buffer == NULL || max_len == 0) return;
    size_t offset = 0;
    buffer[0] = '\0';
    append_tree_json(root, buffer, &offset, max_len);
}

/* Struct for traversal JSON array accumulation */
typedef struct {
    char *buffer;
    size_t offset;
    size_t max_len;
    bool is_first;
} TraversalAccumulator;

static void traversal_visit(const AVLNode *node, void *user_data) {
    TraversalAccumulator *acc = (TraversalAccumulator*)user_data;
    if (acc->offset >= acc->max_len - 1) return;

    char node_buf[1024];
    avl_node_to_json(node, node_buf, sizeof(node_buf));

    int written;
    if (acc->is_first) {
        written = snprintf(acc->buffer + acc->offset, acc->max_len - acc->offset, "%s", node_buf);
        acc->is_first = false;
    } else {
        written = snprintf(acc->buffer + acc->offset, acc->max_len - acc->offset, ",%s", node_buf);
    }
    if (written > 0) acc->offset += (size_t)written;
}

/* Serialize traversal to JSON array */
void avl_traversal_to_json(const AVLNode *root, TraversalType type, char *buffer, size_t max_len) {
    if (buffer == NULL || max_len == 0) return;

    TraversalAccumulator acc;
    acc.buffer = buffer;
    acc.offset = 0;
    acc.max_len = max_len;
    acc.is_first = true;

    acc.buffer[acc.offset++] = '[';
    acc.buffer[acc.offset] = '\0';

    if (type == TRAVERSAL_INORDER) {
        avl_inorder(root, traversal_visit, &acc);
    } else if (type == TRAVERSAL_PREORDER) {
        avl_preorder(root, traversal_visit, &acc);
    } else {
        avl_postorder(root, traversal_visit, &acc);
    }

    if (acc.offset < acc.max_len - 2) {
        acc.buffer[acc.offset++] = ']';
        acc.buffer[acc.offset] = '\0';
    } else {
        acc.buffer[acc.max_len - 2] = ']';
        acc.buffer[acc.max_len - 1] = '\0';
    }
}
