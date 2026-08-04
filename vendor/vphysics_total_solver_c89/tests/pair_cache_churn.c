#include <stdio.h>
#include "vp_api.h"

#define ARENA_BYTES 4000000UL
static vp_u8 g_mem[ARENA_BYTES];

int main(void)
{
    vpWorldDesc desc;
    vpWorld* world;
    vpBodyId body[80];
    vp_u16 i;
    vp_u32 key = 0;

    desc.maxBodies = 80;
    desc.maxContacts = 8;
    desc.maxJoints = 1;
    desc.solverIters = 1;
    world = vpWorldInit(g_mem, ARENA_BYTES, &desc);
    if (!world) return 1;

    for (i = 0; i < 80; ++i) {
        body[i] = vpBodyCreateKinematic(world);
        if (!body[i]) return 2;
    }

    for (i = 0; i < 2200; ++i) {
        vp_u16 a = (vp_u16)(i % 79);
        vp_u16 b = (vp_u16)(((i / 79) + 1) % 79);
        if (a == b) b = (vp_u16)((b + 1) % 79);
        world->stepId += 1;
        vpContactsBegin(world);
        vpContactsAddKeyed(world, ++key, body[a], body[b],
            vpVec3_make(0,0,0), vpVec3_make(VP_FX_ONE,0,0),
            1, VP_FX_ONE, 0);
        vpContactsFinalize(world, VP_FX_ONE);
        if (world->contactCount != 1) return 3;
    }

    printf("pair_cache_churn: OK pairs=%lu\n", (unsigned long)key);
    return 0;
}
