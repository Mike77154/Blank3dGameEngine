#ifndef BLANK3D_CONFIG_H
#define BLANK3D_CONFIG_H

#include "conf_total.h"

#ifdef __cplusplus
extern "C" {
#endif

#define B3D_CONFIG_TEXT_CAPACITY 32768
#define B3D_CONFIG_ARENA_CAPACITY 65536

typedef struct Blank3DConfigTag {
    char text[B3D_CONFIG_TEXT_CAPACITY];
    unsigned char arena[B3D_CONFIG_ARENA_CAPACITY];
    conf_ctx_t parser;
    int loaded;
    int audio_enabled;
    int start_first_person;
    int lock_mouse;
    int initial_weapon;
    int gatling_spinup_ms;
    int slingshot_charge_ms;
    long mouse_sensitivity_q16;
    long pitch_min_q16;
    long pitch_max_q16;
    long camera_distance_q16;
    long camera_height_q16;
    long camera_shoulder_q16;
    long eye_height_q16;
    long zoom_fov_q16;
    long sniper_zoom_fov_q16;
    long zoom_speed_q16;
    int ammo_9mm;
    int ammo_shells;
    int ammo_magnum;
    int ammo_sniper;
    int ammo_grenades;
    int ammo_rockets;
    int ammo_gatling;
    int ammo_stones;
    long damage_multiplier_q16;
    long speed_multiplier_q16;
    long recoil_multiplier_q16;
    int gravity_enabled;
    long gravity_fall_speed_q16;
    long jump_gravity_q16;
    int flag_weapon_enabled;
    int flag_can_fire;
    int flag_can_reload;
    int flag_active_reload;
    char status[128];
} Blank3DConfig;

void blank3d_config_defaults(Blank3DConfig *config);
int blank3d_config_load(Blank3DConfig *config, const char *path);
const char *blank3d_config_status(const Blank3DConfig *config);

#ifdef __cplusplus
}
#endif

#endif
