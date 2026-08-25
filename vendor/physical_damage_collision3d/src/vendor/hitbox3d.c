#include "hitbox3d.h"

static int hit3_aabb_of_shape(const hb3_shape *s, hb3_aabb *out_aabb)
{
    return hb3_shape_get_aabb(s, out_aabb);
}

void hit3_attack_init(hit3_attack *a, int owner_id)
{
    int i;
    if (a == 0) {
        return;
    }
    a->active = 1;
    a->owner_id = owner_id;
    a->group_mask = 1;
    a->hit_mask = HB3_ALL_MASK;
    a->hitbox_count = 0;
    for (i = 0; i < HIT3_MAX_HITBOXES_PER_ATTACK; ++i) {
        a->hitboxes[i].active = 0;
        a->hitboxes[i].has_prev = 0;
        a->hitboxes[i].user_tag = 0;
    }
}

void hit3_attack_clear(hit3_attack *a)
{
    int i;
    if (a == 0) {
        return;
    }
    for (i = 0; i < HIT3_MAX_HITBOXES_PER_ATTACK; ++i) {
        a->hitboxes[i].active = 0;
        a->hitboxes[i].has_prev = 0;
        a->hitboxes[i].user_tag = 0;
    }
    a->hitbox_count = 0;
}

void hit3_attack_set_masks(hit3_attack *a, int group_mask, int hit_mask)
{
    if (a == 0) {
        return;
    }
    a->group_mask = group_mask;
    a->hit_mask = hit_mask;
}

static int hit3_prepare_hitbox(hit3_attack *a, int slot, hit3_box **out_hb)
{
    hit3_box *hb;
    if (a == 0 || out_hb == 0) {
        return HIT3_ERR_BAD_ARG;
    }
    if (slot < 0 || slot >= HIT3_MAX_HITBOXES_PER_ATTACK) {
        return HIT3_ERR_BAD_ARG;
    }
    hb = &a->hitboxes[slot];
    if (!hb->active) {
        hb->active = 1;
        hb->has_prev = 0;
        a->hitbox_count += 1;
    } else {
        hb->prev = hb->curr;
        hb->has_prev = 1;
    }
    *out_hb = hb;
    return HIT3_OK;
}

int hit3_attack_set_sphere(hit3_attack *a, int slot, int hit_id,
                           hb3_v3 center, hb3_fx radius,
                           int hit_flags, int user_tag)
{
    int r;
    hit3_box *hb;
    r = hit3_prepare_hitbox(a, slot, &hb);
    if (r != HIT3_OK) {
        return r;
    }
    hb->curr = hb3_shape_make_sphere(hit_id, center, radius, hit_flags,
                                     a->group_mask, a->hit_mask);
    hb->user_tag = user_tag;
    return HIT3_OK;
}

int hit3_attack_set_capsule(hit3_attack *a, int slot, int hit_id,
                            hb3_v3 p0, hb3_v3 p1, hb3_fx radius,
                            int hit_flags, int user_tag)
{
    int r;
    hit3_box *hb;
    r = hit3_prepare_hitbox(a, slot, &hb);
    if (r != HIT3_OK) {
        return r;
    }
    hb->curr = hb3_shape_make_capsule(hit_id, p0, p1, radius, hit_flags,
                                      a->group_mask, a->hit_mask);
    hb->user_tag = user_tag;
    return HIT3_OK;
}

int hit3_attack_set_obb(hit3_attack *a, int slot, int hit_id,
                        hb3_v3 center, hb3_v3 half_extents,
                        hb3_mat3 basis, int hit_flags, int user_tag)
{
    int r;
    hit3_box *hb;
    r = hit3_prepare_hitbox(a, slot, &hb);
    if (r != HIT3_OK) {
        return r;
    }
    hb->curr = hb3_shape_make_obb(hit_id, center, half_extents, basis, hit_flags,
                                  a->group_mask, a->hit_mask);
    hb->user_tag = user_tag;
    return HIT3_OK;
}

