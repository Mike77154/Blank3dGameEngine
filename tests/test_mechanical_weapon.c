#include <stdio.h>
#include <string.h>

#include "../src/blank3d_mechanical_weapon.h"

static int failures = 0;

#define CHECK(expr, text) do { \
    if (!(expr)) { \
        ++failures; \
        printf("FAIL: %s\n", text); \
    } else { \
        printf("PASS: %s\n", text); \
    } \
} while (0)

static const nm89_geometry_packet *find_packet(
    const Blank3DMechanicalWeapon *weapon, int mesh_id)
{
    int i;
    const nm89_geometry_packet *packet;
    for (i = 0; i < blank3d_mechanical_weapon_packet_count(weapon); ++i) {
        packet = blank3d_mechanical_weapon_packet(weapon, i);
        if (packet && packet->mesh_id == mesh_id) return packet;
    }
    return 0;
}

int main(void)
{
    Blank3DMechanicalWeapon weapon;
    GAtt89_Xform attachment;
    const nm89_geometry_packet *body;
    const nm89_geometry_packet *magazine;
    const nm89_pose *slide_pose;
    const nm89_pose *magazine_pose;
    const nm89_pose *muzzle_socket;
    nm89_fx slide_home;
    int i;

    memset(&weapon, 0, sizeof(weapon));
    CHECK(blank3d_mechanical_weapon_init(&weapon), "bridge initializes");
    CHECK(blank3d_mechanical_weapon_packet_count(&weapon) == 4,
          "four mechanical geometry bindings are flushed");

    attachment = gatt89_xform_identity();
    attachment.pos.x = gatt89_from_int(3);
    attachment.pos.y = gatt89_from_int(2);
    attachment.pos.z = gatt89_from_int(-4);
    blank3d_mechanical_weapon_set_attachment(&weapon, &attachment);
    CHECK(blank3d_mechanical_weapon_update(&weapon, 16U),
          "GAttach object transform steps NationalMecanicanimal89");
    body = find_packet(&weapon, B3D_MECH89_MESH_BODY);
    CHECK(body != 0, "body packet exists");
    CHECK(body && body->world.m[0][3] == nm89_fx_from_int(3),
          "attached object propagates world X");
    CHECK(body && body->world.m[1][3] == nm89_fx_from_int(2),
          "attached object propagates world Y");

    slide_pose = blank3d_mechanical_weapon_part_pose(
        &weapon, B3D_MECH89_PART_SLIDE);
    slide_home = (nm89_fx)((long)weapon.presentation.action_home_z * 16L);
    CHECK(slide_pose && slide_pose->resolved.move.z == slide_home,
          "slide begins at constrained home");

    muzzle_socket = blank3d_mechanical_weapon_muzzle_socket(&weapon);
    CHECK(muzzle_socket != 0,
          "mechanism exposes a muzzle socket without drawing muzzle FX");

    blank3d_mechanical_weapon_trigger_fire(&weapon);
    CHECK(blank3d_mechanical_weapon_update(&weapon, 16U),
          "fire action advances one deterministic tick");
    slide_pose = blank3d_mechanical_weapon_part_pose(
        &weapon, B3D_MECH89_PART_SLIDE);
    CHECK(slide_pose && slide_pose->resolved.move.z > slide_home,
          "fire clip recoils the slide");
    CHECK(blank3d_mechanical_weapon_packet_count(&weapon) == 4,
          "muzzle effect is not a mechanical geometry packet");
    CHECK(weapon.emitted_event_count > 0,
          "mechanical marker event reaches provider");

    for (i = 0; i < 8; ++i)
        CHECK(blank3d_mechanical_weapon_update(&weapon, 16U),
              "fire clip recovery tick");
    slide_pose = blank3d_mechanical_weapon_part_pose(
        &weapon, B3D_MECH89_PART_SLIDE);
    CHECK(slide_pose && slide_pose->resolved.move.z == slide_home,
          "slide returns to home");

    blank3d_mechanical_weapon_set_reload(&weapon, 1, 250U, 1000U);
    CHECK(blank3d_mechanical_weapon_update(&weapon, 16U),
          "reload provider evaluates extraction phase");
    magazine_pose = blank3d_mechanical_weapon_part_pose(
        &weapon, B3D_MECH89_PART_MAGAZINE);
    CHECK(magazine_pose &&
          magazine_pose->resolved.move.y < nm89_fx_from_ratio(-5, 16),
          "magazine travels downward during extraction");
    magazine = find_packet(&weapon, B3D_MECH89_MESH_MAGAZINE);
    CHECK(magazine && magazine->visible != 0U,
          "magazine remains visible before removal window");

    blank3d_mechanical_weapon_set_reload(&weapon, 1, 450U, 1000U);
    CHECK(blank3d_mechanical_weapon_update(&weapon, 16U),
          "reload provider evaluates removed phase");
    magazine = find_packet(&weapon, B3D_MECH89_MESH_MAGAZINE);
    CHECK(magazine && magazine->visible == 0U,
          "magazine visibility is provider-controlled");

    blank3d_mechanical_weapon_set_reload(&weapon, 1, 850U, 1000U);
    CHECK(blank3d_mechanical_weapon_update(&weapon, 16U),
          "reload provider evaluates insertion phase");
    magazine = find_packet(&weapon, B3D_MECH89_MESH_MAGAZINE);
    CHECK(magazine && magazine->visible != 0U,
          "magazine reappears for insertion");
    magazine_pose = blank3d_mechanical_weapon_part_pose(
        &weapon, B3D_MECH89_PART_MAGAZINE);
    CHECK(magazine_pose &&
          magazine_pose->resolved.move.y >= nm89_fx_from_ratio(-9, 10),
          "magazine motion remains inside mechanical constraint");

    blank3d_mechanical_weapon_set_reload(&weapon, 0, 0U, 1000U);
    CHECK(blank3d_mechanical_weapon_update(&weapon, 16U),
          "reload provider returns to idle");
    magazine_pose = blank3d_mechanical_weapon_part_pose(
        &weapon, B3D_MECH89_PART_MAGAZINE);
    CHECK(magazine_pose &&
          magazine_pose->resolved.move.y == nm89_fx_from_ratio(-5, 16),
          "magazine returns exactly to home");

    if (failures != 0) {
        printf("mechanical weapon bridge: %d failure(s)\n", failures);
        return 1;
    }
    printf("mechanical weapon bridge: all tests passed\n");
    return 0;
}
