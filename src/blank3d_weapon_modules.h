#ifndef BLANK3D_WEAPON_MODULES_H
#define BLANK3D_WEAPON_MODULES_H

#ifdef __cplusplus
extern "C" {
#endif

#define B3D_WEAPON_ID_PISTOL 1
#define B3D_WEAPON_ID_MACHINE_GUN 2
#define B3D_WEAPON_ID_SHOTGUN 3
#define B3D_WEAPON_ID_MAGNUM 4
#define B3D_WEAPON_ID_SNIPER 5
#define B3D_WEAPON_ID_GRENADE_LAUNCHER 6
#define B3D_WEAPON_ID_ROCKET_LAUNCHER 7
#define B3D_WEAPON_ID_GATLING 8
#define B3D_WEAPON_ID_SLINGSHOT 9
#define B3D_WEAPON_MODULE_CAPACITY 16
#define B3D_WEAPON_NAME_CAPACITY 64

#define B3D_TRIGGER_STANDARD 0
#define B3D_TRIGGER_SPINUP 1
#define B3D_TRIGGER_CHARGE_RELEASE 2

#define B3D_PHYSICS_LINEAR 0
#define B3D_PHYSICS_GRAVITY 1
#define B3D_PHYSICS_BOLT3D 2

#define B3D_TRAIL_NONE 0
#define B3D_TRAIL_BULLET 1
#define B3D_TRAIL_TRACER 2
#define B3D_TRAIL_HEAVY 3
#define B3D_TRAIL_ARC 4

#define B3D_OPTIC_NONE 0
#define B3D_OPTIC_GENERIC 1
#define B3D_OPTIC_SNIPER 2

#define B3D_AUDIO_PROFILE_NONE 0
#define B3D_AUDIO_PROFILE_PISTOL 1
#define B3D_AUDIO_PROFILE_SMG 2
#define B3D_AUDIO_PROFILE_SHOTGUN 3
#define B3D_AUDIO_PROFILE_MAGNUM 4
#define B3D_AUDIO_PROFILE_SNIPER 5
#define B3D_AUDIO_PROFILE_LAUNCHER 6
#define B3D_AUDIO_PROFILE_HEAVY 7

#define B3D_AUDIO_ACTION_NONE 0
#define B3D_AUDIO_ACTION_PISTOL 1
#define B3D_AUDIO_ACTION_MACHINE 2
#define B3D_AUDIO_ACTION_PUMP 3
#define B3D_AUDIO_ACTION_REVOLVER 4
#define B3D_AUDIO_ACTION_RIFLE 5

#define B3D_AUDIO_MAG_NONE 0
#define B3D_AUDIO_MAG_PISTOL_POLYMER 1
#define B3D_AUDIO_MAG_PISTOL_METAL 2
#define B3D_AUDIO_MAG_SMG_STEEL 3
#define B3D_AUDIO_MAG_RIFLE_POLYMER 4
#define B3D_AUDIO_MAG_RIFLE_ALUMINUM 5
#define B3D_AUDIO_MAG_SNIPER_BOX 6
#define B3D_AUDIO_MAG_DRUM_HEAVY 7

#define B3D_AUDIO_AMMO_NONE 0
#define B3D_AUDIO_AMMO_BOX_MAG 1
#define B3D_AUDIO_AMMO_TUBE 2
#define B3D_AUDIO_AMMO_LOOSE_SHELLS 3
#define B3D_AUDIO_AMMO_BELT_BOX 4

#define B3D_AUDIO_MUZZLE_BARE 0
#define B3D_AUDIO_MUZZLE_BRAKE 1
#define B3D_AUDIO_MUZZLE_COMPENSATOR 2
#define B3D_AUDIO_MUZZLE_PORTED 3

#define B3D_AUDIO_SHELL_NONE 0
#define B3D_AUDIO_SHELL_PISTOL_BRASS 1
#define B3D_AUDIO_SHELL_STEEL_CASE 2
#define B3D_AUDIO_SHELL_SHOTGUN_PLASTIC 3
#define B3D_AUDIO_SHELL_MAGNUM_BRASS 4
#define B3D_AUDIO_SHELL_RIFLE_BRASS 5
#define B3D_AUDIO_SHELL_SHOTGUN_BRASS 6
#define B3D_AUDIO_SHELL_RIMFIRE_BRASS 7

#define B3D_AUDIO_PROJECTILE_AUTO 0
#define B3D_AUDIO_PROJECTILE_NONE 1
#define B3D_AUDIO_PROJECTILE_NEAR_MISS 2
#define B3D_AUDIO_PROJECTILE_SUPERSONIC 3
#define B3D_AUDIO_PROJECTILE_PELLET_SWARM 4
#define B3D_AUDIO_PROJECTILE_TRACER 5

#define B3D_AUDIO_EXPLOSION_NONE 0
#define B3D_AUDIO_EXPLOSION_GRENADE 1
#define B3D_AUDIO_EXPLOSION_ROCKET 2

/* Runtime composition loaded from INI. Strings are owned by this object, so
   no parser buffer or temporary token may leak into the live registry. */
typedef struct Blank3DWeaponModulesTag {
    int weapon_id;
    char name[B3D_WEAPON_NAME_CAPACITY];
    int trigger_model;
    int physics_backend;
    int trail_profile;
    int optic_profile;
    int sway_enabled;
    int aim_query_enabled;
    int emit_muzzle;
    int emit_casing;
    int emit_trail;
    int projectile_mesh_id;
    int casing_mesh_id;
    char scope_preset_name[B3D_WEAPON_NAME_CAPACITY];

    unsigned short charge_time_ms;
    long charge_min_speed_q16;
    long charge_max_speed_q16;
    long gravity_q16;
    long bounce_q16;
    long drag_q16;
    long mass_q16;

    int audio_enabled;
    int audio_profile;
    int audio_action;
    int audio_magazine;
    int audio_ammo;
    int audio_muzzle;
    int audio_shell;
    int audio_detachable_magazine;
    int audio_emits_casing;
    int audio_projectile;
    int audio_explosion;
    int audio_continuous_rocket;
    int audio_fire_gain_q15;
    int audio_pressure_energy_q15;
    int audio_ammo_motion_q15;
    int audio_magazine_velocity_q15;
    int audio_magazine_gain_q15;
    int audio_reload_remove_motion_q15;
    int audio_reload_insert_motion_q15;
    int audio_dry_receiver_impulse;
    unsigned long audio_action_speed_q16;
    int audio_casing_velocity;
    int audio_casing_angular_velocity;
    int audio_casing_gain_q15;
    int audio_projectile_gain_q15;
    int audio_projectile_proximity_q15;
    int audio_projectile_instance_limit;
    int audio_explosion_gain_q15;
    int audio_rocket_whistle_gain_q15;
    int audio_rocket_spin_gain_q15;
} Blank3DWeaponModules;

void blank3d_weapon_modules_reset(void);
void blank3d_weapon_modules_load_defaults(void);
int blank3d_weapon_modules_register(const Blank3DWeaponModules *modules);
int blank3d_weapon_modules_count(void);
const Blank3DWeaponModules *blank3d_weapon_modules_get(int weapon_id);
int blank3d_weapon_modules_has_casing(int weapon_id);
int blank3d_weapon_modules_has_muzzle(int weapon_id);
int blank3d_weapon_modules_has_trail(int weapon_id);
long blank3d_weapon_modules_charge_speed_q16(int weapon_id,
                                             unsigned int charge_ms);
long blank3d_weapon_modules_charge_ratio_q16(int weapon_id,
                                             unsigned int charge_ms);

#ifdef __cplusplus
}
#endif

#endif
