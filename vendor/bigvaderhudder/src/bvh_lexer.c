#include "bvh_lexer.h"

static int bvh_ident_start(int c) {
    return (isalpha((unsigned char)c) || c == '_') ? 1 : 0;
}

static int bvh_ident_part(int c) {
    if (isalnum((unsigned char)c)) return 1;
    if (c == '_' || c == '.' || c == '-') return 1;
    return 0;
}

static int bvh_match3(const char *s, unsigned long pos, unsigned long len, const char *lit) {
    if (pos + 3UL > len) return 0;
    if (s[pos] != lit[0]) return 0;
    if (s[pos + 1UL] != lit[1]) return 0;
    if (s[pos + 2UL] != lit[2]) return 0;
    return 1;
}

void bvh_lexer_init(BVH_Lexer *lx, const char *src) {
    if (!lx) return;
    lx->src = src ? src : "";
    lx->len = bvh_cstr_len(lx->src);
    lx->pos = 0UL;
    lx->line = 1;
    lx->col = 1;
    lx->at_line_head = 1;
    lx->has_peek = 0;
}

static void bvh_lex_step(BVH_Lexer *lx, unsigned long n) {
    unsigned long i;
    for (i = 0UL; i < n && lx->pos < lx->len; i++) {
        char c;
        c = lx->src[lx->pos];
        lx->pos++;
        if (c == '\n') {
            lx->line++;
            lx->col = 1;
            lx->at_line_head = 1;
        } else {
            lx->col++;
            if (c != ' ' && c != '\t' && c != '\r') lx->at_line_head = 0;
        }
    }
}

static void bvh_skip_spaces_and_comments(BVH_Lexer *lx) {
    int again;
    again = 1;
    while (again) {
        again = 0;
        while (lx->pos < lx->len && bvh_is_space_no_eol((unsigned char)lx->src[lx->pos])) {
            bvh_lex_step(lx, 1UL);
        }
        if (lx->pos < lx->len && lx->src[lx->pos] == '#' && lx->at_line_head) {
            while (lx->pos < lx->len && lx->src[lx->pos] != '\n') bvh_lex_step(lx, 1UL);
            again = 1;
            continue;
        }
        if (lx->pos + 1UL < lx->len && lx->src[lx->pos] == '/' && lx->src[lx->pos + 1UL] == '/') {
            while (lx->pos < lx->len && lx->src[lx->pos] != '\n') bvh_lex_step(lx, 1UL);
            again = 1;
            continue;
        }
    }
}

static BVH_Token bvh_make_token(BVH_Lexer *lx, BVH_TokenType type, unsigned long start, unsigned long len, int line, int col) {
    BVH_Token t;
    t.type = type;
    t.start = lx->src + start;
    t.len = len;
    t.line = line;
    t.col = col;
    return t;
}

