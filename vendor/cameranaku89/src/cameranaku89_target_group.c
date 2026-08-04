/* cameranaku89_target_group.c - CC0 1.0 Universal. */
#include "cameranaku89_target_group.h"

static cnk_fx cnk_tg_min(cnk_fx a, cnk_fx b)
{
    return a < b ? a : b;
}

static cnk_fx cnk_tg_max(cnk_fx a, cnk_fx b)
{
    return a > b ? a : b;
}

static cnk_fx cnk_tg_len_approx(cnk_vec3 v)
{
    cnk_fx ax;
    cnk_fx ay;
    cnk_fx az;
    cnk_fx hi;
    ax = cnk_fx_abs(v.x);
    ay = cnk_fx_abs(v.y);
    az = cnk_fx_abs(v.z);
    hi = ax;
    if (ay > hi) { hi = ay; }
    if (az > hi) { hi = az; }
    return hi + ((ax + ay + az - hi) >> 2);
}

CNK_API void cnk_target_group_clear(cnk_target_group *group)
{
    int i;
    if (group == 0) {
        return;
    }
    group->count = 0;
    group->center = cnk_vec3_make(0, 0, 0);
    group->minv = cnk_vec3_make(0, 0, 0);
    group->maxv = cnk_vec3_make(0, 0, 0);
    group->radius = 0;
    group->valid = 0;
    for (i = 0; i < CNK_MAX_TARGET_GROUP_ITEMS; ++i) {
        group->items[i].active = 0;
        group->items[i].id = -1;
        group->items[i].weight = 0;
        group->items[i].radius = 0;
        group->items[i].target.valid = 0;
    }
}

CNK_API int cnk_target_group_add(cnk_target_group *group, int id, cnk_target target, cnk_fx weight, cnk_fx radius)
{
    int slot;
    if (group == 0) {
        return -1;
    }
    if (group->count >= CNK_MAX_TARGET_GROUP_ITEMS) {
        return -1;
    }
    slot = group->count;
    group->items[slot].id = id;
    group->items[slot].target = target;
    group->items[slot].weight = weight > 0 ? weight : CNK_ONE;
    group->items[slot].radius = radius >= 0 ? radius : 0;
    group->items[slot].active = 1;
    group->count += 1;
    return slot;
}

CNK_API int cnk_target_group_set(cnk_target_group *group, int slot, cnk_target target, cnk_fx weight, cnk_fx radius)
{
    if (group == 0 || slot < 0 || slot >= CNK_MAX_TARGET_GROUP_ITEMS) {
        return CNK_FALSE;
    }
    if (slot >= group->count) {
        group->count = slot + 1;
    }
    group->items[slot].target = target;
    group->items[slot].weight = weight > 0 ? weight : CNK_ONE;
    group->items[slot].radius = radius >= 0 ? radius : 0;
    group->items[slot].active = 1;
    return CNK_TRUE;
}

CNK_API int cnk_target_group_compute(cnk_target_group *group)
{
    int i;
    int used;
    cnk_vec3 acc;
    cnk_fx total;
    cnk_vec3 minv;
    cnk_vec3 maxv;
    cnk_fx r;
    if (group == 0) {
        return CNK_FALSE;
    }
    acc = cnk_vec3_make(0, 0, 0);
    total = 0;
    used = 0;
    for (i = 0; i < group->count; ++i) {
        if (group->items[i].active != 0 && group->items[i].target.valid != 0) {
            acc = cnk_vec3_add(acc, cnk_vec3_scale(group->items[i].target.pos, group->items[i].weight));
            total += group->items[i].weight;
            used += 1;
        }
    }
    if (used <= 0 || total <= 0) {
        group->valid = 0;
        return CNK_FALSE;
    }
    group->center = cnk_vec3_scale(acc, cnk_fx_div(CNK_ONE, total));
    minv = group->center;
    maxv = group->center;
    r = 0;
    for (i = 0; i < group->count; ++i) {
        if (group->items[i].active != 0 && group->items[i].target.valid != 0) {
            cnk_vec3 p;
            cnk_fx rad;
            cnk_fx dist;
            p = group->items[i].target.pos;
            rad = group->items[i].radius;
            minv.x = cnk_tg_min(minv.x, p.x - rad);
            minv.y = cnk_tg_min(minv.y, p.y - rad);
            minv.z = cnk_tg_min(minv.z, p.z - rad);
            maxv.x = cnk_tg_max(maxv.x, p.x + rad);
            maxv.y = cnk_tg_max(maxv.y, p.y + rad);
            maxv.z = cnk_tg_max(maxv.z, p.z + rad);
            dist = cnk_tg_len_approx(cnk_vec3_sub(p, group->center)) + rad;
            if (dist > r) {
                r = dist;
            }
        }
    }
    group->minv = minv;
    group->maxv = maxv;
    group->radius = r;
    group->valid = 1;
    return CNK_TRUE;
}

CNK_API int cnk_target_group_as_target(cnk_target_group *group, cnk_target *out_target)
{
    int i;
    cnk_vec3 vel;
    cnk_fx total;
    if (group == 0 || out_target == 0) {
        return CNK_FALSE;
    }
    if (cnk_target_group_compute(group) == CNK_FALSE) {
        return CNK_FALSE;
    }
    vel = cnk_vec3_make(0, 0, 0);
    total = 0;
    for (i = 0; i < group->count; ++i) {
        if (group->items[i].active != 0 && group->items[i].target.valid != 0) {
            vel = cnk_vec3_add(vel, cnk_vec3_scale(group->items[i].target.velocity, group->items[i].weight));
            total += group->items[i].weight;
        }
    }
    if (total > 0) {
        vel = cnk_vec3_scale(vel, cnk_fx_div(CNK_ONE, total));
    }
    out_target->pos = group->center;
    out_target->velocity = vel;
    out_target->yaw_deg = 0;
    out_target->pitch_deg = 0;
    out_target->roll_deg = 0;
    out_target->valid = 1;
    return CNK_TRUE;
}
