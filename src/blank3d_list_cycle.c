#include "blank3d_list_cycle.h"

#include <ctype.h>
#include <string.h>

static void b3d_list_name(const char *source, char *out, int capacity)
{
    int i;
    int j;
    int separator;
    unsigned char c;
    if (!out || capacity <= 0) return;
    if (!source) source = "";
    i = 0;
    j = 0;
    separator = 0;
    while (source[i] != '\0' && j + 1 < capacity) {
        c = (unsigned char)source[i++];
        if (isalnum(c)) {
            out[j++] = (char)tolower(c);
            separator = 0;
        } else if (!separator && j > 0) {
            out[j++] = '_';
            separator = 1;
        }
    }
    if (j > 0 && out[j - 1] == '_') --j;
    out[j] = '\0';
}

static int b3d_list_find(const Blank3DListCycleRegistry *registry,
                         const char *name)
{
    char normalized[B3D_LIST_CYCLE_NAME_CAP];
    int i;
    if (!registry || !name) return -1;
    b3d_list_name(name, normalized, (int)sizeof(normalized));
    for (i = 0; i < B3D_LIST_CYCLE_MAX_LISTS; ++i) {
        if (registry->bindings[i].used &&
            strcmp(registry->bindings[i].name, normalized) == 0)
            return i;
    }
    return -1;
}

void blank3d_list_cycle_init(Blank3DListCycleRegistry *registry)
{
    if (!registry) return;
    memset(registry, 0, sizeof(*registry));
}

int blank3d_list_cycle_register(Blank3DListCycleRegistry *registry,
                                const char *name,
                                const Cycler89List *list,
                                void *context,
                                Blank3DListCurrentFn current,
                                Blank3DListActivateFn activate)
{
    char normalized[B3D_LIST_CYCLE_NAME_CAP];
    int slot;
    int i;
    if (!registry || !name || !list || !list->count || !list->read ||
        !current || !activate)
        return 0;
    b3d_list_name(name, normalized, (int)sizeof(normalized));
    if (normalized[0] == '\0') return 0;
    slot = b3d_list_find(registry, normalized);
    if (slot < 0) {
        slot = -1;
        for (i = 0; i < B3D_LIST_CYCLE_MAX_LISTS; ++i) {
            if (!registry->bindings[i].used) {
                slot = i;
                break;
            }
        }
        if (slot < 0) return 0;
        registry->bindings[slot].used = 1;
        ++registry->count;
    }
    strncpy(registry->bindings[slot].name, normalized,
            sizeof(registry->bindings[slot].name) - 1U);
    registry->bindings[slot].name[
        sizeof(registry->bindings[slot].name) - 1U] = '\0';
    registry->bindings[slot].list = *list;
    registry->bindings[slot].context = context;
    registry->bindings[slot].current = current;
    registry->bindings[slot].activate = activate;
    return 1;
}

int blank3d_list_cycle_step(Blank3DListCycleRegistry *registry,
                            const char *name,
                            int direction,
                            cycler89_item *selected_out)
{
    Blank3DListCycleBinding *binding;
    cycler89_item current;
    cycler89_item selected;
    int has_current;
    int result;
    int slot;
    if (!registry || !name) return 0;
    slot = b3d_list_find(registry, name);
    if (slot < 0) return 0;
    binding = &registry->bindings[slot];
    current = 0L;
    has_current = binding->current(binding->context, &current) ? 1 : 0;
    result = cycler89_step(&binding->list, current, has_current,
                           direction, &selected, 0);
    if (result != CYCLER89_OK) return 0;
    if (!binding->activate(binding->context, selected)) return 0;
    if (selected_out) *selected_out = selected;
    return 1;
}

int blank3d_list_cycle_next(Blank3DListCycleRegistry *registry,
                            const char *name,
                            cycler89_item *selected_out)
{
    return blank3d_list_cycle_step(registry, name, 1, selected_out);
}

int blank3d_list_cycle_prev(Blank3DListCycleRegistry *registry,
                            const char *name,
                            cycler89_item *selected_out)
{
    return blank3d_list_cycle_step(registry, name, -1, selected_out);
}
