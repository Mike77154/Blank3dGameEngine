#include "melee3d.h"

static int ml3_find_attack_index(const ml3_world *w, int handle)
{
    int i;
    if (w == 0) {
        return ML3_ERR_BAD_ARG;
    }
    for (i = 0; i < ML3_MAX_ATTACKS; ++i) {
        if (w->attacks[i].active && w->attacks[i].handle == handle) {
            return i;
        }
    }
    return ML3_ERR_NOT_FOUND;
}

static int ml3_hitlog_has(const ml3_attack *atk, int actor_id)
{
    int i;
    for (i = 0; i < atk->hitlog_count; ++i) {
        if (atk->hitlog_actor_ids[i] == actor_id) {
            return 1;
        }
    }
    return 0;
}

static int ml3_hitlog_add(ml3_attack *atk, int actor_id)
{
    if (atk->hitlog_count >= ML3_MAX_HITLOG_PER_ATTACK) {
        return ML3_ERR_FULL;
    }
    atk->hitlog_actor_ids[atk->hitlog_count] = actor_id;
    atk->hitlog_count += 1;
    return ML3_OK;
}

static int ml3_push_event(ml3_world *w, const ml3_event *ev)
{
    if (w->event_count >= ML3_MAX_EVENTS) {
        return ML3_ERR_EVENT_FULL;
    }
    w->events[w->event_tail] = *ev;
    w->event_tail += 1;
    if (w->event_tail >= ML3_MAX_EVENTS) {
        w->event_tail = 0;
    }
    w->event_count += 1;
    return ML3_OK;
}

static int ml3_attack_phase_raw(const ml3_attack *a)
{
    int f;
    if (a == 0 || !a->active) {
        return ML3_ATTACK_DONE;
    }
    f = a->frame;
    if (f < a->startup_frames) {
        return ML3_ATTACK_STARTUP;
    }
    f -= a->startup_frames;
    if (f < a->active_frames) {
        return ML3_ATTACK_ACTIVE;
    }
    f -= a->active_frames;
    if (f < a->recovery_frames) {
        return ML3_ATTACK_RECOVERY;
    }
    return ML3_ATTACK_DONE;
}

static void ml3_advance_attack(ml3_attack *atk)
{
    int phase;
    if (atk == 0 || !atk->active) {
        return;
    }
    atk->frame += 1;
    phase = ml3_attack_phase_raw(atk);
    if (phase == ML3_ATTACK_DONE) {
        atk->active = 0;
        hit3_attack_clear(&atk->hit);
    }
}

static int ml3_masks_allow(int attack_mask, int target_group)
{
    return (attack_mask & target_group) != 0;
}

static void ml3_make_event(ml3_attack *atk, const hit3_box *hitbox,
                           const hb3_hurtbox *hurtbox, int hitbox_index,
                           int hurtbox_index, ml3_event *ev)
{
    ev->attacker_id = atk->owner_id;
    ev->defender_id = hurtbox->owner_id;
    ev->attack_id = atk->attack_id;
    ev->attack_handle = atk->handle;
    ev->hitbox_index = hitbox_index;
    ev->hurtbox_index = hurtbox_index;
    ev->damage = atk->damage;
    ev->stun_frames = atk->stun_frames;
    ev->hitstop_frames = atk->hitstop_frames;
    ev->hit_flags = hitbox->curr.flags;
    ev->hurt_flags = hurtbox->shape.flags;
    ev->hurt_material = hurtbox->material;
    ev->hit_user_tag = hitbox->user_tag;
    ev->hurt_user_tag = hurtbox->user_tag;
    ev->point = hitbox->curr.a;
    ev->normal = hb3_v3_make(0, HB3_FX_ONE, 0);
}

