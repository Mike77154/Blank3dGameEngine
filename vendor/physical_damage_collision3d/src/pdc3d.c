#include "pdc3d.h"
#include <string.h>

static w3d_fx pdc3d_to_w3d_fx(pdc3d_fx v)
{
#if W3D_FX_SHIFT >= HB3_FX_SHIFT
    return (w3d_fx)((long)v * (long)(1L << (W3D_FX_SHIFT - HB3_FX_SHIFT)));
#else
    return (w3d_fx)((long)v / (long)(1L << (HB3_FX_SHIFT - W3D_FX_SHIFT)));
#endif
}

static g3d_fx pdc3d_to_g3d_fx(pdc3d_fx v)
{
#if G3D_FX_SHIFT >= HB3_FX_SHIFT
    return (g3d_fx)((long)v * (long)(1L << (G3D_FX_SHIFT - HB3_FX_SHIFT)));
#else
    return (g3d_fx)((long)v / (long)(1L << (HB3_FX_SHIFT - G3D_FX_SHIFT)));
#endif
}

static t3d_fx pdc3d_to_t3d_fx(pdc3d_fx v)
{
#if T3D_FX_SHIFT >= HB3_FX_SHIFT
    return (t3d_fx)((long)v * (long)(1L << (T3D_FX_SHIFT - HB3_FX_SHIFT)));
#else
    return (t3d_fx)((long)v / (long)(1L << (HB3_FX_SHIFT - T3D_FX_SHIFT)));
#endif
}

static pdc3d_fx t3d_to_pdc3d_fx(t3d_fx v)
{
#if T3D_FX_SHIFT >= HB3_FX_SHIFT
    return (pdc3d_fx)((long)v / (long)(1L << (T3D_FX_SHIFT - HB3_FX_SHIFT)));
#else
    return (pdc3d_fx)((long)v * (long)(1L << (HB3_FX_SHIFT - T3D_FX_SHIFT)));
#endif
}

static w3d_v3 pdc3d_to_w3d_v3(pdc3d_v3 v)
{
    w3d_v3 o;
    o.x = pdc3d_to_w3d_fx(v.x);
    o.y = pdc3d_to_w3d_fx(v.y);
    o.z = pdc3d_to_w3d_fx(v.z);
    return o;
}

static g3d_v3 pdc3d_to_g3d_v3(pdc3d_v3 v)
{
    g3d_v3 o;
    o.x = pdc3d_to_g3d_fx(v.x);
    o.y = pdc3d_to_g3d_fx(v.y);
    o.z = pdc3d_to_g3d_fx(v.z);
    return o;
}

static t3d_v3 pdc3d_to_t3d_v3(pdc3d_v3 v)
{
    t3d_v3 o;
    o.x = pdc3d_to_t3d_fx(v.x);
    o.y = pdc3d_to_t3d_fx(v.y);
    o.z = pdc3d_to_t3d_fx(v.z);
    return o;
}

static pdc3d_v3 t3d_to_pdc3d_v3(t3d_v3 v)
{
    pdc3d_v3 o;
    o.x = t3d_to_pdc3d_fx(v.x);
    o.y = t3d_to_pdc3d_fx(v.y);
    o.z = t3d_to_pdc3d_fx(v.z);
    return o;
}

static int pdc3d_attack_flags_to_w3d(int flags)
{
    int out_flags;
    out_flags = 0;
    if ((flags & PDC3D_ATK_SLASH) != 0) {
        out_flags |= W3D_ATTACK_SLASH;
    }
    if ((flags & PDC3D_ATK_BLUNT) != 0) {
        out_flags |= W3D_ATTACK_BLUNT;
    }
    if ((flags & PDC3D_ATK_BITE) != 0) {
        out_flags |= W3D_ATTACK_BITE;
    }
    if ((flags & PDC3D_ATK_PIERCE) != 0) {
        out_flags |= W3D_ATTACK_PIERCE;
    }
    if ((flags & PDC3D_ATK_FIRE) != 0) {
        out_flags |= W3D_ATTACK_FIRE;
    }
    if ((flags & PDC3D_ATK_ACID) != 0) {
        out_flags |= W3D_ATTACK_ACID;
    }
    return out_flags;
}

static w3d_mat3 pdc3d_to_w3d_mat3(pdc3d_mat3 m)
{
    w3d_mat3 o;
    o.m00 = pdc3d_to_w3d_fx(m.x.x);
    o.m10 = pdc3d_to_w3d_fx(m.x.y);
    o.m20 = pdc3d_to_w3d_fx(m.x.z);
    o.m01 = pdc3d_to_w3d_fx(m.y.x);
    o.m11 = pdc3d_to_w3d_fx(m.y.y);
    o.m21 = pdc3d_to_w3d_fx(m.y.z);
    o.m02 = pdc3d_to_w3d_fx(m.z.x);
    o.m12 = pdc3d_to_w3d_fx(m.z.y);
    o.m22 = pdc3d_to_w3d_fx(m.z.z);
    return o;
}

static int pdc3d_push_event(pdc3d_world *w, const pdc3d_event *ev)
{
    if (w == 0 || ev == 0) {
        return PDC3D_ERR_BAD_ARG;
    }
    if (w->event_count >= PDC3D_MAX_EVENTS) {
        return PDC3D_ERR_FULL;
    }
    w->events[w->event_tail] = *ev;
    w->event_tail += 1;
    if (w->event_tail >= PDC3D_MAX_EVENTS) {
        w->event_tail = 0;
    }
    w->event_count += 1;
    return PDC3D_OK;
}

static pdc3d_fx pdc3d_abs_fx(pdc3d_fx v)
{
    return v < 0 ? -v : v;
}

static pdc3d_v3 pdc3d_v3_sub_local(pdc3d_v3 a, pdc3d_v3 b)
{
    return pdc3d_v3_make(a.x - b.x, a.y - b.y, a.z - b.z);
}

static pdc3d_v3 pdc3d_v3_normalize_manhattan(pdc3d_v3 v, pdc3d_v3 fallback)
{
    long denom;
    pdc3d_v3 out;
    denom = (long)pdc3d_abs_fx(v.x) + (long)pdc3d_abs_fx(v.y) +
            (long)pdc3d_abs_fx(v.z);
    if (denom <= 0L) {
        return fallback;
    }
    out.x = (pdc3d_fx)(((long)v.x * (long)PDC3D_FX_ONE) / denom);
    out.y = (pdc3d_fx)(((long)v.y * (long)PDC3D_FX_ONE) / denom);
    out.z = (pdc3d_fx)(((long)v.z * (long)PDC3D_FX_ONE) / denom);
    return out;
}

