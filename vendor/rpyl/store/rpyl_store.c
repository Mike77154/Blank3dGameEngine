#include "rpyl_store.h"
#include "rpyl_common.h"

static int store_find_index(const RpylStore* s, const char* key) {
    size_t i;
    if (s == 0 || key == 0) return -1;
    for (i = 0u; i < s->count; ++i) {
        if (rpyl_common_streq(s->items[i].key, key)) return (int)i;
    }
    return -1;
}

void rpyl_store_init(RpylStore* s) {
    if (s != 0) rpyl_store_clear(s);
}

void rpyl_store_clear(RpylStore* s) {
    size_t i;
    if (s == 0) return;
    for (i = 0u; i < RPYL_STORE_MAX; i++) {
        s->items[i].key[0] = '\0';
        s->items[i].value[0] = '\0';
        s->items[i].flags = 0UL;
    }
    s->count = 0u;
    s->overflowed = 0;
}

int rpyl_store_set_ex(RpylStore* s, const char* key, const char* value, unsigned long flags) {
    int idx;
    RpylStoreItem* item;
    if (s == 0 || key == 0 || key[0] == '\0') return 0;
    if (!value) value = "";
    idx = store_find_index(s, key);
    if (idx >= 0) {
        item = &s->items[idx];
        if (!rpyl_common_copy(item->value, sizeof(item->value), value)) {
            s->overflowed = 1;
            return 0;
        }
        item->flags = flags;
        return 1;
    }
    if (s->count >= (size_t)RPYL_STORE_MAX) {
        s->overflowed = 1;
        return 0;
    }
    item = &s->items[s->count];
    if (!rpyl_common_copy(item->key, sizeof(item->key), key)) {
        s->overflowed = 1;
        return 0;
    }
    if (!rpyl_common_copy(item->value, sizeof(item->value), value)) {
        item->key[0] = '\0';
        s->overflowed = 1;
        return 0;
    }
    item->flags = flags;
    s->count++;
    return 1;
}

int rpyl_store_set(RpylStore* s, const char* key, const char* value) {
    return rpyl_store_set_ex(s, key, value, 0UL);
}

const char* rpyl_store_get(const RpylStore* s, const char* key) {
    int idx;
    idx = store_find_index(s, key);
    if (idx < 0) return 0;
    return s->items[idx].value;
}

const RpylStoreItem* rpyl_store_find(const RpylStore* s, const char* key) {
    int idx;
    idx = store_find_index(s, key);
    if (idx < 0) return 0;
    return &s->items[idx];
}

int rpyl_store_remove(RpylStore* s, const char* key) {
    int idx;
    size_t i;
    if (!s || !key) return 0;
    idx = store_find_index(s, key);
    if (idx < 0) return 0;
    for (i = (size_t)idx; i + 1u < s->count; i++) s->items[i] = s->items[i + 1u];
    if (s->count > 0u) s->count--;
    s->items[s->count].key[0] = '\0';
    s->items[s->count].value[0] = '\0';
    s->items[s->count].flags = 0UL;
    return 1;
}

int rpyl_store_contains(const RpylStore* s, const char* key) {
    return store_find_index(s, key) >= 0 ? 1 : 0;
}

size_t rpyl_store_count(const RpylStore* s) {
    if (!s) return 0u;
    return s->count;
}

const RpylStoreItem* rpyl_store_at(const RpylStore* s, size_t index) {
    if (!s || index >= s->count) return 0;
    return &s->items[index];
}

int rpyl_store_copy(RpylStore* dst, const RpylStore* src) {
    size_t i;
    if (!dst || !src) return 0;
    rpyl_store_clear(dst);
    for (i = 0u; i < src->count; i++) {
        if (!rpyl_store_set_ex(dst, src->items[i].key, src->items[i].value, src->items[i].flags)) return 0;
    }
    return 1;
}

int rpyl_store_overflowed(const RpylStore* s) {
    if (!s) return 0;
    return s->overflowed ? 1 : 0;
}
