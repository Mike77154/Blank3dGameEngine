#include "gweaponmodules89.h"

#include <string.h>

#define Q16_ONE 65536L
#define Q16_FROM_INT(v) ((long)(v) * Q16_ONE)

static GWeaponModules89 b3d_modules[GWM89_MODULE_CAPACITY];
static int b3d_module_count;

static void b3d_copy(char *dst, const char *src)
{
    if (!dst) return;
    if (!src) src = "";
    strncpy(dst, src, GWM89_NAME_CAPACITY - 1U);
    dst[GWM89_NAME_CAPACITY - 1U] = '\0';
}

static void b3d_default_module(GWeaponModules89 *m,
                               int id, const char *name)
{
    memset(m, 0, sizeof(*m));
    m->weapon_id = id;
    b3d_copy(m->name, name);
    m->trigger_model = GWM89_TRIGGER_STANDARD;
    m->physics_backend = GWM89_PHYSICS_LINEAR;
    m->trail_profile = GWM89_TRAIL_BULLET;
    m->optic_profile = GWM89_OPTIC_GENERIC;
    m->emit_muzzle = 1;
    m->muzzle_image_width_q16 = 45875L;  /* 0.70 Q16 */
    m->muzzle_image_height_q16 = 45875L; /* 0.70 Q16 */
    m->muzzle_image_life_ms = 70U;
    m->muzzle_image_billboard = GWM89_MUZZLE_IMAGE_BILLBOARD_CAMERA;
    m->muzzle_image_blend = GWM89_MUZZLE_IMAGE_BLEND_ADDITIVE;
    m->muzzle_image_glow = 0;
    m->muzzle_image_glow_scale_q16 = 88474L; /* 1.35 Q16 */
    m->muzzle_image_glow_alpha = 96U;
    m->muzzle_light = 0;
    m->muzzle_light_life_ms = 70U;
    m->muzzle_light_intensity_q16 = Q16_FROM_INT(2);
    m->muzzle_light_radius_q16 = Q16_FROM_INT(7);
    m->muzzle_light_r = 255U;
    m->muzzle_light_g = 205U;
    m->muzzle_light_b = 120U;
    m->emit_casing = 1;
    m->emit_trail = 1;
    m->projectile_mesh_id = id;
    m->casing_mesh_id = id;
    m->satellaborner_enabled = 0;
    m->telesearcher_enabled = 0;
    m->telesearcher_reacquire = 1;
    m->telesearcher_gain_q16 = Q16_ONE;
    m->expandible_fire_enabled = 0;
    m->expandible_fire_scale_start_q16 = Q16_ONE;
    m->expandible_fire_scale_end_q16 = Q16_ONE;
    m->expandible_fire_growth_distance_q16 = Q16_ONE;
    m->expandible_fire_kill_distance_q16 = 0L;
    m->expandible_fire_collision_growth = 1;
    m->bullet_circle_enabled = 0;
    m->bullet_circle_count = 1;
    m->bullet_circle_radius_q16 = 0L;
    m->bullet_circle_phase_turn_q16 = 0L;
    m->bullet_spin_enabled = 0;
    m->bullet_spin_start_slot = 0;
    m->bullet_spin_step = 1;
    m->bullet_spin_direction = 1;
    m->bullet_inline_enabled = 0;
    m->bullet_inline_distance_q16 = 0L;
    m->projectile_visual = GWM89_PROJECTILE_VISUAL_DEFAULT;
    m->projectile_visual_radial_start_q16 = Q16_ONE;
    m->projectile_visual_radial_peak_q16 = Q16_ONE;
    m->projectile_visual_radial_end_q16 = Q16_ONE;
    m->projectile_visual_axial_start_q16 = Q16_ONE;
    m->projectile_visual_axial_end_q16 = Q16_ONE;
    m->projectile_sprite_mode = GWM89_PROJECTILE_SPRITE_NONE;
    m->projectile_visual_billboard = 1;
    m->projectile_sprite_loop = GWM89_PROJECTILE_SPRITE_LOOP_FORWARD;
    b3d_copy(m->projectile_visual_clip, "default");
    m->projectile_visual_frame_width = 0U;
    m->projectile_visual_frame_height = 0U;
    m->projectile_visual_columns = 1U;
    m->projectile_visual_rows = 1U;
    m->projectile_visual_margin_x = 0U;
    m->projectile_visual_margin_y = 0U;
    m->projectile_visual_spacing_x = 0U;
    m->projectile_visual_spacing_y = 0U;
    m->projectile_visual_start_x = 0U;
    m->projectile_visual_start_y = 0U;
    m->projectile_visual_step_x = 1;
    m->projectile_visual_step_y = 0;
    m->projectile_visual_frame_count = 1U;
    m->projectile_visual_frame_ms = 33U;
    m->projectile_visual_phase_step = 0U;
    m->projectile_visual_width_scale_q16 = Q16_ONE;
    m->projectile_visual_height_scale_q16 = Q16_ONE;
    m->projectile_visual_glow = 0;
    m->projectile_visual_glow_scale_q16 = 85197L; /* 1.30 Q16 */
    m->projectile_visual_glow_alpha = 96U;
    m->projectile_visual_core = 0;
    m->projectile_visual_core_scale_q16 = 47186L; /* 0.72 Q16 */
    m->projectile_visual_core_alpha = 160U;
    m->projectile_visual_light = 0;
    m->projectile_visual_light_intensity_q16 = Q16_FROM_INT(2);
    m->projectile_visual_light_radius_q16 = Q16_FROM_INT(8);
    m->projectile_visual_light_r = 255U;
    m->projectile_visual_light_g = 128U;
    m->projectile_visual_light_b = 48U;
    b3d_copy(m->crosshair_preset_name, "default");
    m->mass_q16 = Q16_ONE;
    m->audio_enabled = 1;
    m->audio_profile = GWM89_AUDIO_PROFILE_PISTOL;
    m->audio_action = GWM89_AUDIO_ACTION_PISTOL;
    m->audio_magazine = GWM89_AUDIO_MAG_PISTOL_POLYMER;
    m->audio_ammo = GWM89_AUDIO_AMMO_BOX_MAG;
    m->audio_muzzle = GWM89_AUDIO_MUZZLE_BARE;
    m->audio_shell = GWM89_AUDIO_SHELL_PISTOL_BRASS;
    m->audio_detachable_magazine = 1;
    m->audio_emits_casing = 1;
    m->audio_projectile = GWM89_AUDIO_PROJECTILE_AUTO;
    m->audio_fire_gain_q15 = 29200;
    m->audio_pressure_energy_q15 = 0;
    m->audio_ammo_motion_q15 = 11800;
    m->audio_magazine_velocity_q15 = 23500;
    m->audio_magazine_gain_q15 = 24800;
    m->audio_reload_remove_motion_q15 = 27000;
    m->audio_reload_insert_motion_q15 = 30500;
    m->audio_dry_receiver_impulse = 4300;
    m->audio_action_speed_q16 = 65536UL;
    m->audio_casing_velocity = 150;
    m->audio_casing_angular_velocity = 180;
    m->audio_casing_gain_q15 = 24500;
    m->audio_projectile_gain_q15 = 21000;
    m->audio_projectile_proximity_q15 = 23000;
    m->audio_projectile_instance_limit = 24;
    m->audio_explosion_gain_q15 = 29200;
    m->audio_rocket_whistle_gain_q15 = 10500;
    m->audio_rocket_spin_gain_q15 = 5900;
}