static int pdc3d_pose_find_index(const pdc3d_world *w, int actor_id)
{
    int i;
    if (w == 0) {
        return -1;
    }
    for (i = 0; i < PDC3D_MAX_ACTOR_POSES; ++i) {
        if (w->pose_used[i] != 0 && w->pose_actor_ids[i] == actor_id) {
            return i;
        }
    }
    return -1;
}

static int pdc3d_pose_alloc_index(pdc3d_world *w, int actor_id)
{
    int i;
    i = pdc3d_pose_find_index(w, actor_id);
    if (i >= 0) {
        return i;
    }
    if (w == 0) {
        return -1;
    }
    for (i = 0; i < PDC3D_MAX_ACTOR_POSES; ++i) {
        if (w->pose_used[i] == 0) {
            w->pose_used[i] = 1;
            w->pose_actor_ids[i] = actor_id;
            w->pose_positions[i] = pdc3d_v3_make(0, 0, 0);
            w->pose_facing[i] = pdc3d_v3_make(PDC3D_FX_ONE, 0, 0);
            w->pose_lane_masks[i] = 0xfffffffful;
            return i;
        }
    }
    return -1;
}

static void pdc3d_pose_remove(pdc3d_world *w, int actor_id)
{
    int i;
    i = pdc3d_pose_find_index(w, actor_id);
    if (i >= 0) {
        w->pose_used[i] = 0;
        w->pose_actor_ids[i] = 0;
        w->pose_positions[i] = pdc3d_v3_make(0, 0, 0);
        w->pose_facing[i] = pdc3d_v3_make(PDC3D_FX_ONE, 0, 0);
        w->pose_lane_masks[i] = 0u;
    }
}

static pdc3d_v3 pdc3d_source_dir_from_actors(const pdc3d_world *w,
                                             int attacker_id,
                                             int defender_id,
                                             pdc3d_v3 fallback)
{
    int ai;
    int di;
    pdc3d_v3 delta;
    ai = pdc3d_pose_find_index(w, attacker_id);
    di = pdc3d_pose_find_index(w, defender_id);
    if (ai < 0 || di < 0) {
        return fallback;
    }
    /* source_dir means normalized direction from defender toward threat. */
    delta = pdc3d_v3_sub_local(w->pose_positions[ai], w->pose_positions[di]);
    return pdc3d_v3_normalize_manhattan(delta, fallback);
}

static void pdc3d_clear_grab_actor_slot(pdc3d_world *w, int actor_id)
{
    int i;
    if (w == 0) {
        return;
    }
    for (i = 0; i < G3D_MAX_ACTORS; ++i) {
        if (w->grabs.actors[i].active != 0 && w->grabs.actors[i].id == actor_id) {
            memset(&w->grabs.actors[i], 0, sizeof(w->grabs.actors[i]));
        }
    }
    for (i = 0; i < G3D_MAX_GRABS_ACTIVE; ++i) {
        if (w->grabs.grabs[i].active != 0 &&
            (w->grabs.grabs[i].owner_actor_id == actor_id ||
             w->grabs.grabs[i].target_actor_id == actor_id)) {
            w->grabs.grabs[i].active = 0;
        }
    }
}

static void pdc3d_clear_weakspot_actor_slot(pdc3d_world *w, int actor_id)
{
    int i;
    if (w == 0) {
        return;
    }
    for (i = 0; i < W3D_MAX_ACTORS; ++i) {
        if (w->weakspots.actors[i].active != 0 && w->weakspots.actors[i].id == actor_id) {
            memset(&w->weakspots.actors[i], 0, sizeof(w->weakspots.actors[i]));
        }
    }
}

pdc3d_v3 pdc3d_v3_make(pdc3d_fx x, pdc3d_fx y, pdc3d_fx z)
{
    return hb3_v3_make(x, y, z);
}

pdc3d_v3 pdc3d_v3_from_ints(int x, int y, int z)
{
    return hb3_v3_from_ints(x, y, z);
}

pdc3d_mat3 pdc3d_mat3_identity(void)
{
    return hb3_mat3_identity();
}

void pdc3d_world_init(pdc3d_world *w)
{
    pdc3d_bridge b;
    if (w == 0) {
        return;
    }
    ml3_world_init(&w->melee);
    g3d_world_init(&w->grabs);
    t3d_world_init(&w->throws_world);
    w3d_world_init(&w->weakspots);
    pdc3d_damage_table_init(&w->damage_table);
    pdc3d_damage_table_add_defaults(&w->damage_table);
    pdc3d_framedata_world_init(&w->framedata_world);
    pdc3d_defense_world_init(&w->defense_world);
    pdc3d_bridge_init(&b);
    w->bridge = b;
    w->arena = 0;
    w->event_head = 0;
    w->event_tail = 0;
    w->event_count = 0;
    {
        int i;
        for (i = 0; i < PDC3D_MAX_ACTOR_POSES; ++i) {
            w->pose_actor_ids[i] = 0;
            w->pose_positions[i] = pdc3d_v3_make(0, 0, 0);
            w->pose_facing[i] = pdc3d_v3_make(PDC3D_FX_ONE, 0, 0);
            w->pose_lane_masks[i] = 0u;
            w->pose_used[i] = 0;
        }
    }
    w->tick = 0;
}

void pdc3d_world_init_with_arena(pdc3d_world *w, pdc3d_arena *arena)
{
    pdc3d_world_init(w);
    if (w != 0) {
        w->arena = arena;
    }
}

void pdc3d_world_set_bridge(pdc3d_world *w, const pdc3d_bridge *bridge)
{
    if (w == 0) {
        return;
    }
    if (bridge == 0) {
        pdc3d_bridge_init(&w->bridge);
    } else {
        w->bridge = *bridge;
    }
}

int pdc3d_actor_add(pdc3d_world *w, int actor_id, int team)
{
    int r;
    if (w == 0) {
        return PDC3D_ERR_BAD_ARG;
    }
    r = ml3_actor_add(&w->melee, actor_id, team);
    if (r != ML3_OK) {
        return r;
    }
    g3d_add_actor(&w->grabs, actor_id, team, g3d_v3_make(0, 0, 0));
    w3d_add_actor(&w->weakspots, actor_id, w3d_v3_make(0, 0, 0));
    pdc3d_pose_alloc_index(w, actor_id);
    return PDC3D_OK;
}

