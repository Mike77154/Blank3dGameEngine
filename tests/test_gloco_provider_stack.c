#include <stdio.h>
#include <string.h>

#include "blank3d_gloco.h"

/* Focused host stubs: this test exercises the real GLOCO, NumSys, VarRuntime,
   FlagStore, MovementBaseVerbs and Gamlib transform path. Collision/VPhysics
   are covered by their own engine regression tests and by syntax integration. */
int blank3d_vertical_axis_grounded(Blank3DVerticalAxis *axis,
                                   Blank3DVerticalBody *body)
{
    (void)axis; (void)body; return 1;
}
int blank3d_collision_sweep_bullet_mask(Blank3DCollision *collision,
                                        const GWP89_Vec3 *start,
                                        const GWP89_Vec3 *delta,
                                        gwp89_fx radius_fx,
                                        unsigned int target_layers,
                                        Blank3DCollisionHit *out_hit)
{
    (void)collision; (void)start; (void)delta; (void)radius_fx;
    (void)target_layers;
    if (out_hit) memset(out_hit, 0, sizeof(*out_hit));
    return 0;
}
int blank3d_vphysics_create_kinematic_box_q12(Blank3DVPhysics *p, vp_u32 id,
    long x,long y,long z,long hx,long hy,long hz,int c)
{ (void)p;(void)id;(void)x;(void)y;(void)z;(void)hx;(void)hy;(void)hz;(void)c; return 1; }
int blank3d_vphysics_destroy_object(Blank3DVPhysics *p, vp_u32 id)
{ (void)p;(void)id; return 1; }
int blank3d_vphysics_set_position_q12(Blank3DVPhysics *p, vp_u32 id,
    long x,long y,long z)
{ (void)p;(void)id;(void)x;(void)y;(void)z; return 1; }
int blank3d_systems_set_flag(Blank3DSystems *s, const char *key, int value)
{ return flagstore_set_bool(&s->flags, key, value); }

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
    Blank3DVerticalBody player_vertical;
    Blank3DVerticalBody enemy_vertical;
    Transform player;
    Transform enemy;
    mbv89_gamlib3d_adapter fallback;
    mbv89_context verbs;
    mbv89_actor movement_actor;
    unsigned long owner;
    unsigned long enemy_owner;
    ns_id stamina;
    ns_fx stamina_q10;
    vm89_value value;
    const Blank3DGlocoBinding *binding;
    const Blank3DGlocoBinding *enemy_binding;
    GLOCO_Actor *actor;
    GLOCO_Actor *enemy_actor;
    FlagsValue flag_value;
    Vec3 desired;
    long before_z;

    memset(&systems, 0, sizeof(systems));
    ns_init(&systems.numbers);
    flagstore_init(&systems.flags, systems.flag_entries, B3D_FLAG_CAPACITY,
                   systems.flag_pool, B3D_FLAG_POOL_CAPACITY);
    blank3d_variables_init(&vars, &systems);
    owner = 17UL;
    enemy_owner = 18UL;
    if (!blank3d_variables_instance_create(&vars, owner)) return fail("var instance");
    if (!blank3d_variables_instance_create(&vars, enemy_owner)) return fail("enemy var instance");

    memset(&vertical_axis, 0, sizeof(vertical_axis));
    memset(&player_vertical, 0, sizeof(player_vertical));
    memset(&enemy_vertical, 0, sizeof(enemy_vertical));
    transform_init(&player);
    transform_init(&enemy);
    mbv89_gamlib3d_adapter_init(&fallback);
    blank3d_gloco_init(&gloco, &systems, &vars, &vertical_axis, &fallback);
    memset(&collision, 0, sizeof(collision));
    memset(&physics, 0, sizeof(physics));
    blank3d_gloco_attach_physics(&gloco, &collision, &physics);
    if (!blank3d_gloco_bind(&gloco, B3D_PLAYER_ACTOR_ID, owner, &player,
                            &player_vertical, GLOCO_PROFILE_DEFAULT,
                            "config/locomotion/player.ini"))
        return fail("bind player");
    if (!blank3d_gloco_bind(&gloco, 1001, enemy_owner, &enemy,
                            &enemy_vertical, GLOCO_PROFILE_TACTICAL,
                            "config/locomotion/enemy.ini"))
        return fail("bind enemy");

    binding = blank3d_gloco_binding(&gloco, B3D_PLAYER_ACTOR_ID);
    enemy_binding = blank3d_gloco_binding(&gloco, 1001);
    if (!binding || !enemy_binding) return fail("binding lookup");
    actor = gloco_actor_get(&gloco.context, binding->gloco_id);
    enemy_actor = gloco_actor_get(&gloco.context, enemy_binding->gloco_id);
    if (!actor || !enemy_actor) return fail("initial actor lookup");
    if (gloco.context.profiles[actor->profile_id].run_speed != 7L * GLOCO_FX_ONE)
        return fail("player INI profile");
    if (gloco.context.profiles[enemy_actor->profile_id].run_speed != 5L * GLOCO_FX_ONE)
        return fail("enemy INI profile");
    if (actor->profile_id == enemy_actor->profile_id)
        return fail("profiles must be per binding");
    stamina = ns_find_value(&systems.numbers, (ns_owner)owner, "locomotion.stamina");
    if (stamina < 0) return fail("NumSys stamina authority");
    if (ns_get_by_id(&systems.numbers, stamina, &stamina_q10) != NS_OK || stamina_q10 <= 0)
        return fail("stamina initial value");

    mbv89_context_init(&verbs);
    mbv89_actor_init(&movement_actor);
    mbv89_gamlib3d_bind_actor(&movement_actor, &player);
    blank3d_gloco_install_movement_verbs(&gloco, &verbs);
    blank3d_gloco_begin_frame(&gloco, 100U);
    if (blank3d_gloco_mbv_game_provider(&gloco, MBV89_GAME_RUN_FORWARD) != MBV89_HANDLED)
        return fail("movement semantic bridge");
    before_z = player.position.z;
    blank3d_gloco_tick(&gloco, 100U);
    if (player.position.z == before_z) return fail("GLOCO did not move host transform");
    if (!flagstore_get(&systems.flags, "player.grounded", &flag_value) ||
        flag_value.type != FLAGS_VAL_BOOL || !flag_value.as.i)
        return fail("grounded flag publication");

    desired = player.position;
    desired.x += 2L * 4096L;
    blank3d_gloco_begin_frame(&gloco, 16U);
    if (!blank3d_gloco_feed_automotion_position(&gloco, B3D_PLAYER_ACTOR_ID,
                                                &player.position, &desired,
                                                3L * 4096L, 1))
        return fail("GAutomotion desired movement bridge");
    binding = blank3d_gloco_binding(&gloco, B3D_PLAYER_ACTOR_ID);
    if (!binding || binding->input.speed_override != 3L * GLOCO_FX_ONE)
        return fail("GAutomotion speed override conversion");

    if (!blank3d_variables_set_q16(&vars, VR89_SCOPE_INSTANCE, owner,
                                   "locomotion.run_speed", 2L * 65536L))
        return fail("live profile variable");
    blank3d_gloco_begin_frame(&gloco, 16U);
    (void)blank3d_gloco_mbv_game_provider(&gloco, MBV89_GAME_RUN_FORWARD);
    blank3d_gloco_tick(&gloco, 16U);
    actor = gloco_actor_get(&gloco.context, binding->gloco_id);
    if (!actor) return fail("actor lookup");
    if (gloco.context.profiles[actor->profile_id].run_speed != 2L * GLOCO_FX_ONE)
        return fail("VarRuntime profile tuning");
    if (gloco.context.profiles[enemy_actor->profile_id].run_speed != 5L * GLOCO_FX_ONE)
        return fail("per-Thing profile isolation");

    if (!blank3d_variables_get(&vars, VR89_SCOPE_INSTANCE, owner,
                               "locomotion.state", &value) ||
        value.type != VM89_VALUE_FIXED)
        return fail("state publication to VarRuntime");

    blank3d_gloco_begin_frame(&gloco, 100U);
    (void)blank3d_gloco_mbv_game_provider(&gloco, MBV89_GAME_RUN_FORWARD);
    blank3d_gloco_tick(&gloco, 100U);
    if (ns_get_by_id(&systems.numbers, stamina, &stamina_q10) != NS_OK)
        return fail("stamina read after tick");

    before_z = player.position.z;
    blank3d_gloco_begin_frame(&gloco, 100U);
    (void)blank3d_gloco_mbv_game_provider(&gloco, MBV89_GAME_RUN_FORWARD);
    blank3d_gloco_set_suspended(&gloco, B3D_PLAYER_ACTOR_ID, 1);
    blank3d_gloco_tick(&gloco, 100U);
    if (player.position.z != before_z) return fail("special-motion suspension");

    puts("OK: GLOCO89 Blank3D provider stack");
    return 0;
}
