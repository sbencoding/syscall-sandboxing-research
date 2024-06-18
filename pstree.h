#include <stdlib.h>
#include <stdio.h>

#include "sysnr_map.h"

struct ps_syscall_info {
    unsigned long ip;
    unsigned long syscall_nr;
};

struct pstree {
    unsigned long pid;
    unsigned long base_addr;
    char* exe_path;
    struct pstree* child;
    struct pstree* sibling;
    struct pstree* exec;
    struct ps_syscall_info syscall;
    int syscall_usage[N_SYSCALLS];
};

typedef void (*pstree_callback_t)(struct pstree*);

struct pstree* pstree_mknode(const unsigned long pid, char* exe_path) {
    struct pstree* node = (struct pstree*)malloc(sizeof(struct pstree));
    node->pid = pid;
    node->exe_path = exe_path;
    node->base_addr = 0;
    return node;
}

struct pstree* pstree_find(struct pstree* root, const unsigned long pid) {
    if (root->pid == pid) {
        while (root->exec != NULL) root = root->exec;
        return root;
    }
    struct pstree* result = NULL;
    if (root->sibling != NULL) result = pstree_find(root->sibling, pid);
    if (result != NULL) return result;
    if (root->child != NULL) result = pstree_find(root->child, pid);
    if (result != NULL) return result;
    return NULL;
}

//TODO: can reuse `pstree_foreach`
void pstree_free(struct pstree* root) {
    if (root->sibling != NULL) pstree_free(root->sibling);
    if (root->child != NULL) pstree_free(root->child);
    if (root->exec != NULL) pstree_free(root->exec);
    free(root->exe_path);
    free(root);
}

void pstree_print(struct pstree* root, int ident) {
    for (int i = 0; i < ident; ++i) putchar('-');
    printf("%lu (%s) @ 0x%llx\n", root->pid, root->exe_path, root->base_addr);
    if (root->child) pstree_print(root->child, ident + 2);
    if (root->sibling) pstree_print(root->sibling, ident);
    if (root->exec) {
        putchar('>');
        pstree_print(root->exec, ident);
    }
}

void pstree_foreach(struct pstree* root, pstree_callback_t cb) {
    if (root->exec != NULL) pstree_foreach(root->exec, cb);
    if (root->sibling != NULL) pstree_foreach(root->sibling, cb);
    if (root->child != NULL) pstree_foreach(root->child, cb);
    cb(root);
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

void pstree_insert_exec(struct pstree* parent, struct pstree* exec) {
    while (parent->exec != NULL) parent = parent->exec;
    parent->exec = exec;
}