int pdc3d_actor_remove(pdc3d_world *w, int actor_id)
{
    if (w == 0) {
        return PDC3D_ERR_BAD_ARG;
    }
    pdc3d_defense_actor_remove(&w->defense_world, actor_id);
    pdc3d_framedata_actor_remove(&w->framedata_world, actor_id);
    pdc3d_clear_grab_actor_slot(w, actor_id);
    pdc3d_clear_weakspot_actor_slot(w, actor_id);
    pdc3d_pose_remove(w, actor_id);
    return ml3_actor_remove(&w->melee, actor_id);
}

int pdc3d_actor_set_masks(pdc3d_world *w, int actor_id, int group_mask, int hit_mask)
{
    if (w == 0) {
        return PDC3D_ERR_BAD_ARG;
    }
    return ml3_actor_set_masks(&w->melee, actor_id, group_mask, hit_mask);
}

int pdc3d_actor_set_pose(pdc3d_world *w, int actor_id,
                         pdc3d_v3 pos, pdc3d_v3 facing_dir,
                         unsigned int lane_mask)
{
    int i;
    if (w == 0) {
        return PDC3D_ERR_BAD_ARG;
    }
    i = pdc3d_pose_alloc_index(w, actor_id);
    if (i < 0) {
        return PDC3D_ERR_FULL;
    }
    w->pose_positions[i] = pos;
    w->pose_facing[i] = pdc3d_v3_normalize_manhattan(facing_dir,
                                                     pdc3d_v3_make(PDC3D_FX_ONE, 0, 0));
    w->pose_lane_masks[i] = lane_mask;
    (void)g3d_set_actor_pos(&w->grabs, actor_id, pdc3d_to_g3d_v3(pos));
    (void)w3d_set_actor_pos(&w->weakspots, actor_id, pdc3d_to_w3d_v3(pos));
    (void)pdc3d_framedata_actor_set_pose(&w->framedata_world, actor_id,
                                          pos, facing_dir.x >= 0 ? 1 : -1,
                                          lane_mask);
    (void)pdc3d_defense_set_facing(&w->defense_world, actor_id,
                                    w->pose_facing[i]);
    return PDC3D_OK;
}

int pdc3d_actor_clear_grabs(pdc3d_world *w, int actor_id)
{
    if (w == 0) {
        return PDC3D_ERR_BAD_ARG;
    }
    pdc3d_clear_grab_actor_slot(w, actor_id);
    return PDC3D_OK;
}

int pdc3d_actor_clear_weakspots(pdc3d_world *w, int actor_id)
{
    int i;
    if (w == 0) {
        return PDC3D_ERR_BAD_ARG;
    }
    for (i = 0; i < W3D_MAX_ACTORS; ++i) {
        if (w->weakspots.actors[i].active != 0 && w->weakspots.actors[i].id == actor_id) {
            int j;
            for (j = 0; j < W3D_MAX_WEAKSPOTS_PER_ACTOR; ++j) {
                w->weakspots.actors[i].weakspots[j].active = 0;
            }
            w->weakspots.actors[i].weakspot_count = 0;
            return PDC3D_OK;
        }
    }
    return PDC3D_ERR_NOT_FOUND;
}

int pdc3d_actor_clear_hurtboxes(pdc3d_world *w, int actor_id)
{
    int i;
    if (w == 0) {
        return PDC3D_ERR_BAD_ARG;
    }
    for (i = 0; i < G3D_MAX_ACTORS; ++i) {
        if (w->grabs.actors[i].active != 0 && w->grabs.actors[i].id == actor_id) {
            w->grabs.actors[i].hurtbox_count = 0;
        }
    }
    return ml3_actor_clear_hurtboxes(&w->melee, actor_id);
}

int pdc3d_actor_add_hurt_sphere(pdc3d_world *w, int actor_id, int hurt_id,
                                pdc3d_v3 center, pdc3d_fx radius,
                                int material_flags, int user_tag)
{
    if (w == 0) {
        return PDC3D_ERR_BAD_ARG;
    }
    g3d_actor_add_hurt_sphere(&w->grabs, actor_id, pdc3d_to_g3d_v3(center),
                              pdc3d_to_g3d_fx(radius), material_flags);
    return ml3_actor_add_hurt_sphere(&w->melee, actor_id, hurt_id,
                                     center, radius, material_flags, user_tag);
}

int pdc3d_actor_add_hurt_capsule(pdc3d_world *w, int actor_id, int hurt_id,
                                 pdc3d_v3 a, pdc3d_v3 b, pdc3d_fx radius,
                                 int material_flags, int user_tag)
{
    if (w == 0) {
        return PDC3D_ERR_BAD_ARG;
    }
    g3d_actor_add_hurt_capsule(&w->grabs, actor_id, pdc3d_to_g3d_v3(a),
                               pdc3d_to_g3d_v3(b), pdc3d_to_g3d_fx(radius),
                               material_flags);
    return ml3_actor_add_hurt_capsule(&w->melee, actor_id, hurt_id,
                                      a, b, radius, material_flags, user_tag);
}

int pdc3d_actor_add_hurt_obb(pdc3d_world *w, int actor_id, int hurt_id,
                             pdc3d_v3 center, pdc3d_v3 half_extents,
                             pdc3d_mat3 basis, int material_flags,
                             int user_tag)
{
    if (w == 0) {
        return PDC3D_ERR_BAD_ARG;
    }
    return ml3_actor_add_hurt_obb(&w->melee, actor_id, hurt_id,
                                  center, half_extents, basis,
                                  material_flags, user_tag);
}

int pdc3d_actor_add_weakspot_sphere(pdc3d_world *w, int actor_id,
                                    int weakspot_id, pdc3d_v3 center,
                                    pdc3d_fx radius, int mult_num,
                                    int mult_den, int durability,
                                    int flags)
{
    if (w == 0) {
        return PDC3D_ERR_BAD_ARG;
    }
    return w3d_add_weakspot_sphere(&w->weakspots, actor_id, weakspot_id,
                                   pdc3d_to_w3d_v3(center),
                                   pdc3d_to_w3d_fx(radius), mult_num,
                                   mult_den, durability, flags);
}

