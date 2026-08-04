#include <stdio.h>
#include <string.h>

#include "../src/blank3d_attachment.h"

static int failures = 0;

#define CHECK(expr, text) do { \
    if (!(expr)) { \
        ++failures; \
        printf("FAIL: %s\n", text); \
    } else { \
        printf("PASS: %s\n", text); \
    } \
} while (0)

int main(void)
{
    Blank3DAttachmentWorld attachments;
    soq3d_pose hand;
    GAtt89_Xform local;
    GAtt89_Xform object_world;
    int socket_id;
    int attachment_id;

    memset(&attachments, 0, sizeof(attachments));
    blank3d_attachment_init(&attachments);
    CHECK(attachments.initialized, "GAttach bridge initializes");

    socket_id = blank3d_attachment_define_socket(
        &attachments, 7, "weapon_r", B3D_ATTACH89_SOCKET_WEAPON_R, 0);
    CHECK(socket_id >= 0, "character exposes named weapon socket");

    hand = soq3d_pose_identity();
    hand.position.x = soq3d_fx_from_int(10);
    hand.position.y = soq3d_fx_from_int(4);
    hand.position.z = soq3d_fx_from_int(-3);
    CHECK(blank3d_attachment_publish_socket_pose(
        &attachments, 7, B3D_ATTACH89_SOCKET_WEAPON_R, &hand),
        "Soquete3D publishes carrier socket to GAttach");

    local = gatt89_xform_identity();
    local.pos.z = gatt89_from_int(-1);
    attachment_id = blank3d_attachment_attach_object(
        &attachments, 7, 900, "rifle", "weapon_r", &local);
    CHECK(attachment_id >= 0, "arbitrary object attaches to character");
    CHECK(blank3d_attachment_update(&attachments),
          "attachment resolver updates object world pose");
    CHECK(blank3d_attachment_get_object_xform(
        &attachments, 900, &object_world),
        "attached object world transform is queryable");
    CHECK(gatt89_to_int(object_world.pos.x) == 10,
          "object follows character socket X");
    CHECK(gatt89_to_int(object_world.pos.y) == 4,
          "object follows character socket Y");
    CHECK(gatt89_to_int(object_world.pos.z) == -4,
          "object local offset composes after socket");

    hand.position.x = soq3d_fx_from_int(14);
    CHECK(blank3d_attachment_publish_socket_pose(
        &attachments, 7, B3D_ATTACH89_SOCKET_WEAPON_R, &hand),
        "carrier can republish an animated socket");
    CHECK(blank3d_attachment_update(&attachments),
          "object follows the new carrier pose");
    CHECK(blank3d_attachment_get_object_xform(
        &attachments, 900, &object_world) &&
        gatt89_to_int(object_world.pos.x) == 14,
        "object is not owned by the animator; GAttach moves its root");

    CHECK(blank3d_attachment_set_object_visible(
        &attachments, 7, 900, "rifle", 0) == GATTACH89_OK,
        "attachment visibility can be controlled independently");
    CHECK(blank3d_attachment_detach_object(
        &attachments, 7, 900, "rifle") == GATTACH89_OK,
        "object detaches without touching its animation rig");

    if (failures != 0) {
        printf("attachment stack: %d failure(s)\n", failures);
        return 1;
    }
    printf("attachment stack: all tests passed\n");
    return 0;
}
