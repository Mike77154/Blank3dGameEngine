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
    EAI_Vec3 p3;
    EAI_NodeId n0;
    EAI_NodeId n1;
    EAI_NodeId n2;
    EAI_NodeId n3;
    EAI_EntityId e;
    EAI_Path path;

    ops.raycast_world = no_block;
    ops.raycast_entity = 0;
    ops.edge_cost = 0;

    eai_init(&ctx, &ops, 0);

    eai_vec3_set(&p0, EAI_FX_FROM_RAW(0), EAI_FX_FROM_RAW(0), EAI_FX_FROM_RAW(0));
    eai_vec3_set(&p1, EAI_FX_FROM_RAW(65536), EAI_FX_FROM_RAW(0), EAI_FX_FROM_RAW(0));
    eai_vec3_set(&p2, EAI_FX_FROM_RAW(131072), EAI_FX_FROM_RAW(0), EAI_FX_FROM_RAW(0));
    eai_vec3_set(&p3, EAI_FX_FROM_RAW(65536), EAI_FX_FROM_RAW(65536), EAI_FX_FROM_RAW(0));

    n0 = eai_nav_add_node(&ctx, &p0, EAI_INVALID_ID, 0);
    n1 = eai_nav_add_node(&ctx, &p1, EAI_INVALID_ID, 0);
    n2 = eai_nav_add_node(&ctx, &p2, EAI_INVALID_ID, 0);
    n3 = eai_nav_add_node(&ctx, &p3, EAI_INVALID_ID, 0);

    TEST_ASSERT(n0 != EAI_INVALID_ID);
    TEST_ASSERT(n3 != EAI_INVALID_ID);

    eai_nav_add_bidirectional_edge(&ctx, n0, n1, EAI_FX_FROM_RAW(65536), 0);
    eai_nav_add_bidirectional_edge(&ctx, n1, n2, EAI_FX_FROM_RAW(65536), 0);
    eai_nav_add_bidirectional_edge(&ctx, n0, n3, EAI_FX_FROM_RAW(98304), 0);
    eai_nav_add_bidirectional_edge(&ctx, n3, n2, EAI_FX_FROM_RAW(98304), 0);

    e = eai_entity_create(&ctx);
    TEST_ASSERT(e != EAI_INVALID_ID);

    TEST_ASSERT(eai_nav_find_path(&ctx, e, n0, n2, &path));
    TEST_ASSERT(path.count >= 2);
    TEST_ASSERT(path.nodes[0] == n0);
    TEST_ASSERT(path.nodes[path.count - 1u] == n2);

    TEST_PASS();
}