int pdc3d_actor_add_weakspot_capsule(pdc3d_world *w, int actor_id,
                                     int weakspot_id, pdc3d_v3 a,
                                     pdc3d_v3 b, pdc3d_fx radius,
                                     int mult_num, int mult_den,
                                     int durability, int flags)
{
    if (w == 0) {
        return PDC3D_ERR_BAD_ARG;
    }
    return w3d_add_weakspot_capsule(&w->weakspots, actor_id, weakspot_id,
                                    pdc3d_to_w3d_v3(a), pdc3d_to_w3d_v3(b),
                                    pdc3d_to_w3d_fx(radius), mult_num,
                                    mult_den, durability, flags);
}

int pdc3d_actor_add_weakspot_obb(pdc3d_world *w, int actor_id,
                                 int weakspot_id, pdc3d_v3 center,
                                 pdc3d_v3 half_extents, pdc3d_mat3 basis,
                                 int mult_num, int mult_den,
                                 int durability, int flags)
{
    if (w == 0) {
        return PDC3D_ERR_BAD_ARG;
    }
    return w3d_add_weakspot_obb(&w->weakspots, actor_id, weakspot_id,
                                pdc3d_to_w3d_v3(center),
                                pdc3d_to_w3d_v3(half_extents),
                                pdc3d_to_w3d_mat3(basis), mult_num,
                                mult_den, durability, flags);
}

int pdc3d_actor_set_weakspot_filter(pdc3d_world *w, int actor_id,
                                    int weakspot_id, int required_attack_flags,
                                    int blocked_attack_flags)
{
    if (w == 0) {
        return PDC3D_ERR_BAD_ARG;
    }
    return w3d_set_weakspot_attack_filter(&w->weakspots, actor_id,
                                          weakspot_id,
                                          pdc3d_attack_flags_to_w3d(required_attack_flags),
                                          pdc3d_attack_flags_to_w3d(blocked_attack_flags));
}

int pdc3d_attack_begin(pdc3d_world *w, int owner_id,
                       const pdc3d_attack_profile *profile,
                       int *out_handle)
{
    int r;
    if (w == 0 || profile == 0 || out_handle == 0) {
        return PDC3D_ERR_BAD_ARG;
    }
    r = ml3_attack_begin(&w->melee, owner_id, profile->attack_id,
                         profile->startup_frames, profile->active_frames,
                         profile->recovery_frames, profile->damage,
                         profile->stun_frames, profile->hitstop_frames,
                         profile->flags, out_handle);
    if (r == ML3_OK) {
        ml3_attack_set_masks(&w->melee, *out_handle,
                             profile->group_mask, profile->hit_mask);
    }
    return r;
}

int pdc3d_attack_end(pdc3d_world *w, int handle)
{
    if (w == 0) {
        return PDC3D_ERR_BAD_ARG;
    }
    return ml3_attack_end(&w->melee, handle);
}

int pdc3d_attack_clear_hitboxes(pdc3d_world *w, int handle)
{
    if (w == 0) {
        return PDC3D_ERR_BAD_ARG;
    }
    return ml3_attack_clear_hitboxes(&w->melee, handle);
}

int pdc3d_attack_set_hit_sphere(pdc3d_world *w, int handle, int slot,
                                int hit_id, pdc3d_v3 center,
                                pdc3d_fx radius, int attack_flags,
                                int user_tag)
{
    if (w == 0) {
        return PDC3D_ERR_BAD_ARG;
    }
    return ml3_attack_set_hit_sphere(&w->melee, handle, slot, hit_id,
                                     center, radius, attack_flags, user_tag);
}

int pdc3d_attack_set_hit_capsule(pdc3d_world *w, int handle, int slot,
                                 int hit_id, pdc3d_v3 p0, pdc3d_v3 p1,
                                 pdc3d_fx radius, int attack_flags,
                                 int user_tag)
{
    if (w == 0) {
        return PDC3D_ERR_BAD_ARG;
    }
    return ml3_attack_set_hit_capsule(&w->melee, handle, slot, hit_id,
                                      p0, p1, radius, attack_flags, user_tag);
}

int pdc3d_attack_set_hit_obb(pdc3d_world *w, int handle, int slot,
                             int hit_id, pdc3d_v3 center,
                             pdc3d_v3 half_extents, pdc3d_mat3 basis,
                             int attack_flags, int user_tag)
{
    if (w == 0) {
        return PDC3D_ERR_BAD_ARG;
    }
    return ml3_attack_set_hit_obb(&w->melee, handle, slot, hit_id,
                                  center, half_extents, basis,
                                  attack_flags, user_tag);
}

int pdc3d_attack_set_hit_capsule_from_sockets(pdc3d_world *w, int handle,
                                             int slot, int hit_id,
                                             int owner_actor_id,
                                             int socket_a, int socket_b,
                                             pdc3d_fx radius,
                                             int attack_flags,
                                             int user_tag)
{
    hb3_v3 a;
    hb3_v3 b;
    if (w == 0 || w->bridge.socket_pose == 0) {
        return PDC3D_ERR_BAD_ARG;
    }
    a = w->bridge.socket_pose(w->bridge.user, owner_actor_id, socket_a);
    b = w->bridge.socket_pose(w->bridge.user, owner_actor_id, socket_b);
    return pdc3d_attack_set_hit_capsule(w, handle, slot, hit_id,
                                        a, b, radius,
                                        attack_flags, user_tag);
}

int pdc3d_grab_start(pdc3d_world *w, int owner_actor_id,
                     const g3d_grab_def *def,
                     const g3d_shape *shapes, int shape_count)
{
    if (w == 0) {
        return PDC3D_ERR_BAD_ARG;
    }
    return g3d_start_grab(&w->grabs, owner_actor_id, def, shapes, shape_count);
}

int pdc3d_throw_add_body(pdc3d_world *w, int body_id,
                         pdc3d_v3 pos, pdc3d_fx radius)
{
    if (w == 0) {
        return PDC3D_ERR_BAD_ARG;
    }
    return t3d_add_body(&w->throws_world, body_id,
                        pdc3d_to_t3d_v3(pos), pdc3d_to_t3d_fx(radius));
}

