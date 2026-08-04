#ifndef RPYL_COMMON_H
#define RPYL_COMMON_H

#include <stddef.h>

#define RPYL_UNUSED(x) ((void)(x))
#define RPYL_ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))

#ifdef __cplusplus
extern "C" {
#endif

const char* rpyl_common_build_profile(void);
const char* rpyl_common_version(void);

size_t rpyl_common_strlen(const char* s);
int rpyl_common_streq(const char* a, const char* b);
int rpyl_common_streq_nocase_ascii(const char* a, const char* b);
int rpyl_common_copy(char* dst, size_t dst_size, const char* src);
int rpyl_common_append(char* dst, size_t dst_size, const char* src);
int rpyl_common_append_char(char* dst, size_t dst_size, char ch);
unsigned long rpyl_common_hash(const char* s);
size_t rpyl_common_align_up(size_t value, size_t align, int* ok);
int rpyl_common_is_space(int c);
int rpyl_common_is_digit(int c);
int rpyl_common_is_ident_start(int c);
int rpyl_common_is_ident_char(int c);
int rpyl_common_is_identifier(const char* s);
int rpyl_common_parse_ulong(const char* s, unsigned long* out_value);
int rpyl_common_parse_long(const char* s, long* out_value);

#ifdef __cplusplus
}
#endif

#endif
