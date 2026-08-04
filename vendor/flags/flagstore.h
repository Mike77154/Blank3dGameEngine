/* flagstore.h - fixed-size key/value store (no malloc, no stdio) - C89
 *
 * Rough C89 equivalent of flagstore.py
 */
#ifndef FLAGSTORE_H
#define FLAGSTORE_H

#include "flags_pool.h"
#include "flags_value.h"

typedef struct FlagStoreEntry {
    unsigned long hash;
    const char *key;     /* points into store pool */
    FlagsValue val;      /* if STR, points into store pool */
    int used;            /* 0=empty, 1=occupied */
} FlagStoreEntry;

typedef struct FlagStore {
    FlagStoreEntry *entries;
    int cap;
    int count;
    FlagsPool pool;
} FlagStore;

/* Initialize store with caller-provided entry array and pool buffer.
 * entries_cap: number of slots in entries[]
 */
void flagstore_init(FlagStore *st,
                    FlagStoreEntry *entries, int entries_cap,
                    char *pool_buf, int pool_cap);

/* Clear all keys/values (also resets pool). */
void flagstore_clear(FlagStore *st);

/* Set any value type. Returns 1 on success, 0 on failure (OOM or table full). */
int flagstore_set(FlagStore *st, const char *key, const FlagsValue *val);

/* Convenience setters */
int flagstore_set_bool(FlagStore *st, const char *key, int b);
int flagstore_set_int(FlagStore *st, const char *key, long i);
int flagstore_set_fx(FlagStore *st, const char *key, flags_fx_t fx);
int flagstore_set_str(FlagStore *st, const char *key, const char *s);

/* Get value. Returns 1 if found, 0 if missing. */
int flagstore_get(const FlagStore *st, const char *key, FlagsValue *out_val);

/* Toggle bool key (missing -> false -> true). Returns new value in out_new (optional). */
int flagstore_toggle(FlagStore *st, const char *key, int *out_new);

/* Serialize store to JSON text (flat object).
 * Returns number of bytes written (excluding trailing NUL), or 0 on overflow/error.
 * Note: fixed-point values are written as JSON numbers with 4 fractional digits.
 */
int flagstore_to_json(const FlagStore *st, char *out, int out_cap, int pretty);

#endif /* FLAGSTORE_H */
