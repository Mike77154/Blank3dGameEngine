#include "vehicleprovider89.h"
#include <stdio.h>

static gveh_i32 fake_move(void *user,
    const vehicleprovider89_movement_request *request)
{
    gveh_i32 *count;
    count = (gveh_i32 *)user;
    if (!request) return VEHICLEPROVIDER89_DECLINED;
    (*count)++;
    return VEHICLEPROVIDER89_HANDLED;
}

static gveh_i32 fake_phys(void *user,
    const vehicleprovider89_physics_request *request)
{
    gveh_i32 *count;
    count = (gveh_i32 *)user;
    if (!request) return VEHICLEPROVIDER89_DECLINED;
    (*count)++;
    return VEHICLEPROVIDER89_HANDLED;
}

int main(void)
{
    vehicleprovider89_movement mp;
    vehicleprovider89_physics pp;
    vehicleprovider89_movement_request mr;
    vehicleprovider89_physics_request pr;
    gveh_i32 mc;
    gveh_i32 pc;

    mc = 0;
    pc = 0;
    vehicleprovider89_movement_clear(&mp);
    vehicleprovider89_physics_clear(&pp);
    mp.user = &mc;
    mp.module_mask = VEHICLEPROVIDER89_MODULE_CAR;
    mp.step = fake_move;
    pp.user = &pc;
    pp.phase_mask = VEHICLEPROVIDER89_PHYS_INTEGRATE;
    pp.step = fake_phys;

    mr.runtime = 0;
    mr.vehicle = 0;
    mr.input = 0;
    mr.basis.fwd.x = mr.basis.fwd.y = mr.basis.fwd.z = 0;
    mr.basis.right = mr.basis.fwd;
    mr.basis.up = mr.basis.fwd;
    mr.dt = 0;
    mr.module = VEHICLEPROVIDER89_MODULE_CAR;
    if (vehicleprovider89_movement_try(&mp, &mr) != VEHICLEPROVIDER89_HANDLED) return 1;
    mr.module = VEHICLEPROVIDER89_MODULE_TANK;
    if (vehicleprovider89_movement_try(&mp, &mr) != VEHICLEPROVIDER89_DECLINED) return 2;

    pr.runtime = 0;
    pr.vehicle = 0;
    pr.input = 0;
    pr.basis = mr.basis;
    pr.dt = 0;
    pr.phase = VEHICLEPROVIDER89_PHYS_INTEGRATE;
    if (vehicleprovider89_physics_try(&pp, &pr) != VEHICLEPROVIDER89_HANDLED) return 3;
    pr.phase = VEHICLEPROVIDER89_PHYS_GRAVITY;
    if (vehicleprovider89_physics_try(&pp, &pr) != VEHICLEPROVIDER89_DECLINED) return 4;
    if (mc != 1 || pc != 1) return 5;
    printf("vehicleprovider89: OK\n");
    return 0;
}
