#ifndef BVH_PARSER_H_INCLUDED
#define BVH_PARSER_H_INCLUDED

#include "bvh_ast.h"
#include "bvh_lexer.h"

typedef struct BVH_Parser {
    BVH_Context *ctx;
    BVH_Lexer lx;
    BVH_Error *err;
} BVH_Parser;

int bvh_parse_loaded(BVH_Context *ctx, BVH_Error *err);

#endif
