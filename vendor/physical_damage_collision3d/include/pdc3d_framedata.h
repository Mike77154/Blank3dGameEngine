#ifndef PDC3D_FRAMEDATA_H
#define PDC3D_FRAMEDATA_H

#include "pdc3d_config.h"
#include "pdc3d_damage.h"
#include "hurtbox3d.h"
#include "fd3d89.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PDC3D_MAX_FRAMEDATA_ACTORS
#define PDC3D_MAX_FRAMEDATA_ACTORS 64
#endif

#define PDC3D_FDRES_NONE             0x00000000u
#define PDC3D_FDRES_HAS_ATTACKER     0x00000001u
#define PDC3D_FDRES_HAS_DEFENDER     0x00000002u
#define PDC3D_FDRES_ATTACK_ACTIVE    0x00000004u
#define PDC3D_FDRES_ATTACK_INACTIVE  0x00000008u
#define PDC3D_FDRES_CAN_HIT          0x00000010u
#define PDC3D_FDRES_INVINCIBLE       0x00000020u
#define PDC3D_FDRES_STRIKE_INVULN    0x00000040u
#define PDC3D_FDRES_PROJECTILE_INVULN 0x00000080u
#define PDC3D_FDRES_THROW_INVULN     0x00000100u
#define PDC3D_FDRES_SIDE_STEP_EVADE  0x00000200u
#define PDC3D_FDRES_COUNTER_WINDOW   0x00000400u
#define PDC3D_FDRES_ARMOR            0x00000800u
#define PDC3D_FDRES_SUPER_ARMOR      0x00001000u
#define PDC3D_FDRES_HITSTOP          0x00002000u
#define PDC3D_FDRES_HITSTUN          0x00004000u
#define PDC3D_FDRES_BLOCKSTUN        0x00008000u
#define PDC3D_FDRES_CAN_CANCEL       0x00010000u
#define PDC3D_FDRES_CONTACT_APPLIED  0x00020000u

#define PDC3D_FD_FACE_Z_POS FD3D_FACE_Z_POS
#define PDC3D_FD_FACE_X_POS FD3D_FACE_X_POS
#define PDC3D_FD_FACE_Z_NEG FD3D_FACE_Z_NEG
#define PDC3D_FD_FACE_X_NEG FD3D_FACE_X_NEG

#define PDC3D_FD_LANE_CENTER FD3D_LANE_CENTER
#define PDC3D_FD_LANE_LEFT   FD3D_LANE_LEFT
#define PDC3D_FD_LANE_RIGHT  FD3D_LANE_RIGHT
#define PDC3D_FD_LANE_ALL    FD3D_LANE_ALL

/* Public aliases keep users on the PDC3D surface while preserving static data. */
typedef FD3D_Volume pdc3d_fd_volume;
typedef FD3D_HitDef pdc3d_fd_hitdef;
typedef FD3D_Frame pdc3d_fd_frame;
typedef FD3D_Action pdc3d_fd_action;
typedef FD3D_Callbacks pdc3d_fd_callbacks;

typedef struct pdc3d_framedata_actor_s {
    int used;
    int actor_id;
    FD3D_Entity fd;
} pdc3d_framedata_actor;

typedef struct pdc3d_framedata_world_s {
    pdc3d_framedata_actor actors[PDC3D_MAX_FRAMEDATA_ACTORS];
} pdc3d_framedata_world;

typedef struct pdc3d_framedata_result_s {
    int attacker_registered;
    int defender_registered;
    int attack_active;
    int can_hit;
    int side_step_evaded;
    int counter_window;
    int armor;
    int super_armor;
    int hitstop_frames;
    int hitstun_frames;
    int blockstop_frames;
    int blockstun_frames;
    int frame_advantage_hit;
    int frame_advantage_block;
    unsigned int attr_mask;
    unsigned int cancel_on_hit_mask;
    unsigned int cancel_on_block_mask;
    unsigned int attack_hit_id;
    unsigned int result_flags;
} pdc3d_framedata_result;

FD3D_Fx pdc3d_fd_from_pdc_fx(hb3_fx v);
hb3_fx pdc3d_fd_to_pdc_fx(FD3D_Fx v);
FD3D_Volume pdc3d_fd_sphere_from_pdc(hb3_v3 center, hb3_fx radius,
                                      unsigned int mask,
                                      unsigned int group,
                                      unsigned int lane_mask);
FD3D_Volume pdc3d_fd_aabb_from_ints(int x1, int y1, int z1,
                                    int x2, int y2, int z2,
                                    unsigned int mask,
                                    unsigned int group,
                                    unsigned int lane_mask);
unsigned int pdc3d_fd_attack_flags_to_attr(int attack_flags);

void pdc3d_framedata_result_init(pdc3d_framedata_result *r);
void pdc3d_framedata_world_init(pdc3d_framedata_world *fw);

int pdc3d_framedata_actor_create(pdc3d_framedata_world *fw,
                                 int actor_id,
                                 int team,
                                 int hp);
int pdc3d_framedata_actor_remove(pdc3d_framedata_world *fw,
                                 int actor_id);
FD3D_Entity *pdc3d_framedata_actor_find(pdc3d_framedata_world *fw,
                                        int actor_id);
const FD3D_Entity *pdc3d_framedata_actor_find_const(const pdc3d_framedata_world *fw,
                                                    int actor_id);
int pdc3d_framedata_actor_set_action(pdc3d_framedata_world *fw,
                                     int actor_id,
                                     const pdc3d_fd_action *action);
int pdc3d_framedata_actor_set_pose(pdc3d_framedata_world *fw,
                                   int actor_id,
                                   hb3_v3 pos,
                                   int facing,
                                   unsigned int lane_mask);
int pdc3d_framedata_actor_set_nothitby(pdc3d_framedata_world *fw,
                                       int actor_id,
                                       unsigned int attr_mask,
                                       int frames);
int pdc3d_framedata_actor_set_hitby(pdc3d_framedata_world *fw,
                                    int actor_id,
                                    unsigned int attr_mask,
                                    int frames);
int pdc3d_framedata_actor_tick(pdc3d_framedata_world *fw,
                               int actor_id,
                               const pdc3d_fd_callbacks *callbacks,
                               void *user);
void pdc3d_framedata_tick_all(pdc3d_framedata_world *fw,
                              const pdc3d_fd_callbacks *callbacks,
                              void *user);
int pdc3d_framedata_actor_is_done(const pdc3d_framedata_world *fw,
                                  int actor_id);
int pdc3d_framedata_actor_frame_info(const pdc3d_framedata_world *fw,
                                     int actor_id,
                                     unsigned int *out_action_id,
                                     unsigned short *out_frame_index,
                                     unsigned short *out_frame_tick,
                                     unsigned short *out_total_tick,
                                     unsigned long *out_frame_flags);

int pdc3d_framedata_check_damage(const pdc3d_framedata_world *fw,
                                 int defender_id,
                                 int attack_flags,
                                 pdc3d_framedata_result *out_result);
int pdc3d_framedata_resolve_damage(pdc3d_framedata_world *fw,
                                   int attacker_id,
                                   int defender_id,
                                   int was_blocked,
                                   pdc3d_damage_packet *io_packet,
                                   pdc3d_framedata_result *out_result);
int pdc3d_framedata_frame_advantage(const pdc3d_framedata_world *fw,
                                    int attacker_id,
                                    int use_blockstun);

#ifdef __cplusplus
}
#endif

#endif
