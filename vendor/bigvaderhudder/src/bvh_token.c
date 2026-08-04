#include "bvh_token.h"

const char *bvh_token_name(BVH_TokenType type) {
    switch (type) {
        case BVH_TOK_EOF: return "EOF";
        case BVH_TOK_EOL: return "EOL";
        case BVH_TOK_IDENT: return "IDENT";
        case BVH_TOK_NUMBER: return "NUMBER";
        case BVH_TOK_STRING: return "STRING";
        case BVH_TOK_COLOR: return "COLOR";
        case BVH_TOK_COMMA: return "COMMA";
        case BVH_TOK_PLUS: return "PLUS";
        case BVH_TOK_MINUS: return "MINUS";
        case BVH_TOK_SLASH: return "SLASH";
        case BVH_TOK_DOTDOT: return "DOTDOT";
        case BVH_TOK_BLOCK_OPEN: return "BLOCK_OPEN";
        case BVH_TOK_BLOCK_CLOSE: return "BLOCK_CLOSE";
        case BVH_TOK_OTHER: return "OTHER";
        default: return "UNKNOWN";
    }
}
