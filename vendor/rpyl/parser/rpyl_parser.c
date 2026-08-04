#include "rpyl_parser.h"

#include "rpyl_config.h"
#include "rpyl_lexer.h"
#include "rpyl_token.h"
#include "rpyl_port.h"

#if RPYL_ENABLE_STDIO
#include <stdio.h>
#else
#ifndef EOF
#define EOF (-1)
#endif
#endif
#include <string.h>

typedef struct RpylParserState {
    RpylToken tok;
    RpylStream* src;
    RpylArena* arena;
    RpylLexerState lexer;
} RpylParserState;

static void next_tok(RpylParserState* ps) {
    (void)rpyl_lex_stream_state(&ps->lexer, ps->src, &ps->tok);
}

static void safe_copy(char* dst, size_t dst_size, const char* src_str) {
    rpyl_strcpy_trunc(dst, dst_size, src_str ? src_str : "");
}

static void append_word(char* out, size_t out_size, const char* word) {
    size_t remain;
    if (!out || out_size == 0u || !word) return;
    if (out[0] != 0) {
        remain = out_size - strlen(out) - 1u;
        if (remain > 0u) strncat(out, " ", remain);
    }
    remain = out_size - strlen(out) - 1u;
    if (remain > 0u) strncat(out, word, remain);
}

static void skip_to_eol(RpylParserState* ps) {
    while (ps->tok.type != TOK_NEWLINE && ps->tok.type != TOK_EOF) next_tok(ps);
    if (ps->tok.type == TOK_NEWLINE) next_tok(ps);
}

static AstNode* node_new(RpylParserState* ps, AstNodeType type) {
    AstNode* n;
    n = ast_new_ex(type, ps->arena);
    if (n) {
        n->line = ps->tok.line;
        n->column = ps->tok.column;
    }
    return n;
}

static AstNode* make_call(RpylParserState* ps, const char* name) {
    AstNode* n;
    n = node_new(ps, AST_CALL);
    if (!n) return 0;
    safe_copy(n->name, sizeof(n->name), name);
    return n;
}

static int token_is_atom(RpylTokenType t) {
    return (t == TOK_IDENTIFIER || t == TOK_STRING || t == TOK_NUMBER) ? 1 : 0;
}

static AstNode* parse_call_with_name(RpylParserState* ps, const char* name_already_consumed) {
    AstNode* n;
    n = make_call(ps, name_already_consumed);
    if (!n) return 0;
    while (token_is_atom(ps->tok.type) || ps->tok.type == TOK_EQUAL) {
        if (ps->tok.type == TOK_EQUAL) {
            next_tok(ps);
            continue;
        }
        if (n->arg_count < (int)(sizeof(n->args) / sizeof(n->args[0]))) {
            safe_copy(n->args[n->arg_count], sizeof(n->args[n->arg_count]), ps->tok.text);
            n->arg_count++;
        }
        next_tok(ps);
    }
    if (ps->tok.type == TOK_NEWLINE) next_tok(ps);
    return n;
}

static AstNode* parse_call_with_preargs(RpylParserState* ps, const char* name_already_consumed, char pre_args[][RPYL_AST_MAX_ARG_TEXT], int pre_argc) {
    AstNode* n;
    int i;
    n = make_call(ps, name_already_consumed);
    if (!n) return 0;
    for (i = 0; i < pre_argc; i++) {
        if (n->arg_count < (int)(sizeof(n->args) / sizeof(n->args[0]))) {
            safe_copy(n->args[n->arg_count], sizeof(n->args[n->arg_count]), pre_args[i]);
            n->arg_count++;
        }
    }
    while (token_is_atom(ps->tok.type) || ps->tok.type == TOK_EQUAL) {
        if (ps->tok.type == TOK_EQUAL) {
            next_tok(ps);
            continue;
        }
        if (n->arg_count < (int)(sizeof(n->args) / sizeof(n->args[0]))) {
            safe_copy(n->args[n->arg_count], sizeof(n->args[n->arg_count]), ps->tok.text);
            n->arg_count++;
        }
        next_tok(ps);
    }
    if (ps->tok.type == TOK_NEWLINE) next_tok(ps);
    return n;
}

