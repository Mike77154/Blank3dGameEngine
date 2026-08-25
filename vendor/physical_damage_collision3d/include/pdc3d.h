#ifndef PDC3D_H
#define PDC3D_H

#include "pdc3d_config.h"
#include "pdc3d_arena.h"
#include "pdc3d_bridge.h"
#include "pdc3d_damage.h"
#include "pdc3d_framedata.h"
#include "pdc3d_defense.h"

#include "hurtbox3d.h"
#include "hitbox3d.h"
#include "melee3d.h"
#include "grab3d.h"
#include "throws3d.h"
#include "weakspots3d.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int pdc3d_fx;
typedef hb3_v3 pdc3d_v3;
typedef hb3_mat3 pdc3d_mat3;

#define PDC3D_FX_SHIFT HB3_FX_SHIFT
#define PDC3D_FX_ONE   HB3_FX_ONE
#define PDC3D_FX_FROM_INT(x) ((pdc3d_fx)((x) * PDC3D_FX_ONE))
#define PDC3D_FX_TO_INT(x)   ((int)((x) / PDC3D_FX_ONE))

#define PDC3D_OK              0
#define PDC3D_ERR_FULL       -1
#define PDC3D_ERR_NOT_FOUND  -2
#define PDC3D_ERR_BAD_ARG    -3
#define PDC3D_ERR_INACTIVE   -4

#define PDC3D_EVENT_DAMAGE        1
#define PDC3D_EVENT_GRAB          2
#define PDC3D_EVENT_THROW         3
#define PDC3D_EVENT_WEAKSPOT      4
#define PDC3D_EVENT_WORLD_IMPACT  5
#define PDC3D_EVENT_DEFENSE       6
#define PDC3D_EVENT_COUNTER       7

#define PDC3D_ATTACK_ONE_HIT_ONLY ML3_FLAG_ONE_HIT_ONLY
#define PDC3D_ATTACK_NO_SELF_HIT  ML3_FLAG_NO_SELF_HIT
#define PDC3D_ATTACK_NO_TEAM_HIT  ML3_FLAG_NO_TEAM_HIT
#define PDC3D_ATTACK_USE_GROUPS   ML3_FLAG_USE_GROUPS

typedef struct pdc3d_event_s {
    int type;
    int subtype;
    pdc3d_damage_packet damage;
    pdc3d_defense_result defense;
    pdc3d_framedata_result framedata;
    hb3_v3 point;
    hb3_v3 normal;
    int actor_a;
    int actor_b;
    int value;
} pdc3d_event;

typedef struct pdc3d_world_s {
    ml3_world melee;
    g3d_world grabs;
    t3d_world throws_world;
    w3d_world weakspots;
    pdc3d_damage_table damage_table;
    pdc3d_framedata_world framedata_world;
    pdc3d_defense_world defense_world;
    pdc3d_bridge bridge;
    pdc3d_arena *arena;
    pdc3d_event events[PDC3D_MAX_EVENTS];
    int event_head;
    int event_tail;
    int event_count;
    int pose_actor_ids[PDC3D_MAX_ACTOR_POSES];
    pdc3d_v3 pose_positions[PDC3D_MAX_ACTOR_POSES];
    pdc3d_v3 pose_facing[PDC3D_MAX_ACTOR_POSES];
    unsigned int pose_lane_masks[PDC3D_MAX_ACTOR_POSES];
    int pose_used[PDC3D_MAX_ACTOR_POSES];
    int tick;
} pdc3d_world;

typedef struct pdc3d_attack_profile_s {
    int attack_id;
    int startup_frames;
    int active_frames;
    int recovery_frames;
    int damage;
    int stun_frames;
    int hitstop_frames;
    int flags;
    int group_mask;
    int hit_mask;
} pdc3d_attack_profile;

pdc3d_v3 pdc3d_v3_make(pdc3d_fx x, pdc3d_fx y, pdc3d_fx z);
pdc3d_v3 pdc3d_v3_from_ints(int x, int y, int z);
pdc3d_mat3 pdc3d_mat3_identity(void);

void pdc3d_world_init(pdc3d_world *w);
void pdc3d_world_init_with_arena(pdc3d_world *w, pdc3d_arena *arena);
void pdc3d_world_set_bridge(pdc3d_world *w, const pdc3d_bridge *bridge);

