#include <stdio.h>
#include <string.h>

#include "blank3d_systems.h"

#define NPC_ACTOR_ID 1000
#define NPC_KIND 2
#define NPC_TEAM 2
#define MACHINE_GUN_ID 2
#define MACHINE_GUN_AMMO_ID 1

static int expect_true(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        return 0;
    }
    return 1;
}

static void clear_actor_storage(GWP89_Manager *manager)
{
    if (!manager) return;
    memset(manager->users, 0, sizeof(manager->users));
    memset(manager->ammo_bank, 0, sizeof(manager->ammo_bank));
    memset(manager->events, 0, sizeof(manager->events));
    manager->event_head = 0;
    manager->event_tail = 0;
    manager->event_count = 0;
    manager->status[0] = '\0';
}

static void make_input(GWP89_FireInput *input, int actor_id,
                       int actor_kind, int team_id,
                       int trigger_flags, unsigned short dt_ms)
{
    memset(input, 0, sizeof(*input));
    input->actor_id = actor_id;
    input->actor_kind = actor_kind;
    input->team_id = team_id;
    input->view_style = GWP89_VIEW_FPS;
    input->trigger_flags = trigger_flags;
    input->dt_ms = dt_ms;
    input->zoom_fx = GWP89_FIX_ONE;
    input->socket_origin = gwp89_v3(0, 0, 0);
    input->socket_forward = gwp89_v3(0, 0, GWP89_FIX_ONE);
    input->socket_right = gwp89_v3(GWP89_FIX_ONE, 0, 0);
    input->socket_up = gwp89_v3(0, GWP89_FIX_ONE, 0);
    input->camera_origin = input->socket_origin;
    input->camera_forward = input->socket_forward;
}

int main(void)
{
    Blank3DSystems systems;
    GWP89_Manager npc;
    GWP89_FireInput npc_input;
    GWP89_FireInput player_input;
    int slot;
    int i;
    int result;
    int player_clip_before;
    int npc_user_slot;
    int ok;

    ok = 1;
    blank3d_systems_init(&systems);

    npc = systems.weapons;
    clear_actor_storage(&npc);

    ok &= expect_true(gwp89_bind_actor(&npc, NPC_ACTOR_ID,
                                       NPC_KIND, NPC_TEAM) >= 0,
                      "NPC actor should bind in isolated manager");
    slot = gwp89_find_weapon_slot_by_id(&npc, MACHINE_GUN_ID);
    ok &= expect_true(slot >= 0, "machine gun profile should exist");
    ok &= expect_true(gwp89_equip_slot(&npc, NPC_ACTOR_ID,
                                       slot, 1) == GWP89_OK,
                      "NPC should equip machine gun with full clip");
    (void)gwp89_set_ammo(&npc, NPC_ACTOR_ID,
                         MACHINE_GUN_AMMO_ID, 9999);

    make_input(&npc_input, NPC_ACTOR_ID, NPC_KIND, NPC_TEAM,
               GWP89_TRIGGER_DOWN, 75U);

    for (i = 0; i < 40; ++i) {
        result = gwp89_try_fire(&npc, &npc_input);
        ok &= expect_true(result == GWP89_OK,
                          "NPC should fire every machine-gun cadence tick");
        gwp89_clear_events(&npc);
    }
    ok &= expect_true(gwp89_query_clip(&npc, NPC_ACTOR_ID,
                                       MACHINE_GUN_ID) == 0,
                      "NPC clip should reach zero without touching player");

    result = gwp89_try_fire(&npc, &npc_input);
    ok &= expect_true(result == GWP89_NO_AMMO,
                      "empty NPC clip should report NO_AMMO");
    ok &= expect_true(gwp89_begin_reload(&npc, NPC_ACTOR_ID) == GWP89_OK,
                      "NPC should enter the normal reload state machine");
    npc_user_slot = gwp89_find_user_slot(&npc, NPC_ACTOR_ID);
    ok &= expect_true(npc_user_slot >= 0 &&
                      npc.users[npc_user_slot].reload_active,
                      "NPC reload state should be actor-local and active");

    ok &= expect_true(blank3d_systems_equip_id(&systems, 1) == GWP89_OK,
                      "player should equip pistol");
    player_clip_before = blank3d_systems_clip(&systems);
    make_input(&player_input, B3D_PLAYER_ACTOR_ID,
               B3D_PLAYER_KIND, B3D_PLAYER_TEAM,
               GWP89_TRIGGER_PRESSED | GWP89_TRIGGER_DOWN, 100U);
    result = gwp89_try_fire(&systems.weapons, &player_input);
    ok &= expect_true(result == GWP89_OK,
                      "player should fire while NPC is reloading");
    ok &= expect_true(blank3d_systems_clip(&systems) ==
                      player_clip_before - 1,
                      "NPC reload must not change or lock player clip");

    make_input(&npc_input, NPC_ACTOR_ID, NPC_KIND, NPC_TEAM,
               GWP89_TRIGGER_NONE, 75U);
    for (i = 0; i < 12; ++i)
        (void)gwp89_update_actor(&npc, &npc_input);
    ok &= expect_true(gwp89_query_clip(&npc, NPC_ACTOR_ID,
                                       MACHINE_GUN_ID) == 40,
                      "NPC reload should refill its own clip");
    ok &= expect_true(gwp89_query_ammo(&npc, NPC_ACTOR_ID,
                                       MACHINE_GUN_AMMO_ID,
                                       MACHINE_GUN_ID) == 9959,
                      "NPC reload should consume only NPC reserve");

    /* Reproduce the old runtime coupling: player death used to write the
       shared weapon.can_fire flag, which also cancelled NPC fire before it
       could see NO_AMMO and begin reloading. Health is now actor-local. */
    blank3d_systems_damage_player(&systems, 1000);
    make_input(&player_input, B3D_PLAYER_ACTOR_ID,
               B3D_PLAYER_KIND, B3D_PLAYER_TEAM,
               GWP89_TRIGGER_PRESSED | GWP89_TRIGGER_DOWN, 100U);
    result = gwp89_try_fire(&systems.weapons, &player_input);
    ok &= expect_true(result == GWP89_CANCELLED,
                      "downed player should be blocked by player-local gate");

    make_input(&npc_input, NPC_ACTOR_ID, NPC_KIND, NPC_TEAM,
               GWP89_TRIGGER_DOWN, 75U);
    result = gwp89_try_fire(&npc, &npc_input);
    ok &= expect_true(result == GWP89_OK,
                      "player death must not disable NPC weapon manager");

    (void)blank3d_systems_set_flag(&systems, "weapon.can_fire", 0);
    result = gwp89_try_fire(&npc, &npc_input);
    ok &= expect_true(result == GWP89_OK,
                      "player can_fire flag must not alias NPC can_fire");

    if (!ok) return 1;
    printf("Blank3D actor weapon isolation test: OK\n");
    return 0;
}
