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
    EAI_Vec3 pa;
    EAI_Vec3 pb;
    EAI_Vec3 dir;
    EAI_EntityId a;
    EAI_EntityId b;
    EAI_Event ev;

    ops.raycast_world = no_block;
    ops.raycast_entity = 0;
    ops.edge_cost = 0;

    eai_init(&ctx, &ops, 0);

    a = eai_entity_create(&ctx);
    b = eai_entity_create(&ctx);

    eai_vec3_set(&pa, EAI_FX_FROM_RAW(0), EAI_FX_FROM_RAW(0), EAI_FX_FROM_RAW(0));
    eai_vec3_set(&pb, EAI_FX_FROM_RAW(262144), EAI_FX_FROM_RAW(0), EAI_FX_FROM_RAW(0));
    eai_vec3_set(&dir, EAI_FX_FROM_RAW(65536), EAI_FX_FROM_RAW(0), EAI_FX_FROM_RAW(0));

    eai_entity_set_position(&ctx, a, &pa);
    eai_entity_set_position(&ctx, b, &pb);
    eai_entity_set_forward(&ctx, a, &dir);
    eai_entity_set_view(&ctx, a, EAI_FX_FROM_RAW(655360), EAI_FX_FROM_RAW(32768));

    eai_update(&ctx, EAI_FX_FROM_RAW(6554));

    TEST_ASSERT(eai_perception_is_visible(&ctx, a, b));
    TEST_ASSERT(ctx.entities[a].memory.last_seen_entity == b);

    TEST_ASSERT(eai_events_pop(&ctx, &ev));
    TEST_ASSERT(ev.type == EAI_EVENT_SEE_TARGET);

    TEST_PASS();
}
