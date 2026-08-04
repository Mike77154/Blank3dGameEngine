#ifndef DDSL_STORE_H
#define DDSL_STORE_H

#include <stddef.h> /* size_t */
#include "types/fixed.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DDSL_MAX_FLAGS
#define DDSL_MAX_FLAGS 256
#endif

#ifndef DDSL_MAX_KEY_LEN
#define DDSL_MAX_KEY_LEN 64
#endif

#ifndef DDSL_MAX_VALUE_LEN
#define DDSL_MAX_VALUE_LEN 128
#endif

typedef struct ddsl_entry {
    char key[DDSL_MAX_KEY_LEN];
    char value[DDSL_MAX_VALUE_LEN];
    int used;
} ddsl_entry;

typedef struct ddsl_store {
    ddsl_entry items[DDSL_MAX_FLAGS];
} ddsl_store;

typedef int (*ddsl_store_iter_fn)(const char *key, const char *value, void *user);

void ddsl_store_init(ddsl_store *st);
void ddsl_store_clear(ddsl_store *st);

void ddsl_norm_key(const char *src, char *dst, size_t dst_size);

int ddsl_store_set(ddsl_store *st, const char *key, const char *value);
const char *ddsl_store_get(const ddsl_store *st, const char *key);

int ddsl_store_has(const ddsl_store *st, const char *key);
int ddsl_store_unset(ddsl_store *st, const char *key);
int ddsl_store_count(const ddsl_store *st);
int ddsl_store_foreach(const ddsl_store *st, ddsl_store_iter_fn fn, void *user);

/* Helpers numéricos */
int ddsl_store_set_num(ddsl_store *st, const char *key, ddsl_fixed v);
ddsl_fixed ddsl_store_get_num(const ddsl_store *st, const char *key, ddsl_fixed fallback, int *ok);

#ifdef __cplusplus
}
#endif

#endif /* DDSL_STORE_H */
