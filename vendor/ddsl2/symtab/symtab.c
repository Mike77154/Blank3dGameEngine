#include "symtab/symtab.h"

#include <string.h>

static void sym_copy(char *dst, int cap, const char *src) {
    int n;
    if (!dst || cap <= 0) return;
    if (!src) src = "";
    n = (int)strlen(src);
    if (n > cap - 1) n = cap - 1;
    if (n > 0) memcpy(dst, src, (size_t)n);
    dst[n] = '\0';
}

void ddsl_symtab_init(ddsl_symtab *tab) {
    if (!tab) return;
    memset(tab, 0, sizeof(*tab));
}

int ddsl_symtab_put(ddsl_symtab *tab, const char *name, int id) {
    int i;
    int slot;
    if (!tab || !name) return 0;
    slot = -1;
    for (i = 0; i < DDSL_MAX_SYMBOLS; ++i) {
        if (tab->items[i].used) {
            if (strcmp(tab->items[i].name, name) == 0) {
                tab->items[i].id = id;
                return 1;
            }
        } else if (slot < 0) {
            slot = i;
        }
    }
    if (slot < 0) return 0;
    tab->items[slot].used = 1;
    tab->items[slot].id = id;
    sym_copy(tab->items[slot].name, (int)sizeof(tab->items[slot].name), name);
    tab->count++;
    return 1;
}

int ddsl_symtab_get(const ddsl_symtab *tab, const char *name, int *out_id) {
    int i;
    if (out_id) *out_id = 0;
    if (!tab || !name) return 0;
    for (i = 0; i < DDSL_MAX_SYMBOLS; ++i) {
        if (tab->items[i].used && strcmp(tab->items[i].name, name) == 0) {
            if (out_id) *out_id = tab->items[i].id;
            return 1;
        }
    }
    return 0;
}

int ddsl_symtab_count(const ddsl_symtab *tab) {
    if (!tab) return 0;
    return tab->count;
}
