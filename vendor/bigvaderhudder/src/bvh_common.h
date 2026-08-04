#ifndef BVH_COMMON_H_INCLUDED
#define BVH_COMMON_H_INCLUDED

#include "../include/bvh.h"
#include <string.h>
#include <ctype.h>

#define BVH_UNUSED(x) (void)(x)
#define BVH_TRUE 1
#define BVH_FALSE 0

void bvh_error_set(BVH_Error *err, int line, int col, const char *msg);
int bvh_slice_eq_lit(const char *s, unsigned long n, const char *lit);
int bvh_lit_eq(const char *a, const char *b);
unsigned long bvh_cstr_len(const char *s);
void bvh_copy_slice(char *dst, unsigned long cap, const char *s, unsigned long n);
int bvh_is_space_no_eol(int c);
int bvh_is_digit_ch(int c);
int bvh_is_hex_ch(int c);
int bvh_hex_value(int c);

#endif
