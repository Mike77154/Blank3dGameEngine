#include <stdio.h>
#include "gattach89.h"
#include "gattach89_render_bridge.h"

#define DEMO_MAX_ENTITIES 16
#define ENTITY_PLAYER 1
#define ENTITY_PISTOL 2
#define ENTITY_MUZZLE_FLASH 3

typedef struct DemoEntity {
    int used;
    int visible;
    GAtt89_Xform xform;
} DemoEntity;

typedef struct DemoWorld {
    DemoEntity entities[DEMO_MAX_ENTITIES];
} DemoWorld;

static int demo_get_entity(void *user, int entity_id, GAtt89_Xform *out_xform)
{
    DemoWorld *dw;
    if (user == 0 || out_xform == 0) return 0;
    if (entity_id < 0 || entity_id >= DEMO_MAX_ENTITIES) return 0;
    dw = (DemoWorld *)user;
    if (!dw->entities[entity_id].used) return 0;
    *out_xform = dw->entities[entity_id].xform;
    return 1;
}

static int demo_get_bone(void *user, int entity_id, int bone_index, GAtt89_Xform *out_xform)
{
    DemoWorld *dw;
    GAtt89_Xform base;
    GAtt89_Xform bone_local;

    if (user == 0 || out_xform == 0) return 0;
    if (entity_id < 0 || entity_id >= DEMO_MAX_ENTITIES) return 0;
    dw = (DemoWorld *)user;
    if (!dw->entities[entity_id].used) return 0;

    base = dw->entities[entity_id].xform;
    bone_local = gatt89_xform_identity();

    if (bone_index == 10) {
        bone_local.pos.x = gatt89_from_int(2);
        bone_local.pos.y = gatt89_from_int(5);
        bone_local.pos.z = gatt89_from_int(1);
    } else if (bone_index == 20) {
        bone_local.pos.x = gatt89_from_int(0);
        bone_local.pos.y = gatt89_from_int(7);
        bone_local.pos.z = gatt89_from_int(-2);
    } else {
        return 0;
    }

    *out_xform = gatt89_xform_compose(&base, &bone_local);
    return 1;
}

static int demo_get_visible(void *user, int entity_id)
{
    DemoWorld *dw;
    if (user == 0) return 0;
    if (entity_id < 0 || entity_id >= DEMO_MAX_ENTITIES) return 0;
    dw = (DemoWorld *)user;
    if (!dw->entities[entity_id].used) return 0;
    return dw->entities[entity_id].visible;
}

static void demo_set_entity(void *user, int entity_id, const GAtt89_Xform *xform)
{
    DemoWorld *dw;
    if (user == 0 || xform == 0) return;
    if (entity_id < 0 || entity_id >= DEMO_MAX_ENTITIES) return;
    dw = (DemoWorld *)user;
    if (!dw->entities[entity_id].used) return;
    dw->entities[entity_id].xform = *xform;
}

static void demo_set_part(void *user, int entity_id, const char *part_name, int visible)
{
    (void)user;
    printf("part entity=%d name=%s visible=%d\n", entity_id, part_name, visible);
}

static void demo_on_event(void *user, const GAtt89_Event *ev)
{
    (void)user;
    if (ev != 0 && ev->kind == GATTACH89_EVENT_ATTACHMENT_RESOLVED) {
        printf("attachment %d child=%d pos=(%d,%d,%d)\n",
               ev->attachment_id,
               ev->child_entity_id,
               gatt89_to_int(ev->xform.pos.x),
               gatt89_to_int(ev->xform.pos.y),
               gatt89_to_int(ev->xform.pos.z));
    }
}

int main(void)
{
    GAtt89_World aw;
    DemoWorld dw;
    GAtt89_Callbacks cb;
    GAtt89_Output out;
    GAtt89_RenderQueue rq;
    GAtt89_Xform local;
    GAtt89_Xform weapon_offset;
    int i;
    int socket_hand;
    int pistol_attach;

    for (i = 0; i < DEMO_MAX_ENTITIES; ++i) {
        dw.entities[i].used = 0;
        dw.entities[i].visible = 1;
        dw.entities[i].xform = gatt89_xform_identity();
    }

    dw.entities[ENTITY_PLAYER].used = 1;
    dw.entities[ENTITY_PLAYER].visible = 1;
    dw.entities[ENTITY_PLAYER].xform = gatt89_xform_identity();
    dw.entities[ENTITY_PLAYER].xform.pos.x = gatt89_from_int(100);
    dw.entities[ENTITY_PLAYER].xform.pos.y = gatt89_from_int(0);
    dw.entities[ENTITY_PLAYER].xform.pos.z = gatt89_from_int(30);

    dw.entities[ENTITY_PISTOL].used = 1;
    dw.entities[ENTITY_PISTOL].visible = 1;
    dw.entities[ENTITY_PISTOL].xform = gatt89_xform_identity();

    dw.entities[ENTITY_MUZZLE_FLASH].used = 1;
    dw.entities[ENTITY_MUZZLE_FLASH].visible = 1;
    dw.entities[ENTITY_MUZZLE_FLASH].xform = gatt89_xform_identity();

    gatt89_world_init(&aw);

    local = gatt89_xform_identity();
    local.pos.x = gatt89_from_int(1);
    local.pos.y = gatt89_from_int(0);
    local.pos.z = gatt89_from_int(0);

    socket_hand = gatt89_socket_add(&aw, ENTITY_PLAYER, "hand_r", GATTACH89_SOCKET_BONE, 10, GATTACH89_INVALID_ID, &local);
    if (socket_hand < 0) return 2;

    local = gatt89_xform_identity();
    local.pos.x = gatt89_from_int(3);
    local.pos.y = gatt89_from_int(0);
    local.pos.z = gatt89_from_int(0);
    gatt89_socket_add(&aw, ENTITY_PISTOL, "muzzle", GATTACH89_SOCKET_ENTITY, GATTACH89_INVALID_ID, GATTACH89_INVALID_ID, &local);

    weapon_offset = gatt89_xform_identity();
    pistol_attach = gatt89_attach_add(&aw, ENTITY_PLAYER, ENTITY_PISTOL, "pistol_visible", "hand_r", GATTACH89_ATTACH_ENTITY,
                                      GATTACH89_AF_CALL_CHILD_SETTER | GATTACH89_AF_RENDER_PACKET | GATTACH89_AF_INHERIT_PARENT_VIS,
                                      &weapon_offset);
    if (pistol_attach < 0) return 3;

    gatt89_part_add(&aw, ENTITY_PLAYER, "helmet", 0);
    gatt89_part_add(&aw, ENTITY_PLAYER, "backpack", 1);

    cb.user = &dw;
    cb.get_entity_xform = demo_get_entity;
    cb.get_bone_xform = demo_get_bone;
    cb.get_entity_visible = demo_get_visible;
    cb.set_entity_xform = demo_set_entity;
    cb.set_part_visible = demo_set_part;
    cb.on_event = demo_on_event;

    gatt89_update(&aw, &cb, &out);
    gatt89_render_queue_from_output(&out, &aw, &rq);

    printf("render packets=%d dropped=%d\n", rq.packet_count, rq.dropped_packet_count);
    printf("pistol final=(%d,%d,%d)\n",
           gatt89_to_int(dw.entities[ENTITY_PISTOL].xform.pos.x),
           gatt89_to_int(dw.entities[ENTITY_PISTOL].xform.pos.y),
           gatt89_to_int(dw.entities[ENTITY_PISTOL].xform.pos.z));

    return 0;
}
