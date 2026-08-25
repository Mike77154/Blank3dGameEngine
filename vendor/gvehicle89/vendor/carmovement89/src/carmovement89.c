#include "carmovement89.h"

static gveh_fx carmovement89_nonzero_abs(gveh_fx v, gveh_fx minv)
{
    v = gveh_fx_abs(v);
    if (v < minv) v = minv;
    return v;
}

static gveh_vec3 carmovement89_point_world(const gveh_body *body, gveh_basis basis, gveh_vec3 local)
{
    return gveh_v3_add(body->pos, gveh_basis_local_to_world(basis, local));
}

void carmovement89_step(gveh_surface *surfaces,
                        gveh_world_i *world,
                        gveh_profile *profile,
                        gveh_body *body,
                        gveh_basis basis,
                        const gveh_input *input,
                        gveh_fx dt,
                        gveh_i32 *grounded_count_out,
                        gveh_fx *grounded_ratio_out,
                        gveh_fx_queue *fxq)
{
    gveh_i32 i;
    gveh_i32 grounded;
    gveh_fx avg_spin;
    gveh_wheel *w;
    gveh_vec3 wp;
    gveh_ground_hit hit;
    gveh_fx max_down;
    gveh_fx dist;
    gveh_fx comp;
    gveh_fx load;
    gveh_fx fwd_speed;
    gveh_fx side_speed;
    gveh_i32 steer_units;
    gveh_basis wb;
    gveh_vec3 wfwd;
    gveh_vec3 wright;
    const gveh_surface *surf;
    gveh_fx drive;
    gveh_fx brake;
    gveh_fx brake_sign;
    gveh_fx rolling;
    gveh_fx capx;
    gveh_fx capy;
    gveh_fx fx;
    gveh_fx fy;
    gveh_fx slipx;
    gveh_fx slipy;
    gveh_vec3 force;
    gveh_fx yaw_t;
    gveh_fx handslide;
    gveh_fx speed_abs;

    if (surfaces == 0 || world == 0 || profile == 0 || body == 0 || input == 0) return;
    grounded = 0;
    avg_spin = 0;
    i = 0;
    while (i < profile->wheel_count) {
        avg_spin += gveh_fx_abs(profile->wheels[i].spin_vel);
        i++;
    }
    if (profile->wheel_count > 0) avg_spin /= profile->wheel_count;
    gveh_drivetrain_update(&profile->drive, input->throttle, input->brake, avg_spin, dt);

    i = 0;
    while (i < profile->wheel_count) {
        w = &profile->wheels[i];
        wp = carmovement89_point_world(body, basis, w->local_pos);
        max_down = profile->susp.rest_len + w->radius + GVEH_FX_HALF;
        w->grounded = GVEH_FALSE;
        if (world->ground_probe != 0 && world->ground_probe(world->user, wp, max_down, &hit)) {
            dist = wp.y - hit.point.y;
            comp = profile->susp.rest_len + w->radius - dist;
            if (comp > 0) {
                w->grounded = GVEH_TRUE;
                grounded++;
                w->compression = gveh_fx_clamp(comp, 0, profile->susp.max_len);
                w->surface_id = hit.surface_id;
                surf = gveh_surface_get(surfaces, hit.surface_id);
                load = gveh_fx_mul(w->compression, profile->susp.spring_k);
                load += profile->mass / profile->wheel_count;
                w->normal_load = load;
                steer_units = gveh_fx_to_int(gveh_fx_mul(input->steer, w->steer_max) * 24);
                w->steer_angle = steer_units;
                wb = gveh_basis_yaw_pitch_roll(body->yaw + steer_units, 0, 0);
                wfwd = wb.fwd;
                wright = wb.right;
                fwd_speed = gveh_v3_dot(body->vel, wfwd);
                side_speed = gveh_v3_dot(body->vel, wright);
                speed_abs = carmovement89_nonzero_abs(fwd_speed, GVEH_FX_ONE);
                slipx = gveh_fx_div(gveh_fx_mul(w->spin_vel, w->radius) - fwd_speed, speed_abs);
                slipy = gveh_fx_div(side_speed, speed_abs + GVEH_FX_ONE);
                capx = gveh_fx_mul(load, surf->mu_long);
                capx = gveh_fx_mul(capx, GVEH_FX_ONE + (gveh_tire_lut(profile->tire.slip_x, profile->tire.force_x, slipx) / 4));
                capy = gveh_fx_mul(gveh_fx_mul(load, surf->mu_lat), gveh_tire_lut(profile->tire.slip_y, profile->tire.force_y, slipy));
                if (capy < load / 4) capy = load / 4;
                drive = gveh_fx_mul(profile->drive.drive_torque, w->drive_bias);
                brake_sign = 0;
                if (fwd_speed > GVEH_FX_HALF) brake_sign = -GVEH_FX_ONE;
                if (fwd_speed < -GVEH_FX_HALF) brake_sign = GVEH_FX_ONE;
                brake = gveh_fx_mul(profile->drive.brake_torque, w->brake_bias);
                rolling = gveh_fx_mul(fwd_speed, surf->rolling);
                fx = drive + gveh_fx_mul(brake, brake_sign) - rolling;
                fx = gveh_fx_clamp(fx, -capx, capx);
                handslide = GVEH_FX_ONE + gveh_fx_mul(input->handbrake, profile->slide_power);
                fy = -gveh_fx_mul(side_speed, load / 3);
                fy = gveh_fx_div(fy, handslide);
                fy = gveh_fx_clamp(fy, -capy, capy);
                force = gveh_v3_add(gveh_v3_scale(wfwd, fx), gveh_v3_scale(wright, fy));
                force.y += gveh_fx_mul(load, profile->gravity);
                gveh_body_add_force_at(body, force, wp);
                yaw_t = gveh_fx_mul(input->steer, gveh_fx_mul(fwd_speed, profile->yaw_power));
                gveh_body_add_yaw_torque(body, yaw_t);
                w->spin_vel += gveh_fx_mul(drive - brake, dt) / (w->radius + 1);
                w->spin_vel = gveh_fx_clamp(w->spin_vel, -profile->max_speed, profile->max_speed);
                w->spin += gveh_fx_mul(w->spin_vel, dt);
                if (fxq != 0 && gveh_fx_abs(slipy) > GVEH_FX_HALF && surf->fx_skid) {
                    gveh_fx_push(fxq, GVEH_FX_EVENT_SKID, 1, wp.x, wp.y, wp.z);
                }
                if (fxq != 0 && surf->fx_dust && gveh_fx_abs(fwd_speed) > gveh_fx_from_int(5)) {
                    gveh_fx_push(fxq, GVEH_FX_EVENT_DUST, 1, wp.x, wp.y, wp.z);
                }
            }
        }
        i++;
    }
    if (grounded_count_out != 0) *grounded_count_out = grounded;
    if (grounded_ratio_out != 0) {
        if (profile->wheel_count > 0) *grounded_ratio_out = (GVEH_FX_ONE * grounded) / profile->wheel_count;
        else *grounded_ratio_out = 0;
    }
}
