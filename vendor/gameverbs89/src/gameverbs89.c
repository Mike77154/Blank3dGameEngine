#include "gameverbs89.h"
#include <string.h>

static void gverb89_copy(char *dst, int cap, const char *src)
{
    int i;
    if (!dst || cap <= 0) return;
    if (!src) src = "";
    i = 0;
    while (src[i] != '\0' && i + 1 < cap) {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
}

void gverb89_init(gverb89_registry *registry)
{
    if (!registry) return;
    memset(registry, 0, sizeof(*registry));
}

void gverb89_clear(gverb89_registry *registry)
{
    gverb89_init(registry);
}

static int gverb89_register(gverb89_registry *registry, int kind, const char *name,
                         gverb89_condition_fn condition, gverb89_action_fn action,
                         void *user)
{
    int i;
    int free_slot;
    if (!registry || !name || !*name) return 0;
    if (kind == GVERB89_KIND_CONDITION && !condition) return 0;
    if (kind == GVERB89_KIND_ACTION && !action) return 0;
    free_slot = -1;
    for (i = 0; i < GVERB89_MAX_ENTRIES; ++i) {
        if (registry->entries[i].used) {
            if (registry->entries[i].kind == kind &&
                strcmp(registry->entries[i].name, name) == 0) {
                registry->entries[i].condition = condition;
                registry->entries[i].action = action;
                registry->entries[i].user = user;
                return 1;
            }
        } else if (free_slot < 0) free_slot = i;
    }
    if (free_slot < 0) return 0;
    registry->entries[free_slot].used = 1;
    registry->entries[free_slot].kind = kind;
    registry->entries[free_slot].condition = condition;
    registry->entries[free_slot].action = action;
    registry->entries[free_slot].user = user;
    gverb89_copy(registry->entries[free_slot].name, GVERB89_NAME_CAP, name);
    ++registry->count;
    return 1;
}

int gverb89_register_condition(gverb89_registry *registry, const char *name,
                            gverb89_condition_fn fn, void *user)
{
    return gverb89_register(registry, GVERB89_KIND_CONDITION, name, fn, 0, user);
}

int gverb89_register_action(gverb89_registry *registry, const char *name,
                         gverb89_action_fn fn, void *user)
{
    return gverb89_register(registry, GVERB89_KIND_ACTION, name, 0, fn, user);
}

static gverb89_entry *gverb89_find(gverb89_registry *registry, int kind,
                             const char *name)
{
    int i;
    if (!registry || !name) return 0;
    for (i = 0; i < GVERB89_MAX_ENTRIES; ++i)
        if (registry->entries[i].used && registry->entries[i].kind == kind &&
            strcmp(registry->entries[i].name, name) == 0)
            return &registry->entries[i];
    return 0;
}

int gverb89_has(const gverb89_registry *registry, int kind, const char *name)
{
    int i;
    if (!registry || !name) return 0;
    for (i = 0; i < GVERB89_MAX_ENTRIES; ++i)
        if (registry->entries[i].used && registry->entries[i].kind == kind &&
            strcmp(registry->entries[i].name, name) == 0) return 1;
    return 0;
}

int gverb89_query(gverb89_registry *registry, const gverb89_call *call,
               gverb89_result *out)
{
    gverb89_entry *entry;
    if (out) memset(out, 0, sizeof(*out));
    if (!registry || !call || !call->name) return GVERB89_ERROR;
    entry = gverb89_find(registry, GVERB89_KIND_CONDITION, call->name);
    if (!entry || !entry->condition) return GVERB89_UNHANDLED;
    return entry->condition(entry->user, call, out);
}

int gverb89_perform(gverb89_registry *registry, const gverb89_call *call)
{
    gverb89_entry *entry;
    if (!registry || !call || !call->name) return GVERB89_ERROR;
    entry = gverb89_find(registry, GVERB89_KIND_ACTION, call->name);
    if (!entry || !entry->action) return GVERB89_UNHANDLED;
    return entry->action(entry->user, call);
}

int gverb89_count(const gverb89_registry *registry, int kind)
{
    int i;
    int count;
    if (!registry) return 0;
    count = 0;
    for (i = 0; i < GVERB89_MAX_ENTRIES; ++i)
        if (registry->entries[i].used && registry->entries[i].kind == kind)
            ++count;
    return count;
}

const char *gverb89_name_at(const gverb89_registry *registry, int kind, int ordinal)
{
    int i;
    int count;
    if (!registry || ordinal < 0) return 0;
    count = 0;
    for (i = 0; i < GVERB89_MAX_ENTRIES; ++i) {
        if (registry->entries[i].used && registry->entries[i].kind == kind) {
            if (count == ordinal) return registry->entries[i].name;
            ++count;
        }
    }
    return 0;
}