static void ml3_resolve_attack(ml3_world *w, ml3_attack *atk)
{
    int hb_i;
    int q_count;
    int query_indices[ML3_MAX_QUERY_ACTORS];
    hb3_aabb query_box;
    if (w == 0 || atk == 0 || !atk->active) {
        return;
    }
    if (ml3_attack_phase_raw(atk) != ML3_ATTACK_ACTIVE) {
        return;
    }

    for (hb_i = 0; hb_i < HIT3_MAX_HITBOXES_PER_ATTACK; ++hb_i) {
        hit3_box *hitbox;
        int qi;
        hitbox = &atk->hit.hitboxes[hb_i];
        if (!hitbox->active) {
            continue;
        }
        if (hit3_hitbox_get_query_aabb(hitbox, &query_box) != HIT3_OK) {
            continue;
        }
        q_count = hb3_world_query_actor_indices(&w->hurt, &query_box, query_indices,
                                                ML3_MAX_QUERY_ACTORS);
        for (qi = 0; qi < q_count; ++qi) {
            hb3_actor *actor;
            int hurt_i;
            int actor_index;
            actor_index = query_indices[qi];
            if (actor_index < 0 || actor_index >= HB3_MAX_ACTORS) {
                continue;
            }
            actor = &w->hurt.actors[actor_index];
            if (!actor->active) {
                continue;
            }
            if ((atk->flags & ML3_FLAG_NO_SELF_HIT) != 0 && actor->id == atk->owner_id) {
                continue;
            }
            if ((atk->flags & ML3_FLAG_NO_TEAM_HIT) != 0) {
                int owner_index;
                owner_index = hb3_actor_find_index(&w->hurt, atk->owner_id);
                if (owner_index >= 0 && w->hurt.actors[owner_index].team == actor->team) {
                    continue;
                }
            }
            if ((atk->flags & ML3_FLAG_USE_GROUPS) != 0) {
                if (!ml3_masks_allow(atk->hit.hit_mask, actor->group_mask)) {
                    continue;
                }
            }
            if (ml3_hitlog_has(atk, actor->id)) {
                continue;
            }
            for (hurt_i = 0; hurt_i < actor->hurtbox_count; ++hurt_i) {
                hb3_hurtbox *hurt;
                ml3_event ev;
                hurt = &actor->hurtboxes[hurt_i];
                if (!hurt->active) {
                    continue;
                }
                if (hit3_hitbox_intersects_hurtbox(hitbox, hurt)) {
                    ml3_make_event(atk, hitbox, hurt, hb_i, hurt_i, &ev);
                    ml3_push_event(w, &ev);
                    ml3_hitlog_add(atk, actor->id);
                    if ((atk->flags & ML3_FLAG_ONE_HIT_ONLY) != 0) {
                        return;
                    }
                    break;
                }
            }
        }
    }
}

void ml3_world_init(ml3_world *w)
{
    int i;
    if (w == 0) {
        return;
    }
    hb3_world_init(&w->hurt);
    for (i = 0; i < ML3_MAX_ATTACKS; ++i) {
        w->attacks[i].active = 0;
        w->attacks[i].handle = 0;
        w->attacks[i].hitlog_count = 0;
    }
    w->event_head = 0;
    w->event_tail = 0;
    w->event_count = 0;
    w->next_attack_handle = 1;
    w->tick = 0;
    w->flags = 0;
}

int ml3_world_set_broadphase_grid(ml3_world *w, hb3_v3 origin,
                                  hb3_fx cell_size,
                                  int cells_x, int cells_y, int cells_z)
{
    if (w == 0) {
        return ML3_ERR_BAD_ARG;
    }
    return hb3_world_set_broadphase_grid(&w->hurt, origin, cell_size,
                                         cells_x, cells_y, cells_z);
}

void ml3_world_disable_broadphase(ml3_world *w)
{
    if (w != 0) {
        hb3_world_disable_broadphase(&w->hurt);
    }
}

int ml3_world_rebuild_broadphase(ml3_world *w)
{
    if (w == 0) {
        return ML3_ERR_BAD_ARG;
    }
    return hb3_world_rebuild_broadphase(&w->hurt);
}

int ml3_world_broadphase_ref_count(const ml3_world *w)
{
    if (w == 0) {
        return 0;
    }
    return hb3_world_broadphase_ref_count(&w->hurt);
}

int ml3_world_broadphase_overflowed(const ml3_world *w)
{
    if (w == 0) {
        return 0;
    }
    return hb3_world_broadphase_overflowed(&w->hurt);
}

int ml3_actor_add(ml3_world *w, int actor_id, int team)
{
    if (w == 0) {
        return ML3_ERR_BAD_ARG;
    }
    return hb3_actor_add(&w->hurt, actor_id, team);
}

int ml3_actor_remove(ml3_world *w, int actor_id)
{
    if (w == 0) {
        return ML3_ERR_BAD_ARG;
    }
    return hb3_actor_remove(&w->hurt, actor_id);
}

int ml3_actor_set_masks(ml3_world *w, int actor_id, int group_mask, int hit_mask)
{
    if (w == 0) {
        return ML3_ERR_BAD_ARG;
    }
    return hb3_actor_set_masks(&w->hurt, actor_id, group_mask, hit_mask);
}

