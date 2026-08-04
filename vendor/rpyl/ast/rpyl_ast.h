#ifndef RPYL_AST_H
#define RPYL_AST_H

#include "rpyl_config.h"
#include "rpyl_arena.h"

typedef enum {
    AST_DEFINE,
    AST_BLOCK,
    AST_CALL
} AstNodeType;

typedef struct AstNode AstNode;

typedef enum {
    AST_ALLOC_POOL = 0,
    AST_ALLOC_ARENA = 1
} AstAllocKind;

struct AstNode {
    AstNodeType type;
    char name[RPYL_AST_MAX_NAME];
    unsigned char alloc_kind;
    char value[RPYL_AST_MAX_VALUE];
    char args[RPYL_AST_MAX_ARGS][RPYL_AST_MAX_ARG_TEXT];
    int arg_count;
    int line;
    int column;
    char source_file[64];
    AstNode* children;
    int child_count;
    AstNode* next;
};

AstNode* ast_new(AstNodeType type);
AstNode* ast_new_in_arena(AstNodeType type, RpylArena* arena);
AstNode* ast_new_ex(AstNodeType type, RpylArena* arena);
void ast_add_child(AstNode* parent, AstNode* child);
void ast_release(AstNode* node);
void ast_pool_reset(void);

#endif /* RPYL_AST_H */
