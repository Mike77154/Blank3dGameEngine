#include "polysym/polysym.h"

#include <string.h>

static void ps_copy(char *dst, int cap, const char *src) {
    int n;
    if (!dst || cap <= 0) return;
    if (!src) src = "";
    n = (int)strlen(src);
    if (n > cap - 1) n = cap - 1;
    if (n > 0) memcpy(dst, src, (size_t)n);
    dst[n] = '\0';
}

void ddsl_polysym_init(ddsl_polysym *ps) {
    if (!ps) return;
    memset(ps, 0, sizeof(*ps));
}

int ddsl_polysym_put(ddsl_polysym *ps, const char *domain, const char *name, int id) {
    int i;
    int slot;
    if (!ps || !domain || !name) return 0;
    slot = -1;
    for (i = 0; i < DDSL_MAX_POLYSYMS; ++i) {
        if (ps->items[i].used) {
            if (strcmp(ps->items[i].domain, domain) == 0 && strcmp(ps->items[i].name, name) == 0) {
                ps->items[i].id = id;
                return 1;
            }
        } else if (slot < 0) {
            slot = i;
        }
    }
    if (slot < 0) return 0;
    ps->items[slot].used = 1;
    ps->items[slot].id = id;
    ps_copy(ps->items[slot].domain, (int)sizeof(ps->items[slot].domain), domain);
    ps_copy(ps->items[slot].name, (int)sizeof(ps->items[slot].name), name);
    ps->count++;
    return 1;
}

int ddsl_polysym_get(const ddsl_polysym *ps, const char *domain, const char *name, int *out_id) {
    int i;
    if (out_id) *out_id = 0;
    if (!ps || !domain || !name) return 0;
    for (i = 0; i < DDSL_MAX_POLYSYMS; ++i) {
        if (ps->items[i].used && strcmp(ps->items[i].domain, domain) == 0 && strcmp(ps->items[i].name, name) == 0) {
            if (out_id) *out_id = ps->items[i].id;
            return 1;
        }
    }
    return 0;
}