static AstNode* parse_say_string(RpylParserState* ps) {
    AstNode* n;
    n = make_call(ps, "say");
    if (!n) return 0;

    while (ps->tok.type == TOK_STRING) {
        if (n->arg_count < (int)(sizeof(n->args) / sizeof(n->args[0]))) {
            safe_copy(n->args[n->arg_count], sizeof(n->args[n->arg_count]), ps->tok.text);
            n->arg_count++;
        }
        next_tok(ps);
    }

    if (ps->tok.type == TOK_NEWLINE) next_tok(ps);
    else skip_to_eol(ps);
    return n;
}

static void parse_block_name(RpylParserState* ps, char* out, size_t out_size) {
    out[0] = 0;
    while (ps->tok.type == TOK_IDENTIFIER || ps->tok.type == TOK_STRING) {
        append_word(out, out_size, ps->tok.text);
        next_tok(ps);
        if (ps->tok.type == TOK_COLON) break;
    }
}

static AstNode* parse_named_block(RpylParserState* ps, const char* block_name) {
    AstNode* block;
    block = node_new(ps, AST_BLOCK);
    if (!block) return 0;
    safe_copy(block->name, sizeof(block->name), block_name);

    if (ps->tok.type != TOK_COLON) {
        skip_to_eol(ps);
        return block;
    }

    next_tok(ps);
    if (ps->tok.type == TOK_NEWLINE) next_tok(ps);
    if (ps->tok.type == TOK_INDENT) next_tok(ps);

    while (ps->tok.type != TOK_DEDENT && ps->tok.type != TOK_EOF) {
        if (ps->tok.type == TOK_NEWLINE) {
            next_tok(ps);
            continue;
        }

        if (ps->tok.type == TOK_STRING) {
            AstNode* s;
            s = parse_say_string(ps);
            if (s) ast_add_child(block, s);
            continue;
        }

        if (ps->tok.type == TOK_IDENTIFIER || ps->tok.type == TOK_DEFINE || ps->tok.type == TOK_DEFAULT) {
            char first_name[RPYL_AST_MAX_NAME];
            char name_buf[RPYL_PARSER_MAX_BLOCK_NAME];
            char pre_args[RPYL_PARSER_MAX_PREARGS][RPYL_AST_MAX_ARG_TEXT];
            int pre_argc;
            AstNode* nested;
            AstNode* call;

            safe_copy(first_name, sizeof(first_name), ps->tok.text);
            safe_copy(name_buf, sizeof(name_buf), ps->tok.text);
            next_tok(ps);

            pre_argc = 0;
            while (ps->tok.type == TOK_IDENTIFIER || ps->tok.type == TOK_STRING || ps->tok.type == TOK_NUMBER) {
                if (pre_argc < (int)(sizeof(pre_args) / sizeof(pre_args[0]))) {
                    safe_copy(pre_args[pre_argc], sizeof(pre_args[pre_argc]), ps->tok.text);
                    pre_argc++;
                }
                append_word(name_buf, sizeof(name_buf), ps->tok.text);
                next_tok(ps);
                if (ps->tok.type == TOK_COLON) break;
            }

            if (ps->tok.type == TOK_COLON) {
                nested = parse_named_block(ps, name_buf);
                if (nested) ast_add_child(block, nested);
            } else {
                if (pre_argc == 0) call = parse_call_with_name(ps, first_name);
                else call = parse_call_with_preargs(ps, first_name, pre_args, pre_argc);
                if (call) ast_add_child(block, call);
            }
            continue;
        }

        next_tok(ps);
    }

    if (ps->tok.type == TOK_DEDENT) next_tok(ps);
    return block;
}

static AstNode* parse_block(RpylParserState* ps) {
    char name_buf[RPYL_PARSER_MAX_BLOCK_NAME];
    AstNode* b;
    parse_block_name(ps, name_buf, sizeof(name_buf));
    if (name_buf[0] == 0) {
        skip_to_eol(ps);
        b = node_new(ps, AST_BLOCK);
        return b;
    }
    return parse_named_block(ps, name_buf);
}

static AstNode* parse_assignment_node(RpylParserState* ps) {
    AstNode* d;
    d = node_new(ps, AST_DEFINE);
    if (!d) return 0;
    next_tok(ps);
    if (ps->tok.type != TOK_IDENTIFIER) {
        skip_to_eol(ps);
        return d;
    }
    safe_copy(d->name, sizeof(d->name), ps->tok.text);
    next_tok(ps);
    if (ps->tok.type == TOK_EQUAL) next_tok(ps);
    if (token_is_atom(ps->tok.type)) {
        safe_copy(d->value, sizeof(d->value), ps->tok.text);
        next_tok(ps);
    }
    skip_to_eol(ps);
    return d;
}

