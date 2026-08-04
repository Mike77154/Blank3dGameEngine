#include "rpyl_lexer.h"

#include "rpyl_port.h"

#include <ctype.h>
#if RPYL_ENABLE_STDIO
#include <stdio.h>
#else
#ifndef EOF
#define EOF (-1)
#endif
#endif
#include <string.h>

static RpylLexerState g_legacy_lexer;
static char g_legacy_label_kw[RPYL_LEXER_MAX_LABEL_KW] = "label";
static int g_legacy_initialized = 0;

static int is_alpha_c(int c) {
    if (c == EOF) return 0;
    return isalpha((unsigned char)c) ? 1 : 0;
}

static int is_alnum_c(int c) {
    if (c == EOF) return 0;
    return isalnum((unsigned char)c) ? 1 : 0;
}

static int is_digit_c(int c) {
    if (c == EOF) return 0;
    return isdigit((unsigned char)c) ? 1 : 0;
}

static int is_space_c(int c) {
    if (c == EOF) return 0;
    return isspace((unsigned char)c) ? 1 : 0;
}

void rpyl_lexer_state_init(RpylLexerState* st, const char* label_keyword) {
    if (!st) return;
    memset(st, 0, sizeof(*st));
    rpyl_strcpy_trunc(st->label_keyword, sizeof(st->label_keyword),
                      (label_keyword && label_keyword[0]) ? label_keyword : "label");
    st->line = 1;
    st->column = 1;
    st->offset = 0ul;
    st->bol = 1;
}

static void ensure_legacy(void) {
    if (!g_legacy_initialized) {
        rpyl_lexer_state_init(&g_legacy_lexer, g_legacy_label_kw);
        g_legacy_initialized = 1;
    }
}

void rpyl_lexer_set_label_keyword(const char* kw) {
    if (!kw || !kw[0]) return;
    rpyl_strcpy_trunc(g_legacy_label_kw, sizeof(g_legacy_label_kw), kw);
    if (g_legacy_initialized) {
        rpyl_strcpy_trunc(g_legacy_lexer.label_keyword, sizeof(g_legacy_lexer.label_keyword), kw);
    }
}

void rpyl_lexer_reset(void) {
    rpyl_lexer_state_init(&g_legacy_lexer, g_legacy_label_kw);
    g_legacy_initialized = 1;
}

static void push_indent(RpylLexerState* st, int n) {
    if (!st) return;
    if (st->indent_top + 1 >= (int)(sizeof(st->indent_stack) / sizeof(st->indent_stack[0]))) {
        return;
    }
    st->indent_stack[++st->indent_top] = n;
}

static int pop_indent(RpylLexerState* st) {
    int v;
    if (!st || st->indent_top <= 0) return 0;
    v = st->indent_stack[st->indent_top];
    st->indent_stack[st->indent_top] = 0;
    st->indent_top--;
    if (st->indent_top > 0) st->current_indent = st->indent_stack[st->indent_top];
    else st->current_indent = 0;
    return v;
}

static void token_basic(RpylToken* out, RpylTokenType type, int line) {
    if (!out) return;
    out->type = type;
    out->line = line;
    out->column = 1;
    out->offset = 0ul;
    if (RPYL_TOKEN_MAX_TEXT > 0) out->text[0] = 0;
}

#if RPYL_ENABLE_STDIO
/* FILE* stream adapter ------------------------------------------------ */

typedef struct {
    FILE* f;
} RpylFileStream;

static int file_getc(void* user) {
    RpylFileStream* fs;
    fs = (RpylFileStream*)user;
    if (!fs || !fs->f) return EOF;
    return fgetc(fs->f);
}

static int file_ungetc(int c, void* user) {
    RpylFileStream* fs;
    fs = (RpylFileStream*)user;
    if (!fs || !fs->f) return EOF;
    return ungetc(c, fs->f);
}

#endif

/* Lexer core ---------------------------------------------------------- */

static int emit_eof_or_dedent(RpylLexerState* st, RpylToken* out) {
    if (st->indent_top > 0) {
        pop_indent(st);
        token_basic(out, TOK_DEDENT, st->line);
        return 1;
    }
    token_basic(out, TOK_EOF, st->line);
    return 0;
}

