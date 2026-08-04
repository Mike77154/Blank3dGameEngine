/* flags_state.h - parser/state for flags files (C89, no stdio, no malloc, no float)
 *
 * Rough C89 equivalent of flags_helper.py (FlagState) with:
 * - INI-like format: key=value, key+=delta, comments '#'
 * - JSON object format (flattened with dotted keys)
 */
#ifndef FLAGS_STATE_H
#define FLAGS_STATE_H

#include "flags_io.h"
#include "flags_pool.h"
#include "flags_value.h"
#include "flagstore.h"

#define FLAGS_ADD_SUFFIX "__add__"

typedef struct FlagsKV {
    const char *key;  /* points into state pool */
    FlagsValue val;   /* if STR, points into state pool */
} FlagsKV;

typedef struct FlagState {
    const char *path; /* not owned */
    long mtime;
    FlagsKV *items;
    int items_cap;
    int items_count;
    FlagsPool pool;
} FlagState;

/* Initialize FlagState with caller-provided storage. */
void flagstate_init(FlagState *st,
                    const char *path,
                    FlagsKV *items, int items_cap,
                    char *pool_buf, int pool_cap);

/* Try to load+parse file through FlagsIO.
 * Returns:
 *   1 => changed and parsed OK
 *   0 => no change / missing / not loaded
 *  -1 => read/parse error (mtime NOT updated)
 */
int flagstate_load(FlagState *st,
                   const FlagsIO *io,
                   char *file_buf, int file_buf_cap,
                   FlagsLogFn log_fn, void *log_user);

/* Query a parsed key (linear scan). Returns 1 if found. */
int flagstate_get(const FlagState *st, const char *key, FlagsValue *out_val);

/* Convenience boolean getter (matches python behavior):
 * - If missing: return default_val
 * - If bool: return it
 * - Else if string matches true/false patterns: use that
 * - Else: return default_val
 */
int flagstate_get_bool(const FlagState *st, const char *key, int default_val);

/* Export parsed items array (read-only). */
const FlagsKV *flagstate_items(const FlagState *st, int *out_count);

/* Apply "+=" deltas stored as keys ending in "__add__".
 * - mem: destination store (numbers kept as fixed-point)
 * - prefix: if non-null, only apply deltas whose base key starts with prefix
 * - clamp: optional, in fixed-point
 * - target_map: optional mapping from base_key -> dst_key (like python's target_map dict)
 */
typedef struct FlagsKeyMap {
    const char *src;
    const char *dst;
} FlagsKeyMap;

void flagstate_apply_adds(const FlagState *st,
                          FlagStore *mem,
                          const char *prefix,
                          int has_lo, flags_fx_t lo,
                          int has_hi, flags_fx_t hi,
                          const FlagsKeyMap *target_map, int target_map_count);

#endif /* FLAGS_STATE_H */
