#include <stdlib.h>
#include <stdio.h>

struct ps_syscall_info {
    unsigned long ip;
    unsigned long syscall_nr;
};

struct pstree {
    unsigned long pid;
    struct pstree* child;
    struct pstree* sibling;
    struct ps_syscall_info syscall;
};

struct pstree* pstree_mknode(const unsigned long pid) {
    struct pstree* node = (struct pstree*)malloc(sizeof(struct pstree));
    node->pid = pid;
    return node;
}

struct pstree* pstree_find(struct pstree* root, const unsigned long pid) {
    if (root->pid == pid) return root;
    struct pstree* result = NULL;
    if (root->sibling != NULL) result = pstree_find(root->sibling, pid);
    if (result != NULL) return result;
    if (root->child != NULL) result = pstree_find(root->child, pid);
    if (result != NULL) return result;
    return NULL;
}

void pstree_free(struct pstree* root) {
    if (root->sibling != NULL) pstree_free(root->sibling);
    if (root->child != NULL) pstree_free(root->child);
    free(root);
}

void pstree_print(struct pstree* root, int ident) {
    for (int i = 0; i < ident; ++i) putchar('-');
    printf("%lu\n", root->pid);
    if (root->child) pstree_print(root->child, ident + 2);
    if (root->sibling) pstree_print(root->sibling, ident);
}

void pstree_insert_child(struct pstree* parent, struct pstree* child) {
    if (parent->child == NULL) parent->child = child;
    else {
        struct pstree* cur = parent->child;

        while (cur->sibling != NULL) {
            cur = cur->sibling;
        }

        cur->sibling = child;
    }
}
