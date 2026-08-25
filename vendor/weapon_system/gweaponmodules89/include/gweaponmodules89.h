#ifndef GWEAPONMODULES89_H
#define GWEAPONMODULES89_H

#include "morethanone89.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GWM89_WEAPON_ID_PISTOL 1
#define GWM89_WEAPON_ID_MACHINE_GUN 2
#define GWM89_WEAPON_ID_SHOTGUN 3
#define GWM89_WEAPON_ID_MAGNUM 4
#define GWM89_WEAPON_ID_SNIPER 5
#define GWM89_WEAPON_ID_GRENADE_LAUNCHER 6
#define GWM89_WEAPON_ID_ROCKET_LAUNCHER 7
#define GWM89_WEAPON_ID_GATLING 8
#define GWM89_WEAPON_ID_SLINGSHOT 9
#define GWM89_WEAPON_ID_HAND_GRENADE 10
#define GWM89_WEAPON_ID_UZI 11
#define GWM89_WEAPON_ID_SHANGO 12
#define GWM89_WEAPON_ID_HOMING_ROCKET_LAUNCHER 13
#define GWM89_WEAPON_ID_FLAMETHROWER 14
#define GWM89_WEAPON_ID_BUSTER 15
#define GWM89_MODULE_CAPACITY 16
#define GWM89_NAME_CAPACITY 64

#define GWM89_TRIGGER_STANDARD 0
#define GWM89_TRIGGER_SPINUP 1
#define GWM89_TRIGGER_CHARGE_RELEASE 2
#define GWM89_TRIGGER_PRESS_CHARGE_RELEASE 3

#define GWM89_PHYSICS_LINEAR 0
#define GWM89_PHYSICS_GRAVITY 1
#define GWM89_PHYSICS_BOLT3D 2

#define GWM89_TRAIL_NONE 0
#define GWM89_TRAIL_BULLET 1
#define GWM89_TRAIL_TRACER 2
#define GWM89_TRAIL_HEAVY 3
#define GWM89_TRAIL_ARC 4

#define GWM89_OPTIC_NONE 0
#define GWM89_OPTIC_GENERIC 1
#define GWM89_OPTIC_SNIPER 2

#define GWM89_AUDIO_PROFILE_NONE 0
#define GWM89_AUDIO_PROFILE_PISTOL 1
#define GWM89_AUDIO_PROFILE_SMG 2
#define GWM89_AUDIO_PROFILE_SHOTGUN 3
#define GWM89_AUDIO_PROFILE_MAGNUM 4
#define GWM89_AUDIO_PROFILE_SNIPER 5
#define GWM89_AUDIO_PROFILE_LAUNCHER 6
#define GWM89_AUDIO_PROFILE_HEAVY 7

#define GWM89_AUDIO_ACTION_NONE 0
#define GWM89_AUDIO_ACTION_PISTOL 1
#define GWM89_AUDIO_ACTION_MACHINE 2
#define GWM89_AUDIO_ACTION_PUMP 3
#define GWM89_AUDIO_ACTION_REVOLVER 4
#define GWM89_AUDIO_ACTION_RIFLE 5

#define GWM89_AUDIO_MAG_NONE 0
#define GWM89_AUDIO_MAG_PISTOL_POLYMER 1
#define GWM89_AUDIO_MAG_PISTOL_METAL 2
#define GWM89_AUDIO_MAG_SMG_STEEL 3
#define GWM89_AUDIO_MAG_RIFLE_POLYMER 4
#define GWM89_AUDIO_MAG_RIFLE_ALUMINUM 5
#define GWM89_AUDIO_MAG_SNIPER_BOX 6
#define GWM89_AUDIO_MAG_DRUM_HEAVY 7

#define GWM89_AUDIO_AMMO_NONE 0
#define GWM89_AUDIO_AMMO_BOX_MAG 1
#define GWM89_AUDIO_AMMO_TUBE 2
#define GWM89_AUDIO_AMMO_LOOSE_SHELLS 3
#define GWM89_AUDIO_AMMO_BELT_BOX 4

#define GWM89_AUDIO_MUZZLE_BARE 0
#define GWM89_AUDIO_MUZZLE_BRAKE 1
#define GWM89_AUDIO_MUZZLE_COMPENSATOR 2
#define GWM89_AUDIO_MUZZLE_PORTED 3

#define GWM89_AUDIO_SHELL_NONE 0
#define GWM89_AUDIO_SHELL_PISTOL_BRASS 1
#define GWM89_AUDIO_SHELL_STEEL_CASE 2
#define GWM89_AUDIO_SHELL_SHOTGUN_PLASTIC 3
#define GWM89_AUDIO_SHELL_MAGNUM_BRASS 4
#define GWM89_AUDIO_SHELL_RIFLE_BRASS 5
#define GWM89_AUDIO_SHELL_SHOTGUN_BRASS 6
#define GWM89_AUDIO_SHELL_RIMFIRE_BRASS 7

