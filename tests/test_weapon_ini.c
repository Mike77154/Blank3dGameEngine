#include "blank3d_weapon_ini.h"
#include "blank3d_weapon_modules.h"
#include "gweapon89.h"
#include "blank3d_weapon_host_io.h"

#include <stdio.h>
#include <string.h>

static const GWP89_WeaponProfile *by_id(const GWP89_Manager *manager, int id)
{
    int slot;
    slot = gwp89_find_weapon_slot_by_id(manager, id);
    return slot >= 0 ? gwp89_get_weapon(manager, slot) : 0;
}

static int shared_muzzle_ok(int weapon_id, unsigned short life_ms)
{
    const Blank3DWeaponModules *m;
    m = blank3d_weapon_modules_get(weapon_id);
    if (!m) return 0;
    if (!m->emit_muzzle ||
        strcmp(m->muzzle_image_path,
               "config/weapons/assets/pistol_muzzle_flash.jpg") != 0 ||
        m->muzzle_image_blend != GWM89_MUZZLE_IMAGE_BLEND_ADDITIVE ||
        m->muzzle_image_billboard != GWM89_MUZZLE_IMAGE_BILLBOARD_VIEW ||
        !m->muzzle_image_glow || !m->muzzle_light ||
        m->muzzle_image_life_ms != life_ms ||
        m->muzzle_image_width_q16 <= 0L ||
        m->muzzle_image_height_q16 <= 0L ||
        m->muzzle_image_glow_scale_q16 <= 65536L ||
        m->muzzle_image_glow_alpha == 0U ||
        m->muzzle_light_intensity_q16 <= 0L ||
        m->muzzle_light_radius_q16 <= 0L)
        return 0;
    return 1;
}