int ml3_actor_clear_hurtboxes(ml3_world *w, int actor_id)
{
    if (w == 0) {
        return ML3_ERR_BAD_ARG;
    }
    return hb3_actor_clear_hurtboxes(&w->hurt, actor_id);
}

int ml3_actor_add_hurt_sphere(ml3_world *w, int actor_id, int hurt_id,
                              hb3_v3 center, hb3_fx radius,
                              int hurt_flags, int user_tag)
{
    if (w == 0) {
        return ML3_ERR_BAD_ARG;
    }
    return hb3_actor_add_hurt_sphere(&w->hurt, actor_id, hurt_id,
                                     center, radius, hurt_flags, user_tag);
}

int ml3_actor_add_hurt_capsule(ml3_world *w, int actor_id, int hurt_id,
                               hb3_v3 p0, hb3_v3 p1, hb3_fx radius,
                               int hurt_flags, int user_tag)
{
    if (w == 0) {
        return ML3_ERR_BAD_ARG;
    }
    return hb3_actor_add_hurt_capsule(&w->hurt, actor_id, hurt_id,
                                      p0, p1, radius, hurt_flags, user_tag);
}

int ml3_actor_add_hurt_obb(ml3_world *w, int actor_id, int hurt_id,
                           hb3_v3 center, hb3_v3 half_extents,
                           hb3_mat3 basis, int hurt_flags, int user_tag)
{
    if (w == 0) {
        return ML3_ERR_BAD_ARG;
    }
    return hb3_actor_add_hurt_obb(&w->hurt, actor_id, hurt_id,
                                  center, half_extents, basis, hurt_flags,
                                  user_tag);
}

int ml3_attack_begin(ml3_world *w, int owner_id, int attack_id,
                     int startup_frames, int active_frames,
                     int recovery_frames, int damage,
                     int stun_frames, int hitstop_frames,
                     int flags, int *out_handle)
{
    int i;
    int free_i;
    ml3_attack *a;
    if (w == 0 || out_handle == 0) {
        return ML3_ERR_BAD_ARG;
    }
    free_i = -1;
    for (i = 0; i < ML3_MAX_ATTACKS; ++i) {
        if (!w->attacks[i].active) {
            free_i = i;
            break;
        }
    }
    if (free_i < 0) {
        return ML3_ERR_FULL;
    }
    a = &w->attacks[free_i];
    a->active = 1;
    a->handle = w->next_attack_handle++;
    if (w->next_attack_handle <= 0) {
        w->next_attack_handle = 1;
    }
    a->owner_id = owner_id;
    a->attack_id = attack_id;
    a->frame = 0;
    a->startup_frames = startup_frames < 0 ? 0 : startup_frames;
    a->active_frames = active_frames < 0 ? 0 : active_frames;
    a->recovery_frames = recovery_frames < 0 ? 0 : recovery_frames;
    a->damage = damage;
    a->stun_frames = stun_frames;
    a->hitstop_frames = hitstop_frames;
    a->flags = flags;
    a->hitlog_count = 0;
    hit3_attack_init(&a->hit, owner_id);
    *out_handle = a->handle;
    return ML3_OK;
}

int ml3_attack_end(ml3_world *w, int handle)
{
    int i;
    i = ml3_find_attack_index(w, handle);
    if (i < 0) {
        return i;
    }
    w->attacks[i].active = 0;
    hit3_attack_clear(&w->attacks[i].hit);
    return ML3_OK;
}

int ml3_attack_clear_hitboxes(ml3_world *w, int handle)
{
    int i;
    i = ml3_find_attack_index(w, handle);
    if (i < 0) {
        return i;
    }
    hit3_attack_clear(&w->attacks[i].hit);
    return ML3_OK;
}

int ml3_attack_set_masks(ml3_world *w, int handle, int group_mask, int hit_mask)
{
    int i;
    i = ml3_find_attack_index(w, handle);
    if (i < 0) {
        return i;
    }
    hit3_attack_set_masks(&w->attacks[i].hit, group_mask, hit_mask);
    return ML3_OK;
}