int pdc3d_actor_add(pdc3d_world *w, int actor_id, int team);
int pdc3d_actor_remove(pdc3d_world *w, int actor_id);
int pdc3d_actor_set_masks(pdc3d_world *w, int actor_id, int group_mask, int hit_mask);
int pdc3d_actor_set_pose(pdc3d_world *w, int actor_id,
                         pdc3d_v3 pos, pdc3d_v3 facing_dir,
                         unsigned int lane_mask);
int pdc3d_actor_clear_hurtboxes(pdc3d_world *w, int actor_id);
int pdc3d_actor_clear_grabs(pdc3d_world *w, int actor_id);
int pdc3d_actor_clear_weakspots(pdc3d_world *w, int actor_id);
int pdc3d_actor_add_hurt_sphere(pdc3d_world *w, int actor_id, int hurt_id,
                                pdc3d_v3 center, pdc3d_fx radius,
                                int material_flags, int user_tag);
int pdc3d_actor_add_hurt_capsule(pdc3d_world *w, int actor_id, int hurt_id,
                                 pdc3d_v3 a, pdc3d_v3 b, pdc3d_fx radius,
                                 int material_flags, int user_tag);
int pdc3d_actor_add_hurt_obb(pdc3d_world *w, int actor_id, int hurt_id,
                             pdc3d_v3 center, pdc3d_v3 half_extents,
                             pdc3d_mat3 basis, int material_flags,
                             int user_tag);

int pdc3d_actor_add_weakspot_sphere(pdc3d_world *w, int actor_id,
                                    int weakspot_id, pdc3d_v3 center,
                                    pdc3d_fx radius, int mult_num,
                                    int mult_den, int durability,
                                    int flags);
int pdc3d_actor_add_weakspot_capsule(pdc3d_world *w, int actor_id,
                                     int weakspot_id, pdc3d_v3 a,
                                     pdc3d_v3 b, pdc3d_fx radius,
                                     int mult_num, int mult_den,
                                     int durability, int flags);
int pdc3d_actor_add_weakspot_obb(pdc3d_world *w, int actor_id,
                                 int weakspot_id, pdc3d_v3 center,
                                 pdc3d_v3 half_extents, pdc3d_mat3 basis,
                                 int mult_num, int mult_den,
                                 int durability, int flags);
int pdc3d_actor_set_weakspot_filter(pdc3d_world *w, int actor_id,
                                    int weakspot_id, int required_attack_flags,
                                    int blocked_attack_flags);

int pdc3d_attack_begin(pdc3d_world *w, int owner_id,
                       const pdc3d_attack_profile *profile,
                       int *out_handle);
int pdc3d_attack_end(pdc3d_world *w, int handle);
int pdc3d_attack_clear_hitboxes(pdc3d_world *w, int handle);
int pdc3d_attack_set_hit_sphere(pdc3d_world *w, int handle, int slot,
                                int hit_id, pdc3d_v3 center,
                                pdc3d_fx radius, int attack_flags,
                                int user_tag);
int pdc3d_attack_set_hit_capsule(pdc3d_world *w, int handle, int slot,
                                 int hit_id, pdc3d_v3 p0, pdc3d_v3 p1,
                                 pdc3d_fx radius, int attack_flags,
                                 int user_tag);
int pdc3d_attack_set_hit_obb(pdc3d_world *w, int handle, int slot,
                             int hit_id, pdc3d_v3 center,
                             pdc3d_v3 half_extents, pdc3d_mat3 basis,
                             int attack_flags, int user_tag);
int pdc3d_attack_set_hit_capsule_from_sockets(pdc3d_world *w, int handle,
                                             int slot, int hit_id,
                                             int owner_actor_id,
                                             int socket_a, int socket_b,
                                             pdc3d_fx radius,
                                             int attack_flags,
                                             int user_tag);

int pdc3d_grab_start(pdc3d_world *w, int owner_actor_id,
                     const g3d_grab_def *def,
                     const g3d_shape *shapes, int shape_count);
int pdc3d_throw_add_body(pdc3d_world *w, int body_id,
                         pdc3d_v3 pos, pdc3d_fx radius);
int pdc3d_throw_start_arc(pdc3d_world *w, int attacker_id, int body_id,
                          const t3d_throw_def *def,
                          pdc3d_v3 start_pos, pdc3d_v3 target_pos,
                          int travel_ticks);


int pdc3d_fd_actor_enable(pdc3d_world *w, int actor_id,
                                  int team, int hp);
