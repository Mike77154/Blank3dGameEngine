#include "watermovement89.h"

static gveh_vec3 watermovement89_point_world(const gveh_body *body, gveh_basis basis, gveh_vec3 local)
{
    return gveh_v3_add(body->pos, gveh_basis_local_to_world(basis, local));
}

void watermovement89_step(gveh_world_i *world,
                          gveh_profile *profile,
                          gveh_body *body,
                          gveh_basis basis,
                          const gveh_input *input,
                          gveh_i32 tick,
                          gveh_fx dt,
                          gveh_fx_queue *fxq)
{
    gveh_i32 i;
    gveh_buoy *buoy;
    gveh_vec3 wp;
    gveh_fx wh;
    gveh_fx depth;
    gveh_vec3 upf;
    gveh_vec3 drag;
    gveh_fx thrust;

    if (world == 0 || profile == 0 || body == 0 || input == 0) return;
    i = 0;
    while (i < profile->buoy_count) {
        buoy = &profile->buoys[i];
        wp = watermovement89_point_world(body, basis, buoy->local_pos);
        wh = 0;
        if (world->water_sample != 0) wh = world->water_sample(world->user, wp.x, wp.z, tick);
        depth = wh + buoy->radius - wp.y;
        if (depth > 0) {
            upf = gveh_v3(0, gveh_fx_mul(depth, buoy->buoyancy_k), 0);
            drag = gveh_v3_scale(body->vel, -buoy->water_drag);
            gveh_body_add_force_at(body, upf, wp);
            gveh_body_add_force_at(body, drag, wp);
            if (fxq != 0) gveh_fx_push(fxq, GVEH_FX_EVENT_SPLASH, (gveh_i16)gveh_fx_to_int(depth * 16), wp.x, wp.y, wp.z);
        }
        i++;
    }
    if (profile->buoy_count > 0) {
        thrust = gveh_fx_mul(profile->drive.drive_torque + gveh_fx_from_int(300), input->throttle);
        gveh_body_add_force(body, gveh_v3_scale(basis.fwd, thrust));
        gveh_body_add_yaw_torque(body, gveh_fx_mul(input->steer, profile->yaw_power * 120));
    }
    (void)dt;
}
