#ifndef AVL_H
#define AVL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* Status and Error Codes */
typedef enum {
    AVL_OK = 0,
    AVL_ERR_DUPLICATE = 1,
    AVL_ERR_NOT_FOUND = 2,
    AVL_ERR_MEMORY = 3,
    AVL_ERR_EMPTY = 4,
    AVL_ERR_INVALID = 5
} AVLError;

/* Traversal Types */
typedef enum {
    TRAVERSAL_INORDER = 0,
    TRAVERSAL_PREORDER = 1,
    TRAVERSAL_POSTORDER = 2
} TraversalType;

/* Rotation Types for Diagnostics and Visualization */
typedef enum {
    ROTATION_NONE = 0,
    ROTATION_SINGLE_RIGHT = 1, // LL Case
    ROTATION_SINGLE_LEFT = 2,  // RR Case
    ROTATION_LEFT_RIGHT = 3,   // LR Case
    ROTATION_RIGHT_LEFT = 4    // RL Case
} RotationType;

/* Student Record Node Structure for AVL Tree */
typedef struct AVLNode {
    int student_id;               /* Unique Identifier / AVL Balancing Key */
    char full_name[100];          /* Student Full Name */
    char gender[16];              /* Gender (e.g. Male, Female, Other) */
    char dob[16];                 /* Date of Birth (YYYY-MM-DD) */
    char address[256];            /* Address */
    char contact[64];             /* Contact Number or Email */
    char course[64];              /* Academic Course / Major */
    int height;                   /* Node height in AVL tree */
    struct AVLNode *left;         /* Left subtree pointer */
    struct AVLNode *right;        /* Right subtree pointer */
} AVLNode;

/* Structure for passing record data when creating/inserting */
typedef struct {
    int student_id;
    char full_name[100];
    char gender[16];
    char dob[16];
    char address[256];
    char contact[64];
    char course[64];
} StudentData;

/* AVL Tree Management Functions */
AVLNode* avl_create_node(const StudentData *data);
int avl_height(const AVLNode *node);
int avl_balance_factor(const AVLNode *node);
int avl_max(int a, int b);

/* Node Insert & Delete with Balancing and Rotation Logging */
AVLNode* avl_insert(AVLNode *root, const StudentData *data, AVLError *status, char *rotation_log, size_t log_size);
AVLNode* avl_delete(AVLNode *root, int student_id, AVLError *status, char *rotation_log, size_t log_size);

/* Lookup & Verification */
AVLNode* avl_search(AVLNode *root, int student_id);
int avl_count(const AVLNode *root);
bool avl_is_balanced(const AVLNode *root);

/* Traversals */
typedef void (*AVLVisitCallback)(const AVLNode *node, void *user_data);
void avl_inorder(const AVLNode *root, AVLVisitCallback callback, void *user_data);
void avl_preorder(const AVLNode *root, AVLVisitCallback callback, void *user_data);
void avl_postorder(const AVLNode *root, AVLVisitCallback callback, void *user_data);

/* Memory Deallocation */
void avl_free(AVLNode *root);

/* JSON Serialization for REST API */
void avl_node_to_json(const AVLNode *node, char *buffer, size_t max_len);
void avl_tree_to_json(const AVLNode *root, char *buffer, size_t max_len);
void avl_traversal_to_json(const AVLNode *root, TraversalType type, char *buffer, size_t max_len);

#endif /* AVL_H */
