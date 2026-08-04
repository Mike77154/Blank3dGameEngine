#ifndef BVH_LEXER_H_INCLUDED
#define BVH_LEXER_H_INCLUDED

#include "bvh_token.h"

typedef struct BVH_Lexer {
    const char *src;
    unsigned long len;
    unsigned long pos;
    int line;
    int col;
    int at_line_head;
    int has_peek;
    BVH_Token peek;
} BVH_Lexer;

void bvh_lexer_init(BVH_Lexer *lx, const char *src);
BVH_Token bvh_lexer_next(BVH_Lexer *lx);
BVH_Token bvh_lexer_peek(BVH_Lexer *lx);
void bvh_lexer_capture_line_rest(BVH_Lexer *lx, const char **out_start, unsigned long *out_len);
void bvh_lexer_skip_line(BVH_Lexer *lx);

#endif