#define GWM89_AUDIO_PROJECTILE_AUTO 0
#define GWM89_AUDIO_PROJECTILE_NONE 1
#define GWM89_AUDIO_PROJECTILE_NEAR_MISS 2
#define GWM89_AUDIO_PROJECTILE_SUPERSONIC 3
#define GWM89_AUDIO_PROJECTILE_PELLET_SWARM 4
#define GWM89_AUDIO_PROJECTILE_TRACER 5

#define GWM89_AUDIO_EXPLOSION_NONE 0
#define GWM89_AUDIO_EXPLOSION_GRENADE 1
#define GWM89_AUDIO_EXPLOSION_ROCKET 2

#define GWM89_PROJECTILE_VISUAL_DEFAULT 0
#define GWM89_PROJECTILE_VISUAL_SHANGO_PRESS 1
#define GWM89_PROJECTILE_VISUAL_EXPANDIBLE_FIRE 2

/* Generic sprite-source mode for projectile billboards. These values describe
   authoring/input representation only; gameplay physics remain independent. */
#define GWM89_PROJECTILE_SPRITE_NONE      0
#define GWM89_PROJECTILE_SPRITE_STATIC    1
#define GWM89_PROJECTILE_SPRITE_SEQUENCE  2
#define GWM89_PROJECTILE_SPRITE_RENLIST   3
#define GWM89_PROJECTILE_SPRITE_TILECELL  4
#define GWM89_PROJECTILE_SPRITE_GMSTRIP   5

#define GWM89_PROJECTILE_SPRITE_LOOP_NONE      0
#define GWM89_PROJECTILE_SPRITE_LOOP_FORWARD   1
#define GWM89_PROJECTILE_SPRITE_LOOP_REVERSE   2
#define GWM89_PROJECTILE_SPRITE_LOOP_PINGPONG  3
#define GWM89_PROJECTILE_SPRITE_LOOP_HOLD      4

#define GWM89_MUZZLE_IMAGE_BILLBOARD_CAMERA 0
#define GWM89_MUZZLE_IMAGE_BILLBOARD_VIEW   1
#define GWM89_MUZZLE_IMAGE_BILLBOARD_FIXED  2
#define GWM89_MUZZLE_IMAGE_BLEND_ALPHA      0
#define GWM89_MUZZLE_IMAGE_BLEND_ADDITIVE   1

/* Runtime composition loaded from INI. Strings are owned by this object, so
   no parser buffer or temporary token may leak into the live registry. */
