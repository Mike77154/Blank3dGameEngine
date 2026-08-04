#include "rpyl_symtab.h"
#include "rpyl_common.h"

static int sym_find_index(const RpylSymtab* t, const char* name) {
    size_t i;
    if (t == 0 || name == 0) return -1;
    for (i = 0u; i < t->count; ++i) {
        if (rpyl_common_streq(t->items[i].name, name)) return (int)i;
    }
    return -1;
}

void rpyl_symtab_init(RpylSymtab* t) {
    if (t != 0) rpyl_symtab_clear(t);
}

void rpyl_symtab_clear(RpylSymtab* t) {
    size_t i;
    if (t == 0) return;
    for (i = 0u; i < RPYL_SYMTAB_MAX; i++) {
        t->items[i].name[0] = '\0';
        t->items[i].value = 0UL;
        t->items[i].kind = RPYL_SYM_OTHER;
        t->items[i].flags = 0UL;
    }
    t->count = 0u;
    t->overflowed = 0;
}

int rpyl_symtab_put_kind(RpylSymtab* t, const char* name, unsigned long value, RpylSymKind kind, unsigned long flags) {
    int idx;
    RpylSym* s;
    if (t == 0 || name == 0 || name[0] == '\0') return 0;
    idx = sym_find_index(t, name);
    if (idx >= 0) {
        s = &t->items[idx];
        s->value = value;
        s->kind = kind;
        s->flags = flags;
        return 1;
    }
    if (t->count >= (size_t)RPYL_SYMTAB_MAX) {
        t->overflowed = 1;
        return 0;
    }
    s = &t->items[t->count];
    if (!rpyl_common_copy(s->name, sizeof(s->name), name)) {
        t->overflowed = 1;
        return 0;
    }
    s->value = value;
    s->kind = kind;
    s->flags = flags;
    t->count++;
    return 1;
}

int rpyl_symtab_put(RpylSymtab* t, const char* name, unsigned long value) {
    return rpyl_symtab_put_kind(t, name, value, RPYL_SYM_OTHER, 0UL);
}

int rpyl_symtab_get(const RpylSymtab* t, const char* name, unsigned long* value_out) {
    int idx;
    idx = sym_find_index(t, name);
    if (idx < 0) return 0;
    if (value_out != 0) *value_out = t->items[idx].value;
    return 1;
}

int rpyl_symtab_get_kind(const RpylSymtab* t, const char* name, RpylSymKind kind, unsigned long* value_out) {
    int idx;
    idx = sym_find_index(t, name);
    if (idx < 0) return 0;
    if (t->items[idx].kind != kind) return 0;
    if (value_out != 0) *value_out = t->items[idx].value;
    return 1;
}

int rpyl_symtab_contains(const RpylSymtab* t, const char* name) {
    return sym_find_index(t, name) >= 0 ? 1 : 0;
}

int rpyl_symtab_remove(RpylSymtab* t, const char* name) {
    int idx;
    size_t i;
    if (!t || !name) return 0;
    idx = sym_find_index(t, name);
    if (idx < 0) return 0;
    for (i = (size_t)idx; i + 1u < t->count; i++) {
        t->items[i] = t->items[i + 1u];
    }
    if (t->count > 0u) t->count--;
    t->items[t->count].name[0] = '\0';
    t->items[t->count].value = 0UL;
    t->items[t->count].kind = RPYL_SYM_OTHER;
    t->items[t->count].flags = 0UL;
    return 1;
}

size_t rpyl_symtab_count(const RpylSymtab* t) {
    if (!t) return 0u;
    return t->count;
}

const RpylSym* rpyl_symtab_at(const RpylSymtab* t, size_t index) {
    if (!t || index >= t->count) return 0;
    return &t->items[index];
}

int rpyl_symtab_overflowed(const RpylSymtab* t) {
    if (!t) return 0;
    return t->overflowed ? 1 : 0;
}
