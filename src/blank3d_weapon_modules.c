#include "blank3d_weapon_modules.h"

#include <string.h>

#define Q16_ONE 65536L
#define Q16_FROM_INT(v) ((long)(v) * Q16_ONE)

static Blank3DWeaponModules b3d_modules[B3D_WEAPON_MODULE_CAPACITY];
static int b3d_module_count;

static void b3d_copy(char *dst, const char *src)
{
    if (!dst) return;
    if (!src) src = "";
    strncpy(dst, src, B3D_WEAPON_NAME_CAPACITY - 1U);
    dst[B3D_WEAPON_NAME_CAPACITY - 1U] = '\0';
}

static void b3d_default_module(Blank3DWeaponModules *m,
                               int id, const char *name)
{
    memset(m, 0, sizeof(*m));
    m->weapon_id = id;
    b3d_copy(m->name, name);
    m->trigger_model = B3D_TRIGGER_STANDARD;
    m->physics_backend = B3D_PHYSICS_LINEAR;
    m->trail_profile = B3D_TRAIL_BULLET;
    m->optic_profile = B3D_OPTIC_GENERIC;
    m->emit_muzzle = 1;
    m->emit_casing = 1;
    m->emit_trail = 1;
    m->projectile_mesh_id = id;
    m->casing_mesh_id = id;
    m->mass_q16 = Q16_ONE;
    m->audio_enabled = 1;
    m->audio_profile = B3D_AUDIO_PROFILE_PISTOL;
    m->audio_action = B3D_AUDIO_ACTION_PISTOL;
    m->audio_magazine = B3D_AUDIO_MAG_PISTOL_POLYMER;
    m->audio_ammo = B3D_AUDIO_AMMO_BOX_MAG;
    m->audio_muzzle = B3D_AUDIO_MUZZLE_BARE;
    m->audio_shell = B3D_AUDIO_SHELL_PISTOL_BRASS;
    m->audio_detachable_magazine = 1;
    m->audio_emits_casing = 1;
    m->audio_projectile = B3D_AUDIO_PROJECTILE_AUTO;
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

void blank3d_weapon_modules_reset(void)
{
    memset(b3d_modules, 0, sizeof(b3d_modules));
    b3d_module_count = 0;
}

int blank3d_weapon_modules_register(const Blank3DWeaponModules *modules)
{
    int i;
    if (!modules || modules->weapon_id <= 0) return 0;
    for (i = 0; i < b3d_module_count; ++i) {
        if (b3d_modules[i].weapon_id == modules->weapon_id) {
            b3d_modules[i] = *modules;
            return 1;
        }
    }
    if (b3d_module_count >= B3D_WEAPON_MODULE_CAPACITY) return 0;
    b3d_modules[b3d_module_count++] = *modules;
    return 1;
}

void blank3d_weapon_modules_load_defaults(void)
{
    Blank3DWeaponModules m;
    blank3d_weapon_modules_reset();

    b3d_default_module(&m, 1, "pistol");
    m.projectile_mesh_id = 1; m.casing_mesh_id = 1;
    (void)blank3d_weapon_modules_register(&m);

    b3d_default_module(&m, 2, "machine_gun");
    m.trail_profile = B3D_TRAIL_TRACER;
    m.projectile_mesh_id = 2; m.casing_mesh_id = 2;
    m.audio_profile = B3D_AUDIO_PROFILE_SMG;
    m.audio_action = B3D_AUDIO_ACTION_MACHINE;
    m.audio_magazine = B3D_AUDIO_MAG_SMG_STEEL;
    m.audio_muzzle = B3D_AUDIO_MUZZLE_COMPENSATOR;
    m.audio_shell = B3D_AUDIO_SHELL_STEEL_CASE;
    m.audio_fire_gain_q15 = 27400;
    m.audio_dry_receiver_impulse = 3800;
    m.audio_action_speed_q16 = 78643UL;
    (void)blank3d_weapon_modules_register(&m);

    b3d_default_module(&m, 3, "shotgun");
    m.optic_profile = B3D_OPTIC_NONE;
    m.projectile_mesh_id = 3; m.casing_mesh_id = 3;
    m.audio_profile = B3D_AUDIO_PROFILE_SHOTGUN;
    m.audio_action = B3D_AUDIO_ACTION_PUMP;
    m.audio_magazine = B3D_AUDIO_MAG_RIFLE_POLYMER;
    m.audio_ammo = B3D_AUDIO_AMMO_TUBE;
    m.audio_shell = B3D_AUDIO_SHELL_SHOTGUN_PLASTIC;
    m.audio_detachable_magazine = 0;
    m.audio_projectile = B3D_AUDIO_PROJECTILE_PELLET_SWARM;
    m.audio_fire_gain_q15 = 30700;
    m.audio_dry_receiver_impulse = 5200;
    m.audio_action_speed_q16 = 58982UL;
    (void)blank3d_weapon_modules_register(&m);

    b3d_default_module(&m, 4, "magnum");
    m.trail_profile = B3D_TRAIL_HEAVY;
    m.projectile_mesh_id = 4; m.casing_mesh_id = 4;
    m.audio_profile = B3D_AUDIO_PROFILE_MAGNUM;
    m.audio_action = B3D_AUDIO_ACTION_REVOLVER;
    m.audio_magazine = B3D_AUDIO_MAG_PISTOL_METAL;
    m.audio_ammo = B3D_AUDIO_AMMO_LOOSE_SHELLS;
    m.audio_muzzle = B3D_AUDIO_MUZZLE_PORTED;
    m.audio_shell = B3D_AUDIO_SHELL_MAGNUM_BRASS;
    m.audio_detachable_magazine = 0;
    m.audio_fire_gain_q15 = 31500;
    m.audio_dry_receiver_impulse = 6100;
    m.audio_action_speed_q16 = 52429UL;
    (void)blank3d_weapon_modules_register(&m);

    b3d_default_module(&m, 5, "sniper");
    m.trail_profile = B3D_TRAIL_TRACER;
    m.optic_profile = B3D_OPTIC_SNIPER;
    m.sway_enabled = 1;
    m.aim_query_enabled = 1;
    m.projectile_mesh_id = 5; m.casing_mesh_id = 5;
    b3d_copy(m.scope_preset_name, "re5_psg1_game_scope");
    m.audio_profile = B3D_AUDIO_PROFILE_SNIPER;
    m.audio_action = B3D_AUDIO_ACTION_RIFLE;
    m.audio_magazine = B3D_AUDIO_MAG_SNIPER_BOX;
    m.audio_muzzle = B3D_AUDIO_MUZZLE_BRAKE;
    m.audio_shell = B3D_AUDIO_SHELL_RIFLE_BRASS;
    m.audio_fire_gain_q15 = 31000;
    m.audio_dry_receiver_impulse = 5600;
    m.audio_action_speed_q16 = 49152UL;
    (void)blank3d_weapon_modules_register(&m);

    b3d_default_module(&m, 6, "grenade_launcher");
    m.physics_backend = B3D_PHYSICS_GRAVITY;
    m.trail_profile = B3D_TRAIL_ARC;
    m.projectile_mesh_id = 6; m.casing_mesh_id = 0;
    m.gravity_q16 = -Q16_FROM_INT(7);
    m.bounce_q16 = Q16_ONE / 4L;
    m.audio_profile = B3D_AUDIO_PROFILE_LAUNCHER;
    m.audio_action = B3D_AUDIO_ACTION_PUMP;
    m.audio_magazine = B3D_AUDIO_MAG_RIFLE_ALUMINUM;
    m.audio_ammo = B3D_AUDIO_AMMO_LOOSE_SHELLS;
    m.audio_shell = B3D_AUDIO_SHELL_SHOTGUN_BRASS;
    m.audio_detachable_magazine = 0;
    m.audio_projectile = B3D_AUDIO_PROJECTILE_TRACER;
    m.audio_explosion = B3D_AUDIO_EXPLOSION_GRENADE;
    m.audio_fire_gain_q15 = 30600;
    m.audio_dry_receiver_impulse = 5900;
    m.audio_action_speed_q16 = 45875UL;
    (void)blank3d_weapon_modules_register(&m);

    b3d_default_module(&m, 7, "rocket_launcher");
    m.trail_profile = B3D_TRAIL_HEAVY;
    m.projectile_mesh_id = 7; m.casing_mesh_id = 0;
    m.emit_casing = 0;
    b3d_copy(m.scope_preset_name, "rpg7_pgo7");
    m.audio_profile = B3D_AUDIO_PROFILE_LAUNCHER;
    m.audio_action = B3D_AUDIO_ACTION_RIFLE;
    m.audio_magazine = B3D_AUDIO_MAG_RIFLE_ALUMINUM;
    m.audio_ammo = B3D_AUDIO_AMMO_LOOSE_SHELLS;
    m.audio_shell = B3D_AUDIO_SHELL_RIFLE_BRASS;
    m.audio_detachable_magazine = 0;
    m.audio_emits_casing = 0;
    m.audio_projectile = B3D_AUDIO_PROJECTILE_TRACER;
    m.audio_explosion = B3D_AUDIO_EXPLOSION_ROCKET;
    m.audio_continuous_rocket = 1;
    m.audio_fire_gain_q15 = 31500;
    m.audio_dry_receiver_impulse = 6400;
    m.audio_action_speed_q16 = 42598UL;
    (void)blank3d_weapon_modules_register(&m);

    b3d_default_module(&m, 8, "gatling_gun");
    m.trigger_model = B3D_TRIGGER_SPINUP;
    m.trail_profile = B3D_TRAIL_TRACER;
    m.optic_profile = B3D_OPTIC_NONE;
    m.projectile_mesh_id = 2; m.casing_mesh_id = 2;
    m.audio_profile = B3D_AUDIO_PROFILE_HEAVY;
    m.audio_action = B3D_AUDIO_ACTION_MACHINE;
    m.audio_magazine = B3D_AUDIO_MAG_DRUM_HEAVY;
    m.audio_ammo = B3D_AUDIO_AMMO_BELT_BOX;
    m.audio_muzzle = B3D_AUDIO_MUZZLE_COMPENSATOR;
    m.audio_shell = B3D_AUDIO_SHELL_RIFLE_BRASS;
    m.audio_projectile = B3D_AUDIO_PROJECTILE_TRACER;
    m.audio_magazine_velocity_q15 = 28500;
    m.audio_magazine_gain_q15 = 22800;
    m.audio_fire_gain_q15 = 23800;
    m.audio_dry_receiver_impulse = 3600;
    m.audio_action_speed_q16 = 98304UL;
    (void)blank3d_weapon_modules_register(&m);

    b3d_default_module(&m, 9, "slingshot");
    m.trigger_model = B3D_TRIGGER_CHARGE_RELEASE;
    m.physics_backend = B3D_PHYSICS_BOLT3D;
    m.trail_profile = B3D_TRAIL_ARC;
    m.optic_profile = B3D_OPTIC_NONE;
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
    m.audio_profile = B3D_AUDIO_PROFILE_NONE;
    m.audio_action = B3D_AUDIO_ACTION_NONE;
    m.audio_magazine = B3D_AUDIO_MAG_NONE;
    m.audio_ammo = B3D_AUDIO_AMMO_NONE;
    m.audio_shell = B3D_AUDIO_SHELL_NONE;
    m.audio_detachable_magazine = 0;
    m.audio_emits_casing = 0;
    m.audio_projectile = B3D_AUDIO_PROJECTILE_NONE;
    m.audio_fire_gain_q15 = 0;
    m.audio_dry_receiver_impulse = 0;
    (void)blank3d_weapon_modules_register(&m);
}

int blank3d_weapon_modules_count(void)
{
    if (b3d_module_count == 0) blank3d_weapon_modules_load_defaults();
    return b3d_module_count;
}

const Blank3DWeaponModules *blank3d_weapon_modules_get(int weapon_id)
{
    int i;
    if (b3d_module_count == 0) blank3d_weapon_modules_load_defaults();
    for (i = 0; i < b3d_module_count; ++i) {
        if (b3d_modules[i].weapon_id == weapon_id) return &b3d_modules[i];
    }
    return 0;
}

int blank3d_weapon_modules_has_casing(int weapon_id)
{
    const Blank3DWeaponModules *m;
    m = blank3d_weapon_modules_get(weapon_id);
    return m ? m->emit_casing : 1;
}

int blank3d_weapon_modules_has_muzzle(int weapon_id)
{
    const Blank3DWeaponModules *m;
    m = blank3d_weapon_modules_get(weapon_id);
    return m ? m->emit_muzzle : 1;
}

int blank3d_weapon_modules_has_trail(int weapon_id)
{
    const Blank3DWeaponModules *m;
    m = blank3d_weapon_modules_get(weapon_id);
    return m ? m->emit_trail : 1;
}

long blank3d_weapon_modules_charge_ratio_q16(int weapon_id,
                                             unsigned int charge_ms)
{
    const Blank3DWeaponModules *m;
    unsigned long scaled;
    m = blank3d_weapon_modules_get(weapon_id);
    if (!m || m->trigger_model != B3D_TRIGGER_CHARGE_RELEASE ||
        m->charge_time_ms == 0U) return Q16_ONE;
    if (charge_ms >= (unsigned int)m->charge_time_ms) return Q16_ONE;
    scaled = (unsigned long)charge_ms * (unsigned long)Q16_ONE;
    return (long)(scaled / (unsigned long)m->charge_time_ms);
}

long blank3d_weapon_modules_charge_speed_q16(int weapon_id,
                                             unsigned int charge_ms)
{
    const Blank3DWeaponModules *m;
    long ratio;
    long span;
    m = blank3d_weapon_modules_get(weapon_id);
    if (!m) return Q16_ONE;
    if (m->trigger_model != B3D_TRIGGER_CHARGE_RELEASE)
        return m->charge_max_speed_q16 > 0L ? m->charge_max_speed_q16 : Q16_ONE;
    ratio = blank3d_weapon_modules_charge_ratio_q16(weapon_id, charge_ms);
    span = m->charge_max_speed_q16 - m->charge_min_speed_q16;
    return m->charge_min_speed_q16 + (span * ratio) / Q16_ONE;
}
