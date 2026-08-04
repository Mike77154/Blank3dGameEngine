#ifndef RPYL_LEXER_H
#define RPYL_LEXER_H

#include "rpyl_config.h"

#if RPYL_ENABLE_STDIO
#include <stdio.h>
#endif
#include "rpyl_stream.h"
#include "rpyl_token.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RpylLexerState {
    char label_keyword[RPYL_LEXER_MAX_LABEL_KW];
    int indent_stack[RPYL_LEXER_MAX_INDENT];
    int indent_top;
    int current_indent;
    int line;
    int column;
    unsigned long offset;
    int pending_dedent;
    int bol;
} RpylLexerState;

void rpyl_lexer_state_init(RpylLexerState* st, const char* label_keyword);
int rpyl_lex_stream_state(RpylLexerState* st, RpylStream* s, RpylToken* out);

/* Reset legacy global lexer state (line counter + indentation stack). */
void rpyl_lexer_reset(void);

/* Configure the legacy global keyword treated as TOK_LABEL (default: "label"). */
void rpyl_lexer_set_label_keyword(const char* kw);

/* Legacy global-state lexer. Prefer rpyl_lex_stream_state for context-local use. */
int rpyl_lex_stream(RpylStream* s, RpylToken* out);

#if RPYL_ENABLE_STDIO
/* Backwards-compatible FILE* wrapper. */
int rpyl_lex(FILE* f, RpylToken* out);
#endif

#ifdef __cplusplus
}
#endif

#endif
