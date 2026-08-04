#include <stdio.h>
#include <string.h>

#include "blank3d_systems.h"

static GWP89_Vec3 v3(int x, int y, int z)
{
    return gwp89_v3(gwp89_fx_from_int(x),
                    gwp89_fx_from_int(y),
                    gwp89_fx_from_int(z));
}

int main(void)
{
    Blank3DSystems systems;
    Blank3DListCycleRegistry list_cycles;
    cycler89_item selected_item;
    GWP89_Vec3 origin;
    GWP89_Vec3 forward;
    GWP89_Vec3 right;
    GWP89_Vec3 up;
    GWP89_Event event;
    int projectile_events;
    int initial_reserve;
    int pistol_clip_after_shot;
    int shotgun_clip_before_blocked_fire;
    int shotgun_projectile_events;
    int gatling_projectile_events;
    int gatling_casing_events;
    int sling_projectile_events;
    int sling_casing_events;
    int sling_muzzle_events;
    static Blank3DSystems rate_systems;
    int rate_weapon_slot;
    int rate_user_slot;
    int rate_projectile_events;
    const GWP89_UserState *rate_user;

    blank3d_systems_init(&systems);
    blank3d_list_cycle_init(&list_cycles);
    if (!blank3d_systems_register_cycle_lists(&systems, &list_cycles))
        return 67;
    if (blank3d_systems_player_health(&systems) != 100) return 1;
    if (blank3d_systems_set_multiplier_q16(&systems, GWP89_NUM_DAMAGE_FX,
                                           2L * 65536L) != NS_OK) return 19;
    if (blank3d_systems_get_multiplier_q16(&systems, GWP89_NUM_DAMAGE_FX,
                                           0L) != 2L * 65536L) return 20;
    if (!blank3d_systems_get_flag(&systems, "weapon.can_fire", 0)) return 2;
    if (blank3d_systems_inventory_count(&systems, 1) != 1) return 3;
    if (blank3d_systems_clip(&systems) != 15) return 4;

    origin = v3(0, 1, 0);
    forward = v3(0, 0, -1);
    right = v3(1, 0, 0);
    up = v3(0, 1, 0);
    initial_reserve = blank3d_systems_reserve(&systems);

    blank3d_systems_update(&systems, 16U, 1, 1, 0,
                           GWP89_VIEW_FPS, GWP89_FIX_ONE,
                           &origin, &forward, &right, &up,
                           &origin, &forward);
    if (blank3d_systems_clip(&systems) != 14) return 5;
    pistol_clip_after_shot = blank3d_systems_clip(&systems);
    if (blank3d_systems_reserve(&systems) != initial_reserve) return 6;

    projectile_events = 0;
    while (blank3d_systems_poll_event(&systems, &event)) {
        if (event.type == GWP89_EVENT_PROJECTILE_REQUEST) {
            projectile_events++;
            if (event.damage_fx <= 0L || event.speed_fx <= 0L) return 7;
        }
    }
    if (projectile_events != 1) return 8;

    blank3d_systems_damage_player(&systems, 25);
    if (blank3d_systems_player_health(&systems) != 75) return 9;
    blank3d_systems_heal_player(&systems, 10);
    if (blank3d_systems_player_health(&systems) != 85) return 10;

    if (blank3d_systems_equip_id(&systems, 3) != GWP89_OK) return 11;
    if (blank3d_systems_weapon_id(&systems) != 3) return 12;
    if (strcmp(blank3d_systems_weapon_name(&systems), "shotgun") != 0) return 13;
    shotgun_clip_before_blocked_fire = blank3d_systems_clip(&systems);

    blank3d_systems_set_flag(&systems, "weapon.can_fire", 0);
    blank3d_systems_update(&systems, 16U, 1, 1, 0,
                           GWP89_VIEW_FPS, GWP89_FIX_ONE,
                           &origin, &forward, &right, &up,
                           &origin, &forward);
    if (blank3d_systems_clip(&systems) != shotgun_clip_before_blocked_fire) return 14;

    blank3d_systems_set_flag(&systems, "weapon.can_fire", 1);
    while (blank3d_systems_poll_event(&systems, &event)) { }
    blank3d_systems_update(&systems, 600U, 1, 1, 0,
                           GWP89_VIEW_FPS, GWP89_FIX_ONE,
                           &origin, &forward, &right, &up,
                           &origin, &forward);
    shotgun_projectile_events = 0;
    while (blank3d_systems_poll_event(&systems, &event)) {
        if (event.type == GWP89_EVENT_PROJECTILE_REQUEST) {
            if (event.weapon_id != 3 || event.projectile_mesh_id != 3)
                return 33;
            /* The seven pellets must remain a forward cone, not cardinal rays. */
            if (event.direction.z > -3000L) return 34;
            if (event.direction.x < -1000L || event.direction.x > 1000L)
                return 35;
            if (event.direction.y < -1000L || event.direction.y > 1000L)
                return 36;
            shotgun_projectile_events++;
        }
    }
    if (shotgun_projectile_events != 7) return 37;

    if (blank3d_systems_equip_id(&systems, 1) != GWP89_OK) return 15;
    if (blank3d_systems_clip(&systems) != pistol_clip_after_shot) return 16;

    if (gwp89_set_ammo(&systems.weapons, B3D_PLAYER_ACTOR_ID, 1, 73) != 73) return 17;
    if (blank3d_systems_reserve(&systems) != 73) return 18;

    if (blank3d_systems_equip_id(&systems, 2) != GWP89_OK) return 40;
    if (blank3d_systems_cycle_next(&systems) != GWP89_OK) return 41;
    if (blank3d_systems_weapon_id(&systems) != 4) return 42;
    if (strcmp(blank3d_systems_weapon_name(&systems), "magnum") != 0) return 43;
    if (blank3d_systems_cycle_next(&systems) != GWP89_OK) return 44;
    if (blank3d_systems_weapon_id(&systems) != 8) return 45;
    if (!blank3d_list_cycle_prev(&list_cycles, "active_weapon",
                                 &selected_item) ||
        selected_item != 4L || blank3d_systems_weapon_id(&systems) != 4)
        return 68;
    if (!blank3d_list_cycle_next(&list_cycles, "ACTIVE-WEAPON",
                                 &selected_item) ||
        selected_item != 8L || blank3d_systems_weapon_id(&systems) != 8)
        return 69;

    /* The cycle must compact itself to the weapons currently owned. */
    {
        int wid;
        for (wid = 2; wid <= 9; ++wid) {
            if (wid == 3) continue;
            (void)gkinv_remove_item(&systems.item_db, &systems.inventory,
                                    (gkinv_u16)wid, 1U);
        }
        if (blank3d_systems_equip_id(&systems, 1) != GWP89_OK) return 60;
        if (blank3d_systems_cycle_next(&systems) != GWP89_OK) return 61;
        if (blank3d_systems_weapon_id(&systems) != 3) return 62;
        if (blank3d_systems_cycle_next(&systems) != GWP89_OK) return 63;
        if (blank3d_systems_weapon_id(&systems) != 1) return 64;
        if (blank3d_systems_cycle_prev(&systems) != GWP89_OK) return 65;
        if (blank3d_systems_weapon_id(&systems) != 3) return 66;
        for (wid = 2; wid <= 9; ++wid) {
            if (wid == 3) continue;
            (void)gkinv_add_item(&systems.item_db, &systems.inventory,
                                 (gkinv_u16)wid, 1U, 0);
        }
    }

    if (blank3d_systems_equip_id(&systems, 8) != GWP89_OK) return 21;
    if (blank3d_systems_weapon_id(&systems) != 8) return 22;
    if (strcmp(blank3d_systems_weapon_name(&systems), "gatling_gun") != 0) return 23;
    if (blank3d_systems_clip(&systems) != 300) return 24;
    if (blank3d_systems_reserve(&systems) != 900) return 25;
    blank3d_systems_set_flag(&systems, "weapon.can_fire", 1);
    while (blank3d_systems_poll_event(&systems, &event)) { }
    blank3d_systems_update(&systems, 40U, 1, 1, 0,
                           GWP89_VIEW_FPS, GWP89_FIX_ONE,
                           &origin, &forward, &right, &up,
                           &origin, &forward);
    gatling_projectile_events = 0;
    gatling_casing_events = 0;
    while (blank3d_systems_poll_event(&systems, &event)) {
        if (event.type == GWP89_EVENT_CASING_REQUEST) {
            if (event.shell_mesh_id != 2) return 38;
            gatling_casing_events++;
        }
        if (event.type == GWP89_EVENT_PROJECTILE_REQUEST) {
            if (event.weapon_id != 8 || event.projectile_id != 2 ||
                event.projectile_mesh_id != 2)
                return 30;
            gatling_projectile_events++;
        }
    }
    if (gatling_projectile_events != 1) return 31;
    if (gatling_casing_events != 1) return 39;
    if (blank3d_systems_clip(&systems) != 299) return 32;
    if (blank3d_systems_cycle_next(&systems) != GWP89_OK) return 26;
    if (blank3d_systems_weapon_id(&systems) != 5) return 27;
    if (blank3d_systems_cycle_prev(&systems) != GWP89_OK) return 28;
    if (blank3d_systems_weapon_id(&systems) != 8) return 29;

    if (blank3d_systems_equip_id(&systems, 9) != GWP89_OK) return 46;
    if (strcmp(blank3d_systems_weapon_name(&systems), "slingshot") != 0)
        return 47;
    if (blank3d_systems_reserve(&systems) != 60) return 48;
    while (blank3d_systems_poll_event(&systems, &event)) { }
    blank3d_systems_set_launch_speed_q16(&systems, 30L * 65536L);
    blank3d_systems_update(&systems, 400U, 1, 1, 0,
                           GWP89_VIEW_FPS, GWP89_FIX_ONE,
                           &origin, &forward, &right, &up,
                           &origin, &forward);
    sling_projectile_events = 0;
    sling_casing_events = 0;
    sling_muzzle_events = 0;
    while (blank3d_systems_poll_event(&systems, &event)) {
        if (event.type == GWP89_EVENT_PROJECTILE_REQUEST) {
            if (event.weapon_id != 9 || event.projectile_mesh_id != 9)
                return 49;
            if (event.speed_fx < gwp89_fx_from_int(29) ||
                event.speed_fx > gwp89_fx_from_int(31)) return 50;
            sling_projectile_events++;
        } else if (event.type == GWP89_EVENT_CASING_REQUEST) {
            sling_casing_events++;
        } else if (event.type == GWP89_EVENT_MUZZLE_REQUEST) {
            sling_muzzle_events++;
        }
    }
    if (sling_projectile_events != 1) return 51;
    if (sling_casing_events != 0 || sling_muzzle_events != 0) return 52;
    if (blank3d_systems_clip(&systems) != 0) return 53;

    /* Changing projectile velocity must not change the automatic-fire clock. */
    blank3d_systems_init(&rate_systems);
    if (blank3d_systems_equip_id(&rate_systems,
                                  B3D_WEAPON_ID_MACHINE_GUN) != GWP89_OK)
        return 70;
    rate_weapon_slot = gwp89_find_weapon_slot_by_id(
        &rate_systems.weapons, B3D_WEAPON_ID_MACHINE_GUN);
    rate_user_slot = gwp89_find_user_slot(&rate_systems.weapons,
                                          B3D_PLAYER_ACTOR_ID);
    if (rate_weapon_slot < 0 || rate_user_slot < 0) return 71;
    rate_systems.weapons.weapons[rate_weapon_slot].speed_fx =
        gwp89_fx_from_text("5");
    while (blank3d_systems_poll_event(&rate_systems, &event)) { }
    blank3d_systems_update(&rate_systems, 16U, 1, 1, 0,
                           GWP89_VIEW_FPS, GWP89_FIX_ONE,
                           &origin, &forward, &right, &up,
                           &origin, &forward);
    rate_user = &rate_systems.weapons.users[rate_user_slot];
    if (rate_user->cooldown_ms_left != 75U) return 72;
    rate_projectile_events = 0;
    while (blank3d_systems_poll_event(&rate_systems, &event)) {
        if (event.type == GWP89_EVENT_PROJECTILE_REQUEST) {
            if (event.speed_fx != gwp89_fx_from_text("5")) return 73;
            rate_projectile_events++;
        }
    }
    if (rate_projectile_events != 1) return 74;

    rate_systems.weapons.weapons[rate_weapon_slot].speed_fx =
        gwp89_fx_from_text("200");
    blank3d_systems_update(&rate_systems, 50U, 1, 0, 0,
                           GWP89_VIEW_FPS, GWP89_FIX_ONE,
                           &origin, &forward, &right, &up,
                           &origin, &forward);
    rate_projectile_events = 0;
    while (blank3d_systems_poll_event(&rate_systems, &event)) {
        if (event.type == GWP89_EVENT_PROJECTILE_REQUEST)
            rate_projectile_events++;
    }
    if (rate_projectile_events != 0) return 75;
    blank3d_systems_update(&rate_systems, 25U, 1, 0, 0,
                           GWP89_VIEW_FPS, GWP89_FIX_ONE,
                           &origin, &forward, &right, &up,
                           &origin, &forward);
    rate_user = &rate_systems.weapons.users[rate_user_slot];
    if (rate_user->cooldown_ms_left != 75U) return 76;
    rate_projectile_events = 0;
    while (blank3d_systems_poll_event(&rate_systems, &event)) {
        if (event.type == GWP89_EVENT_PROJECTILE_REQUEST) {
            if (event.speed_fx != gwp89_fx_from_text("200")) return 77;
            rate_projectile_events++;
        }
    }
    if (rate_projectile_events != 1) return 78;

    puts("Blank3D systems integration test: OK");
    return 0;
}
