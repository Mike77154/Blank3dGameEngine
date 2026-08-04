#ifndef DDSL_UTIL_H
#define DDSL_UTIL_H

#include <stddef.h>
#include "common/strview.h"

#ifdef __cplusplus
extern "C" {
#endif

int ddsl_util_cstr_len(const char *s);
int ddsl_util_copy_cstr(char *dst, int dst_cap, const char *src);
int ddsl_util_sv_eq_ci(ddsl_strview sv, const char *cstr);

#ifdef __cplusplus
}
#endif

#endif /* DDSL_UTIL_H */
