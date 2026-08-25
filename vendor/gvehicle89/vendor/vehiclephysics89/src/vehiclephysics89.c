#include "vehiclephysics89.h"

static gveh_vec3 vehiclephysics89_point_world(const gveh_body *body, gveh_basis basis, gveh_vec3 local)
{
    return gveh_v3_add(body->pos, gveh_basis_local_to_world(basis, local));
}

void vehiclephysics89_apply_gravity(gveh_body *body, gveh_fx mass, gveh_fx gravity)
{
    gveh_vec3 force;
    if (body == 0) return;
    force = gveh_v3(0, -gveh_fx_mul(mass, gravity), 0);
    gveh_body_add_force(body, force);
}

void vehiclephysics89_apply_masspoints(gveh_body *body,
                                       const gveh_masspoint *masspoints,
                                       gveh_i32 masspoint_count,
                                       gveh_world_i *world,
                                       gveh_basis basis,
                                       gveh_fx gravity,
                                       gveh_fx dt)
{
    gveh_i32 i;
    gveh_vec3 wp;
    gveh_ground_hit hit;
    gveh_fx depth;
    gveh_vec3 force;
    gveh_fx mag;
    const gveh_masspoint *mp;

    if (body == 0 || masspoints == 0 || world == 0 || world->sphere_probe == 0) return;
    i = 0;
    while (i < masspoint_count) {
        mp = &masspoints[i];
        wp = vehiclephysics89_point_world(body, basis, mp->local_pos);
        if (world->sphere_probe(world->user, wp, mp->radius, &hit)) {
            depth = hit.point.y + mp->radius - wp.y;
            if (depth > 0) {
                mag = gveh_fx_mul(depth, gravity * 2);
                mag = gveh_fx_mul(mag, mp->mass_share);
                force = gveh_v3_scale(hit.normal, mag);
                gveh_body_add_force_at(body, force, wp);
                body->vel.y += gveh_fx_mul(depth, dt) * 4;
                if (mp->bounce > 0 && body->vel.y < 0) {
                    body->vel.y = -gveh_fx_mul(body->vel.y, mp->bounce);
                }
            }
        }
        i++;
    }
}

void vehiclephysics89_apply_linear_drag(gveh_body *body, gveh_fx speed_forward)
{
    gveh_fx drag;
    gveh_vec3 dragv;
    if (body == 0) return;
    drag = gveh_fx_abs(speed_forward) / 64;
    dragv = gveh_v3_scale(body->vel, -drag);
    gveh_body_add_force(body, dragv);
}

void vehiclephysics89_integrate_with_retention(gveh_body *body, gveh_fx dt,
                                                 gveh_fx velocity_retention)
{
    if (body == 0) return;
    gveh_body_step_damped(body, dt, velocity_retention);
}

void vehiclephysics89_integrate(gveh_body *body, gveh_fx dt)
{
    if (body == 0) return;
    gveh_body_step(body, dt);
}
