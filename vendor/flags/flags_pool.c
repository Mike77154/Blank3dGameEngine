/* flags_pool.c - simple bump string pool (no malloc) - C89 */
#include "flags_pool.h"

static int _cstr_len(const char *s)
{
    int n;
    if (!s) return 0;
    n = 0;
    while (s[n] != '\0') n++;
    return n;
}

void flags_pool_init(FlagsPool *p, char *buf, int cap)
{
    if (!p) return;
    p->buf = buf;
    p->cap = cap;
    p->used = 0;
}

void flags_pool_reset(FlagsPool *p)
{
    if (!p) return;
    p->used = 0;
}

const char *flags_pool_dup(FlagsPool *p, const char *s, int len)
{
    int i;
    char *dst;
    if (!p || !p->buf || p->cap <= 0) return 0;
    if (!s) s = "";
    if (len < 0) len = 0;

    /* +1 for NUL */
    if (p->used + len + 1 > p->cap) return 0;

    dst = p->buf + p->used;
    for (i = 0; i < len; ++i) {
        dst[i] = s[i];
    }
    dst[len] = '\0';

    p->used += (len + 1);
    return (const char *)dst;
}

const char *flags_pool_dup_cstr(FlagsPool *p, const char *s)
{
    int n;
    n = _cstr_len(s);
    return flags_pool_dup(p, s, n);
}

const char *flags_pool_cat2(FlagsPool *p, const char *a, const char *b)
{
    int la, lb, i;
    char *dst;
    if (!p || !p->buf) return 0;
    if (!a) a = "";
    if (!b) b = "";

    la = _cstr_len(a);
    lb = _cstr_len(b);

    if (p->used + la + lb + 1 > p->cap) return 0;
    dst = p->buf + p->used;

    for (i = 0; i < la; ++i) dst[i] = a[i];
    for (i = 0; i < lb; ++i) dst[la + i] = b[i];
    dst[la + lb] = '\0';

    p->used += (la + lb + 1);
    return (const char *)dst;
}
