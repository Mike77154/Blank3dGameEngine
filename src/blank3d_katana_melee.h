#ifndef BLANK3D_KATANA_MELEE_H
#define BLANK3D_KATANA_MELEE_H

#include "../vendor/gamlib3d/gamlib3d_transform.h"
#include "../vendor/soquete3d/soquete3d.h"
#include "nationalmecanicanimal89.h"
#include "melee3d.h"

#ifdef __cplusplus
extern "C" {
#endif

#define B3D_KATANA_MAX_HITS 32
#define B3D_KATANA_MAX_TARGETS 32
#define B3D_KATANA_ACTION_SLASH 1
#define B3D_KATANA_EVENT_ACTIVE_BEGIN 1001
#define B3D_KATANA_EVENT_ACTIVE_END   1002
#define B3D_KATANA_ATTACK_ID 9101

typedef struct Blank3DKatanaConfigTag {
    int enabled;
    int debug_draw;
    unsigned short preset_index;
    int mesh_scale_thickness_percent;
    int mesh_scale_width_percent;
    int mesh_scale_length_percent;
    int mount_lateral_cm;
    int mount_height_cm;
    int mount_forward_cm;
    unsigned short tick_ms;
    unsigned short duration_frames;
    unsigned short active_start_frame;
    unsigned short active_end_frame;
    unsigned short active_start_ms;
    unsigned short active_end_ms;
    int damage;
    int stun_frames;
    int hitstop_frames;
    int one_hit_per_enemy;
    int slash_start_yaw_deg;
    int slash_contact_yaw_deg;
    int slash_end_yaw_deg;
    int slash_pitch_deg;
    int slash_roll_deg;
    int blade_base_cm;
    int blade_tip_cm;
    int damage_radius_cm; /* legacy capsule-era alias */
    int damage_box_half_width_cm;
    int damage_box_half_thickness_cm;
} Blank3DKatanaConfig;

typedef struct Blank3DKatanaHitTag {
    int attacker_id;
    int defender_id;
    int damage;
    int stun_frames;
    int hitstop_frames;
    int point_x_q12;
    int point_y_q12;
    int point_z_q12;
} Blank3DKatanaHit;

typedef struct Blank3DKatanaMeleeTag {
    int initialized;
    int enabled;
    int root_valid;
    int attacking;
    int collision_window;
    int attack_handle;
    int packet_valid;
    unsigned int tick_remainder_ms;
    unsigned int swing_serial;
    Blank3DKatanaConfig config;
    nm89_rig rig;
    nm89_provider provider;
    nm89_i16 provider_id;
    nm89_i16 root_part;
    nm89_i16 blade_part;
    nm89_i16 clip_id;
    nm89_matrix root_world;
    nm89_geometry_packet packet;
    ml3_world collision;
    int registered_targets[B3D_KATANA_MAX_TARGETS];
    int registered_target_count;
    Blank3DKatanaHit hits[B3D_KATANA_MAX_HITS];
    int hit_head;
    int hit_tail;
    int hit_count;
} Blank3DKatanaMelee;

void blank3d_katana_config_defaults(Blank3DKatanaConfig *config);
int blank3d_katana_config_load(Blank3DKatanaConfig *config,
                               const char *path);
int blank3d_katana_melee_init(Blank3DKatanaMelee *melee,
                              const Blank3DKatanaConfig *config);
void blank3d_katana_melee_set_root_pose(Blank3DKatanaMelee *melee,
                                        const soq3d_pose *pose);
void blank3d_katana_melee_clear_targets(Blank3DKatanaMelee *melee);
int blank3d_katana_melee_set_target_q12(Blank3DKatanaMelee *melee,
                                        int actor_id,
                                        int team,
                                        int alive,
                                        int x_q12,
                                        int y_q12,
                                        int z_q12,
                                        int height_q12,
                                        int radius_q12);
int blank3d_katana_melee_trigger(Blank3DKatanaMelee *melee);
int blank3d_katana_melee_update(Blank3DKatanaMelee *melee,
                                unsigned short frame_ms);
int blank3d_katana_melee_poll_hit(Blank3DKatanaMelee *melee,
                                  Blank3DKatanaHit *out_hit);
const nm89_geometry_packet *blank3d_katana_melee_packet(
    const Blank3DKatanaMelee *melee);
int blank3d_katana_melee_is_attacking(const Blank3DKatanaMelee *melee);
void blank3d_katana_melee_debug_draw(const Blank3DKatanaMelee *melee,
                                     hb3_debug_line_fn line_fn,
                                     void *user);

#ifdef __cplusplus
}
#endif

#endif
