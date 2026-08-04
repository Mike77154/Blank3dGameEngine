#include "blank3d_actor_equipment.h"

#include <stdio.h>
#include <string.h>

#define PLAYER_ID 1
#define GUNNER_ID 1000
#define ALLY_ID 2000

#define CHECK(c, m) do { if (!(c)) { printf("FAIL: %s\n", (m)); return 1; } } while (0)

static void make_profile(GWP89_WeaponProfile *profile,
                         int weapon_id,
                         const char *name,
                         unsigned short reload_ms)
{
    memset(profile, 0, sizeof(*profile));
    profile->active = 1;
    profile->weapon_id = weapon_id;
    profile->clip_size = weapon_id == 2 ? 40 : 15;
    profile->ammo_per_shot = 1;
    profile->reload_ms = reload_ms;
    profile->cooldown_ms = 75U;
    strncpy(profile->name, name, sizeof(profile->name) - 1U);
    profile->name[sizeof(profile->name) - 1U] = '\0';
}

int main(void)
{
    Blank3DAttachmentWorld attachments;
    Blank3DWeaponPresentationRegistry presentations;
    Blank3DActorEquipmentSystem equipment;
    GWP89_Manager manager;
    GWP89_WeaponProfile pistol;
    GWP89_WeaponProfile machine_gun;
    soq3d_pose player_socket;
    soq3d_pose gunner_socket;
    soq3d_pose ally_socket;
    Blank3DActorEquipmentInstance *gunner_instance;
    const Blank3DWeaponPresentation *machine_presentation;
    const nm89_geometry_packet *gunner_packet;
    const nm89_pose *action_pose;
    char status[160];
    int pistol_slot;
    int machine_slot;

    blank3d_attachment_init(&attachments);
    CHECK(blank3d_weapon_presentation_load_manifest(&presentations,
        "config/weapons/weapons.ini", status, sizeof(status)) == 9,
        "presentation manifest should expose all weapon models");
    blank3d_actor_equipment_init(&equipment, &attachments, &presentations);
    CHECK(equipment.initialized, "equipment service should initialize");

    CHECK(blank3d_actor_equipment_define_socket(&equipment,
        PLAYER_ID, "weapon_r", B3D_ATTACH89_SOCKET_WEAPON_R, 0),
        "player socket should be accepted");
    CHECK(blank3d_actor_equipment_define_socket(&equipment,
        GUNNER_ID, "weapon_r", B3D_ATTACH89_SOCKET_WEAPON_R, 0),
        "gunner socket should be accepted through same API");
    CHECK(blank3d_actor_equipment_define_socket(&equipment,
        ALLY_ID, "weapon_r", B3D_ATTACH89_SOCKET_WEAPON_R, 0),
        "ally socket should be accepted through same API");

    player_socket = soq3d_pose_identity();
    player_socket.position.x = 10 * 65536L;
    gunner_socket = soq3d_pose_identity();
    gunner_socket.position.x = -8 * 65536L;
    ally_socket = soq3d_pose_identity();
    ally_socket.position.z = 6 * 65536L;
    CHECK(blank3d_attachment_publish_socket_pose(&attachments,
        PLAYER_ID, B3D_ATTACH89_SOCKET_WEAPON_R, &player_socket),
        "player carrier pose should publish");
    CHECK(blank3d_attachment_publish_socket_pose(&attachments,
        GUNNER_ID, B3D_ATTACH89_SOCKET_WEAPON_R, &gunner_socket),
        "gunner carrier pose should publish");
    CHECK(blank3d_attachment_publish_socket_pose(&attachments,
        ALLY_ID, B3D_ATTACH89_SOCKET_WEAPON_R, &ally_socket),
        "ally carrier pose should publish");

    gwp89_init(&manager);
    make_profile(&pistol, 1, "pistol", 700U);
    make_profile(&machine_gun, 2, "machine_gun", 900U);
    pistol_slot = gwp89_add_weapon(&manager, &pistol);
    machine_slot = gwp89_add_weapon(&manager, &machine_gun);
    CHECK(pistol_slot >= 0 && machine_slot >= 0,
          "test weapon profiles should register");
    CHECK(gwp89_bind_actor(&manager, PLAYER_ID, 1, 1) >= 0,
          "player should bind");
    CHECK(gwp89_bind_actor(&manager, GUNNER_ID, 2, 2) >= 0,
          "gunner should bind");
    CHECK(gwp89_bind_actor(&manager, ALLY_ID, 3, 1) >= 0,
          "ally should bind through the same weapon manager API");
    CHECK(gwp89_equip_slot(&manager, PLAYER_ID, pistol_slot, 1) == GWP89_OK,
          "player should equip pistol");
    CHECK(gwp89_equip_slot(&manager, GUNNER_ID, machine_slot, 1) == GWP89_OK,
          "gunner should equip machine gun");
    CHECK(gwp89_equip_slot(&manager, ALLY_ID, pistol_slot, 1) == GWP89_OK,
          "ally should equip pistol");

    CHECK(blank3d_actor_equipment_equip(&equipment, PLAYER_ID,
        B3D_EQUIPMENT_ACTOR_PLAYER, 1),
          "GAttach should mount pistol object on player");
    CHECK(blank3d_actor_equipment_equip(&equipment, GUNNER_ID,
        B3D_EQUIPMENT_ACTOR_ENEMY, 2),
          "GAttach should mount machine gun object on gunner");
    CHECK(blank3d_actor_equipment_equip(&equipment, ALLY_ID,
        B3D_EQUIPMENT_ACTOR_ALLY, 1),
          "GAttach should mount a weapon object on an ally");
    CHECK(blank3d_attachment_update(&attachments),
          "all actor attachments should resolve together");
    CHECK(blank3d_actor_equipment_sync_actor(&equipment, &manager,
        PLAYER_ID, 1, 16U), "player object should animate");
    CHECK(blank3d_actor_equipment_sync_actor(&equipment, &manager,
        GUNNER_ID, B3D_EQUIPMENT_ACTOR_ENEMY, 16U),
        "gunner object should animate identically");
    CHECK(blank3d_actor_equipment_sync_actor(&equipment, &manager,
        ALLY_ID, B3D_EQUIPMENT_ACTOR_ALLY, 16U),
        "ally object should animate without carrier-specific code");

    CHECK(blank3d_actor_equipment_model_id(&equipment, PLAYER_ID) == 1,
          "player should receive pistol model id");
    CHECK(blank3d_actor_equipment_model_id(&equipment, GUNNER_ID) == 2,
          "gunner should receive machine-gun model id");
    CHECK(strcmp(blank3d_actor_equipment_model_name(&equipment, GUNNER_ID),
                 "machine_gun_model") == 0,
          "weapon profile should give GAttach an opaque model name");
    CHECK(blank3d_actor_equipment_model_id(&equipment, ALLY_ID) == 1,
          "ally should receive pistol model id");
    CHECK(blank3d_actor_equipment_object_id(&equipment, PLAYER_ID) !=
          blank3d_actor_equipment_object_id(&equipment, GUNNER_ID),
          "each carrier should own a distinct attached object instance");
    CHECK(blank3d_actor_equipment_packet_count(&equipment, PLAYER_ID) == 4,
          "player model should expose four animated parts");
    CHECK(blank3d_actor_equipment_packet_count(&equipment, GUNNER_ID) == 4,
          "gunner model should expose four animated parts");
    CHECK(blank3d_actor_equipment_packet_count(&equipment, ALLY_ID) == 4,
          "ally model should expose the same mechanical provider contract");
    gunner_packet = blank3d_actor_equipment_packet(&equipment, GUNNER_ID, 0);
    CHECK(gunner_packet != 0 && gunner_packet->resource_id == 2,
          "mechanimador packet should retain the selected model resource id");
    machine_presentation = blank3d_weapon_presentation_find(&presentations, 2);
    CHECK(machine_presentation != 0 &&
          machine_presentation->recoil_z == gwp89_fx_from_text("0.08"),
          "presentation values should remain Q20.12 until animator boundary");

    gunner_instance = blank3d_actor_equipment_find(&equipment, GUNNER_ID);
    CHECK(gunner_instance != 0, "gunner equipment instance should exist");
    blank3d_actor_equipment_trigger_fire(&equipment, GUNNER_ID, 2);
    CHECK(blank3d_actor_equipment_sync_actor(&equipment, &manager,
        GUNNER_ID, B3D_EQUIPMENT_ACTOR_ENEMY, 16U),
        "gunner fire event should tick animator");
    action_pose = blank3d_mechanical_weapon_part_pose(
        &gunner_instance->animator, B3D_MECH89_PART_SLIDE);
    CHECK(action_pose != 0 && action_pose->resolved.move.z !=
          (nm89_fx)(gunner_instance->animator.presentation.action_home_z * 16L),
          "machine-gun mechanism should move after fire");

    CHECK(gwp89_equip_slot(&manager, GUNNER_ID, pistol_slot, 1) == GWP89_OK,
          "same gunner can switch to another weapon model");
    CHECK(blank3d_actor_equipment_sync_actor(&equipment, &manager,
        GUNNER_ID, B3D_EQUIPMENT_ACTOR_ENEMY, 16U),
        "weapon system should reassign model and rig");
    CHECK(blank3d_attachment_update(&attachments),
          "replacement object should resolve through GAttach");
    CHECK(blank3d_actor_equipment_sync_actor(&equipment, &manager,
        GUNNER_ID, B3D_EQUIPMENT_ACTOR_ENEMY, 16U),
        "replacement model should animate");
    CHECK(blank3d_actor_equipment_model_id(&equipment, GUNNER_ID) == 1,
          "gunner should now carry pistol model without carrier-specific code");

    puts("Blank3D universal actor weapon assembly: OK");
    return 0;
}
