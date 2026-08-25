#include "blank3d_weapon_ini.h"
#include "blank3d_weapon_modules.h"
#include "blank3d_weapon_host_io.h"
#include "gweapon89.h"
#include "expandiblefire89.h"

#include <stdio.h>

static long abs_long(long v)
{
    return v < 0L ? -v : v;
}

int main(void)
{
    GWP89_Manager manager;
    const GWP89_WeaponProfile *weapon;
    const Blank3DWeaponModules *modules;
    ef89_config config;
    ef89_state state;
    ef89_result result;
    ef89_vec3 position;
    ef89_fx radius;
    ef89_fx mesh_scale;
    int slot;
    int loaded;
    char status[160];

    gwp89_init(&manager);
    (void)blank3d_weapon_host_io_bind(&manager);
    loaded = blank3d_weapon_ini_load_manifest(
        &manager, "config/weapons/weapons.ini", status, sizeof(status));
    if (loaded != 15) return 1;
    slot = gwp89_find_weapon_slot_by_id(&manager, 14);
    if (slot < 0) return 2;
    weapon = gwp89_get_weapon(&manager, slot);
    modules = blank3d_weapon_modules_get(14);
    if (!weapon || !modules || !modules->expandible_fire_enabled) return 3;

    expandiblefire89_config_default(&config);
    config.scale_start_fx = modules->expandible_fire_scale_start_q16 / 16L;
    config.scale_end_fx = modules->expandible_fire_scale_end_q16 / 16L;
    config.growth_distance_fx =
        modules->expandible_fire_growth_distance_q16 / 16L;
    config.kill_distance_fx =
        modules->expandible_fire_kill_distance_q16 / 16L;

    position.x = 0L; position.y = 0L; position.z = 0L;
    expandiblefire89_init(&state, position, &config);
    if (!expandiblefire89_step(&state, &config, position, &result) ||
        !result.alive) return 4;

    radius = expandiblefire89_apply_scale(
        (ef89_fx)weapon->projectile_radius_fx, result.scale_fx);
    mesh_scale = expandiblefire89_apply_scale(
        (ef89_fx)weapon->projectile_mesh_scale_fx, result.scale_fx);
    if (radius >= weapon->projectile_radius_fx ||
        mesh_scale >= weapon->projectile_mesh_scale_fx) return 5;

    position.z = config.growth_distance_fx;
    if (!expandiblefire89_step(&state, &config, position, &result) ||
        !result.alive || result.scale_fx != config.scale_end_fx) return 6;
    radius = expandiblefire89_apply_scale(
        (ef89_fx)weapon->projectile_radius_fx, result.scale_fx);
    mesh_scale = expandiblefire89_apply_scale(
        (ef89_fx)weapon->projectile_mesh_scale_fx, result.scale_fx);
    if (radius <= weapon->projectile_radius_fx ||
        mesh_scale <= weapon->projectile_mesh_scale_fx) return 7;
    if (abs_long(radius - gwp89_fx_from_text("0.48")) > 4L) return 8;
    if (abs_long(mesh_scale - gwp89_fx_from_text("1.008")) > 5L) return 9;

    position.z = config.kill_distance_fx;
    if (!expandiblefire89_step(&state, &config, position, &result) ||
        result.alive || state.active) return 10;

    puts("Blank3D ID14 INI -> expandiblefire89 mechanical stack: PASS");
    return 0;
}