int ml3_attack_set_hit_sphere(ml3_world *w, int handle, int slot, int hit_id,
                              hb3_v3 center, hb3_fx radius,
                              int hit_flags, int user_tag)
{
    int i;
    i = ml3_find_attack_index(w, handle);
    if (i < 0) {
        return i;
    }
    return hit3_attack_set_sphere(&w->attacks[i].hit, slot, hit_id,
                                  center, radius, hit_flags, user_tag);
}

int ml3_attack_set_hit_capsule(ml3_world *w, int handle, int slot, int hit_id,
                               hb3_v3 p0, hb3_v3 p1, hb3_fx radius,
                               int hit_flags, int user_tag)
{
    int i;
    i = ml3_find_attack_index(w, handle);
    if (i < 0) {
        return i;
    }
    return hit3_attack_set_capsule(&w->attacks[i].hit, slot, hit_id,
                                   p0, p1, radius, hit_flags, user_tag);
}

int ml3_attack_set_hit_obb(ml3_world *w, int handle, int slot, int hit_id,
                           hb3_v3 center, hb3_v3 half_extents,
                           hb3_mat3 basis, int hit_flags, int user_tag)
{
    int i;
    i = ml3_find_attack_index(w, handle);
    if (i < 0) {
        return i;
    }
    return hit3_attack_set_obb(&w->attacks[i].hit, slot, hit_id,
                               center, half_extents, basis, hit_flags,
                               user_tag);
}

int ml3_attack_get_phase(const ml3_world *w, int handle)
{
    int i;
    i = ml3_find_attack_index(w, handle);
    if (i < 0) {
        return ML3_ATTACK_DONE;
    }
    return ml3_attack_phase_raw(&w->attacks[i]);
}

int ml3_attack_is_active_window(const ml3_world *w, int handle)
{
    return ml3_attack_get_phase(w, handle) == ML3_ATTACK_ACTIVE;
}

void ml3_tick(ml3_world *w)
{
    int i;
    if (w == 0) {
        return;
    }
    if (w->hurt.bp.mode == HB3_BP_GRID) {
        hb3_world_rebuild_broadphase(&w->hurt);
    }
    for (i = 0; i < ML3_MAX_ATTACKS; ++i) {
        if (w->attacks[i].active) {
            ml3_resolve_attack(w, &w->attacks[i]);
        }
    }
    for (i = 0; i < ML3_MAX_ATTACKS; ++i) {
        if (w->attacks[i].active) {
            ml3_advance_attack(&w->attacks[i]);
        }
    }
    w->tick += 1;
    w->hurt.tick = w->tick;
}

int ml3_poll_event(ml3_world *w, ml3_event *out_event)
{
    if (w == 0 || out_event == 0) {
        return 0;
    }
    if (w->event_count <= 0) {
        return 0;
    }
    *out_event = w->events[w->event_head];
    w->event_head += 1;
    if (w->event_head >= ML3_MAX_EVENTS) {
        w->event_head = 0;
    }
    w->event_count -= 1;
    return 1;
}

int ml3_peek_event_count(const ml3_world *w)
{
    if (w == 0) {
        return 0;
    }
    return w->event_count;
}

void ml3_debug_draw_world_lines(const ml3_world *w, hb3_debug_line_fn line_fn,
                                void *user, int flags)
{
    int i;
    int hb_flags;
    if (w == 0 || line_fn == 0) {
        return;
    }
    hb_flags = 0;
    if ((flags & ML3_DEBUG_HURTBOXES) != 0) {
        hb_flags |= HB3_DEBUG_HURTBOXES;
    }
    if ((flags & ML3_DEBUG_GRID) != 0) {
        hb_flags |= HB3_DEBUG_GRID;
    }
    if (hb_flags != 0) {
        hb3_debug_draw_world_lines(&w->hurt, line_fn, user, hb_flags);
    }
    if ((flags & ML3_DEBUG_HITBOXES) != 0) {
        for (i = 0; i < ML3_MAX_ATTACKS; ++i) {
            if (w->attacks[i].active) {
                hit3_attack_debug_draw(&w->attacks[i].hit, line_fn, user);
            }
        }
    }
}

int ml3_debug_count_actors(const ml3_world *w)
{
    if (w == 0) {
        return 0;
    }
    return hb3_debug_count_actors(&w->hurt);
}

int ml3_debug_count_attacks(const ml3_world *w)
{
    int i;
    int n;
    if (w == 0) {
        return 0;
    }
    n = 0;
    for (i = 0; i < ML3_MAX_ATTACKS; ++i) {
        if (w->attacks[i].active) {
            n += 1;
        }
    }
    return n;
}