static int read_number_tail(RpylStream* s, RpylToken* out, int first, int second, int saw_dot) {
    int c;
    int i;

    i = 0;
    if (first != 0) {
        if (i < (int)sizeof(out->text) - 1) out->text[i++] = (char)first;
    }
    if (second != 0) {
        if (i < (int)sizeof(out->text) - 1) out->text[i++] = (char)second;
    }

    while (1) {
        c = rpyl_stream_getc(s);
        if (is_digit_c(c)) {
            if (i < (int)sizeof(out->text) - 1) out->text[i++] = (char)c;
            continue;
        }
        if (c == '.' && !saw_dot) {
            int d;
            d = rpyl_stream_getc(s);
            if (!is_digit_c(d)) {
                rpyl_stream_ungetc(s, d);
                rpyl_stream_ungetc(s, c);
                break;
            }
            saw_dot = 1;
            if (i < (int)sizeof(out->text) - 1) out->text[i++] = '.';
            if (i < (int)sizeof(out->text) - 1) out->text[i++] = (char)d;
            continue;
        }
        break;
    }

    out->text[i] = 0;
    rpyl_stream_ungetc(s, c);
    out->type = TOK_NUMBER;
    return 1;
}

int rpyl_lex_stream_state(RpylLexerState* st, RpylStream* s, RpylToken* out) {
    int c;

    if (!st || !s || !out) return 0;
    if (RPYL_TOKEN_MAX_TEXT > 0) out->text[0] = 0;

    for (;;) {
        if (st->pending_dedent > 0) {
            st->pending_dedent--;
            token_basic(out, TOK_DEDENT, st->line);
            return 1;
        }

        c = rpyl_stream_getc(s);
        if (c == EOF) {
            return emit_eof_or_dedent(st, out);
        }

        if (st->bol) {
            int spaces;
            spaces = 0;
            while (c == ' ') {
                spaces++;
                st->column++;
                st->offset++;
                c = rpyl_stream_getc(s);
            }

            if (c == '\n') {
                st->bol = 1;
                token_basic(out, TOK_NEWLINE, st->line);
                st->line++;
                return 1;
            }

            if (c == '#') {
                while ((c = rpyl_stream_getc(s)) != '\n' && c != EOF) {
                }
                if (c == EOF) return emit_eof_or_dedent(st, out);
                st->bol = 1;
                token_basic(out, TOK_NEWLINE, st->line);
                st->line++;
                return 1;
            }

            st->bol = 0;

            if (spaces > st->current_indent) {
                push_indent(st, spaces);
                st->current_indent = spaces;
                token_basic(out, TOK_INDENT, st->line);
                rpyl_stream_ungetc(s, c);
                return 1;
            }

            if (spaces < st->current_indent) {
                while (st->indent_top > 0 && st->indent_stack[st->indent_top] > spaces) {
                    pop_indent(st);
                    st->pending_dedent++;
                }
                st->current_indent = spaces;
                if (st->pending_dedent > 0) {
                    st->pending_dedent--;
                    token_basic(out, TOK_DEDENT, st->line);
                    rpyl_stream_ungetc(s, c);
                    return 1;
                }
            }
        }

        if (c == '\n') {
            st->bol = 1;
            token_basic(out, TOK_NEWLINE, st->line);
            st->line++;
            return 1;
        }

        if (is_space_c(c)) {
            continue;
        }

        if (c == '#') {
            while ((c = rpyl_stream_getc(s)) != '\n' && c != EOF) {
            }
            if (c == EOF) return emit_eof_or_dedent(st, out);
            st->bol = 1;
            token_basic(out, TOK_NEWLINE, st->line);
            st->line++;
            return 1;
        }

        if (c == '$') {
            int i;
            i = 0;
            out->text[i++] = '$';
            c = rpyl_stream_getc(s);
            if (c == EOF) {
                out->text[i] = 0;
                out->type = TOK_IDENTIFIER;
                out->line = st->line;
                return 1;
            }
            if (!(is_alpha_c(c) || c == '_')) {
                rpyl_stream_ungetc(s, c);
                out->text[i] = 0;
                out->type = TOK_IDENTIFIER;
                out->line = st->line;
                return 1;
            }
            if (i < (int)sizeof(out->text) - 1) out->text[i++] = (char)c;
            while ((c = rpyl_stream_getc(s)), is_alnum_c(c) || c == '_') {
                if (i < (int)sizeof(out->text) - 1) out->text[i++] = (char)c;
            }
            out->text[i] = 0;
            rpyl_stream_ungetc(s, c);
            out->type = TOK_IDENTIFIER;
            out->line = st->line;
            out->column = st->column;
            out->offset = st->offset;
            return 1;
        }

        if (c == '-') {
            int d;
            d = rpyl_stream_getc(s);
            if (is_digit_c(d)) {
                out->line = st->line;
                out->column = st->column;
                out->offset = st->offset;
                return read_number_tail(s, out, '-', d, 0);
            }
            if (d == '.') {
                int e;
                e = rpyl_stream_getc(s);
                if (is_digit_c(e)) {
                    out->line = st->line;
                    out->column = st->column;
                    out->offset = st->offset;
                    rpyl_stream_ungetc(s, e);
                    return read_number_tail(s, out, '-', '.', 1);
                }
                rpyl_stream_ungetc(s, e);
            }
            rpyl_stream_ungetc(s, d);
            continue;
        }

        if (c == '.') {
            int d;
            d = rpyl_stream_getc(s);
            if (is_digit_c(d)) {
                out->line = st->line;
                out->column = st->column;
                out->offset = st->offset;
                return read_number_tail(s, out, '.', d, 1);
            }
            rpyl_stream_ungetc(s, d);
        }

        if (is_alpha_c(c) || c == '_' || c == '.') {
            int i;
            i = 0;
            out->text[i++] = (char)c;
            while ((c = rpyl_stream_getc(s)), is_alnum_c(c) || c == '_' || c == '.') {
                if (i < (int)sizeof(out->text) - 1) out->text[i++] = (char)c;
            }
            out->text[i] = 0;
            rpyl_stream_ungetc(s, c);

            if (strcmp(out->text, "define") == 0) out->type = TOK_DEFINE;
            else if (strcmp(out->text, "default") == 0) out->type = TOK_DEFAULT;
            else if (strcmp(out->text, "init") == 0) out->type = TOK_INIT;
            else if (strcmp(out->text, st->label_keyword) == 0) out->type = TOK_LABEL;
            else out->type = TOK_IDENTIFIER;

            out->line = st->line;
            return 1;
        }

        if (c == '"' || c == '\'' || c == '`') {
            int quote;
            int i;
            quote = c;
            i = 0;
            while ((c = rpyl_stream_getc(s)) != EOF) {
                if (c == quote) break;
                if (c == '\n') {
                    rpyl_stream_ungetc(s, c);
                    break;
                }
                if (c == '\\') {
                    int e;
                    e = rpyl_stream_getc(s);
                    if (e == EOF) break;
                    switch (e) {
                        case 'n': c = '\n'; break;
                        case 't': c = '\t'; break;
                        case 'r': c = '\r'; break;
                        case '\\': c = '\\'; break;
                        case '"': c = '"'; break;
                        case '\'': c = '\''; break;
                        default: c = e; break;
                    }
                }
                if (i < (int)sizeof(out->text) - 1) out->text[i++] = (char)c;
            }
            out->text[i] = 0;
            out->type = TOK_STRING;
            out->line = st->line;
            out->column = st->column;
            out->offset = st->offset;
            return 1;
        }

        if (is_digit_c(c)) {
            out->line = st->line;
            out->column = st->column;
            out->offset = st->offset;
            return read_number_tail(s, out, c, 0, 0);
        }

        if (c == ':') {
            token_basic(out, TOK_COLON, st->line);
            return 1;
        }

        if (c == '=') {
            token_basic(out, TOK_EQUAL, st->line);
            return 1;
        }

        /* Unknown character: skip and continue. */
    }
}

int rpyl_lex_stream(RpylStream* s, RpylToken* out) {
    ensure_legacy();
    return rpyl_lex_stream_state(&g_legacy_lexer, s, out);
}

#if RPYL_ENABLE_STDIO
int rpyl_lex(FILE* f, RpylToken* out) {
    RpylFileStream fs;
    RpylStream s;

    if (!f || !out) return 0;
    ensure_legacy();

    fs.f = f;
    s.user = &fs;
    s.getc_fn = file_getc;
    s.ungetc_fn = file_ungetc;

    return rpyl_lex_stream_state(&g_legacy_lexer, &s, out);
}
#endif
