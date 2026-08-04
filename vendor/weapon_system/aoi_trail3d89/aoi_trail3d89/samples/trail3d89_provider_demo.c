#include "trail3d89.h"
#include <stdio.h>

/*
   Example external engine state. It can live in a static array, arena,
   component table, physics system, skeleton system, or any other owner.
*/
typedef struct object_transform_s {
    int x;
    int y;
    int z;
    int scale_x;
    int scale_y;
    int scale_z;
    int rot00;
    int rot01;
    int rot02;
    int rot10;
    int rot11;
    int rot12;
    int rot20;
    int rot21;
    int rot22;
} object_transform;

static object_transform g_sword;

static int sword_transform_provider(
    void *user,
    int trail_id,
    int tick,
    t3d89_transform *out_transform
)
{
    object_transform *object;

    (void)trail_id;
    (void)tick;
    object = (object_transform *)user;
    if (object == 0 || out_transform == 0) {
        return -1;
    }

    out_transform->tx = object->x;
    out_transform->ty = object->y;
    out_transform->tz = object->z;

    out_transform->m00 = object->rot00;
    out_transform->m01 = object->rot01;
    out_transform->m02 = object->rot02;
    out_transform->m10 = object->rot10;
    out_transform->m11 = object->rot11;
    out_transform->m12 = object->rot12;
    out_transform->m20 = object->rot20;
    out_transform->m21 = object->rot21;
    out_transform->m22 = object->rot22;

    out_transform->sx = object->scale_x;
    out_transform->sy = object->scale_y;
    out_transform->sz = object->scale_z;
    return T3D89_PROVIDER_READY;
}

static void set_identity_object(object_transform *object)
{
    object->x = 0;
    object->y = 0;
    object->z = 0;
    object->scale_x = T3D89_FP_ONE;
    object->scale_y = T3D89_FP_ONE;
    object->scale_z = T3D89_FP_ONE;
    object->rot00 = T3D89_FP_ONE;
    object->rot01 = 0;
    object->rot02 = 0;
    object->rot10 = 0;
    object->rot11 = T3D89_FP_ONE;
    object->rot12 = 0;
    object->rot20 = 0;
    object->rot21 = 0;
    object->rot22 = T3D89_FP_ONE;
}

int main(void)
{
    t3d89_ctx ctx;
    t3d89_desc desc;
    t3d89_sample local_sockets;
    int trail_id;
    int tick;
    int rc;

    t3d89_init(&ctx);
    t3d89_default_desc(&desc);
    desc.mode = T3D89_MODE_SOCKET_SWEEP;
    desc.life_ticks = 8;
    desc.sampler_mask = T3D89_SAMPLE_FORCE;
    trail_id = t3d89_create(&ctx, &desc);

    /* Sword sockets in object-local space. */
    t3d89_sample_clear(&local_sockets);
    local_sockets.a_x = 0;
    local_sockets.a_y = 0;
    local_sockets.a_z = 0;
    local_sockets.b_x = 0;
    local_sockets.b_y = t3d89_fp_from_int(2);
    local_sockets.b_z = 0;
    local_sockets.x = 0;
    local_sockets.y = T3D89_FP_ONE;
    local_sockets.z = 0;
    local_sockets.flags = T3D89_EMIT_HAS_AB;

    set_identity_object(&g_sword);
    rc = t3d89_bind_transform_provider(
        &ctx,
        trail_id,
        sword_transform_provider,
        &g_sword,
        &local_sockets,
        T3D89_PROVIDER_AUTOSTEP
    );
    if (rc != T3D89_OK) {
        printf("bind failed rc=%d\n", rc);
        return 1;
    }

    tick = 0;
    while (tick < 6) {
        /* The external engine/solver changes the object transform. */
        g_sword.x = t3d89_fp_from_int(tick);
        g_sword.y = t3d89_fp_from_int(tick / 2);

        /* trail3d89 asks the provider and emits the transformed sockets. */
        rc = t3d89_tick(&ctx, 1);
        if (rc != T3D89_OK) {
            printf("tick failed rc=%d\n", rc);
            return 1;
        }
        tick++;
    }

    printf("provider_calls=%d points=%d\n",
           t3d89_get_provider_call_count(&ctx, trail_id),
           t3d89_get_point_count(&ctx, trail_id));
    return 0;
}
