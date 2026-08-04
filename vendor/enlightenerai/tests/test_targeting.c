#include "enlightenerai.h"
#include "test_common.h"

static int no_block(void* user, const EAI_Vec3* from, const EAI_Vec3* to, EAI_U32 mask)
{
    (void)user;
    (void)from;
    (void)to;
    (void)mask;
    return 0;
}

int main(void)
{
    EAI_Context ctx;
    EAI_WorldOps ops;
    EAI_Vec3 p0;
    EAI_Vec3 p1;
    EAI_Vec3 p2;
    EAI_Vec3 dir;
    EAI_EntityId self_id;
    EAI_EntityId enemy_near;
    EAI_EntityId enemy_far;
    EAI_EntityId best;

    ops.raycast_world = no_block;
    ops.raycast_entity = 0;
    ops.edge_cost = 0;

    eai_init(&ctx, &ops, 0);

    self_id = eai_entity_create(&ctx);
    enemy_near = eai_entity_create(&ctx);
    enemy_far = eai_entity_create(&ctx);

    eai_entity_set_team(&ctx, self_id, 0);
    eai_entity_set_team(&ctx, enemy_near, 1);
    eai_entity_set_team(&ctx, enemy_far, 1);

    eai_targeting_set_hostile(&ctx, 0, 1, 1);

    eai_vec3_set(&p0, EAI_FX_FROM_RAW(0), EAI_FX_FROM_RAW(0), EAI_FX_FROM_RAW(0));
    eai_vec3_set(&p1, EAI_FX_FROM_RAW(262144), EAI_FX_FROM_RAW(0), EAI_FX_FROM_RAW(0));
    eai_vec3_set(&p2, EAI_FX_FROM_RAW(655360), EAI_FX_FROM_RAW(0), EAI_FX_FROM_RAW(0));
    eai_vec3_set(&dir, EAI_FX_FROM_RAW(65536), EAI_FX_FROM_RAW(0), EAI_FX_FROM_RAW(0));

    eai_entity_set_position(&ctx, self_id, &p0);
    eai_entity_set_position(&ctx, enemy_near, &p1);
    eai_entity_set_position(&ctx, enemy_far, &p2);
    eai_entity_set_forward(&ctx, self_id, &dir);
    eai_entity_set_view(&ctx, self_id, EAI_FX_FROM_RAW(1310720), EAI_FX_FROM_RAW(0));

    eai_update(&ctx, EAI_FX_FROM_RAW(6554));

    best = eai_targeting_select_best(&ctx, self_id);
    TEST_ASSERT(best == enemy_near);

    TEST_PASS();
}