#if RPYL_ENABLE_STDIO
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

typedef struct {
    const unsigned char* data;
    size_t len;
    size_t pos;
    int has_push;
    int push;
} RpylBufferStream;

static int buf_getc(void* user) {
    RpylBufferStream* bs;
    bs = (RpylBufferStream*)user;
    if (!bs) return EOF;
    if (bs->has_push) {
        bs->has_push = 0;
        return bs->push;
    }
    if (bs->pos >= bs->len) return EOF;
    return (int)bs->data[bs->pos++];
}

static int buf_ungetc(int c, void* user) {
    RpylBufferStream* bs;
    bs = (RpylBufferStream*)user;
    if (!bs) return EOF;
    if (c == EOF) return EOF;
    if (bs->has_push) return EOF;
    bs->has_push = 1;
    bs->push = c;
    return c;
}

AstNode* rpyl_parse_stream_ex_with_label(RpylStream* s, RpylArena* arena, const char* label_keyword) {
    RpylParserState ps;
    AstNode* root;

    if (!s) return 0;
    memset(&ps, 0, sizeof(ps));
    ps.src = s;
    ps.arena = arena;
    rpyl_lexer_state_init(&ps.lexer, label_keyword);

    next_tok(&ps);
    root = node_new(&ps, AST_BLOCK);
    if (!root) return 0;
    safe_copy(root->name, sizeof(root->name), "ROOT");

    while (ps.tok.type != TOK_EOF) {
        if (ps.tok.type == TOK_DEFINE || ps.tok.type == TOK_DEFAULT) {
            AstNode* d;
            d = parse_assignment_node(&ps);
            if (d) ast_add_child(root, d);
        } else if (ps.tok.type == TOK_LABEL) {
            next_tok(&ps);
            if (ps.tok.type == TOK_IDENTIFIER || ps.tok.type == TOK_STRING) {
                AstNode* b;
                b = parse_block(&ps);
                if (b) ast_add_child(root, b);
            } else {
                skip_to_eol(&ps);
            }
        } else if (ps.tok.type == TOK_IDENTIFIER || ps.tok.type == TOK_STRING) {
            AstNode* b;
            b = parse_block(&ps);
            if (b) ast_add_child(root, b);
        } else {
            next_tok(&ps);
        }
    }
    return root;
}

AstNode* rpyl_parse_stream_ex(RpylStream* s, RpylArena* arena) {
    return rpyl_parse_stream_ex_with_label(s, arena, (const char*)0);
}

AstNode* rpyl_parse_stream(RpylStream* s) {
    return rpyl_parse_stream_ex(s, (RpylArena*)0);
}

#if RPYL_ENABLE_STDIO
AstNode* rpyl_parse_ex_with_label(FILE* f, RpylArena* arena, const char* label_keyword) {
    RpylFileStream fs;
    RpylStream s;
    if (!f) return 0;
    fs.f = f;
    s.user = &fs;
    s.getc_fn = file_getc;
    s.ungetc_fn = file_ungetc;
    return rpyl_parse_stream_ex_with_label(&s, arena, label_keyword);
}

AstNode* rpyl_parse_ex(FILE* f, RpylArena* arena) {
    return rpyl_parse_ex_with_label(f, arena, (const char*)0);
}

AstNode* rpyl_parse(FILE* f) {
    return rpyl_parse_ex(f, (RpylArena*)0);
}
#endif

AstNode* rpyl_parse_buffer_ex_with_label(const char* buf, size_t len, RpylArena* arena, const char* label_keyword) {
    RpylBufferStream bs;
    RpylStream s;
    if (!buf && len > 0u) return 0;
    bs.data = (const unsigned char*)(buf ? buf : "");
    bs.len = len;
    bs.pos = 0u;
    bs.has_push = 0;
    bs.push = 0;
    s.user = &bs;
    s.getc_fn = buf_getc;
    s.ungetc_fn = buf_ungetc;
    return rpyl_parse_stream_ex_with_label(&s, arena, label_keyword);
}

AstNode* rpyl_parse_buffer_ex(const char* buf, size_t len, RpylArena* arena) {
    return rpyl_parse_buffer_ex_with_label(buf, len, arena, (const char*)0);
}

AstNode* rpyl_parse_buffer(const char* buf, size_t len) {
    return rpyl_parse_buffer_ex(buf, len, (RpylArena*)0);
}
