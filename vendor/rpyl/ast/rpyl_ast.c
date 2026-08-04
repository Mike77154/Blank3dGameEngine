#include "rpyl_ast.h"

#include <string.h>

static AstNode g_fallback_nodes[RPYL_FALLBACK_AST_NODES];
static int g_fallback_used = 0;

void ast_pool_reset(void) {
    g_fallback_used = 0;
}

AstNode* ast_new_ex(AstNodeType type, RpylArena* arena) {
    AstNode* n;
    if (arena) {
        n = (AstNode*)rpyl_arena_alloc_zero(arena, sizeof(AstNode), sizeof(void*));
        if (!n) return (AstNode*)0;
        n->alloc_kind = (unsigned char)AST_ALLOC_ARENA;
    } else {
        if (g_fallback_used >= RPYL_FALLBACK_AST_NODES) return (AstNode*)0;
        n = &g_fallback_nodes[g_fallback_used++];
        memset(n, 0, sizeof(*n));
        n->alloc_kind = (unsigned char)AST_ALLOC_POOL;
    }

    n->type = type;
    n->name[0] = 0;
    n->value[0] = 0;
    n->arg_count = 0;
    n->child_count = 0;
    n->children = (AstNode*)0;
    n->next = (AstNode*)0;
    return n;
}

AstNode* ast_new(AstNodeType type) {
    return ast_new_ex(type, (RpylArena*)0);
}

AstNode* ast_new_in_arena(AstNodeType type, RpylArena* arena) {
    return ast_new_ex(type, arena);
}

void ast_add_child(AstNode* parent, AstNode* child) {
    AstNode* n;
    if (!parent || !child) return;
    child->next = (AstNode*)0;
    if (!parent->children) {
        parent->children = child;
    } else {
        n = parent->children;
        while (n->next) n = n->next;
        n->next = child;
    }
    parent->child_count++;
}

void ast_release(AstNode* node) {
    (void)node;
}