int main(void)
{
    GWP89_Manager manager;
    const GWP89_WeaponProfile *pistol;
    const GWP89_WeaponProfile *shotgun;
    const GWP89_WeaponProfile *machine_gun;
    const GWP89_WeaponProfile *gatling;
    const GWP89_WeaponProfile *sniper;
    const GWP89_WeaponProfile *hand_grenade;
    const GWP89_WeaponProfile *uzi;
    const GWP89_WeaponProfile *shango;
    const GWP89_WeaponProfile *homing_rocket;
    const GWP89_WeaponProfile *flamethrower;
    const GWP89_WeaponProfile *buster;
    const GWP89_WeaponProfile *slow_probe;
    const GWP89_WeaponProfile *fast_probe;
    GWP89_Manager rate_manager;
    const Blank3DWeaponModules *modules;
    char status[160];
    int loaded;

    gwp89_init(&manager);
    (void)blank3d_weapon_host_io_bind(&manager);
    status[0] = '\0';
    loaded = blank3d_weapon_ini_load_manifest(
        &manager, "config/weapons/weapons.ini", status, sizeof(status));
    if (loaded != 15 || manager.weapon_count != 15) {
        printf("weapon INI count failed: %d / %d (%s)\n",
               loaded, manager.weapon_count, status);
        return 1;
    }

    pistol = by_id(&manager, B3D_WEAPON_ID_PISTOL);
    modules = blank3d_weapon_modules_get(B3D_WEAPON_ID_PISTOL);
    if (!pistol || !modules ||
        strcmp(modules->crosshair_preset_name, "pistol_crisp") != 0 ||
        strcmp(modules->crosshair_preset_first_person,
               "blank3d_pistol_first_person") != 0 ||
        strcmp(modules->crosshair_preset_third_person,
               "blank3d_pistol_third_person") != 0) {
        puts("pistol camera-specific crosshair INI failed");
        return 11;
    }

    if (!shared_muzzle_ok(B3D_WEAPON_ID_PISTOL, 58U) ||
        !shared_muzzle_ok(B3D_WEAPON_ID_MACHINE_GUN, 42U) ||
        !shared_muzzle_ok(B3D_WEAPON_ID_SHOTGUN, 68U) ||
        !shared_muzzle_ok(B3D_WEAPON_ID_MAGNUM, 62U) ||
        !shared_muzzle_ok(B3D_WEAPON_ID_SNIPER, 64U) ||
        !shared_muzzle_ok(B3D_WEAPON_ID_GATLING, 30U) ||
        !shared_muzzle_ok(B3D_WEAPON_ID_UZI, 40U)) {
        puts("shared firearm muzzle image/glow/light catalog failed");
        return 17;
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
        modules->audio_projectile_instance_limit != 12 ||
        strcmp(modules->crosshair_preset_name,
               "shotgun_dynamic") != 0) {
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
        modules->audio_reload_insert_motion_q15 != 30500 ||
        !modules->bullet_circle_enabled ||
        modules->bullet_circle_count != 6 ||
        modules->bullet_circle_radius_q16 !=
            (long)gwp89_fx_from_text("0.18") * 16L ||
        modules->bullet_circle_phase_turn_q16 != 16384L ||
        !modules->bullet_spin_enabled ||
        modules->bullet_spin_start_slot != 0 ||
        modules->bullet_spin_step != 1 ||
        modules->bullet_spin_direction != 1 ||
        !modules->bullet_inline_enabled ||
        modules->bullet_inline_distance_q16 !=
            (long)gwp89_fx_from_text("125") * 16L) {
        puts("gatling INI profile failed");
        return 4;
    }

    sniper = by_id(&manager, B3D_WEAPON_ID_SNIPER);
    modules = blank3d_weapon_modules_get(B3D_WEAPON_ID_SNIPER);
    if (!sniper || !modules || modules->optic_profile != B3D_OPTIC_SNIPER ||
        !modules->sway_enabled || !modules->aim_query_enabled ||
        strcmp(modules->scope_recipe_path,
               "config/scope/hud/sniper_re5_psg1.ini") != 0 ||
        strcmp(modules->scope_preset_name,
               "re5_psg1_game_scope") != 0 ||
        strcmp(modules->crosshair_preset_name,
               "sniper_hairline") != 0) {
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
        modules->audio_pressure_energy_q15 != 32000 ||
        strcmp(modules->crosshair_preset_name,
               "projectile_lead_broken") != 0) {
        puts("rocket audio INI failed");
        return 7;
    }

    uzi = by_id(&manager, 11);
    modules = blank3d_weapon_modules_get(11);
    if (!uzi || !modules || modules->satellaborner_enabled ||
        modules->telesearcher_enabled || !modules->telesearcher_reacquire ||
        modules->telesearcher_gain_q16 != 32768L) {
        puts("Uzi satellite/homing module parser failed");
        return 12;
    }

    shango = by_id(&manager, 12);
    modules = blank3d_weapon_modules_get(12);
    if (!shango || !modules || shango->projectile_mesh_id != 11 ||
        !modules->satellaborner_enabled || !modules->telesearcher_enabled ||
        modules->telesearcher_reacquire ||
        modules->projectile_visual != B3D_PROJECTILE_VISUAL_SHANGO_PRESS ||
        modules->projectile_visual_expand_ms != 180U ||
        modules->projectile_visual_crush_ms != 520U ||
        modules->projectile_visual_radial_peak_q16 !=
            (long)gwp89_fx_from_text("2.35") * 16L ||
        modules->projectile_visual_axial_end_q16 !=
            (long)gwp89_fx_from_text("0.28") * 16L ||
        !modules->projectile_visual_unlit ||
        !modules->projectile_visual_additive) {
        puts("Shango orbital/visual INI profile failed");
        return 13;
    }


    homing_rocket = by_id(&manager, 13);
    modules = blank3d_weapon_modules_get(13);
    if (!homing_rocket || !modules ||
        homing_rocket->projectile_id != 13 ||
        homing_rocket->projectile_mesh_id != 12 ||
        homing_rocket->ammo_id != 12 ||
        modules->satellaborner_enabled ||
        !modules->telesearcher_enabled ||
        modules->telesearcher_reacquire ||
        modules->telesearcher_gain_q16 !=
            (long)gwp89_fx_from_text("0.28") * 16L ||
        modules->audio_explosion != B3D_AUDIO_EXPLOSION_ROCKET ||
        strcmp(modules->crosshair_preset_name, "target_lock") != 0) {
        puts("homing rocket launcher Telesearcher INI profile failed");
        return 14;
    }

    flamethrower = by_id(&manager, 14);
    modules = blank3d_weapon_modules_get(14);
    if (!flamethrower || !modules ||
        flamethrower->projectile_id != 14 ||
        flamethrower->projectile_mesh_id != 13 ||
        flamethrower->ammo_id != 13 ||
        flamethrower->fire_mode != GWP89_FIRE_AUTO ||
        flamethrower->cooldown_ms != 45U ||
        modules->satellaborner_enabled ||
        modules->telesearcher_enabled ||
        !modules->expandible_fire_enabled ||
        modules->expandible_fire_scale_start_q16 !=
            (long)gwp89_fx_from_text("0.25") * 16L ||
        modules->expandible_fire_scale_end_q16 !=
            (long)gwp89_fx_from_text("2.40") * 16L ||
        modules->expandible_fire_growth_distance_q16 !=
            (long)gwp89_fx_from_text("5.20") * 16L ||
        modules->expandible_fire_kill_distance_q16 !=
            (long)gwp89_fx_from_text("6.25") * 16L ||
        !modules->expandible_fire_collision_growth ||
        modules->projectile_visual !=
            B3D_PROJECTILE_VISUAL_EXPANDIBLE_FIRE ||
        !modules->projectile_visual_unlit ||
        !modules->projectile_visual_additive ||
        strcmp(modules->projectile_visual_image,
               "flamethrower_fireloop_128_50_atlas") != 0 ||
        modules->projectile_visual_frame_width != 128U ||
        modules->projectile_visual_frame_height != 128U ||
        modules->projectile_visual_columns != 8U ||
        modules->projectile_visual_rows != 7U ||
        modules->projectile_visual_frame_count != 50U ||
        modules->projectile_visual_frame_ms != 33U ||
        modules->projectile_visual_phase_step != 7U ||
        modules->projectile_visual_width_scale_q16 !=
            (long)gwp89_fx_from_text("1.55") * 16L ||
        modules->projectile_visual_height_scale_q16 !=
            (long)gwp89_fx_from_text("2.10") * 16L ||
        !modules->projectile_visual_glow ||
        !modules->projectile_visual_core ||
        !modules->projectile_visual_light) {
        puts("flamethrower expandiblefire89 INI profile failed");
        return 15;
    }

    buster = by_id(&manager, B3D_WEAPON_ID_BUSTER);
    modules = blank3d_weapon_modules_get(B3D_WEAPON_ID_BUSTER);
    if (!buster || !modules || buster->projectile_id != 15 ||
        buster->projectile_mesh_id != 14 ||
        modules->trigger_model != B3D_TRIGGER_PRESS_CHARGE_RELEASE ||
        !modules->player_physical_projectile_enabled ||
        !modules->more_than_one.enabled ||
        modules->more_than_one.activation_ms != 250U ||
        modules->more_than_one.stage_count != 3 ||
        modules->more_than_one.stages[0].projectile_id != 16 ||
        modules->more_than_one.stages[1].projectile_id != 17 ||
        modules->more_than_one.stages[2].projectile_id != 18 ||
        modules->more_than_one.stages[2].projectile_mesh_id != 17 ||
        modules->more_than_one.stages[2].damage_q16 !=
            (long)gwp89_fx_from_text("52.0") * 16L) {
        puts("Buster morethanone89 INI profile failed");
        return 16;
    }

    hand_grenade = by_id(&manager, B3D_WEAPON_ID_HAND_GRENADE);
    modules = blank3d_weapon_modules_get(B3D_WEAPON_ID_HAND_GRENADE);
    if (!hand_grenade || !modules ||
        hand_grenade->projectile_mesh_id != 10 ||
        hand_grenade->projectile_life_ms != 4200U ||
        modules->physics_backend != B3D_PHYSICS_GRAVITY ||
        modules->emit_muzzle || modules->emit_casing ||
        !modules->emit_trail || modules->audio_enabled != 0) {
        puts("hand-grenade INI profile failed");
        return 10;
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
        return 11;
    }

    printf("Blank3D weapon INI registry test: OK (%s)\n", status);
    return 0;
}