void gweaponmodules89_reset(void)
{
    memset(b3d_modules, 0, sizeof(b3d_modules));
    b3d_module_count = 0;
}

int gweaponmodules89_register(const GWeaponModules89 *modules)
{
    int i;
    if (!modules || modules->weapon_id <= 0) return 0;
    for (i = 0; i < b3d_module_count; ++i) {
        if (b3d_modules[i].weapon_id == modules->weapon_id) {
            b3d_modules[i] = *modules;
            return 1;
        }
    }
    if (b3d_module_count >= GWM89_MODULE_CAPACITY) return 0;
    b3d_modules[b3d_module_count++] = *modules;
    return 1;
}

void gweaponmodules89_load_defaults(void)
{
    GWeaponModules89 m;
    gweaponmodules89_reset();

    b3d_default_module(&m, 1, "pistol");
    m.projectile_mesh_id = 1; m.casing_mesh_id = 1;
    (void)gweaponmodules89_register(&m);

    b3d_default_module(&m, 2, "machine_gun");
    m.trail_profile = GWM89_TRAIL_TRACER;
    m.projectile_mesh_id = 2; m.casing_mesh_id = 2;
    m.audio_profile = GWM89_AUDIO_PROFILE_SMG;
    m.audio_action = GWM89_AUDIO_ACTION_MACHINE;
    m.audio_magazine = GWM89_AUDIO_MAG_SMG_STEEL;
    m.audio_muzzle = GWM89_AUDIO_MUZZLE_COMPENSATOR;
    m.audio_shell = GWM89_AUDIO_SHELL_STEEL_CASE;
    m.audio_fire_gain_q15 = 27400;
    m.audio_dry_receiver_impulse = 3800;
    m.audio_action_speed_q16 = 78643UL;
    (void)gweaponmodules89_register(&m);

    b3d_default_module(&m, 3, "shotgun");
    m.optic_profile = GWM89_OPTIC_NONE;
    m.projectile_mesh_id = 3; m.casing_mesh_id = 3;
    m.audio_profile = GWM89_AUDIO_PROFILE_SHOTGUN;
    m.audio_action = GWM89_AUDIO_ACTION_PUMP;
    m.audio_magazine = GWM89_AUDIO_MAG_RIFLE_POLYMER;
    m.audio_ammo = GWM89_AUDIO_AMMO_TUBE;
    m.audio_shell = GWM89_AUDIO_SHELL_SHOTGUN_PLASTIC;
    m.audio_detachable_magazine = 0;
    m.audio_projectile = GWM89_AUDIO_PROJECTILE_PELLET_SWARM;
    m.audio_fire_gain_q15 = 30700;
    m.audio_dry_receiver_impulse = 5200;
    m.audio_action_speed_q16 = 58982UL;
    (void)gweaponmodules89_register(&m);

    b3d_default_module(&m, 4, "magnum");
    m.trail_profile = GWM89_TRAIL_HEAVY;
    m.projectile_mesh_id = 4; m.casing_mesh_id = 4;
    m.audio_profile = GWM89_AUDIO_PROFILE_MAGNUM;
    m.audio_action = GWM89_AUDIO_ACTION_REVOLVER;
    m.audio_magazine = GWM89_AUDIO_MAG_PISTOL_METAL;
    m.audio_ammo = GWM89_AUDIO_AMMO_LOOSE_SHELLS;
    m.audio_muzzle = GWM89_AUDIO_MUZZLE_PORTED;
    m.audio_shell = GWM89_AUDIO_SHELL_MAGNUM_BRASS;
    m.audio_detachable_magazine = 0;
    m.audio_fire_gain_q15 = 31500;
    m.audio_dry_receiver_impulse = 6100;
    m.audio_action_speed_q16 = 52429UL;
    (void)gweaponmodules89_register(&m);

    b3d_default_module(&m, 5, "sniper");
    m.trail_profile = GWM89_TRAIL_TRACER;
    m.optic_profile = GWM89_OPTIC_SNIPER;
    m.sway_enabled = 1;
    m.aim_query_enabled = 1;
    m.projectile_mesh_id = 5; m.casing_mesh_id = 5;
    b3d_copy(m.scope_recipe_path, "config/scope/hud/sniper_re5_psg1.ini");
    b3d_copy(m.scope_preset_name, "re5_psg1_game_scope");
    m.audio_profile = GWM89_AUDIO_PROFILE_SNIPER;
    m.audio_action = GWM89_AUDIO_ACTION_RIFLE;
    m.audio_magazine = GWM89_AUDIO_MAG_SNIPER_BOX;
    m.audio_muzzle = GWM89_AUDIO_MUZZLE_BRAKE;
    m.audio_shell = GWM89_AUDIO_SHELL_RIFLE_BRASS;
    m.audio_fire_gain_q15 = 31000;
    m.audio_dry_receiver_impulse = 5600;
    m.audio_action_speed_q16 = 49152UL;
    (void)gweaponmodules89_register(&m);

    b3d_default_module(&m, 6, "grenade_launcher");
    m.physics_backend = GWM89_PHYSICS_GRAVITY;
    m.trail_profile = GWM89_TRAIL_ARC;
    m.projectile_mesh_id = 6; m.casing_mesh_id = 0;
    m.gravity_q16 = -Q16_FROM_INT(7);
    m.bounce_q16 = Q16_ONE / 4L;
    m.audio_profile = GWM89_AUDIO_PROFILE_LAUNCHER;
    m.audio_action = GWM89_AUDIO_ACTION_PUMP;
    m.audio_magazine = GWM89_AUDIO_MAG_RIFLE_ALUMINUM;
    m.audio_ammo = GWM89_AUDIO_AMMO_LOOSE_SHELLS;
    m.audio_shell = GWM89_AUDIO_SHELL_SHOTGUN_BRASS;
    m.audio_detachable_magazine = 0;
    m.audio_projectile = GWM89_AUDIO_PROJECTILE_TRACER;
    m.audio_explosion = GWM89_AUDIO_EXPLOSION_GRENADE;
    m.audio_fire_gain_q15 = 30600;
    m.audio_dry_receiver_impulse = 5900;
    m.audio_action_speed_q16 = 45875UL;
    (void)gweaponmodules89_register(&m);

    b3d_default_module(&m, 7, "rocket_launcher");
    m.trail_profile = GWM89_TRAIL_HEAVY;
    m.projectile_mesh_id = 7; m.casing_mesh_id = 0;
    m.emit_casing = 0;
    b3d_copy(m.scope_preset_name, "rpg7_pgo7");
    m.audio_profile = GWM89_AUDIO_PROFILE_LAUNCHER;
    m.audio_action = GWM89_AUDIO_ACTION_RIFLE;
    m.audio_magazine = GWM89_AUDIO_MAG_RIFLE_ALUMINUM;
    m.audio_ammo = GWM89_AUDIO_AMMO_LOOSE_SHELLS;
    m.audio_shell = GWM89_AUDIO_SHELL_RIFLE_BRASS;
    m.audio_detachable_magazine = 0;
    m.audio_emits_casing = 0;
    m.audio_projectile = GWM89_AUDIO_PROJECTILE_TRACER;
    m.audio_explosion = GWM89_AUDIO_EXPLOSION_ROCKET;
    m.audio_continuous_rocket = 1;
    m.audio_fire_gain_q15 = 31500;
    m.audio_dry_receiver_impulse = 6400;
    m.audio_action_speed_q16 = 42598UL;
    (void)gweaponmodules89_register(&m);

    b3d_default_module(&m, 8, "gatling_gun");
    m.trigger_model = GWM89_TRIGGER_SPINUP;
    m.trail_profile = GWM89_TRAIL_TRACER;
    m.optic_profile = GWM89_OPTIC_NONE;
    m.projectile_mesh_id = 2; m.casing_mesh_id = 2;
    m.audio_profile = GWM89_AUDIO_PROFILE_HEAVY;
    m.audio_action = GWM89_AUDIO_ACTION_MACHINE;
    m.audio_magazine = GWM89_AUDIO_MAG_DRUM_HEAVY;
    m.audio_ammo = GWM89_AUDIO_AMMO_BELT_BOX;
    m.audio_muzzle = GWM89_AUDIO_MUZZLE_COMPENSATOR;
    m.audio_shell = GWM89_AUDIO_SHELL_RIFLE_BRASS;
    m.audio_projectile = GWM89_AUDIO_PROJECTILE_TRACER;
    m.audio_magazine_velocity_q15 = 28500;
    m.audio_magazine_gain_q15 = 22800;
    m.audio_fire_gain_q15 = 23800;
    m.audio_dry_receiver_impulse = 3600;
    m.audio_action_speed_q16 = 98304UL;
    (void)gweaponmodules89_register(&m);

    b3d_default_module(&m, 9, "slingshot");
    m.trigger_model = GWM89_TRIGGER_CHARGE_RELEASE;
    m.physics_backend = GWM89_PHYSICS_BOLT3D;
    m.trail_profile = GWM89_TRAIL_ARC;
    m.optic_profile = GWM89_OPTIC_NONE;
    m.emit_muzzle = 0;
    m.emit_casing = 0;
    m.projectile_mesh_id = 9; m.casing_mesh_id = 0;
    m.charge_time_ms = 900U;
    m.charge_min_speed_q16 = Q16_FROM_INT(14);
    m.charge_max_speed_q16 = Q16_FROM_INT(46);
    m.gravity_q16 = -Q16_FROM_INT(13);
    m.bounce_q16 = (Q16_ONE * 35L) / 100L;
    m.drag_q16 = (Q16_ONE * 2L) / 100L;
    m.mass_q16 = (Q16_ONE * 18L) / 100L;
    m.audio_enabled = 0;
    m.audio_profile = GWM89_AUDIO_PROFILE_NONE;
    m.audio_action = GWM89_AUDIO_ACTION_NONE;
    m.audio_magazine = GWM89_AUDIO_MAG_NONE;
    m.audio_ammo = GWM89_AUDIO_AMMO_NONE;
    m.audio_shell = GWM89_AUDIO_SHELL_NONE;
    m.audio_detachable_magazine = 0;
    m.audio_emits_casing = 0;
    m.audio_projectile = GWM89_AUDIO_PROJECTILE_NONE;
    m.audio_fire_gain_q15 = 0;
    m.audio_dry_receiver_impulse = 0;
    (void)gweaponmodules89_register(&m);

    b3d_default_module(&m, 10, "hand_grenade");
    m.physics_backend = GWM89_PHYSICS_GRAVITY;
    m.trail_profile = GWM89_TRAIL_ARC;
    m.optic_profile = GWM89_OPTIC_NONE;
    m.emit_muzzle = 0;
    m.emit_casing = 0;
    m.projectile_mesh_id = 10; m.casing_mesh_id = 0;
    m.gravity_q16 = -Q16_FROM_INT(12);
    m.bounce_q16 = (Q16_ONE * 38L) / 100L;
    m.drag_q16 = (Q16_ONE * 2L) / 100L;
    m.mass_q16 = Q16_ONE;
    m.audio_profile = GWM89_AUDIO_PROFILE_LAUNCHER;
    m.audio_action = GWM89_AUDIO_ACTION_NONE;
    m.audio_magazine = GWM89_AUDIO_MAG_NONE;
    m.audio_ammo = GWM89_AUDIO_AMMO_LOOSE_SHELLS;
    m.audio_shell = GWM89_AUDIO_SHELL_NONE;
    m.audio_detachable_magazine = 0;
    m.audio_emits_casing = 0;
    m.audio_projectile = GWM89_AUDIO_PROJECTILE_NONE;
    m.audio_explosion = GWM89_AUDIO_EXPLOSION_GRENADE;
    m.audio_fire_gain_q15 = 0;
    (void)gweaponmodules89_register(&m);
}

