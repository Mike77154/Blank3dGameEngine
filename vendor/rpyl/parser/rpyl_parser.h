#ifndef RPYL_PARSER_H
#define RPYL_PARSER_H

#include <stddef.h>

#include "rpyl_config.h"
#if RPYL_ENABLE_STDIO
#include <stdio.h>
#endif

#include "rpyl_ast.h"
#include "rpyl_stream.h"

#ifdef __cplusplus
extern "C" {
#endif

AstNode* rpyl_parse_stream_ex_with_label(RpylStream* s, RpylArena* arena, const char* label_keyword);
AstNode* rpyl_parse_stream_ex(RpylStream* s, RpylArena* arena);
AstNode* rpyl_parse_stream(RpylStream* s);

#if RPYL_ENABLE_STDIO
AstNode* rpyl_parse_ex_with_label(FILE* f, RpylArena* arena, const char* label_keyword);
AstNode* rpyl_parse_ex(FILE* f, RpylArena* arena);
AstNode* rpyl_parse(FILE* f);
#endif

AstNode* rpyl_parse_buffer_ex_with_label(const char* buf, size_t len, RpylArena* arena, const char* label_keyword);
AstNode* rpyl_parse_buffer_ex(const char* buf, size_t len, RpylArena* arena);
AstNode* rpyl_parse_buffer(const char* buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif
