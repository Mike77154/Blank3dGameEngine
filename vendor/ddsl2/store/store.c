#include "store/store.h"

#include <string.h>
#include <ctype.h>
static void ddsl_snake_case(const char *src, char *dst, size_t dst_size) {
    size_t i;
    size_t j;
    int last_underscore;

    if (!dst || dst_size == 0) return;
    if (!src) {
        dst[0] = '\0';
        return;
    }

    j = 0;
    last_underscore = 0;

    for (i = 0; src[i] != '\0' && j + 1 < dst_size; ++i) {
        unsigned char c = (unsigned char)src[i];
        if (isalnum(c)) {
            dst[j++] = (char)tolower(c);
            last_underscore = 0;
        } else {
            if (!last_underscore && j > 0) {
                dst[j++] = '_';
                last_underscore = 1;
            }
        }
    }

    if (j > 0 && dst[j - 1] == '_') j--;
    dst[j] = '\0';

    if (j == 0 && dst_size > 1) {
        dst[0] = '_';
        dst[1] = '\0';
    }
}

void ddsl_norm_key(const char *src, char *dst, size_t dst_size) {
    ddsl_snake_case(src, dst, dst_size);
}

void ddsl_store_init(ddsl_store *st) {
    if (!st) return;
    memset(st, 0, sizeof(*st));
}

void ddsl_store_clear(ddsl_store *st) {
    ddsl_store_init(st);
}

static const char *ddsl_get_norm(const ddsl_store *st, const char *key_norm) {
    int i;
    if (!st || !key_norm) return NULL;
    for (i = 0; i < DDSL_MAX_FLAGS; ++i) {
        if (st->items[i].used && strcmp(st->items[i].key, key_norm) == 0) {
            return st->items[i].value;
        }
    }
    return NULL;
}

static void ddsl_copy_text(char *dst, int capacity, const char *src) {
    int i;
    if (!dst || capacity <= 0) return;
    if (!src) src = "";
    i = 0;
    while (i + 1 < capacity && src[i] != '\0') {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
}

static int ddsl_set_norm(ddsl_store *st, const char *key_norm, const char *value) {
    int i;
    if (!st || !key_norm || !value) return 0;

    for (i = 0; i < DDSL_MAX_FLAGS; ++i) {
        if (st->items[i].used && strcmp(st->items[i].key, key_norm) == 0) {
            ddsl_copy_text(st->items[i].value, DDSL_MAX_VALUE_LEN, value);
            return 1;
        }
    }

    for (i = 0; i < DDSL_MAX_FLAGS; ++i) {
        if (!st->items[i].used) {
            st->items[i].used = 1;
            ddsl_copy_text(st->items[i].key, DDSL_MAX_KEY_LEN, key_norm);
            ddsl_copy_text(st->items[i].value, DDSL_MAX_VALUE_LEN, value);
            return 1;
        }
    }

    return 0;
}

int ddsl_store_set(ddsl_store *st, const char *key, const char *value) {
    char key_norm[DDSL_MAX_KEY_LEN];
    if (!st || !key || !value) return 0;
    ddsl_norm_key(key, key_norm, sizeof(key_norm));
    return ddsl_set_norm(st, key_norm, value);
}

const char *ddsl_store_get(const ddsl_store *st, const char *key) {
    char key_norm[DDSL_MAX_KEY_LEN];
    if (!st || !key) return NULL;
    ddsl_norm_key(key, key_norm, sizeof(key_norm));
    return ddsl_get_norm(st, key_norm);
}

int ddsl_store_has(const ddsl_store *st, const char *key) {
    return ddsl_store_get(st, key) != NULL;
}

int ddsl_store_unset(ddsl_store *st, const char *key) {
    int i;
    char key_norm[DDSL_MAX_KEY_LEN];
    if (!st || !key) return 0;
    ddsl_norm_key(key, key_norm, sizeof(key_norm));

    for (i = 0; i < DDSL_MAX_FLAGS; ++i) {
        if (st->items[i].used && strcmp(st->items[i].key, key_norm) == 0) {
            st->items[i].used = 0;
            st->items[i].key[0] = '\0';
            st->items[i].value[0] = '\0';
            return 1;
        }
    }
    return 0;
}

int ddsl_store_count(const ddsl_store *st) {
    int i;
    int c;
    if (!st) return 0;
    c = 0;
    for (i = 0; i < DDSL_MAX_FLAGS; ++i) if (st->items[i].used) c++;
    return c;
}

int ddsl_store_foreach(const ddsl_store *st, ddsl_store_iter_fn fn, void *user) {
    int i;
    int visited;
    if (!st || !fn) return 0;
    visited = 0;
    for (i = 0; i < DDSL_MAX_FLAGS; ++i) {
        if (st->items[i].used) {
            visited++;
            if (!fn(st->items[i].key, st->items[i].value, user)) break;
        }
    }
    return visited;
}

int ddsl_store_set_num(ddsl_store *st, const char *key, ddsl_fixed v) {
    char buf[64];
    if (!st || !key) return 0;
    ddsl_fixed_to_cstr(v, buf, (int)sizeof(buf));
    return ddsl_store_set(st, key, buf);
}

static ddsl_fixed ddsl_parse_fixed_value(const char *s, int *ok) {
    ddsl_fixed v;
    if (!s) {
        if (ok) *ok = 0;
        return DDSL_FIXED_ZERO;
    }
    if (!ddsl_fixed_parse_cstr(s, &v)) {
        if (ok) *ok = 0;
        return DDSL_FIXED_ZERO;
    }
    if (ok) *ok = 1;
    return v;
}

ddsl_fixed ddsl_store_get_num(const ddsl_store *st, const char *key, ddsl_fixed fallback, int *ok) {
    const char *v;
    int ok_local;
    ddsl_fixed d;

    if (ok) *ok = 0;
    if (!st || !key) return fallback;

    v = ddsl_store_get(st, key);
    if (!v) return fallback;

    d = ddsl_parse_fixed_value(v, &ok_local);
    if (!ok_local) return fallback;

    if (ok) *ok = 1;
    return d;
}
