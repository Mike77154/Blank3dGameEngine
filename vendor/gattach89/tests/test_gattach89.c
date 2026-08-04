#include <stdio.h>
#include "gattach89.h"

typedef struct TestCtx {
    GAtt89_Xform entity[4];
    int used[4];
    int visible[4];
} TestCtx;

static int test_get_entity(void *user, int entity_id, GAtt89_Xform *out_xform)
{
    TestCtx *ctx;
    if (user == 0 || out_xform == 0) return 0;
    if (entity_id < 0 || entity_id >= 4) return 0;
    ctx = (TestCtx *)user;
    if (!ctx->used[entity_id]) return 0;
    *out_xform = ctx->entity[entity_id];
    return 1;
}

static int test_get_bone(void *user, int entity_id, int bone_index, GAtt89_Xform *out_xform)
{
    GAtt89_Xform base;
    GAtt89_Xform local;
    if (!test_get_entity(user, entity_id, &base)) return 0;
    local = gatt89_xform_identity();
    if (bone_index != 1) return 0;
    local.pos.x = gatt89_from_int(2);
    local.pos.y = gatt89_from_int(3);
    local.pos.z = gatt89_from_int(4);
    *out_xform = gatt89_xform_compose(&base, &local);
    return 1;
}

static int test_visible(void *user, int entity_id)
{
    TestCtx *ctx;
    if (user == 0) return 0;
    if (entity_id < 0 || entity_id >= 4) return 0;
    ctx = (TestCtx *)user;
    return ctx->visible[entity_id];
}

static void test_set_entity(void *user, int entity_id, const GAtt89_Xform *xform)
{
    TestCtx *ctx;
    if (user == 0 || xform == 0) return;
    if (entity_id < 0 || entity_id >= 4) return;
    ctx = (TestCtx *)user;
    ctx->entity[entity_id] = *xform;
}

static int expect_int(const char *name, int got, int want)
{
    if (got != want) {
        printf("FAIL %s got=%d want=%d\n", name, got, want);
        return 1;
    }
    return 0;
}

int main(void)
{
    GAtt89_World w;
    TestCtx ctx;
    GAtt89_Callbacks cb;
    GAtt89_Output out;
    GAtt89_Xform x;
    GAtt89_Xform local;
    int i;
    int sid;
    int aid;
    int failed;

    failed = 0;
    for (i = 0; i < 4; ++i) {
        ctx.entity[i] = gatt89_xform_identity();
        ctx.used[i] = 0;
        ctx.visible[i] = 1;
    }
    ctx.used[1] = 1;
    ctx.used[2] = 1;
    ctx.entity[1].pos.x = gatt89_from_int(10);
    ctx.entity[1].pos.y = gatt89_from_int(20);
    ctx.entity[1].pos.z = gatt89_from_int(30);

    cb.user = &ctx;
    cb.get_entity_xform = test_get_entity;
    cb.get_bone_xform = test_get_bone;
    cb.get_entity_visible = test_visible;
    cb.set_entity_xform = test_set_entity;
    cb.set_part_visible = 0;
    cb.on_event = 0;

    gatt89_world_init(&w);
    local = gatt89_xform_identity();
    local.pos.x = gatt89_from_int(5);
    local.pos.y = gatt89_from_int(0);
    local.pos.z = gatt89_from_int(0);

    sid = gatt89_socket_add(&w, 1, "hand_r", GATTACH89_SOCKET_BONE, 1, GATTACH89_INVALID_ID, &local);
    aid = gatt89_attach_add(&w, 1, 2, "weapon", "hand_r", GATTACH89_ATTACH_ENTITY, GATTACH89_AF_CALL_CHILD_SETTER, 0);

    failed += expect_int("sid valid", sid >= 0, 1);
    failed += expect_int("aid valid", aid >= 0, 1);
    failed += expect_int("update", gatt89_update(&w, &cb, &out), GATTACH89_OK);
    failed += expect_int("child x", gatt89_to_int(ctx.entity[2].pos.x), 17);
    failed += expect_int("child y", gatt89_to_int(ctx.entity[2].pos.y), 23);
    failed += expect_int("child z", gatt89_to_int(ctx.entity[2].pos.z), 34);

    failed += expect_int("find", gatt89_attach_find(&w, 1, 2, "weapon"), aid);
    failed += expect_int("part add", gatt89_part_add(&w, 1, "helmet", 1) >= 0, 1);
    i = gatt89_part_find(&w, 1, "helmet");
    failed += expect_int("part visible", gatt89_part_is_visible(&w, i), 1);
    failed += expect_int("hide part", gatt89_part_set_visible(&w, i, 0), GATTACH89_OK);
    failed += expect_int("part hidden", gatt89_part_is_visible(&w, i), 0);

    x = gatt89_xform_identity();
    failed += expect_int("get attach world", gatt89_attach_get_world(&w, aid, &x), GATTACH89_OK);
    failed += expect_int("world x", gatt89_to_int(x.pos.x), 17);

    if (failed) {
        printf("gattach89 tests failed=%d\n", failed);
        return 1;
    }
    printf("gattach89 tests ok\n");
    return 0;
}
