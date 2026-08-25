#include <stdio.h>
#include <string.h>

#include "blank3d_gloco.h"

int blank3d_vertical_axis_grounded(Blank3DVerticalAxis *axis,
                                   Blank3DVerticalBody *body)
{
    (void)axis;
    (void)body;
    return 1;
}

int blank3d_vphysics_create_kinematic_box_q12(Blank3DVPhysics *p, vp_u32 id,
    long x, long y, long z, long hx, long hy, long hz, int c)
{
    (void)p; (void)id; (void)x; (void)y; (void)z;
    (void)hx; (void)hy; (void)hz; (void)c;
    return 1;
}

int blank3d_vphysics_destroy_object(Blank3DVPhysics *p, vp_u32 id)
{
    (void)p; (void)id; return 1;
}

int blank3d_vphysics_set_position_q12(Blank3DVPhysics *p, vp_u32 id,
    long x, long y, long z)
{
    (void)p; (void)id; (void)x; (void)y; (void)z; return 1;
}

int blank3d_systems_set_flag(Blank3DSystems *s, const char *key, int value)
{
    return flagstore_set_bool(&s->flags, key, value);
}

static int fail(const char *msg)
{
    fprintf(stderr, "FAIL: %s\n", msg);
    return 1;
}

int main(void)
{
    Blank3DSystems systems;
    Blank3DVariables vars;
    Blank3DGloco gloco;
    Blank3DCollision collision;
    Blank3DVPhysics physics;
    Blank3DVerticalAxis vertical_axis;
    Blank3DVerticalBody vertical_body;
    Transform player;
    mbv89_gamlib3d_adapter fallback;
    unsigned long owner;
    long before_x;
    long before_z;
    const Blank3DGlocoBinding *binding;
    const GLOCO_Actor *actor;

    memset(&systems, 0, sizeof(systems));
    ns_init(&systems.numbers);
    flagstore_init(&systems.flags, systems.flag_entries, B3D_FLAG_CAPACITY,
                   systems.flag_pool, B3D_FLAG_POOL_CAPACITY);
    blank3d_variables_init(&vars, &systems);
    owner = 77UL;
    if (!blank3d_variables_instance_create(&vars, owner))
        return fail("variable instance");

    memset(&vertical_axis, 0, sizeof(vertical_axis));
    memset(&vertical_body, 0, sizeof(vertical_body));
    memset(&physics, 0, sizeof(physics));
    transform_init(&player);
    mbv89_gamlib3d_adapter_init(&fallback);
    blank3d_collision_init(&collision);

    blank3d_gloco_init(&gloco, &systems, &vars, &vertical_axis, &fallback);
    blank3d_gloco_attach_physics(&gloco, &collision, &physics);
    if (!blank3d_gloco_bind(&gloco, B3D_PLAYER_ACTOR_ID, owner, &player,
                            &vertical_body, GLOCO_PROFILE_DEFAULT,
                            "config/locomotion/player.ini"))
        return fail("bind player");

    before_x = player.position.x;
    before_z = player.position.z;
    blank3d_gloco_begin_frame(&gloco, 100U);
    if (blank3d_gloco_mbv_game_provider(&gloco,
            MBV89_GAME_RUN_FORWARD) != MBV89_HANDLED)
        return fail("run forward bridge");
    blank3d_gloco_tick(&gloco, 100U);

    if (player.position.x == before_x && player.position.z == before_z)
        return fail("ground contact incorrectly blocked horizontal locomotion");

    binding = blank3d_gloco_binding(&gloco, B3D_PLAYER_ACTOR_ID);
    if (!binding) return fail("binding lookup");
    actor = gloco_actor_get_const(&gloco.context, binding->gloco_id);
    if (!actor) return fail("actor lookup");
    if ((actor->flags & GLOCO_FLAG_BLOCKED) != 0U)
        return fail("ground contact published BLOCKED");

    /* The same absolute-Y rule must still block a real vertical wall. */
    if (!blank3d_gloco_unbind(&gloco, B3D_PLAYER_ACTOR_ID))
        return fail("unbind before wall test");
    transform_init(&player);
    player.position.z = 58L * 4096L;
    transform_rotate(&player, G3D_FIX_FROM_INT(180), 0, 0);
    if (!blank3d_gloco_bind(&gloco, B3D_PLAYER_ACTOR_ID, owner, &player,
                            &vertical_body, GLOCO_PROFILE_DEFAULT,
                            "config/locomotion/player.ini"))
        return fail("rebind wall test");
    blank3d_gloco_begin_frame(&gloco, 500U);
    (void)blank3d_gloco_mbv_game_provider(&gloco, MBV89_GAME_RUN_FORWARD);
    blank3d_gloco_tick(&gloco, 500U);
    binding = blank3d_gloco_binding(&gloco, B3D_PLAYER_ACTOR_ID);
    if (!binding) return fail("wall binding lookup");
    actor = gloco_actor_get_const(&gloco.context, binding->gloco_id);
    if (!actor) return fail("wall actor lookup");
    if ((actor->flags & GLOCO_FLAG_BLOCKED) == 0U)
        return fail("vertical wall no longer blocks locomotion");

    puts("OK: GLOCO89 floor passes and vertical wall blocks");
    return 0;
}
