/* flags_loader.c - loader - C89 */
#include "flags_loader.h"

#include <string.h>

static int _cstr_len(const char *s)
{
    int n;
    if (!s) return 0;
    n = 0;
    while (s[n] != '\0') n++;
    return n;
}

static int _cstr_endswith(const char *s, const char *suffix)
{
    int ls, lf, i;
    if (!s || !suffix) return 0;
    ls = _cstr_len(s);
    lf = _cstr_len(suffix);
    if (lf <= 0 || ls < lf) return 0;
    for (i = 0; i < lf; ++i) {
        if (s[ls - lf + i] != suffix[i]) return 0;
    }
    return 1;
}

void flags_loader_init(FlagsLoader *ldr,
                       const char *filepath,
                       FlagStore *flagstore,
                       FlagStore *mem_for_adds,
                       const char *prefix_for_adds,
                       int has_lo, flags_fx_t lo,
                       int has_hi, flags_fx_t hi,
                       const FlagsKeyMap *target_map, int target_map_count,
                       FlagsKV *state_items, int state_items_cap,
                       char *state_pool_buf, int state_pool_cap)
{
    if (!ldr) return;

    ldr->flagstore = flagstore;
    ldr->mem = mem_for_adds;
    ldr->prefix_for_adds = prefix_for_adds;

    ldr->has_lo = has_lo;
    ldr->lo = lo;
    ldr->has_hi = has_hi;
    ldr->hi = hi;

    ldr->target_map = target_map;
    ldr->target_map_count = target_map_count;

    flagstate_init(&ldr->state, filepath, state_items, state_items_cap, state_pool_buf, state_pool_cap);
}

static void _apply_to_flagstore(const FlagState *st, FlagStore *dst)
{
    int n, i;
    const FlagsKV *items;
    if (!st || !dst) return;

    items = flagstate_items(st, &n);
    for (i = 0; i < n; ++i) {
        const FlagsKV *kv;
        kv = &items[i];
        if (!kv->key) continue;
        if (_cstr_endswith(kv->key, FLAGS_ADD_SUFFIX)) continue;
        (void)flagstore_set(dst, kv->key, &kv->val);
    }
}

int flags_loader_load_once(FlagsLoader *ldr,
                           const FlagsIO *io,
                           char *file_buf, int file_buf_cap,
                           FlagsLogFn log_fn, void *log_user)
{
    int rc;
    if (!ldr) return -1;
    rc = flagstate_load(&ldr->state, io, file_buf, file_buf_cap, log_fn, log_user);
    if (rc == 1) {
        _apply_to_flagstore(&ldr->state, ldr->flagstore);
        if (ldr->mem) {
            flagstate_apply_adds(&ldr->state,
                                 ldr->mem,
                                 ldr->prefix_for_adds,
                                 ldr->has_lo, ldr->lo,
                                 ldr->has_hi, ldr->hi,
                                 ldr->target_map, ldr->target_map_count);
        }
    }
    return rc;
}
