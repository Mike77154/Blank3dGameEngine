#include "blank3d_weapon_ini.h"
#include "blank3d_weapon_modules.h"
#include "gweapon89.h"

#include <stdio.h>
#include <string.h>

static const GWP89_WeaponProfile *by_id(const GWP89_Manager *manager, int id)
{
    int slot;
    slot = gwp89_find_weapon_slot_by_id(manager, id);
    return slot >= 0 ? gwp89_get_weapon(manager, slot) : 0;
}

int main(void)
{
    GWP89_Manager manager;
    const GWP89_WeaponProfile *shotgun;
    const GWP89_WeaponProfile *machine_gun;
    const GWP89_WeaponProfile *gatling;
    const GWP89_WeaponProfile *sniper;
    const GWP89_WeaponProfile *slow_probe;
    const GWP89_WeaponProfile *fast_probe;
    GWP89_Manager rate_manager;
    const Blank3DWeaponModules *modules;
    char status[160];
    int loaded;

    gwp89_init(&manager);
    status[0] = '\0';
    loaded = blank3d_weapon_ini_load_manifest(
        &manager, "config/weapons/weapons.ini", status, sizeof(status));
    if (loaded != 9 || manager.weapon_count != 9) {
        printf("weapon INI count failed: %d / %d (%s)\n",
               loaded, manager.weapon_count, status);
        return 1;
    }

    shotgun = by_id(&manager, B3D_WEAPON_ID_SHOTGUN);
    if (!shotgun || shotgun->pellet_count != 7 ||
        shotgun->spread_fx != gwp89_fx_from_text("7.0") ||
        shotgun->projectile_life_ms != 2800U ||
        shotgun->projectile_mesh_id != 3) {
        puts("shotgun INI profile failed");
        return 2;
    }
    modules = blank3d_weapon_modules_get(B3D_WEAPON_ID_SHOTGUN);
    if (!modules || modules->audio_profile != B3D_AUDIO_PROFILE_SHOTGUN ||
        modules->audio_projectile != B3D_AUDIO_PROJECTILE_PELLET_SWARM ||
        modules->audio_action != B3D_AUDIO_ACTION_PUMP ||
        modules->audio_projectile_instance_limit != 12) {
        puts("shotgun module/audio INI failed");
        return 3;
    }

    machine_gun = by_id(&manager, B3D_WEAPON_ID_MACHINE_GUN);
    if (!machine_gun || machine_gun->cooldown_ms != 75U ||
        machine_gun->speed_fx != gwp89_fx_from_text("46.0")) {
        puts("machine-gun cadence/projectile speed split failed");
        return 8;
    }

    gatling = by_id(&manager, B3D_WEAPON_ID_GATLING);
    modules = blank3d_weapon_modules_get(B3D_WEAPON_ID_GATLING);
    if (!gatling || gatling->fire_mode != GWP89_FIRE_AUTO ||
        gatling->cooldown_ms != 35U ||
        gatling->speed_fx != gwp89_fx_from_text("62.0") ||
        gatling->projectile_id != 2 || gatling->projectile_mesh_id != 2 ||
        !modules || modules->trigger_model != B3D_TRIGGER_SPINUP ||
        modules->audio_ammo != B3D_AUDIO_AMMO_BELT_BOX ||
        modules->audio_magazine_velocity_q15 != 28500 ||
        modules->audio_magazine_gain_q15 != 22800 ||
        modules->audio_reload_remove_motion_q15 != 27000 ||
        modules->audio_reload_insert_motion_q15 != 30500) {
        puts("gatling INI profile failed");
        return 4;
    }

    sniper = by_id(&manager, B3D_WEAPON_ID_SNIPER);
    modules = blank3d_weapon_modules_get(B3D_WEAPON_ID_SNIPER);
    if (!sniper || !modules || modules->optic_profile != B3D_OPTIC_SNIPER ||
        !modules->sway_enabled || !modules->aim_query_enabled ||
        strcmp(modules->scope_preset_name,
               "re5_psg1_game_scope") != 0) {
        puts("sniper INI module profile failed");
        return 5;
    }

    modules = blank3d_weapon_modules_get(B3D_WEAPON_ID_SLINGSHOT);
    if (!modules || modules->physics_backend != B3D_PHYSICS_BOLT3D ||
        modules->charge_max_speed_q16 != 46L * 65536L ||
        modules->gravity_q16 != -13L * 65536L ||
        modules->audio_enabled != 0) {
        puts("slingshot Q16/module INI failed");
        return 6;
    }

    modules = blank3d_weapon_modules_get(B3D_WEAPON_ID_ROCKET_LAUNCHER);
    if (!modules || !modules->audio_continuous_rocket ||
        modules->audio_explosion != B3D_AUDIO_EXPLOSION_ROCKET ||
        modules->audio_pressure_energy_q15 != 32000) {
        puts("rocket audio INI failed");
        return 7;
    }

    gwp89_init(&rate_manager);
    loaded = gwp89_load_ini_text(&rate_manager,
        "[weapon:slow_probe]\n"
        "id=77\n"
        "fire_mode=auto\n"
        "fire_rate_bps=20\n"
        "projectile_speed=5\n"
        "[weapon:fast_probe]\n"
        "id=78\n"
        "fire_mode=auto\n"
        "fire_rate_bps=20\n"
        "projectile_speed=200\n");
    slow_probe = by_id(&rate_manager, 77);
    fast_probe = by_id(&rate_manager, 78);
    if (loaded != 2 || !slow_probe || !fast_probe ||
        slow_probe->cooldown_ms != 50U || fast_probe->cooldown_ms != 50U ||
        slow_probe->speed_fx != gwp89_fx_from_text("5") ||
        fast_probe->speed_fx != gwp89_fx_from_text("200")) {
        puts("fire-rate parser decoupling failed");
        return 9;
    }

    printf("Blank3D weapon INI registry test: OK (%s)\n", status);
    return 0;
}
