#ifndef ACTOR_SYSTEM89_H
#define ACTOR_SYSTEM89_H

#ifdef __cplusplus
extern "C" {
#endif

#define AS89_MAX_ACTORS 128
#define AS89_INVALID_INDEX (-1)

/* actor_system89 is intentionally ontology-light. It knows an actor identity,
   an opaque owner-entity key, optional host/user reference, and generic state.
   Faction, inventory, equipment, weapons, AI and concrete actor classes live
   in separate systems keyed by actor_id. */
typedef long as89_scalar;

typedef struct AS89_PositionTag {
    as89_scalar x;
    as89_scalar y;
    as89_scalar z;
} AS89_Position;

typedef struct AS89_ActorTag {
    int used;
    int actor_id;
    unsigned long owner_entity_key;
    unsigned long user_ref;
    int alive;
    int visible;
    unsigned long flags;
} AS89_Actor;

typedef struct AS89_ProviderTag {
    void *user;
    int (*query_alive)(void *user, int actor_id, int stored_alive);
    int (*query_visible)(void *user, int actor_id, int stored_visible);
    int (*query_position)(void *user, int actor_id, AS89_Position *out_position);
    unsigned long (*query_locator)(void *user, int actor_id);
} AS89_Provider;

typedef struct AS89_SystemTag {
    AS89_Actor actors[AS89_MAX_ACTORS];
    int count;
    AS89_Provider provider;
} AS89_System;

void as89_init(AS89_System *system);
void as89_set_provider(AS89_System *system, const AS89_Provider *provider);
int as89_register(AS89_System *system, int actor_id,
                  unsigned long owner_entity_key, unsigned long user_ref,
                  int alive, int visible, unsigned long flags);
int as89_unregister(AS89_System *system, int actor_id);
int as89_find_slot(const AS89_System *system, int actor_id);
AS89_Actor *as89_find(AS89_System *system, int actor_id);
const AS89_Actor *as89_find_const(const AS89_System *system, int actor_id);
int as89_set_owner_entity(AS89_System *system, int actor_id,
                          unsigned long owner_entity_key);
int as89_set_user_ref(AS89_System *system, int actor_id,
                      unsigned long user_ref);
int as89_set_alive(AS89_System *system, int actor_id, int alive);
int as89_set_visible(AS89_System *system, int actor_id, int visible);
int as89_set_flags(AS89_System *system, int actor_id, unsigned long flags);
unsigned long as89_owner_entity(const AS89_System *system, int actor_id);
unsigned long as89_user_ref(const AS89_System *system, int actor_id);
int as89_is_alive(const AS89_System *system, int actor_id);
int as89_is_visible(const AS89_System *system, int actor_id);
int as89_position(const AS89_System *system, int actor_id,
                  AS89_Position *out_position);
unsigned long as89_locator(const AS89_System *system, int actor_id);
int as89_count(const AS89_System *system);

#ifdef __cplusplus
}
#endif

#endif
