#include "enlightenerai.h"
#include "test_common.h"

typedef struct TestWorld
{
    int wall_enabled;
} TestWorld;

static int test_raycast(void* user, const EAI_Vec3* from, const EAI_Vec3* to, EAI_U32 mask)
{
    TestWorld* world;
    (void)mask;

    world = (TestWorld*)user;

    if (!world->wall_enabled)
    {
        return 0;
    }

    if (((from->x < EAI_FX_FROM_RAW(327680) && to->x > EAI_FX_FROM_RAW(327680)) || (from->x > EAI_FX_FROM_RAW(327680) && to->x < EAI_FX_FROM_RAW(327680))))
    {
        return 1;
    }

    return 0;
}

int main(void)
{
    EAI_Context ctx;
    EAI_WorldOps ops;
    TestWorld world;
    EAI_Vec3 p0;
    EAI_Vec3 p1;
    EAI_Vec3 p2;
    EAI_Vec3 p3;
    EAI_EntityId self_id;
    EAI_EntityId threat_id;
    EAI_NodeId n0;
    EAI_NodeId n1;
    EAI_NodeId n2;
    EAI_NodeId n3;
    EAI_NodeId cover;

    world.wall_enabled = 1;
    ops.raycast_world = test_raycast;
    ops.raycast_entity = 0;
    ops.edge_cost = 0;

    eai_init(&ctx, &ops, &world);

    eai_vec3_set(&p0, EAI_FX_FROM_RAW(0), EAI_FX_FROM_RAW(0), EAI_FX_FROM_RAW(0));
    eai_vec3_set(&p1, EAI_FX_FROM_RAW(262144), EAI_FX_FROM_RAW(0), EAI_FX_FROM_RAW(0));
    eai_vec3_set(&p2, EAI_FX_FROM_RAW(393216), EAI_FX_FROM_RAW(65536), EAI_FX_FROM_RAW(0));
    eai_vec3_set(&p3, EAI_FX_FROM_RAW(524288), EAI_FX_FROM_RAW(0), EAI_FX_FROM_RAW(0));

    n0 = eai_nav_add_node(&ctx, &p0, EAI_INVALID_ID, 0);
    n1 = eai_nav_add_node(&ctx, &p1, EAI_INVALID_ID, EAI_NODE_FLAG_COVER);
    n2 = eai_nav_add_node(&ctx, &p2, EAI_INVALID_ID, EAI_NODE_FLAG_COVER);
    n3 = eai_nav_add_node(&ctx, &p3, EAI_INVALID_ID, 0);

    eai_nav_add_bidirectional_edge(&ctx, n0, n1, EAI_FX_FROM_RAW(65536), 0);
    eai_nav_add_bidirectional_edge(&ctx, n1, n2, EAI_FX_FROM_RAW(65536), 0);
    eai_nav_add_bidirectional_edge(&ctx, n2, n3, EAI_FX_FROM_RAW(65536), 0);

    self_id = eai_entity_create(&ctx);
    threat_id = eai_entity_create(&ctx);

    eai_entity_set_position(&ctx, self_id, &p0);
    eai_entity_set_position(&ctx, threat_id, &p3);

    TEST_ASSERT(!eai_tactical_position_exposed(&ctx, &p1, threat_id));
    TEST_ASSERT(eai_tactical_find_cover(&ctx, self_id, threat_id, &cover));
    TEST_ASSERT(cover == n1 || cover == n2);

    TEST_PASS();
}
