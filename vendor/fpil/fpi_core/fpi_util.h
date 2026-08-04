#ifndef FPI_UTIL_H
#define FPI_UTIL_H

#include "fpi_types.h"

char fpi_ascii_tolower(char c);
int fpi_ascii_is_space(char c);
int fpi_ascii_is_digit(char c);
int fpi_ascii_is_ident_start(char c);
int fpi_ascii_is_ident_continue(char c);
int fpi_str_ieq(const char* a, const char* b);
int fpi_str_copy(char* dst, int cap, const char* src);
int fpi_str_copy_lower(char* dst, int cap, const char* src);
FPI_U32 fpi_hash_lower(const char* text);
long fpi_long_abs_sat(long value);

#endif /* FPI_UTIL_H */