int pdc3d_throw_start_arc(pdc3d_world *w, int attacker_id, int body_id,
                          const t3d_throw_def *def,
                          pdc3d_v3 start_pos, pdc3d_v3 target_pos,
                          int travel_ticks)
{
    t3d_v3 vel;
    int r;
    if (w == 0 || def == 0) {
        return PDC3D_ERR_BAD_ARG;
    }
    r = t3d_throw_make_arc_velocity(pdc3d_to_t3d_v3(start_pos),
                                    pdc3d_to_t3d_v3(target_pos),
                                    travel_ticks, def->gravity, &vel);
    if (r != T3D_OK) {
        return r;
    }
    return t3d_start_throw(&w->throws_world, attacker_id, body_id, def,
                           pdc3d_to_t3d_v3(start_pos), vel);
}

int pdc3d_fd_actor_enable(pdc3d_world *w, int actor_id,
                                  int team, int hp)
{
    if (w == 0) {
        return PDC3D_ERR_BAD_ARG;
    }
    return pdc3d_framedata_actor_create(&w->framedata_world, actor_id,
                                         team, hp) != 0
               ? PDC3D_OK : PDC3D_ERR_FULL;
}

int pdc3d_fd_actor_disable(pdc3d_world *w, int actor_id)
{
    if (w == 0) {
        return PDC3D_ERR_BAD_ARG;
    }
    return pdc3d_framedata_actor_remove(&w->framedata_world, actor_id) != 0
               ? PDC3D_OK : PDC3D_ERR_NOT_FOUND;
}

int pdc3d_fd_actor_set_action(pdc3d_world *w, int actor_id,
                                      const pdc3d_fd_action *action)
{
    if (w == 0 || action == 0) {
        return PDC3D_ERR_BAD_ARG;
    }
    return pdc3d_framedata_actor_set_action(&w->framedata_world, actor_id,
                                             action) != 0
               ? PDC3D_OK : PDC3D_ERR_NOT_FOUND;
}

int pdc3d_fd_actor_set_pose(pdc3d_world *w, int actor_id,
                                    pdc3d_v3 pos, int facing,
                                    unsigned int lane_mask)
{
    if (w == 0) {
        return PDC3D_ERR_BAD_ARG;
    }
    return pdc3d_framedata_actor_set_pose(&w->framedata_world, actor_id,
                                           pos, facing, lane_mask) != 0
               ? PDC3D_OK : PDC3D_ERR_NOT_FOUND;
}

int pdc3d_fd_actor_set_nothitby(pdc3d_world *w, int actor_id,
                                        unsigned int attr_mask, int frames)
{
    if (w == 0) {
        return PDC3D_ERR_BAD_ARG;
    }
    return pdc3d_framedata_actor_set_nothitby(&w->framedata_world,
                                               actor_id, attr_mask, frames) != 0
               ? PDC3D_OK : PDC3D_ERR_NOT_FOUND;
}

int pdc3d_fd_actor_set_hitby(pdc3d_world *w, int actor_id,
                                     unsigned int attr_mask, int frames)
{
    if (w == 0) {
        return PDC3D_ERR_BAD_ARG;
    }
    return pdc3d_framedata_actor_set_hitby(&w->framedata_world,
                                            actor_id, attr_mask, frames) != 0
               ? PDC3D_OK : PDC3D_ERR_NOT_FOUND;
}

int pdc3d_fd_actor_frame_info(const pdc3d_world *w, int actor_id,
                                      unsigned int *out_action_id,
                                      unsigned short *out_frame_index,
                                      unsigned short *out_frame_tick,
                                      unsigned short *out_total_tick,
                                      unsigned long *out_frame_flags)
{
    if (w == 0) {
        return PDC3D_ERR_BAD_ARG;
    }
    return pdc3d_framedata_actor_frame_info(&w->framedata_world, actor_id,
                                             out_action_id, out_frame_index,
                                             out_frame_tick, out_total_tick,
                                             out_frame_flags) != 0
               ? PDC3D_OK : PDC3D_ERR_NOT_FOUND;
}

int pdc3d_fd_actor_is_done(const pdc3d_world *w, int actor_id)
{
    if (w == 0) {
        return 1;
    }
    return pdc3d_framedata_actor_is_done(&w->framedata_world, actor_id);
}

int pdc3d_fd_check_actor_damage(const pdc3d_world *w,
                                        int defender_id,
                                        int attack_flags,
                                        pdc3d_framedata_result *out_result)
{
    if (w == 0) {
        return PDC3D_ERR_BAD_ARG;
    }
    return pdc3d_framedata_check_damage(&w->framedata_world, defender_id,
                                         attack_flags, out_result) != 0
               ? PDC3D_OK : PDC3D_ERR_NOT_FOUND;
}

int pdc3d_fd_resolve_actor_damage(pdc3d_world *w,
                                          int attacker_id,
                                          int defender_id,
                                          int was_blocked,
                                          pdc3d_damage_packet *io_packet,
                                          pdc3d_framedata_result *out_result)
{
    if (w == 0 || io_packet == 0) {
        return PDC3D_ERR_BAD_ARG;
    }
    return pdc3d_framedata_resolve_damage(&w->framedata_world, attacker_id,
                                           defender_id, was_blocked,
                                           io_packet, out_result) != 0
               ? PDC3D_OK : PDC3D_ERR_NOT_FOUND;
}

int pdc3d_fd_actor_advantage(const pdc3d_world *w,
                                     int attacker_id,
                                     int use_blockstun)
{
    if (w == 0) {
        return 0;
    }
    return pdc3d_framedata_frame_advantage(&w->framedata_world,
                                            attacker_id, use_blockstun);
}

int pdc3d_defense_actor_enable(pdc3d_world *w, int actor_id,
                               unsigned int enabled_flags,
                               const pdc3d_defense_profile *profile)
{
    if (w == 0) {
        return PDC3D_ERR_BAD_ARG;
    }
    return pdc3d_defense_actor_create(&w->defense_world, actor_id,
                                      enabled_flags, profile) != 0
               ? PDC3D_OK : PDC3D_ERR_FULL;
}

int pdc3d_defense_actor_disable(pdc3d_world *w, int actor_id)
{
    if (w == 0) {
        return PDC3D_ERR_BAD_ARG;
    }
    return pdc3d_defense_actor_remove(&w->defense_world, actor_id) != 0
               ? PDC3D_OK : PDC3D_ERR_NOT_FOUND;
}

int pdc3d_defense_actor_set_flags(pdc3d_world *w, int actor_id,
                                  unsigned int enabled_flags)
{
    if (w == 0) {
        return PDC3D_ERR_BAD_ARG;
    }
    return pdc3d_defense_actor_set_enabled(&w->defense_world, actor_id,
                                           enabled_flags) != 0
               ? PDC3D_OK : PDC3D_ERR_NOT_FOUND;
}

