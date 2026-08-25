#ifndef PDC3D_DEFENSE_H
#define PDC3D_DEFENSE_H

#include "pdc3d_config.h"
#include "pdc3d_damage.h"
#include "hurtbox3d.h"
#include "gblock3d89.h"
#include "gguard3d89.h"
#include "gparry3d89.h"
#include "gshell3d89.h"
#include "gcounter3d89.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PDC3D_MAX_DEFENSE_ACTORS
#define PDC3D_MAX_DEFENSE_ACTORS 64
#endif

#define PDC3D_DEF_BLOCK   0x0001u
#define PDC3D_DEF_GUARD   0x0002u
#define PDC3D_DEF_PARRY   0x0004u
#define PDC3D_DEF_SHELL   0x0008u
#define PDC3D_DEF_COUNTER 0x0010u
#define PDC3D_DEF_ALL     0x001Fu

#define PDC3D_DEFRES_NONE            0x00000000u
#define PDC3D_DEFRES_PARRY_PERFECT   0x00000001u
#define PDC3D_DEFRES_PARRY_NORMAL    0x00000002u
#define PDC3D_DEFRES_PARRY_LATE      0x00000004u
#define PDC3D_DEFRES_GUARDED         0x00000008u
#define PDC3D_DEFRES_GUARD_BROKEN    0x00000010u
#define PDC3D_DEFRES_BLOCKED         0x00000020u
#define PDC3D_DEFRES_BLOCK_BROKEN    0x00000040u
#define PDC3D_DEFRES_SHELL_ABSORB    0x00000080u
#define PDC3D_DEFRES_SHELL_BROKEN    0x00000100u
#define PDC3D_DEFRES_COUNTER_TOKEN   0x00000200u
#define PDC3D_DEFRES_COUNTER_USED    0x00000400u
#define PDC3D_DEFRES_PASSED          0x00000800u
#define PDC3D_DEFRES_ACCEPTED        0x00001000u

#define PDC3D_DEF_EVENT_PARRY        1
#define PDC3D_DEF_EVENT_GUARD        2
#define PDC3D_DEF_EVENT_BLOCK        3
#define PDC3D_DEF_EVENT_SHELL        4
#define PDC3D_DEF_EVENT_COUNTER      5

#define PDC3D_COUNTER_LIGHT          GCTR_COUNTER_LIGHT
#define PDC3D_COUNTER_HEAVY          GCTR_COUNTER_HEAVY
#define PDC3D_COUNTER_THROW          GCTR_COUNTER_THROW
#define PDC3D_COUNTER_SCRIPT         GCTR_COUNTER_SCRIPT

typedef struct pdc3d_defense_actor_s {
    int used;
    int actor_id;
    unsigned int enabled_flags;
    int guard_hold_input;
} pdc3d_defense_actor;

typedef struct pdc3d_defense_profile_s {
    unsigned int enabled_flags;
    GBLK_Profile block;
    GGRD_Profile guard;
    GPRY_Profile parry;
    GCTR_Profile counter;
} pdc3d_defense_profile;

typedef struct pdc3d_defense_result_s {
    int accepted;
    int final_damage;
    int chip_damage;
    int absorbed_damage;
    int stamina_damage;
    int posture_damage;
    int guard_damage;
    int guard_broken;
    int block_broken;
    int shell_broken;
    int parry_result;
    int counter_token_created;
    int counter_used;
    int counter_kind;
    int counter_script_code;
    int attacker_stagger_frames;
    int defender_stun_frames;
    int blockstun_frames;
    int recoil_frames;
    int dot;
    unsigned int defense_flags;
} pdc3d_defense_result;

typedef struct pdc3d_counter_request_s {
    int actor_id;
    int target_id;
    int stamina_available;
    int posture_available;
    hb3_v3 target_rel_pos;
} pdc3d_counter_request;

typedef struct pdc3d_defense_world_s {
    GBLK_Context block;
    GGRD_Context guard;
    GPRY_Context parry;
    GSHL_Context shell;
    GCTR_Context counter;
    pdc3d_defense_actor actors[PDC3D_MAX_DEFENSE_ACTORS];
} pdc3d_defense_world;

void pdc3d_defense_profile_defaults(pdc3d_defense_profile *p);
void pdc3d_defense_world_init(pdc3d_defense_world *d);

int pdc3d_defense_actor_create(pdc3d_defense_world *d,
                               int actor_id,
                               unsigned int enabled_flags,
                               const pdc3d_defense_profile *profile);
int pdc3d_defense_actor_remove(pdc3d_defense_world *d, int actor_id);
int pdc3d_defense_actor_set_enabled(pdc3d_defense_world *d,
                                    int actor_id,
                                    unsigned int enabled_flags);
unsigned int pdc3d_defense_actor_get_enabled(const pdc3d_defense_world *d,
                                             int actor_id);

int pdc3d_defense_set_facing(pdc3d_defense_world *d,
                             int actor_id,
                             hb3_v3 facing_dir);
int pdc3d_defense_set_guard_input(pdc3d_defense_world *d,
                                  int actor_id,
                                  int hold_input);
int pdc3d_defense_press_parry(pdc3d_defense_world *d, int actor_id);
int pdc3d_defense_set_block_resources(pdc3d_defense_world *d,
                                      int actor_id,
                                      int stamina,
                                      int posture);
int pdc3d_defense_set_guard_value(pdc3d_defense_world *d,
                                  int actor_id,
                                  int guard_value);
int pdc3d_defense_set_block_active(pdc3d_defense_world *d,
                                   int actor_id,
                                   int active);
int pdc3d_defense_set_shell_layer(pdc3d_defense_world *d,
                                  int actor_id,
                                  int layer_index,
                                  const GSHL_LayerProfile *profile,
                                  int start_full);
int pdc3d_defense_clear_shell_layer(pdc3d_defense_world *d,
                                    int actor_id,
                                    int layer_index);

void pdc3d_defense_update(pdc3d_defense_world *d,
                          int frames,
                          int current_frame);

int pdc3d_defense_resolve_damage(pdc3d_defense_world *d,
                                 pdc3d_damage_packet *io_packet,
                                 hb3_v3 source_dir,
                                 int current_frame,
                                 pdc3d_defense_result *out_result);

int pdc3d_defense_try_counter(pdc3d_defense_world *d,
                              const pdc3d_counter_request *request,
                              int current_frame,
                              pdc3d_defense_result *out_result);
int pdc3d_defense_count_counter_tokens(const pdc3d_defense_world *d,
                                       int actor_id);

#ifdef __cplusplus
}
#endif

#endif
