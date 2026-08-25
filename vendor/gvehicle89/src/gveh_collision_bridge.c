#include "gveh_collision_bridge.h"

static gveh_fx gveh_collision_min_radius(void)
{
    return gveh_fx_from_int(1);
}

gveh_fx gveh_collision_vehicle_radius(const gveh_vehicle *v)
{
    gveh_fx r;
    r = v->profile.collision_radius;
    if (r < gveh_collision_min_radius()) r = gveh_collision_min_radius();
    return r;
}

gveh_i32 gveh_collision_resolve_pair(gveh_vehicle *a, gveh_vehicle *b, gveh_i16 id_a, gveh_i16 id_b, gveh_collision_contact *out_contact)
{
    gveh_vec3 delta;
    gveh_vec3 n;
    gveh_vec3 relv;
    gveh_fx radius_sum;
    gveh_fx dist;
    gveh_fx pen;
    gveh_fx rel;
    gveh_fx push_a;
    gveh_fx push_b;
    gveh_fx impulse;
    gveh_fx inv_sum;

    if (out_contact != 0) {
        out_contact->hit = 0;
        out_contact->id_a = id_a;
        out_contact->id_b = id_b;
        out_contact->normal = gveh_v3(0, 0, 0);
        out_contact->penetration = 0;
        out_contact->relative_speed = 0;
    }

    delta = gveh_v3_sub(b->body.pos, a->body.pos);
    if (gveh_fx_abs(delta.y) > gveh_fx_from_int(8)) return 0;
    radius_sum = gveh_collision_vehicle_radius(a) + gveh_collision_vehicle_radius(b);
    dist = gveh_v3_len_approx(delta);
    if (dist <= GVEH_FX_EPS) {
        delta = gveh_v3(GVEH_FX_ONE, 0, 0);
        dist = GVEH_FX_ONE;
    }
    if (dist >= radius_sum) return 0;

    n = gveh_v3_norm_approx(delta);
    pen = radius_sum - dist;
    inv_sum = a->body.inv_mass + b->body.inv_mass;
    if (inv_sum <= 0) {
        push_a = pen / 2;
        push_b = pen - push_a;
    } else {
        push_a = gveh_fx_mul(pen, gveh_fx_div(a->body.inv_mass, inv_sum));
        push_b = gveh_fx_mul(pen, gveh_fx_div(b->body.inv_mass, inv_sum));
    }

    a->body.pos = gveh_v3_sub(a->body.pos, gveh_v3_scale(n, push_a));
    b->body.pos = gveh_v3_add(b->body.pos, gveh_v3_scale(n, push_b));

    relv = gveh_v3_sub(b->body.vel, a->body.vel);
    rel = gveh_v3_dot(relv, n);
    if (rel < 0) {
        impulse = -rel / 2;
        a->body.vel = gveh_v3_sub(a->body.vel, gveh_v3_scale(n, impulse));
        b->body.vel = gveh_v3_add(b->body.vel, gveh_v3_scale(n, impulse));
        gveh_fx_push(&a->fxq, GVEH_FX_EVENT_IMPACT, (gveh_i16)gveh_fx_to_int(impulse), a->body.pos.x, a->body.pos.y, a->body.pos.z);
        gveh_fx_push(&b->fxq, GVEH_FX_EVENT_IMPACT, (gveh_i16)gveh_fx_to_int(impulse), b->body.pos.x, b->body.pos.y, b->body.pos.z);
    }

    if (out_contact != 0) {
        out_contact->hit = 1;
        out_contact->normal = n;
        out_contact->penetration = pen;
        out_contact->relative_speed = rel;
    }
    return 1;
}