int pdc3d_defense_actor_set_facing(pdc3d_world *w, int actor_id,
                                   pdc3d_v3 facing_dir)
{
    if (w == 0) {
        return PDC3D_ERR_BAD_ARG;
    }
    return pdc3d_defense_set_facing(&w->defense_world, actor_id,
                                    facing_dir) != 0
               ? PDC3D_OK : PDC3D_ERR_NOT_FOUND;
}

int pdc3d_defense_actor_set_guard_input(pdc3d_world *w, int actor_id,
                                        int hold_input)
{
    if (w == 0) {
        return PDC3D_ERR_BAD_ARG;
    }
    return pdc3d_defense_set_guard_input(&w->defense_world, actor_id,
                                         hold_input) != 0
               ? PDC3D_OK : PDC3D_ERR_NOT_FOUND;
}

int pdc3d_defense_actor_press_parry(pdc3d_world *w, int actor_id)
{
    if (w == 0) {
        return PDC3D_ERR_BAD_ARG;
    }
    return pdc3d_defense_press_parry(&w->defense_world, actor_id) != 0
               ? PDC3D_OK : PDC3D_ERR_NOT_FOUND;
}

int pdc3d_defense_actor_set_block_active(pdc3d_world *w, int actor_id,
                                         int active)
{
    if (w == 0) {
        return PDC3D_ERR_BAD_ARG;
    }
    return pdc3d_defense_set_block_active(&w->defense_world, actor_id,
                                          active) != 0
               ? PDC3D_OK : PDC3D_ERR_NOT_FOUND;
}

int pdc3d_defense_actor_set_block_resources(pdc3d_world *w, int actor_id,
                                            int stamina, int posture)
{
    if (w == 0) {
        return PDC3D_ERR_BAD_ARG;
    }
    return pdc3d_defense_set_block_resources(&w->defense_world, actor_id,
                                             stamina, posture) != 0
               ? PDC3D_OK : PDC3D_ERR_NOT_FOUND;
}

int pdc3d_defense_actor_set_guard_value(pdc3d_world *w, int actor_id,
                                        int guard_value)
{
    if (w == 0) {
        return PDC3D_ERR_BAD_ARG;
    }
    return pdc3d_defense_set_guard_value(&w->defense_world, actor_id,
                                         guard_value) != 0
               ? PDC3D_OK : PDC3D_ERR_NOT_FOUND;
}

int pdc3d_defense_actor_set_shell_layer(pdc3d_world *w, int actor_id,
                                        int layer_index,
                                        const GSHL_LayerProfile *profile,
                                        int start_full)
{
    if (w == 0) {
        return PDC3D_ERR_BAD_ARG;
    }
    return pdc3d_defense_set_shell_layer(&w->defense_world, actor_id,
                                         layer_index, profile, start_full) != 0
               ? PDC3D_OK : PDC3D_ERR_NOT_FOUND;
}

int pdc3d_defense_actor_clear_shell_layer(pdc3d_world *w, int actor_id,
                                          int layer_index)
{
    if (w == 0) {
        return PDC3D_ERR_BAD_ARG;
    }
    return pdc3d_defense_clear_shell_layer(&w->defense_world, actor_id,
                                           layer_index) != 0
               ? PDC3D_OK : PDC3D_ERR_NOT_FOUND;
}

int pdc3d_resolve_incoming_attack(pdc3d_world *w,
                                  pdc3d_damage_packet *io_packet,
                                  pdc3d_v3 source_dir,
                                  pdc3d_defense_result *out_result)
{
    pdc3d_framedata_result fd_result;
    if (w == 0 || io_packet == 0) {
        return PDC3D_ERR_BAD_ARG;
    }
    pdc3d_framedata_resolve_damage(&w->framedata_world,
                                    io_packet->attacker_id,
                                    io_packet->defender_id,
                                    -1,
                                    io_packet,
                                    &fd_result);
    if (fd_result.can_hit == 0) {
        return PDC3D_OK;
    }
    pdc3d_defense_resolve_damage(&w->defense_world, io_packet, source_dir,
                                 w->tick, out_result);
    pdc3d_framedata_resolve_damage(&w->framedata_world,
                                    io_packet->attacker_id,
                                    io_packet->defender_id,
                                    out_result != 0 ? out_result->accepted : 0,
                                    io_packet,
                                    &fd_result);
    return PDC3D_OK;
}


int pdc3d_submit_damage(pdc3d_world *w,
                        pdc3d_damage_packet *io_packet,
                        pdc3d_v3 hit_point,
                        pdc3d_v3 source_dir,
                        pdc3d_defense_result *out_result)
{
    pdc3d_event ev;
    pdc3d_framedata_result fd_result;
    w3d_hit_result weak;
    int result_flags;
    int weak_result;
    if (w == 0 || io_packet == 0) {
        return PDC3D_ERR_BAD_ARG;
    }
    if (io_packet->final_damage <= 0) {
        io_packet->final_damage = io_packet->base_damage;
    }
    io_packet->final_damage = pdc3d_damage_apply_rules(&w->damage_table,
                                                       io_packet->base_damage,
                                                       io_packet->material_flags,
                                                       io_packet->attack_flags,
                                                       &result_flags);
    io_packet->result_flags |= result_flags;
    if (io_packet->weakspot_id == 0) {
        io_packet->weakspot_id = -1;
    }
    memset(&fd_result, 0, sizeof(fd_result));
    pdc3d_framedata_resolve_damage(&w->framedata_world,
                                    io_packet->attacker_id,
                                    io_packet->defender_id,
                                    -1,
                                    io_packet,
                                    &fd_result);
    if (fd_result.can_hit != 0) {
        pdc3d_defense_resolve_damage(&w->defense_world, io_packet,
                                     source_dir, w->tick, out_result);
        pdc3d_framedata_resolve_damage(&w->framedata_world,
                                        io_packet->attacker_id,
                                        io_packet->defender_id,
                                        out_result != 0 ? out_result->accepted : 0,
                                        io_packet,
                                        &fd_result);
        weak_result = w3d_resolve_hit_point(&w->weakspots,
                                            io_packet->defender_id,
                                            pdc3d_to_w3d_v3(hit_point),
                                            io_packet->final_damage,
                                            pdc3d_attack_flags_to_w3d(io_packet->attack_flags),
                                            &weak);
        if (weak_result == W3D_OK && weak.matched != 0) {
            io_packet->final_damage = weak.final_damage;
            io_packet->weakspot_id = weak.weakspot_id;
            io_packet->result_flags |= PDC3D_DMG_WEAKSPOT;
            if (out_result != 0 && out_result->accepted != 0) {
                out_result->final_damage = weak.final_damage;
            }
        }
    }
    memset(&ev, 0, sizeof(ev));
    ev.type = PDC3D_EVENT_DAMAGE;
    ev.subtype = 1;
    ev.damage = *io_packet;
    if (out_result != 0) {
        ev.defense = *out_result;
    }
    ev.framedata = fd_result;
    ev.point = hit_point;
    ev.normal = source_dir;
    ev.actor_a = io_packet->attacker_id;
    ev.actor_b = io_packet->defender_id;
    ev.value = io_packet->final_damage;
    return pdc3d_push_event(w, &ev);
}