BVH_Token bvh_lexer_next(BVH_Lexer *lx) {
    unsigned long start;
    int line;
    int col;
    int c;

    if (lx->has_peek) {
        lx->has_peek = 0;
        return lx->peek;
    }

    bvh_skip_spaces_and_comments(lx);

    start = lx->pos;
    line = lx->line;
    col = lx->col;

    if (lx->pos >= lx->len) return bvh_make_token(lx, BVH_TOK_EOF, start, 0UL, line, col);

    c = (unsigned char)lx->src[lx->pos];

    if (c == '\n') {
        bvh_lex_step(lx, 1UL);
        return bvh_make_token(lx, BVH_TOK_EOL, start, 1UL, line, col);
    }

    if (bvh_match3(lx->src, lx->pos, lx->len, "-=)")) {
        bvh_lex_step(lx, 3UL);
        return bvh_make_token(lx, BVH_TOK_BLOCK_OPEN, start, 3UL, line, col);
    }

    if (bvh_match3(lx->src, lx->pos, lx->len, "(=-")) {
        bvh_lex_step(lx, 3UL);
        return bvh_make_token(lx, BVH_TOK_BLOCK_CLOSE, start, 3UL, line, col);
    }

    if (c == '.' && lx->pos + 1UL < lx->len && lx->src[lx->pos + 1UL] == '.') {
        bvh_lex_step(lx, 2UL);
        return bvh_make_token(lx, BVH_TOK_DOTDOT, start, 2UL, line, col);
    }

    if (c == '#') {
        unsigned long p;
        p = lx->pos + 1UL;
        while (p < lx->len && bvh_is_hex_ch((unsigned char)lx->src[p])) p++;
        if (p > lx->pos + 1UL) {
            while (lx->pos < p) bvh_lex_step(lx, 1UL);
            return bvh_make_token(lx, BVH_TOK_COLOR, start, p - start, line, col);
        }
    }

    if ((c == '-' || c == '+') && lx->pos + 1UL < lx->len && bvh_is_digit_ch((unsigned char)lx->src[lx->pos + 1UL])) {
        unsigned long p;
        p = lx->pos + 1UL;
        while (p < lx->len && bvh_is_digit_ch((unsigned char)lx->src[p])) p++;
        if (p < lx->len && lx->src[p] == '.') {
            p++;
            while (p < lx->len && bvh_is_digit_ch((unsigned char)lx->src[p])) p++;
        }
        while (lx->pos < p) bvh_lex_step(lx, 1UL);
        return bvh_make_token(lx, BVH_TOK_NUMBER, start, p - start, line, col);
    }

    if (bvh_is_digit_ch(c)) {
        unsigned long p2;
        p2 = lx->pos;
        while (p2 < lx->len && bvh_is_digit_ch((unsigned char)lx->src[p2])) p2++;
        if (p2 < lx->len && lx->src[p2] == '.' && !(p2 + 1UL < lx->len && lx->src[p2 + 1UL] == '.')) {
            p2++;
            while (p2 < lx->len && bvh_is_digit_ch((unsigned char)lx->src[p2])) p2++;
        }
        while (lx->pos < p2) bvh_lex_step(lx, 1UL);
        return bvh_make_token(lx, BVH_TOK_NUMBER, start, p2 - start, line, col);
    }

    if (c == '"') {
        bvh_lex_step(lx, 1UL);
        while (lx->pos < lx->len) {
            c = (unsigned char)lx->src[lx->pos];
            if (c == '\\' && lx->pos + 1UL < lx->len) {
                bvh_lex_step(lx, 2UL);
                continue;
            }
            bvh_lex_step(lx, 1UL);
            if (c == '"') break;
        }
        return bvh_make_token(lx, BVH_TOK_STRING, start, lx->pos - start, line, col);
    }

    if (bvh_ident_start(c)) {
        bvh_lex_step(lx, 1UL);
        while (lx->pos < lx->len && bvh_ident_part((unsigned char)lx->src[lx->pos])) bvh_lex_step(lx, 1UL);
        return bvh_make_token(lx, BVH_TOK_IDENT, start, lx->pos - start, line, col);
    }

    if (c == ',') {
        bvh_lex_step(lx, 1UL);
        return bvh_make_token(lx, BVH_TOK_COMMA, start, 1UL, line, col);
    }
    if (c == '+') {
        bvh_lex_step(lx, 1UL);
        return bvh_make_token(lx, BVH_TOK_PLUS, start, 1UL, line, col);
    }
    if (c == '-') {
        bvh_lex_step(lx, 1UL);
        return bvh_make_token(lx, BVH_TOK_MINUS, start, 1UL, line, col);
    }
    if (c == '/') {
        bvh_lex_step(lx, 1UL);
        return bvh_make_token(lx, BVH_TOK_SLASH, start, 1UL, line, col);
    }

    bvh_lex_step(lx, 1UL);
    return bvh_make_token(lx, BVH_TOK_OTHER, start, 1UL, line, col);
}

BVH_Token bvh_lexer_peek(BVH_Lexer *lx) {
    if (!lx->has_peek) {
        lx->peek = bvh_lexer_next(lx);
        lx->has_peek = 1;
    }
    return lx->peek;
}

void bvh_lexer_capture_line_rest(BVH_Lexer *lx, const char **out_start, unsigned long *out_len) {
    unsigned long start;
    unsigned long end;
    unsigned long p;
    int in_quote;
    int prev_space;

    if (!out_start || !out_len) return;

    while (lx->pos < lx->len && bvh_is_space_no_eol((unsigned char)lx->src[lx->pos])) bvh_lex_step(lx, 1UL);

    start = lx->pos;
    p = start;
    in_quote = 0;
    prev_space = 1;

    while (p < lx->len) {
        char c;
        c = lx->src[p];
        if (c == '\n') break;
        if (!in_quote) {
            if (c == '"') {
                in_quote = 1;
                prev_space = 0;
                p++;
                continue;
            }
            if (c == '/' && p + 1UL < lx->len && lx->src[p + 1UL] == '/' && prev_space) break;
        } else {
            if (c == '\\' && p + 1UL < lx->len) {
                p += 2UL;
                prev_space = 0;
                continue;
            }
            if (c == '"') {
                in_quote = 0;
                p++;
                prev_space = 0;
                continue;
            }
        }
        prev_space = bvh_is_space_no_eol((unsigned char)c);
        p++;
    }

    end = p;
    while (end > start && bvh_is_space_no_eol((unsigned char)lx->src[end - 1UL])) end--;
    *out_start = lx->src + start;
    *out_len = end > start ? end - start : 0UL;
    while (lx->pos < end) bvh_lex_step(lx, 1UL);
}

void bvh_lexer_skip_line(BVH_Lexer *lx) {
    while (lx->pos < lx->len && lx->src[lx->pos] != '\n') bvh_lex_step(lx, 1UL);
}

void bvh_dump_tokens(BVH_Context *ctx, FILE *out) {
    BVH_Lexer lx;
    BVH_Token t;
    char text[BVH_TOKEN_TEXT_MAX];
    if (!ctx) return;
    if (!out) out = stdout;
    bvh_lexer_init(&lx, ctx->source);
    for (;;) {
        t = bvh_lexer_next(&lx);
        bvh_copy_slice(text, (unsigned long)sizeof(text), t.start, t.len);
        fprintf(out, "%04d:%03d  %-12s  %s\n", t.line, t.col, bvh_token_name(t.type), text);
        if (t.type == BVH_TOK_EOF) break;
    }
}