typedef struct GWeaponModules89Tag {
    int weapon_id;
    char name[GWM89_NAME_CAPACITY];
    int trigger_model;
    int physics_backend;
    int trail_profile;
    int optic_profile;
    int sway_enabled;
    int aim_query_enabled;
    int emit_muzzle;
    /* Optional raster muzzle presentation.  The weapon system stores only a
       renderer-neutral recipe; the host may decode/render it with any image
       backend. Empty path keeps the legacy/vector/procedural muzzle path. */
    char muzzle_image_path[GWM89_NAME_CAPACITY];
    long muzzle_image_width_q16;
    long muzzle_image_height_q16;
    unsigned short muzzle_image_life_ms;
    int muzzle_image_billboard;
    int muzzle_image_blend;
    /* Optional host-rendered additive halo.  This is a second raster pass
       using the same source image; no pre-baked bloom texture is required. */
    int muzzle_image_glow;
    long muzzle_image_glow_scale_q16;
    unsigned short muzzle_image_glow_alpha;
    /* Optional short-lived point-light pulse at the muzzle. Simulation and
       recipe values remain fixed-point; only the renderer converts them at
       the OpenGL boundary. */
    int muzzle_light;
    unsigned short muzzle_light_life_ms;
    long muzzle_light_intensity_q16;
    long muzzle_light_radius_q16;
    unsigned short muzzle_light_r;
    unsigned short muzzle_light_g;
    unsigned short muzzle_light_b;
    int emit_casing;
    int emit_trail;
    int projectile_mesh_id;
    int casing_mesh_id;

    /* Optional projectile-origin and homing modules. Q16 values are converted
       by the host to its world fixed-point format. */
    int satellaborner_enabled;
    long satellaborner_offset_x_q16;
    long satellaborner_offset_y_q16;
    long satellaborner_offset_z_q16;
    int telesearcher_enabled;
    int telesearcher_reacquire;
    long telesearcher_gain_q16;
    long telesearcher_offset_x_q16;
    long telesearcher_offset_y_q16;
    long telesearcher_offset_z_q16;

    /* Generic expanding projectile behaviour. expandiblefire89 owns the
       distance/growth state; the host applies the returned scale to its
       renderer mesh and, optionally, to the gameplay collision radius. */
    int expandible_fire_enabled;
    long expandible_fire_scale_start_q16;
    long expandible_fire_scale_end_q16;
    long expandible_fire_growth_distance_q16;
    long expandible_fire_kill_distance_q16;
    int expandible_fire_collision_growth;

    /* Generic geometric projectile-origin composition. bulletcircle89 maps a
       slot to a circular muzzle point, bulletspin89 advances that slot per
       accepted shot, and bulletinline89 converges the displaced origin toward
       the shared aim point. */
    int bullet_circle_enabled;
    int bullet_circle_count;
    long bullet_circle_radius_q16;
    long bullet_circle_phase_turn_q16;
    int bullet_spin_enabled;
    int bullet_spin_start_slot;
    int bullet_spin_step;
    int bullet_spin_direction;
    int bullet_inline_enabled;
    long bullet_inline_distance_q16;

    /* Force the player down the real projectile lifetime path instead of the
       camera-hitscan optimization. NPC projectiles are already physical. */
    int player_physical_projectile_enabled;

    /* Optional Buster-style multi-projectile charge selector. Charging/input
       remain external; this is only the fixed stage recipe. */
    mto89_profile more_than_one;

    /* Optional projectile presentation recipe. The core projectile remains
       renderer-agnostic; these fixed-point values only describe visual scale. */
    int projectile_visual;
    unsigned short projectile_visual_expand_ms;
    unsigned short projectile_visual_crush_ms;
    long projectile_visual_radial_start_q16;
    long projectile_visual_radial_peak_q16;
    long projectile_visual_radial_end_q16;
    long projectile_visual_axial_start_q16;
    long projectile_visual_axial_end_q16;
    int projectile_visual_unlit;
    int projectile_visual_additive;

    /* Optional generic sprite skin for any projectile visual. The selected
       mode is implemented by one of the five independent sprite microvendors:
       StaticSprite89, ImageSequencer89, RenList89, TileCell89 or
       GMSpritestrip89. Empty/none leaves the legacy mesh path untouched. */
    int projectile_sprite_mode;
    int projectile_visual_billboard;
    int projectile_sprite_loop;
    char projectile_visual_recipe[GWM89_NAME_CAPACITY];
    char projectile_visual_animation[GWM89_NAME_CAPACITY];
    char projectile_visual_clip[GWM89_NAME_CAPACITY];

    /* Image/atlas request. For static/tile/gmstrip it is normally an
       AssetRoute89 image request. Sequence/RenList frames can override it. */
    char projectile_visual_image[GWM89_NAME_CAPACITY];
    unsigned short projectile_visual_frame_width;
    unsigned short projectile_visual_frame_height;
    unsigned short projectile_visual_columns;
    unsigned short projectile_visual_rows;
    unsigned short projectile_visual_margin_x;
    unsigned short projectile_visual_margin_y;
    unsigned short projectile_visual_spacing_x;
    unsigned short projectile_visual_spacing_y;
    unsigned short projectile_visual_start_x;
    unsigned short projectile_visual_start_y;
    short projectile_visual_step_x;
    short projectile_visual_step_y;
    unsigned short projectile_visual_frame_count;
    unsigned short projectile_visual_frame_ms;
    unsigned short projectile_visual_phase_step;
    long projectile_visual_width_scale_q16;
    long projectile_visual_height_scale_q16;
    int projectile_visual_glow;
    long projectile_visual_glow_scale_q16;
    unsigned short projectile_visual_glow_alpha;
    int projectile_visual_core;
    long projectile_visual_core_scale_q16;
    unsigned short projectile_visual_core_alpha;
    int projectile_visual_light;
    long projectile_visual_light_intensity_q16;
    long projectile_visual_light_radius_q16;
    unsigned short projectile_visual_light_r;
    unsigned short projectile_visual_light_g;
    unsigned short projectile_visual_light_b;

    char scope_recipe_path[GWM89_NAME_CAPACITY];
    char scope_preset_name[GWM89_NAME_CAPACITY]; /* legacy fallback */
    char crosshair_preset_name[GWM89_NAME_CAPACITY];
    /* Optional camera-specific crosshair recipes. Empty = use base preset. */
    char crosshair_preset_first_person[GWM89_NAME_CAPACITY];
    char crosshair_preset_third_person[GWM89_NAME_CAPACITY];

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
} GWeaponModules89;

void gweaponmodules89_reset(void);
void gweaponmodules89_load_defaults(void);
int gweaponmodules89_register(const GWeaponModules89 *modules);
int gweaponmodules89_count(void);
const GWeaponModules89 *gweaponmodules89_get(int weapon_id);
int gweaponmodules89_has_casing(int weapon_id);
int gweaponmodules89_has_muzzle(int weapon_id);
int gweaponmodules89_has_trail(int weapon_id);
long gweaponmodules89_charge_speed_q16(int weapon_id,
                                             unsigned int charge_ms);
long gweaponmodules89_charge_ratio_q16(int weapon_id,
                                             unsigned int charge_ms);

#ifdef __cplusplus
}
#endif

#endif
