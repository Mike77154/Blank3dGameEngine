#ifndef MELEE3D_H
#define MELEE3D_H

/*
    melee3d - attack timing, hitlog and hit events.

    Depends on hurtbox3d + hitbox3d.
    C89, no malloc/realloc/free, no internal heap, no float/double.
*/

#include "hitbox3d.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ML3_MAX_ATTACKS
#define ML3_MAX_ATTACKS 64
#endif

#ifndef ML3_MAX_EVENTS
#define ML3_MAX_EVENTS 128
#endif

#ifndef ML3_MAX_HITLOG_PER_ATTACK
#define ML3_MAX_HITLOG_PER_ATTACK 64
#endif

#ifndef ML3_MAX_QUERY_ACTORS
#define ML3_MAX_QUERY_ACTORS 128
#endif

#define ML3_OK                 0
#define ML3_ERR_FULL          -1
#define ML3_ERR_NOT_FOUND     -2
#define ML3_ERR_BAD_ARG       -3
#define ML3_ERR_EVENT_FULL    -4
#define ML3_ERR_INACTIVE      -5

#define ML3_ATTACK_STARTUP     0
#define ML3_ATTACK_ACTIVE      1
#define ML3_ATTACK_RECOVERY    2
#define ML3_ATTACK_DONE        3

#define ML3_FLAG_ONE_HIT_ONLY  0x0001
#define ML3_FLAG_NO_SELF_HIT   0x0002
#define ML3_FLAG_NO_TEAM_HIT   0x0004
#define ML3_FLAG_USE_GROUPS    0x0008

#define ML3_DEBUG_HURTBOXES    HB3_DEBUG_HURTBOXES
#define ML3_DEBUG_GRID         HB3_DEBUG_GRID
#define ML3_DEBUG_HITBOXES     0x0100
#define ML3_DEBUG_ALL          0x7fff

typedef struct ml3_event_s {
    int attacker_id;
    int defender_id;
    int attack_id;
    int attack_handle;
    int hitbox_index;
    int hurtbox_index;
    int damage;
    int stun_frames;
    int hitstop_frames;
    int hit_flags;
    int hurt_flags;
    int hurt_material;
    int hit_user_tag;
    int hurt_user_tag;
    hb3_v3 point;
    hb3_v3 normal;
} ml3_event;

typedef struct ml3_attack_s {
    int active;
    int handle;
    int owner_id;
    int attack_id;
    int frame;
    int startup_frames;
    int active_frames;
    int recovery_frames;
    int damage;
    int stun_frames;
    int hitstop_frames;
    int flags;
    hit3_attack hit;
    int hitlog_actor_ids[ML3_MAX_HITLOG_PER_ATTACK];
    int hitlog_count;
} ml3_attack;

typedef struct ml3_world_s {
    hb3_world hurt;
    ml3_attack attacks[ML3_MAX_ATTACKS];
    ml3_event events[ML3_MAX_EVENTS];
    int event_head;
    int event_tail;
    int event_count;
    int next_attack_handle;
    int tick;
    int flags;
} ml3_world;

void ml3_world_init(ml3_world *w);

int ml3_world_set_broadphase_grid(ml3_world *w, hb3_v3 origin,
                                  hb3_fx cell_size,
                                  int cells_x, int cells_y, int cells_z);
void ml3_world_disable_broadphase(ml3_world *w);
int ml3_world_rebuild_broadphase(ml3_world *w);
int ml3_world_broadphase_ref_count(const ml3_world *w);
int ml3_world_broadphase_overflowed(const ml3_world *w);

int ml3_actor_add(ml3_world *w, int actor_id, int team);
int ml3_actor_remove(ml3_world *w, int actor_id);
int ml3_actor_set_masks(ml3_world *w, int actor_id, int group_mask, int hit_mask);
int ml3_actor_clear_hurtboxes(ml3_world *w, int actor_id);
int ml3_actor_add_hurt_sphere(ml3_world *w, int actor_id, int hurt_id,
                              hb3_v3 center, hb3_fx radius,
                              int hurt_flags, int user_tag);
int ml3_actor_add_hurt_capsule(ml3_world *w, int actor_id, int hurt_id,
                               hb3_v3 p0, hb3_v3 p1, hb3_fx radius,
                               int hurt_flags, int user_tag);
int ml3_actor_add_hurt_obb(ml3_world *w, int actor_id, int hurt_id,
                           hb3_v3 center, hb3_v3 half_extents,
                           hb3_mat3 basis, int hurt_flags, int user_tag);

int ml3_attack_begin(ml3_world *w, int owner_id, int attack_id,
                     int startup_frames, int active_frames,
                     int recovery_frames, int damage,
                     int stun_frames, int hitstop_frames,
                     int flags, int *out_handle);
int ml3_attack_end(ml3_world *w, int handle);
int ml3_attack_clear_hitboxes(ml3_world *w, int handle);
int ml3_attack_set_masks(ml3_world *w, int handle, int group_mask, int hit_mask);
int ml3_attack_set_hit_sphere(ml3_world *w, int handle, int slot, int hit_id,
                              hb3_v3 center, hb3_fx radius,
                              int hit_flags, int user_tag);
int ml3_attack_set_hit_capsule(ml3_world *w, int handle, int slot, int hit_id,
                               hb3_v3 p0, hb3_v3 p1, hb3_fx radius,
                               int hit_flags, int user_tag);
int ml3_attack_set_hit_obb(ml3_world *w, int handle, int slot, int hit_id,
                           hb3_v3 center, hb3_v3 half_extents,
                           hb3_mat3 basis, int hit_flags, int user_tag);

int ml3_attack_get_phase(const ml3_world *w, int handle);
int ml3_attack_is_active_window(const ml3_world *w, int handle);

void ml3_tick(ml3_world *w);
int ml3_poll_event(ml3_world *w, ml3_event *out_event);
int ml3_peek_event_count(const ml3_world *w);

void ml3_debug_draw_world_lines(const ml3_world *w, hb3_debug_line_fn line_fn,
                                void *user, int flags);
int ml3_debug_count_actors(const ml3_world *w);
int ml3_debug_count_attacks(const ml3_world *w);

#ifdef __cplusplus
}
#endif

#endif
