#include "rpyl_registry.h"
#include "rpyl_common.h"

static int registry_find_index(const RpylRegistry* r, const char* name) {
    size_t i;
    if (r == 0 || name == 0) return -1;
    for (i = 0u; i < r->count; ++i) {
        if (rpyl_common_streq(r->entries[i].name, name)) return (int)i;
    }
    return -1;
}

void rpyl_registry_init(RpylRegistry* r) {
    if (r != 0) rpyl_registry_clear(r);
}

void rpyl_registry_clear(RpylRegistry* r) {
    size_t i;
    if (r == 0) return;
    for (i = 0u; i < RPYL_REGISTRY_MAX; i++) {
        r->entries[i].name[0] = '\0';
        r->entries[i].fn = 0;
        r->entries[i].user = 0;
        r->entries[i].flags = 0UL;
    }
    r->count = 0u;
    r->overflowed = 0;
}

int rpyl_registry_add_ex(RpylRegistry* r, const char* name, RpylRegistryFn fn, void* user, unsigned long flags) {
    int idx;
    RpylRegistryEntry* e;
    if (r == 0 || name == 0 || name[0] == '\0' || fn == 0) return 0;
    idx = registry_find_index(r, name);
    if (idx >= 0) {
        e = &r->entries[idx];
        e->fn = fn;
        e->user = user;
        e->flags = flags;
        return 1;
    }
    if (r->count >= (size_t)RPYL_REGISTRY_MAX) {
        r->overflowed = 1;
        return 0;
    }
    e = &r->entries[r->count];
    if (!rpyl_common_copy(e->name, sizeof(e->name), name)) {
        r->overflowed = 1;
        return 0;
    }
    e->fn = fn;
    e->user = user;
    e->flags = flags;
    r->count++;
    return 1;
}

int rpyl_registry_add(RpylRegistry* r, const char* name, RpylRegistryFn fn, void* user) {
    return rpyl_registry_add_ex(r, name, fn, user, 0UL);
}

RpylRegistryFn rpyl_registry_find(const RpylRegistry* r, const char* name, void** user_out) {
    int idx;
    idx = registry_find_index(r, name);
    if (idx < 0) return 0;
    if (user_out != 0) *user_out = r->entries[idx].user;
    return r->entries[idx].fn;
}

const RpylRegistryEntry* rpyl_registry_entry(const RpylRegistry* r, const char* name) {
    int idx;
    idx = registry_find_index(r, name);
    if (idx < 0) return 0;
    return &r->entries[idx];
}

int rpyl_registry_remove(RpylRegistry* r, const char* name) {
    int idx;
    size_t i;
    if (!r || !name) return 0;
    idx = registry_find_index(r, name);
    if (idx < 0) return 0;
    for (i = (size_t)idx; i + 1u < r->count; i++) r->entries[i] = r->entries[i + 1u];
    if (r->count > 0u) r->count--;
    r->entries[r->count].name[0] = '\0';
    r->entries[r->count].fn = 0;
    r->entries[r->count].user = 0;
    r->entries[r->count].flags = 0UL;
    return 1;
}

int rpyl_registry_dispatch(const RpylRegistry* r, const char* name, const char** args, int argc) {
    void* user;
    RpylRegistryFn fn;
    user = 0;
    fn = rpyl_registry_find(r, name, &user);
    if (!fn) return 0;
    fn(user, args, argc);
    return 1;
}

size_t rpyl_registry_count(const RpylRegistry* r) {
    if (!r) return 0u;
    return r->count;
}

const RpylRegistryEntry* rpyl_registry_at(const RpylRegistry* r, size_t index) {
    if (!r || index >= r->count) return 0;
    return &r->entries[index];
}

int rpyl_registry_overflowed(const RpylRegistry* r) {
    if (!r) return 0;
    return r->overflowed ? 1 : 0;
}