int hit3_hitbox_get_query_aabb(const hit3_box *hb, hb3_aabb *out_aabb)
{
    hb3_aabb prev;
    if (hb == 0 || out_aabb == 0 || !hb->active) {
        return HIT3_ERR_BAD_ARG;
    }
    if (hit3_aabb_of_shape(&hb->curr, out_aabb) != HB3_OK) {
        return HIT3_ERR_BAD_ARG;
    }
    if (hb->has_prev && hit3_aabb_of_shape(&hb->prev, &prev) == HB3_OK) {
        hb3_aabb_merge(out_aabb, &prev);
    }
    return HIT3_OK;
}

static int hit3_swept_sphere_hits(const hit3_box *hb, const hb3_shape *hurt)
{
    hb3_shape swept;
    swept = hb3_shape_make_capsule(hb->curr.id, hb->prev.a, hb->curr.a,
                                   hb->curr.radius, hb->curr.flags,
                                   hb->curr.group_mask, hb->curr.hit_mask);
    return hb3_shape_intersects(&swept, hurt);
}

static int hit3_swept_capsule_hits(const hit3_box *hb, const hb3_shape *hurt)
{
    hb3_shape rail_a;
    hb3_shape rail_b;
    if (hb3_shape_intersects(&hb->prev, hurt)) {
        return 1;
    }
    rail_a = hb3_shape_make_capsule(hb->curr.id, hb->prev.a, hb->curr.a,
                                    hb->curr.radius, hb->curr.flags,
                                    hb->curr.group_mask, hb->curr.hit_mask);
    rail_b = hb3_shape_make_capsule(hb->curr.id, hb->prev.b, hb->curr.b,
                                    hb->curr.radius, hb->curr.flags,
                                    hb->curr.group_mask, hb->curr.hit_mask);
    if (hb3_shape_intersects(&rail_a, hurt)) {
        return 1;
    }
    if (hb3_shape_intersects(&rail_b, hurt)) {
        return 1;
    }
    return 0;
}

static int hit3_swept_obb_conservative_hits(const hit3_box *hb, const hb3_shape *hurt)
{
    hb3_aabb q;
    hb3_aabb h;
    if (hb3_shape_intersects(&hb->prev, hurt)) {
        return 1;
    }
    if (hit3_hitbox_get_query_aabb(hb, &q) != HIT3_OK) {
        return 0;
    }
    if (hb3_shape_get_aabb(hurt, &h) != HB3_OK) {
        return 0;
    }
    return hb3_aabb_intersects(&q, &h);
}

int hit3_hitbox_intersects_hurtbox(const hit3_box *hb, const hb3_hurtbox *hurt)
{
    if (hb == 0 || hurt == 0 || !hb->active || !hurt->active) {
        return 0;
    }
    if (hb3_shape_intersects(&hb->curr, &hurt->shape)) {
        return 1;
    }
    if (!hb->has_prev) {
        return 0;
    }
    if (hb->curr.type == HB3_SHAPE_SPHERE) {
        return hit3_swept_sphere_hits(hb, &hurt->shape);
    }
    if (hb->curr.type == HB3_SHAPE_CAPSULE) {
        return hit3_swept_capsule_hits(hb, &hurt->shape);
    }
    if (hb->curr.type == HB3_SHAPE_OBB) {
        return hit3_swept_obb_conservative_hits(hb, &hurt->shape);
    }
    return 0;
}

int hit3_attack_debug_draw(const hit3_attack *a, hb3_debug_line_fn line_fn,
                           void *user)
{
    int i;
    int count;
    if (a == 0 || line_fn == 0) {
        return 0;
    }
    count = 0;
    for (i = 0; i < HIT3_MAX_HITBOXES_PER_ATTACK; ++i) {
        if (a->hitboxes[i].active) {
            hb3_debug_draw_shape(&a->hitboxes[i].curr, line_fn, user,
                                 HIT3_DEBUG_COLOR_HIT, a->owner_id);
            count += 1;
        }
    }
    return count;
}