int pdc3d_counter_try(pdc3d_world *w,
                      const pdc3d_counter_request *request,
                      pdc3d_defense_result *out_result)
{
    if (w == 0 || request == 0) {
        return PDC3D_ERR_BAD_ARG;
    }
    return pdc3d_defense_try_counter(&w->defense_world, request, w->tick,
                                     out_result) != 0
               ? PDC3D_OK : PDC3D_ERR_INACTIVE;
}

int pdc3d_counter_count_tokens(const pdc3d_world *w, int actor_id)
{
    if (w == 0) {
        return 0;
    }
    return pdc3d_defense_count_counter_tokens(&w->defense_world, actor_id);
}

static void pdc3d_convert_melee_event(pdc3d_world *w, const ml3_event *src)
{
    pdc3d_event ev;
    w3d_hit_result weak;
    pdc3d_v3 source_dir;
    int result_flags;
    int damage;
    int weak_result;
    if (w == 0 || src == 0) {
        return;
    }
    memset(&ev, 0, sizeof(ev));
    damage = pdc3d_damage_apply_rules(&w->damage_table, src->damage,
                                      src->hurt_material, src->hit_flags,
                                      &result_flags);
    ev.type = PDC3D_EVENT_DAMAGE;
    ev.subtype = 0;
    ev.point = src->point;
    ev.normal = src->normal;
    ev.actor_a = src->attacker_id;
    ev.actor_b = src->defender_id;
    ev.value = damage;

    ev.damage.attacker_id = src->attacker_id;
    ev.damage.defender_id = src->defender_id;
    ev.damage.attack_id = src->attack_id;
    ev.damage.hurtbox_index = src->hurtbox_index;
    ev.damage.hitbox_index = src->hitbox_index;
    ev.damage.base_damage = src->damage;
    ev.damage.final_damage = damage;
    ev.damage.stun_frames = src->stun_frames;
    ev.damage.hitstop_frames = src->hitstop_frames;
    ev.damage.attack_flags = src->hit_flags;
    ev.damage.material_flags = src->hurt_material;
    ev.damage.result_flags = result_flags;
    ev.damage.weakspot_id = -1;
    ev.damage.hit_user_tag = src->hit_user_tag;
    ev.damage.hurt_user_tag = src->hurt_user_tag;

    pdc3d_framedata_resolve_damage(&w->framedata_world,
                                    ev.damage.attacker_id,
                                    ev.damage.defender_id,
                                    -1,
                                    &ev.damage,
                                    &ev.framedata);
    if (ev.framedata.can_hit == 0) {
        ev.value = 0;
        pdc3d_push_event(w, &ev);
#if PDC3D_ENABLE_DAMAGE_CALLBACK
        if (w->bridge.damage_event != 0) {
            w->bridge.damage_event(w->bridge.user, &ev.damage);
        }
#endif
        return;
    }
    if (ev.framedata.attacker_registered != 0 &&
        ev.framedata.attack_active != 0) {
        damage = pdc3d_damage_apply_rules(&w->damage_table,
                                          ev.damage.base_damage,
                                          ev.damage.material_flags,
                                          ev.damage.attack_flags,
                                          &result_flags);
        ev.damage.final_damage = damage;
        ev.damage.result_flags = result_flags;
    }

    source_dir = pdc3d_source_dir_from_actors(w, ev.damage.attacker_id,
                                             ev.damage.defender_id,
                                             pdc3d_v3_make(PDC3D_FX_ONE, 0, 0));
    pdc3d_defense_resolve_damage(&w->defense_world, &ev.damage,
                                 source_dir, w->tick, &ev.defense);
    pdc3d_framedata_resolve_damage(&w->framedata_world,
                                    ev.damage.attacker_id,
                                    ev.damage.defender_id,
                                    ev.defense.accepted,
                                    &ev.damage,
                                    &ev.framedata);
    damage = ev.damage.final_damage;

    weak_result = w3d_resolve_hit_point(&w->weakspots, src->defender_id,
                                        pdc3d_to_w3d_v3(src->point),
                                        damage,
                                        pdc3d_attack_flags_to_w3d(src->hit_flags),
                                        &weak);
    if (weak_result == W3D_OK && weak.matched) {
        ev.damage.final_damage = weak.final_damage;
        ev.damage.weakspot_id = weak.weakspot_id;
        ev.damage.result_flags |= PDC3D_DMG_WEAKSPOT;
        ev.value = weak.final_damage;
        if (ev.defense.accepted != 0) {
            ev.defense.final_damage = weak.final_damage;
        }
    } else {
        ev.value = ev.damage.final_damage;
    }

    pdc3d_push_event(w, &ev);
#if PDC3D_ENABLE_DAMAGE_CALLBACK
    if (w->bridge.damage_event != 0) {
        w->bridge.damage_event(w->bridge.user, &ev.damage);
    }
#endif
}

static void pdc3d_drain_melee_events(pdc3d_world *w)
{
    ml3_event ev;
    while (ml3_poll_event(&w->melee, &ev)) {
        pdc3d_convert_melee_event(w, &ev);
    }
}