int pdc3d_fd_actor_disable(pdc3d_world *w, int actor_id);
int pdc3d_fd_actor_set_action(pdc3d_world *w, int actor_id,
                                      const pdc3d_fd_action *action);
int pdc3d_fd_actor_set_pose(pdc3d_world *w, int actor_id,
                                    pdc3d_v3 pos, int facing,
                                    unsigned int lane_mask);
int pdc3d_fd_actor_set_nothitby(pdc3d_world *w, int actor_id,
                                        unsigned int attr_mask, int frames);
int pdc3d_fd_actor_set_hitby(pdc3d_world *w, int actor_id,
                                     unsigned int attr_mask, int frames);
int pdc3d_fd_actor_frame_info(const pdc3d_world *w, int actor_id,
                                      unsigned int *out_action_id,
                                      unsigned short *out_frame_index,
                                      unsigned short *out_frame_tick,
                                      unsigned short *out_total_tick,
                                      unsigned long *out_frame_flags);
int pdc3d_fd_actor_is_done(const pdc3d_world *w, int actor_id);
int pdc3d_fd_check_actor_damage(const pdc3d_world *w,
                                        int defender_id,
                                        int attack_flags,
                                        pdc3d_framedata_result *out_result);
int pdc3d_fd_resolve_actor_damage(pdc3d_world *w,
                                          int attacker_id,
                                          int defender_id,
                                          int was_blocked,
                                          pdc3d_damage_packet *io_packet,
                                          pdc3d_framedata_result *out_result);
int pdc3d_fd_actor_advantage(const pdc3d_world *w,
                                     int attacker_id,
                                     int use_blockstun);

int pdc3d_defense_actor_enable(pdc3d_world *w, int actor_id,
                               unsigned int enabled_flags,
                               const pdc3d_defense_profile *profile);
int pdc3d_defense_actor_disable(pdc3d_world *w, int actor_id);
int pdc3d_defense_actor_set_flags(pdc3d_world *w, int actor_id,
                                  unsigned int enabled_flags);
int pdc3d_defense_actor_set_facing(pdc3d_world *w, int actor_id,
                                   pdc3d_v3 facing_dir);
int pdc3d_defense_actor_set_guard_input(pdc3d_world *w, int actor_id,
                                        int hold_input);
int pdc3d_defense_actor_press_parry(pdc3d_world *w, int actor_id);
int pdc3d_defense_actor_set_block_active(pdc3d_world *w, int actor_id,
                                         int active);
int pdc3d_defense_actor_set_block_resources(pdc3d_world *w, int actor_id,
                                            int stamina, int posture);
int pdc3d_defense_actor_set_guard_value(pdc3d_world *w, int actor_id,
                                        int guard_value);
int pdc3d_defense_actor_set_shell_layer(pdc3d_world *w, int actor_id,
                                        int layer_index,
                                        const GSHL_LayerProfile *profile,
                                        int start_full);
int pdc3d_defense_actor_clear_shell_layer(pdc3d_world *w, int actor_id,
                                          int layer_index);
int pdc3d_resolve_incoming_attack(pdc3d_world *w,
                                  pdc3d_damage_packet *io_packet,
                                  pdc3d_v3 source_dir,
                                  pdc3d_defense_result *out_result);
int pdc3d_submit_damage(pdc3d_world *w,
                        pdc3d_damage_packet *io_packet,
                        pdc3d_v3 hit_point,
                        pdc3d_v3 source_dir,
                        pdc3d_defense_result *out_result);
int pdc3d_counter_try(pdc3d_world *w,
                      const pdc3d_counter_request *request,
                      pdc3d_defense_result *out_result);
int pdc3d_counter_count_tokens(const pdc3d_world *w, int actor_id);

void pdc3d_tick(pdc3d_world *w);
int pdc3d_poll_event(pdc3d_world *w, pdc3d_event *out_event);
int pdc3d_peek_event_count(const pdc3d_world *w);

int pdc3d_world_set_broadphase_grid(pdc3d_world *w, pdc3d_v3 origin,
                                    pdc3d_fx cell_size,
                                    int cells_x, int cells_y, int cells_z);
int pdc3d_world_rebuild_broadphase(pdc3d_world *w);
int pdc3d_world_broadphase_overflowed(const pdc3d_world *w);

void pdc3d_debug_draw_world_lines(const pdc3d_world *w,
                                  hb3_debug_line_fn line_fn,
                                  void *user,
                                  int flags);

#ifdef __cplusplus
}
#endif

#endif
