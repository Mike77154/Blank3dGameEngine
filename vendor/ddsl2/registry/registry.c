#include "registry/registry.h"

#include <string.h>

static void reg_copy(char *dst, int cap, const char *src) {
    int n;
    if (!dst || cap <= 0) return;
    if (!src) src = "";
    n = (int)strlen(src);
    if (n > cap - 1) n = cap - 1;
    if (n > 0) memcpy(dst, src, (size_t)n);
    dst[n] = '\0';
}

void ddsl_registry_init(ddsl_registry *r) {
    if (!r) return;
    memset(r, 0, sizeof(*r));
}

int ddsl_registry_add(ddsl_registry *r, const char *name, ddsl_registry_fn fn, void *user) {
    int i;
    int slot;
    if (!r || !name || !fn) return 0;
    slot = -1;
    for (i = 0; i < DDSL_MAX_REGISTRY_ITEMS; ++i) {
        if (r->items[i].used) {
            if (strcmp(r->items[i].name, name) == 0) {
                r->items[i].fn = fn;
                r->items[i].user = user;
                return 1;
            }
        } else if (slot < 0) {
            slot = i;
        }
    }
    if (slot < 0) return 0;
    r->items[slot].used = 1;
    r->items[slot].fn = fn;
    r->items[slot].user = user;
    reg_copy(r->items[slot].name, (int)sizeof(r->items[slot].name), name);
    return 1;
}

int ddsl_registry_find(const ddsl_registry *r, const char *name, ddsl_registry_fn *out_fn, void **out_user) {
    int i;
    if (out_fn) *out_fn = 0;
    if (out_user) *out_user = 0;
    if (!r || !name) return 0;
    for (i = 0; i < DDSL_MAX_REGISTRY_ITEMS; ++i) {
        if (r->items[i].used && strcmp(r->items[i].name, name) == 0) {
            if (out_fn) *out_fn = r->items[i].fn;
            if (out_user) *out_user = r->items[i].user;
            return 1;
        }
    }
    return 0;
}
