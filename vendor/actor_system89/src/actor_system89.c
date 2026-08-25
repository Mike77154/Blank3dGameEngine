#include "actor_system89.h"
#include <string.h>

void as89_init(AS89_System *system)
{
    if (!system) return;
    memset(system, 0, sizeof(*system));
}

void as89_set_provider(AS89_System *system, const AS89_Provider *provider)
{
    if (!system) return;
    memset(&system->provider, 0, sizeof(system->provider));
    if (provider) system->provider = *provider;
}

int as89_find_slot(const AS89_System *system, int actor_id)
{
    int i;
    if (!system) return AS89_INVALID_INDEX;
    for (i = 0; i < AS89_MAX_ACTORS; ++i)
        if (system->actors[i].used && system->actors[i].actor_id == actor_id)
            return i;
    return AS89_INVALID_INDEX;
}

AS89_Actor *as89_find(AS89_System *system, int actor_id)
{
    int slot;
    slot = as89_find_slot(system, actor_id);
    return slot >= 0 ? &system->actors[slot] : 0;
}

const AS89_Actor *as89_find_const(const AS89_System *system, int actor_id)
{
    int slot;
    slot = as89_find_slot(system, actor_id);
    return slot >= 0 ? &system->actors[slot] : 0;
}

int as89_register(AS89_System *system, int actor_id,
                  unsigned long owner_entity_key, unsigned long user_ref,
                  int alive, int visible, unsigned long flags)
{
    int i;
    AS89_Actor *actor;
    if (!system || actor_id <= 0) return AS89_INVALID_INDEX;
    i = as89_find_slot(system, actor_id);
    if (i < 0) {
        for (i = 0; i < AS89_MAX_ACTORS; ++i)
            if (!system->actors[i].used) break;
        if (i >= AS89_MAX_ACTORS) return AS89_INVALID_INDEX;
        memset(&system->actors[i], 0, sizeof(system->actors[i]));
        system->actors[i].used = 1;
        system->actors[i].actor_id = actor_id;
        ++system->count;
    }
    actor = &system->actors[i];
    actor->owner_entity_key = owner_entity_key;
    actor->user_ref = user_ref;
    actor->alive = alive ? 1 : 0;
    actor->visible = visible ? 1 : 0;
    actor->flags = flags;
    return i;
}

int as89_unregister(AS89_System *system, int actor_id)
{
    int slot;
    if (!system) return 0;
    slot = as89_find_slot(system, actor_id);
    if (slot < 0) return 1;
    memset(&system->actors[slot], 0, sizeof(system->actors[slot]));
    if (system->count > 0) --system->count;
    return 1;
}

int as89_set_owner_entity(AS89_System *system, int actor_id,
                          unsigned long owner_entity_key)
{
    AS89_Actor *actor;
    actor = as89_find(system, actor_id);
    if (!actor) return 0;
    actor->owner_entity_key = owner_entity_key;
    return 1;
}

int as89_set_user_ref(AS89_System *system, int actor_id, unsigned long user_ref)
{
    AS89_Actor *actor;
    actor = as89_find(system, actor_id);
    if (!actor) return 0;
    actor->user_ref = user_ref;
    return 1;
}

int as89_set_alive(AS89_System *system, int actor_id, int alive)
{
    AS89_Actor *actor;
    actor = as89_find(system, actor_id);
    if (!actor) return 0;
    actor->alive = alive ? 1 : 0;
    return 1;
}

int as89_set_visible(AS89_System *system, int actor_id, int visible)
{
    AS89_Actor *actor;
    actor = as89_find(system, actor_id);
    if (!actor) return 0;
    actor->visible = visible ? 1 : 0;
    return 1;
}

int as89_set_flags(AS89_System *system, int actor_id, unsigned long flags)
{
    AS89_Actor *actor;
    actor = as89_find(system, actor_id);
    if (!actor) return 0;
    actor->flags = flags;
    return 1;
}

unsigned long as89_owner_entity(const AS89_System *system, int actor_id)
{
    const AS89_Actor *actor;
    actor = as89_find_const(system, actor_id);
    return actor ? actor->owner_entity_key : 0UL;
}

unsigned long as89_user_ref(const AS89_System *system, int actor_id)
{
    const AS89_Actor *actor;
    actor = as89_find_const(system, actor_id);
    return actor ? actor->user_ref : 0UL;
}

int as89_is_alive(const AS89_System *system, int actor_id)
{
    const AS89_Actor *actor;
    actor = as89_find_const(system, actor_id);
    if (!actor) return 0;
    if (system->provider.query_alive)
        return system->provider.query_alive(system->provider.user,
                                            actor_id, actor->alive) ? 1 : 0;
    return actor->alive;
}

int as89_is_visible(const AS89_System *system, int actor_id)
{
    const AS89_Actor *actor;
    actor = as89_find_const(system, actor_id);
    if (!actor) return 0;
    if (system->provider.query_visible)
        return system->provider.query_visible(system->provider.user,
                                              actor_id, actor->visible) ? 1 : 0;
    return actor->visible;
}

int as89_position(const AS89_System *system, int actor_id,
                  AS89_Position *out_position)
{
    if (!system || !out_position || !as89_find_const(system, actor_id)) return 0;
    if (!system->provider.query_position) return 0;
    return system->provider.query_position(system->provider.user,
                                           actor_id, out_position);
}

unsigned long as89_locator(const AS89_System *system, int actor_id)
{
    if (!system || !as89_find_const(system, actor_id)) return 0UL;
    if (!system->provider.query_locator) return 0UL;
    return system->provider.query_locator(system->provider.user, actor_id);
}

int as89_count(const AS89_System *system)
{ return system ? system->count : 0; }