int gweaponmodules89_count(void)
{
    if (b3d_module_count == 0) gweaponmodules89_load_defaults();
    return b3d_module_count;
}

const GWeaponModules89 *gweaponmodules89_get(int weapon_id)
{
    int i;
    if (b3d_module_count == 0) gweaponmodules89_load_defaults();
    for (i = 0; i < b3d_module_count; ++i) {
        if (b3d_modules[i].weapon_id == weapon_id) return &b3d_modules[i];
    }
    return 0;
}

int gweaponmodules89_has_casing(int weapon_id)
{
    const GWeaponModules89 *m;
    m = gweaponmodules89_get(weapon_id);
    return m ? m->emit_casing : 1;
}

int gweaponmodules89_has_muzzle(int weapon_id)
{
    const GWeaponModules89 *m;
    m = gweaponmodules89_get(weapon_id);
    return m ? m->emit_muzzle : 1;
}

int gweaponmodules89_has_trail(int weapon_id)
{
    const GWeaponModules89 *m;
    m = gweaponmodules89_get(weapon_id);
    return m ? m->emit_trail : 1;
}

long gweaponmodules89_charge_ratio_q16(int weapon_id,
                                             unsigned int charge_ms)
{
    const GWeaponModules89 *m;
    unsigned long scaled;
    m = gweaponmodules89_get(weapon_id);
    if (!m || m->trigger_model != GWM89_TRIGGER_CHARGE_RELEASE ||
        m->charge_time_ms == 0U) return Q16_ONE;
    if (charge_ms >= (unsigned int)m->charge_time_ms) return Q16_ONE;
    scaled = (unsigned long)charge_ms * (unsigned long)Q16_ONE;
    return (long)(scaled / (unsigned long)m->charge_time_ms);
}

long gweaponmodules89_charge_speed_q16(int weapon_id,
                                             unsigned int charge_ms)
{
    const GWeaponModules89 *m;
    long ratio;
    long span;
    m = gweaponmodules89_get(weapon_id);
    if (!m) return Q16_ONE;
    if (m->trigger_model != GWM89_TRIGGER_CHARGE_RELEASE)
        return m->charge_max_speed_q16 > 0L ? m->charge_max_speed_q16 : Q16_ONE;
    ratio = gweaponmodules89_charge_ratio_q16(weapon_id, charge_ms);
    span = m->charge_max_speed_q16 - m->charge_min_speed_q16;
    return m->charge_min_speed_q16 + (span * ratio) / Q16_ONE;
}
