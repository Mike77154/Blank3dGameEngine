#include "enlightenerai.h"
#include <stdio.h>

static int no_block(void* user, const EAI_Vec3* from, const EAI_Vec3* to, EAI_U32 mask)
{
    (void)user;
    (void)from;
    (void)to;
    (void)mask;
    return 0;
}


static void print_fixed(EAI_Fixed value)
{
    long whole;
    long frac;

    if (value < 0)
    {
        putchar('-');
        value = -value;
    }

    whole = (long)(value / EAI_FX_ONE);
    frac = (long)(((value % EAI_FX_ONE) * 1000L) / EAI_FX_ONE);
    printf("%ld.%03ld", whole, frac);
}

static void print_event(void* user, const EAI_Event* event_data)
{
    (void)user;
    printf("event type=%u self=%u other=%u at=(",
           (unsigned)event_data->type,
           (unsigned)event_data->self_id,
           (unsigned)event_data->other_id);
    print_fixed(event_data->position.x);
    putchar(' ');
    print_fixed(event_data->position.y);
    putchar(' ');
    print_fixed(event_data->position.z);
    printf(")\n");
}

int main(void)
{
    EAI_Context ctx;
    EAI_WorldOps ops;
    EAI_Vec3 p0;
    EAI_Vec3 p1;
    EAI_Vec3 p2;
    EAI_Vec3 a_pos;
    EAI_Vec3 b_pos;
    EAI_Vec3 forward;
    EAI_NodeId n0;
    EAI_NodeId n1;
    EAI_NodeId n2;
    EAI_EntityId a;
    EAI_EntityId b;
    EAI_Path path;

    ops.raycast_world = no_block;
    ops.raycast_entity = 0;
    ops.edge_cost = 0;

    eai_init(&ctx, &ops, 0);

    eai_vec3_set(&p0, EAI_FX_FROM_RAW(0), EAI_FX_FROM_RAW(0), EAI_FX_FROM_RAW(0));
    eai_vec3_set(&p1, EAI_FX_FROM_RAW(327680), EAI_FX_FROM_RAW(0), EAI_FX_FROM_RAW(0));
    eai_vec3_set(&p2, EAI_FX_FROM_RAW(655360), EAI_FX_FROM_RAW(0), EAI_FX_FROM_RAW(0));

    n0 = eai_nav_add_node(&ctx, &p0, EAI_INVALID_ID, 0);
    n1 = eai_nav_add_node(&ctx, &p1, EAI_INVALID_ID, 0);
    n2 = eai_nav_add_node(&ctx, &p2, EAI_INVALID_ID, EAI_NODE_FLAG_COVER);

    eai_nav_add_bidirectional_edge(&ctx, n0, n1, EAI_FX_FROM_RAW(327680), 0);
    eai_nav_add_bidirectional_edge(&ctx, n1, n2, EAI_FX_FROM_RAW(327680), 0);

    a = eai_entity_create(&ctx);
    b = eai_entity_create(&ctx);

    eai_entity_set_team(&ctx, a, 0);
    eai_entity_set_team(&ctx, b, 1);
    eai_targeting_set_hostile(&ctx, 0, 1, 1);
    eai_targeting_set_hostile(&ctx, 1, 0, 1);

    eai_vec3_set(&a_pos, EAI_FX_FROM_RAW(0), EAI_FX_FROM_RAW(0), EAI_FX_FROM_RAW(0));
    eai_vec3_set(&b_pos, EAI_FX_FROM_RAW(589824), EAI_FX_FROM_RAW(0), EAI_FX_FROM_RAW(0));
    eai_vec3_set(&forward, EAI_FX_FROM_RAW(65536), EAI_FX_FROM_RAW(0), EAI_FX_FROM_RAW(0));

    eai_entity_set_position(&ctx, a, &a_pos);
    eai_entity_set_position(&ctx, b, &b_pos);
    eai_entity_set_forward(&ctx, a, &forward);

    eai_update(&ctx, EAI_FX_FROM_RAW(6554));

    if (eai_nav_find_path(&ctx, a, n0, n2, &path))
    {
        eai_actions_follow_path(&ctx, a, &path);
        printf("path count=%u\n", (unsigned)path.count);
    }

    eai_fsm_dispatch_events(&ctx, print_event, 0);
    return 0;
}