static void pdc3d_drain_grab_events(pdc3d_world *w)
{
    g3d_grab_event ge;
    while (g3d_poll_event(&w->grabs, &ge)) {
        pdc3d_event ev;
        memset(&ev, 0, sizeof(ev));
        ev.type = PDC3D_EVENT_GRAB;
        ev.subtype = ge.type;
        ev.actor_a = ge.owner_actor_id;
        ev.actor_b = ge.target_actor_id;
        ev.value = ge.value;
        ev.point.x = 0;
        ev.point.y = 0;
        ev.point.z = 0;
        ev.normal.x = 0;
        ev.normal.y = HB3_FX_ONE;
        ev.normal.z = 0;
        ev.damage.attacker_id = ge.owner_actor_id;
        ev.damage.defender_id = ge.target_actor_id;
        ev.damage.attack_id = ge.grab_id;
        ev.damage.hurtbox_index = ge.hurtbox_index;
        ev.damage.hitbox_index = ge.grab_shape_index;
        ev.damage.base_damage = 0;
        ev.damage.final_damage = 0;
        ev.damage.stun_frames = 0;
        ev.damage.hitstop_frames = 0;
        ev.damage.attack_flags = PDC3D_ATK_GRAB;
        ev.damage.material_flags = 0;
        ev.damage.result_flags = 0;
        ev.damage.weakspot_id = -1;
        ev.damage.hit_user_tag = 0;
        ev.damage.hurt_user_tag = 0;
        pdc3d_push_event(w, &ev);
    }
}

static void pdc3d_drain_throw_events(pdc3d_world *w)
{
    t3d_throw_event te;
    while (t3d_poll_event(&w->throws_world, &te)) {
        pdc3d_event ev;
        memset(&ev, 0, sizeof(ev));
        ev.type = PDC3D_EVENT_THROW;
        ev.subtype = te.type;
        ev.actor_a = te.attacker_id;
        ev.actor_b = te.body_id;
        ev.value = te.damage;
        ev.point = t3d_to_pdc3d_v3(te.pos);
        ev.normal.x = 0;
        ev.normal.y = HB3_FX_ONE;
        ev.normal.z = 0;
        ev.damage.attacker_id = te.attacker_id;
        ev.damage.defender_id = te.body_id;
        ev.damage.attack_id = te.throw_id;
        ev.damage.hurtbox_index = -1;
        ev.damage.hitbox_index = -1;
        ev.damage.base_damage = te.damage;
        ev.damage.final_damage = te.damage;
        ev.damage.stun_frames = 0;
        ev.damage.hitstop_frames = 0;
        ev.damage.attack_flags = PDC3D_ATK_BLUNT | PDC3D_ATK_KNOCKDOWN;
        ev.damage.material_flags = 0;
        ev.damage.result_flags = PDC3D_DMG_STAGGER;
        ev.damage.weakspot_id = -1;
        ev.damage.hit_user_tag = 0;
        ev.damage.hurt_user_tag = 0;
        pdc3d_push_event(w, &ev);
    }
}

static void pdc3d_drain_weakspot_events(pdc3d_world *w)
{
    w3d_event we;
    while (w3d_poll_event(&w->weakspots, &we)) {
        pdc3d_event ev;
        memset(&ev, 0, sizeof(ev));
        ev.type = PDC3D_EVENT_WEAKSPOT;
        ev.subtype = we.type;
        ev.actor_a = we.actor_id;
        ev.actor_b = we.weakspot_id;
        ev.value = we.final_damage;
        ev.point.x = 0;
        ev.point.y = 0;
        ev.point.z = 0;
        ev.normal.x = 0;
        ev.normal.y = HB3_FX_ONE;
        ev.normal.z = 0;
        ev.damage.attacker_id = 0;
        ev.damage.defender_id = we.actor_id;
        ev.damage.attack_id = 0;
        ev.damage.hurtbox_index = -1;
        ev.damage.hitbox_index = -1;
        ev.damage.base_damage = we.base_damage;
        ev.damage.final_damage = we.final_damage;
        ev.damage.stun_frames = 0;
        ev.damage.hitstop_frames = 0;
        ev.damage.attack_flags = 0;
        ev.damage.material_flags = PDC3D_MAT_WEAKSPOT;
        ev.damage.result_flags = PDC3D_DMG_WEAKSPOT;
        ev.damage.weakspot_id = we.weakspot_id;
        ev.damage.hit_user_tag = 0;
        ev.damage.hurt_user_tag = 0;
        pdc3d_push_event(w, &ev);
    }
}

void pdc3d_tick(pdc3d_world *w)
{
    if (w == 0) {
        return;
    }
    pdc3d_framedata_tick_all(&w->framedata_world, 0, 0);
    pdc3d_defense_update(&w->defense_world, 1, w->tick);
    ml3_tick(&w->melee);
    g3d_tick(&w->grabs);
    t3d_tick(&w->throws_world);
    pdc3d_drain_melee_events(w);
    pdc3d_drain_grab_events(w);
    pdc3d_drain_throw_events(w);
    pdc3d_drain_weakspot_events(w);
    w->tick += 1;
}

int pdc3d_poll_event(pdc3d_world *w, pdc3d_event *out_event)
{
    if (w == 0 || out_event == 0) {
        return 0;
    }
    if (w->event_count <= 0) {
        return 0;
    }
    *out_event = w->events[w->event_head];
    w->event_head += 1;
    if (w->event_head >= PDC3D_MAX_EVENTS) {
        w->event_head = 0;
    }
    w->event_count -= 1;
    return 1;
}

int pdc3d_peek_event_count(const pdc3d_world *w)
{
    if (w == 0) {
        return 0;
    }
    return w->event_count;
}

int pdc3d_world_set_broadphase_grid(pdc3d_world *w, pdc3d_v3 origin,
                                    pdc3d_fx cell_size,
                                    int cells_x, int cells_y, int cells_z)
{
    if (w == 0) {
        return PDC3D_ERR_BAD_ARG;
    }
    return ml3_world_set_broadphase_grid(&w->melee, origin, cell_size,
                                         cells_x, cells_y, cells_z);
}

int pdc3d_world_rebuild_broadphase(pdc3d_world *w)
{
    if (w == 0) {
        return PDC3D_ERR_BAD_ARG;
    }
    return ml3_world_rebuild_broadphase(&w->melee);
}

int pdc3d_world_broadphase_overflowed(const pdc3d_world *w)
{
    if (w == 0) {
        return 1;
    }
    return ml3_world_broadphase_overflowed(&w->melee);
}

void pdc3d_debug_draw_world_lines(const pdc3d_world *w,
                                  hb3_debug_line_fn line_fn,
                                  void *user,
                                  int flags)
{
    if (w == 0 || line_fn == 0) {
        return;
    }
    ml3_debug_draw_world_lines(&w->melee, line_fn, user, flags);
}
