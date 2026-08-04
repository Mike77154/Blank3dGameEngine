/* flags_pool.h - simple bump string pool (no malloc) - C89 */
#ifndef FLAGS_POOL_H
#define FLAGS_POOL_H

#include "flags_config.h"

typedef struct FlagsPool {
    char *buf;
    int cap;
    int used;
} FlagsPool;

/* Initialize pool with caller-provided buffer. */
void flags_pool_init(FlagsPool *p, char *buf, int cap);

/* Reset pool (invalidates all previous pointers). */
void flags_pool_reset(FlagsPool *p);

/* Duplicate a byte string (len bytes) into pool and NUL-terminate.
 * Returns pointer to stored string, or 0 on OOM.
 */
const char *flags_pool_dup(FlagsPool *p, const char *s, int len);

/* Duplicate a C-string (up to NUL) into pool.
 * Returns pointer to stored string, or 0 on OOM.
 */
const char *flags_pool_dup_cstr(FlagsPool *p, const char *s);

/* Duplicate by formatting concatenation: a + b (both are cstr).
 * Returns pointer or 0 on OOM.
 */
const char *flags_pool_cat2(FlagsPool *p, const char *a, const char *b);

#endif /* FLAGS_POOL_H */
