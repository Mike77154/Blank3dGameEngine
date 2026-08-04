#include "fcasing89.h"

#define FCASING89_NULL_INDEX 65535U

static void fc89_copy_name(char *dst, const char *src)
{
    int i;
    for (i = 0; i < FCASING89_PROFILE_NAME_LEN - 1; i++) {
        if (src[i] == '\0') {
            break;
        }
        dst[i] = src[i];
    }
    dst[i] = '\0';
}

fc89_i32 fcasing89_abs_i32(fc89_i32 v)
{
    if (v < 0) {
        return -v;
    }
    return v;
}

fc89_i32 fcasing89_fix_mul(fc89_i32 a, fc89_i32 b)
{
    return (fc89_i32)((a * b) >> FCASING89_FIX_SHIFT);
}

FCasing89Vec3 fcasing89_vec3(fc89_i32 x, fc89_i32 y, fc89_i32 z)
{
    FCasing89Vec3 v;
    v.x = x;
    v.y = y;
    v.z = z;
    return v;
}

FCasing89Vec3 fcasing89_vec3_add(FCasing89Vec3 a, FCasing89Vec3 b)
{
    FCasing89Vec3 v;
    v.x = a.x + b.x;
    v.y = a.y + b.y;
    v.z = a.z + b.z;
    return v;
}

FCasing89Vec3 fcasing89_vec3_scale_q(FCasing89Vec3 v, fc89_i32 q)
{
    FCasing89Vec3 out;
    out.x = fcasing89_fix_mul(v.x, q);
    out.y = fcasing89_fix_mul(v.y, q);
    out.z = fcasing89_fix_mul(v.z, q);
    return out;
}

FCasing89Vec3 fcasing89_vec3_mul_q(FCasing89Vec3 a, FCasing89Vec3 b)
{
    FCasing89Vec3 out;
    out.x = fcasing89_fix_mul(a.x, b.x);
    out.y = fcasing89_fix_mul(a.y, b.y);
    out.z = fcasing89_fix_mul(a.z, b.z);
    return out;
}

fc89_i32 fcasing89_vec3_dot_q(FCasing89Vec3 a, FCasing89Vec3 b)
{
    return fcasing89_fix_mul(a.x, b.x)
        + fcasing89_fix_mul(a.y, b.y)
        + fcasing89_fix_mul(a.z, b.z);
}

FCasing89Vec3 fcasing89_vec3_add_scaled_q(FCasing89Vec3 base, FCasing89Vec3 dir, fc89_i32 q)
{
    FCasing89Vec3 out;
    out.x = base.x + fcasing89_fix_mul(dir.x, q);
    out.y = base.y + fcasing89_fix_mul(dir.y, q);
    out.z = base.z + fcasing89_fix_mul(dir.z, q);
    return out;
}

fc89_i32 fcasing89_manhattan_units(FCasing89Vec3 a, FCasing89Vec3 b)
{
    fc89_i32 dx;
    fc89_i32 dy;
    fc89_i32 dz;
    dx = fcasing89_abs_i32(a.x - b.x) >> FCASING89_FIX_SHIFT;
    dy = fcasing89_abs_i32(a.y - b.y) >> FCASING89_FIX_SHIFT;
    dz = fcasing89_abs_i32(a.z - b.z) >> FCASING89_FIX_SHIFT;
    return dx + dy + dz;
}

static fc89_u32 fc89_rng_next(FCasing89System *sys)
{
    sys->rng = sys->rng * 1664525UL + 1013904223UL;
    return sys->rng;
}

static fc89_i32 fc89_rand_range(FCasing89System *sys, fc89_i32 min_v, fc89_i32 max_v)
{
    fc89_u32 r;
    fc89_u32 span;
    fc89_i32 out;
    if (max_v <= min_v) {
        return min_v;
    }
    span = (fc89_u32)(max_v - min_v + 1L);
    r = fc89_rng_next(sys);
    out = min_v + (fc89_i32)(r % span);
    return out;
}

static fc89_i32 fc89_time_step(fc89_i32 value_per_sec, fc89_u16 dt_ms)
{
    return (fc89_i32)((value_per_sec * (fc89_i32)dt_ms) / 1000L);
}

static void fc89_push_event(FCasing89System *sys, fc89_u8 type, fc89_u8 audio_id, fc89_u16 casing_index, FCasing89Vec3 pos, fc89_u8 bounce_count)
{
    FCasing89Event *ev;
    fc89_u16 next_tail;
    next_tail = (fc89_u16)((sys->event_tail + 1U) % FCASING89_MAX_EVENTS);
    if (next_tail == sys->event_head) {
        sys->event_head = (fc89_u16)((sys->event_head + 1U) % FCASING89_MAX_EVENTS);
    }
    ev = &sys->events[sys->event_tail];
    ev->type = type;
    ev->audio_id = audio_id;
    ev->casing_index = casing_index;
    ev->pos = pos;
    ev->bounce_count = bounce_count;
    ev->reserved0 = 0U;
    ev->reserved1 = 0U;
    sys->event_tail = next_tail;
}

static void fc89_release_node(FCasing89System *sys, fc89_u16 idx, fc89_u8 event_type)
{
    FCasing89Node *n;
    if (idx >= FCASING89_MAX_CASINGS) {
        return;
    }
    n = &sys->nodes[idx];
    if (n->state == FCASING89_STATE_FREE) {
        return;
    }
    if (event_type != FCASING89_EVENT_NONE) {
        fc89_push_event(sys, event_type, 0U, idx, n->pos, n->bounce_count);
    }
    n->state = FCASING89_STATE_FREE;
    n->next_free = sys->first_free;
    n->age_ms = 0U;
    n->sleep_age_ms = 0U;
    n->alpha = 0U;
    sys->first_free = idx;
}

static fc89_u16 fc89_pop_free(FCasing89System *sys)
{
    fc89_u16 idx;
    if (sys->first_free == FCASING89_NULL_INDEX) {
        return FCASING89_NULL_INDEX;
    }
    idx = sys->first_free;
    sys->first_free = sys->nodes[idx].next_free;
    sys->nodes[idx].next_free = FCASING89_NULL_INDEX;
    return idx;
}

static fc89_i32 fc89_node_recycle_score(const FCasing89System *sys, fc89_u16 idx)
{
    const FCasing89Node *n;
    fc89_i32 score;
    fc89_i32 distance;
    n = &sys->nodes[idx];
    score = (fc89_i32)n->age_ms;
    if (n->state == FCASING89_STATE_FAKE) {
        score += 2000L;
    }
    if (n->state == FCASING89_STATE_SLEEP) {
        score += 1200L;
    }
    score += (fc89_i32)(255U - n->importance) * 8L;
    if (sys->camera.valid != 0U) {
        distance = fcasing89_manhattan_units(sys->camera.pos, n->pos);
        score += distance;
    }
    return score;
}

static fc89_u16 fc89_alloc_node(FCasing89System *sys)
{
    fc89_u16 idx;
    int i;
    fc89_u16 best;
    fc89_i32 best_score;
    fc89_i32 score;
    idx = fc89_pop_free(sys);
    if (idx != FCASING89_NULL_INDEX) {
        return idx;
    }
    if (sys->cfg.recycle_when_full == 0U) {
        sys->stats.dropped_total++;
        return FCASING89_NULL_INDEX;
    }
    best = FCASING89_NULL_INDEX;
    best_score = -2147483000L;
    for (i = 0; i < FCASING89_MAX_CASINGS; i++) {
        if (sys->nodes[i].state != FCASING89_STATE_FREE) {
            score = fc89_node_recycle_score(sys, (fc89_u16)i);
            if (best == FCASING89_NULL_INDEX || score > best_score) {
                best = (fc89_u16)i;
                best_score = score;
            }
        }
    }
    if (best != FCASING89_NULL_INDEX) {
        sys->stats.recycled_total++;
        sys->nodes[best].state = FCASING89_STATE_FREE;
        return best;
    }
    sys->stats.dropped_total++;
    return FCASING89_NULL_INDEX;
}

static void fc89_count_active(const FCasing89System *sys, fc89_u16 *sim_count, fc89_u16 *fake_count)
{
    int i;
    fc89_u16 sim;
    fc89_u16 fake;
    sim = 0U;
    fake = 0U;
    for (i = 0; i < FCASING89_MAX_CASINGS; i++) {
        if (sys->nodes[i].state == FCASING89_STATE_SIM || sys->nodes[i].state == FCASING89_STATE_SLEEP) {
            sim++;
        } else if (sys->nodes[i].state == FCASING89_STATE_FAKE) {
            fake++;
        }
    }
    *sim_count = sim;
    *fake_count = fake;
}

static fc89_u8 fc89_choose_state(FCasing89System *sys, const FCasing89Spawn *spawn, const FCasing89Profile *p)
{
    fc89_u16 sim_count;
    fc89_u16 fake_count;
    fc89_i32 dist;
    fc89_u8 mode;
    mode = sys->cfg.default_mode;
    if (p->mode_hint != FCASING89_MODE_AUTO) {
        mode = p->mode_hint;
    }
    if ((spawn->flags & FCASING89_FLAG_FORCE_SIM) != 0U) {
        return FCASING89_STATE_SIM;
    }
    if ((spawn->flags & FCASING89_FLAG_FORCE_FAKE) != 0U) {
        return FCASING89_STATE_FAKE;
    }
    if (mode == FCASING89_MODE_OFF) {
        return FCASING89_STATE_FREE;
    }
    if (mode == FCASING89_MODE_SIM) {
        return FCASING89_STATE_SIM;
    }
    if (mode == FCASING89_MODE_FAKE) {
        return FCASING89_STATE_FAKE;
    }
    fc89_count_active(sys, &sim_count, &fake_count);
    if (sys->camera.valid != 0U) {
        dist = fcasing89_manhattan_units(spawn->origin, sys->camera.pos);
        if (dist > sys->cfg.fake_distance) {
            sys->stats.budget_fake_total++;
            return FCASING89_STATE_FAKE;
        }
        if (dist > sys->cfg.near_distance && spawn->importance < 192U) {
            sys->stats.budget_fake_total++;
            return FCASING89_STATE_FAKE;
        }
    }
    if (sim_count >= sys->cfg.max_simulated) {
        sys->stats.budget_fake_total++;
        return FCASING89_STATE_FAKE;
    }
    if (fake_count >= sys->cfg.max_fake && sim_count >= sys->cfg.max_simulated) {
        return FCASING89_STATE_FREE;
    }
    return FCASING89_STATE_SIM;
}

void fcasing89_provider_init(FCasing89Provider *provider)
{
    if (provider == 0) {
        return;
    }
    provider->user = 0;
    provider->enabled_mask = 0UL;
    provider->gravity = 0;
    provider->move = 0;
    provider->rotate = 0;
    provider->scale = 0;
    provider->collision = 0;
}

void fcasing89_set_provider(FCasing89System *sys, const FCasing89Provider *provider)
{
    if (sys == 0) {
        return;
    }
    if (provider == 0) {
        fcasing89_provider_init(&sys->provider);
        return;
    }
    sys->provider = *provider;
    sys->provider.enabled_mask &= FCASING89_PROVIDER_ALL;
}

void fcasing89_remove_provider(FCasing89System *sys)
{
    if (sys == 0) {
        return;
    }
    fcasing89_provider_init(&sys->provider);
}

fc89_u32 fcasing89_get_provider_mask(const FCasing89System *sys)
{
    if (sys == 0) {
        return 0UL;
    }
    return sys->provider.enabled_mask & FCASING89_PROVIDER_ALL;
}

void fcasing89_default_config(FCasing89Config *cfg)
{
    if (cfg == 0) {
        return;
    }
    cfg->max_simulated = 48U;
    cfg->max_fake = 48U;
    cfg->max_spawn_per_emit = 8U;
    cfg->max_spawn_per_frame = 12U;
    cfg->recycle_when_full = 1U;
    cfg->default_mode = FCASING89_MODE_AUTO;
    cfg->near_distance = FCASING89_TO_FIX(80);
    cfg->fake_distance = FCASING89_TO_FIX(220);
    cfg->audio_distance = FCASING89_TO_FIX(90);
    cfg->global_floor_y = 0L;
}

void fcasing89_default_profile(FCasing89Profile *profile, fc89_u16 type_id)
{
    if (profile == 0) {
        return;
    }
    fc89_copy_name(profile->name, "casing");
    profile->side_min = FCASING89_TO_FIX(120);
    profile->side_max = FCASING89_TO_FIX(230);
    profile->up_min = FCASING89_TO_FIX(90);
    profile->up_max = FCASING89_TO_FIX(160);
    profile->back_min = FCASING89_TO_FIX(20);
    profile->back_max = FCASING89_TO_FIX(90);
    profile->spin_min = FCASING89_TO_FIX(500);
    profile->spin_max = FCASING89_TO_FIX(1600);
    profile->gravity = -FCASING89_TO_FIX(980);
    profile->bounce_q = (FCASING89_FIX_ONE * 42L) / 100L;
    profile->friction_q = (FCASING89_FIX_ONE * 62L) / 100L;
    profile->radius = FCASING89_TO_FIX(1);
    profile->floor_y = 0L;
    profile->lifetime_ms = 2500U;
    profile->sleep_ms = 700U;
    profile->fade_ms = 600U;
    profile->min_bounce_speed = (fc89_u16)FCASING89_TO_FIX(40);
    profile->count_per_emit = 1U;
    profile->max_bounces = 2U;
    profile->render_model_id = 1U;
    profile->audio_id = 1U;
    profile->mode_hint = FCASING89_MODE_AUTO;
    profile->reserved0 = 0U;
    profile->reserved1 = 0U;
    profile->reserved2 = 0U;

    if (type_id == FCASING89_PROFILE_PISTOL) {
        fc89_copy_name(profile->name, "pistol_9mm");
        profile->side_min = FCASING89_TO_FIX(150);
        profile->side_max = FCASING89_TO_FIX(250);
        profile->up_min = FCASING89_TO_FIX(100);
        profile->up_max = FCASING89_TO_FIX(180);
        profile->back_min = FCASING89_TO_FIX(25);
        profile->back_max = FCASING89_TO_FIX(85);
        profile->spin_min = FCASING89_TO_FIX(700);
        profile->spin_max = FCASING89_TO_FIX(1700);
        profile->lifetime_ms = 2300U;
        profile->render_model_id = 1U;
        profile->audio_id = 1U;
    } else if (type_id == FCASING89_PROFILE_RIFLE) {
        fc89_copy_name(profile->name, "rifle_556");
        profile->side_min = FCASING89_TO_FIX(220);
        profile->side_max = FCASING89_TO_FIX(340);
        profile->up_min = FCASING89_TO_FIX(120);
        profile->up_max = FCASING89_TO_FIX(210);
        profile->back_min = FCASING89_TO_FIX(50);
        profile->back_max = FCASING89_TO_FIX(120);
        profile->spin_min = FCASING89_TO_FIX(1000);
        profile->spin_max = FCASING89_TO_FIX(2400);
        profile->lifetime_ms = 1800U;
        profile->sleep_ms = 500U;
        profile->max_bounces = 1U;
        profile->render_model_id = 2U;
        profile->audio_id = 2U;
    } else if (type_id == FCASING89_PROFILE_SHOTGUN) {
        fc89_copy_name(profile->name, "shotgun_shell");
        profile->side_min = FCASING89_TO_FIX(80);
        profile->side_max = FCASING89_TO_FIX(170);
        profile->up_min = FCASING89_TO_FIX(70);
        profile->up_max = FCASING89_TO_FIX(140);
        profile->back_min = FCASING89_TO_FIX(40);
        profile->back_max = FCASING89_TO_FIX(120);
        profile->spin_min = FCASING89_TO_FIX(250);
        profile->spin_max = FCASING89_TO_FIX(900);
        profile->lifetime_ms = 3200U;
        profile->sleep_ms = 900U;
        profile->radius = FCASING89_TO_FIX(2);
        profile->render_model_id = 3U;
        profile->audio_id = 3U;
    } else if (type_id == FCASING89_PROFILE_HEAVY) {
        fc89_copy_name(profile->name, "heavy_brass");
        profile->side_min = FCASING89_TO_FIX(140);
        profile->side_max = FCASING89_TO_FIX(260);
        profile->up_min = FCASING89_TO_FIX(110);
        profile->up_max = FCASING89_TO_FIX(220);
        profile->back_min = FCASING89_TO_FIX(40);
        profile->back_max = FCASING89_TO_FIX(110);
        profile->spin_min = FCASING89_TO_FIX(600);
        profile->spin_max = FCASING89_TO_FIX(1300);
        profile->lifetime_ms = 2800U;
        profile->sleep_ms = 800U;
        profile->max_bounces = 2U;
        profile->radius = FCASING89_TO_FIX(2);
        profile->render_model_id = 4U;
        profile->audio_id = 4U;
    } else if (type_id == FCASING89_PROFILE_TINY) {
        fc89_copy_name(profile->name, "tiny_brass");
        profile->side_min = FCASING89_TO_FIX(130);
        profile->side_max = FCASING89_TO_FIX(220);
        profile->up_min = FCASING89_TO_FIX(80);
        profile->up_max = FCASING89_TO_FIX(150);
        profile->back_min = FCASING89_TO_FIX(20);
        profile->back_max = FCASING89_TO_FIX(70);
        profile->spin_min = FCASING89_TO_FIX(900);
        profile->spin_max = FCASING89_TO_FIX(2300);
        profile->lifetime_ms = 1200U;
        profile->sleep_ms = 300U;
        profile->max_bounces = 1U;
        profile->render_model_id = 5U;
        profile->audio_id = 1U;
    }
}

void fcasing89_init(FCasing89System *sys, const FCasing89Config *cfg, fc89_u32 seed)
{
    int i;
    FCasing89Config local_cfg;
    if (sys == 0) {
        return;
    }
    if (cfg == 0) {
        fcasing89_default_config(&local_cfg);
        sys->cfg = local_cfg;
    } else {
        sys->cfg = *cfg;
    }
    if (sys->cfg.max_simulated > FCASING89_MAX_CASINGS) {
        sys->cfg.max_simulated = FCASING89_MAX_CASINGS;
    }
    if (sys->cfg.max_fake > FCASING89_MAX_CASINGS) {
        sys->cfg.max_fake = FCASING89_MAX_CASINGS;
    }
    if (sys->cfg.max_spawn_per_emit == 0U) {
        sys->cfg.max_spawn_per_emit = 1U;
    }
    if (sys->cfg.max_spawn_per_frame == 0U) {
        sys->cfg.max_spawn_per_frame = 1U;
    }

    for (i = 0; i < FCASING89_MAX_PROFILES; i++) {
        fcasing89_default_profile(&sys->profiles[i], (fc89_u16)i);
    }
    for (i = 0; i < FCASING89_MAX_EVENTS; i++) {
        sys->events[i].type = FCASING89_EVENT_NONE;
    }
    for (i = 0; i < FCASING89_MAX_CASINGS; i++) {
        sys->nodes[i].state = FCASING89_STATE_FREE;
        sys->nodes[i].flags = 0U;
        sys->nodes[i].bounce_count = 0U;
        sys->nodes[i].alpha = 0U;
        sys->nodes[i].profile_id = 0U;
        sys->nodes[i].age_ms = 0U;
        sys->nodes[i].sleep_age_ms = 0U;
        sys->nodes[i].order = 0U;
        sys->nodes[i].importance = 0U;
        sys->nodes[i].scale = fcasing89_vec3(
            FCASING89_FIX_ONE,
            FCASING89_FIX_ONE,
            FCASING89_FIX_ONE);
        sys->nodes[i].next_free = (fc89_u16)(i + 1);
    }
    sys->nodes[FCASING89_MAX_CASINGS - 1].next_free = FCASING89_NULL_INDEX;
    sys->first_free = 0U;
    sys->event_head = 0U;
    sys->event_tail = 0U;
    sys->order_counter = 0U;
    sys->frame_spawned = 0U;
    sys->initialized = 1U;
    sys->reserved0 = 0U;
    sys->rng = seed;
    if (sys->rng == 0UL) {
        sys->rng = 0x1234ABCDUL;
    }
    sys->camera.pos = fcasing89_vec3(0L, 0L, 0L);
    sys->camera.valid = 0U;
    sys->camera.reserved0 = 0U;
    sys->camera.reserved1 = 0U;
    fcasing89_provider_init(&sys->provider);
    fcasing89_reset(sys);
}

void fcasing89_reset(FCasing89System *sys)
{
    int i;
    if (sys == 0) {
        return;
    }
    for (i = 0; i < FCASING89_MAX_CASINGS; i++) {
        sys->nodes[i].state = FCASING89_STATE_FREE;
        sys->nodes[i].next_free = (fc89_u16)(i + 1);
        sys->nodes[i].age_ms = 0U;
        sys->nodes[i].sleep_age_ms = 0U;
        sys->nodes[i].alpha = 0U;
        sys->nodes[i].scale = fcasing89_vec3(
            FCASING89_FIX_ONE,
            FCASING89_FIX_ONE,
            FCASING89_FIX_ONE);
    }
    sys->nodes[FCASING89_MAX_CASINGS - 1].next_free = FCASING89_NULL_INDEX;
    sys->first_free = 0U;
    sys->event_head = 0U;
    sys->event_tail = 0U;
    sys->frame_spawned = 0U;
    sys->stats.active_total = 0U;
    sys->stats.active_sim = 0U;
    sys->stats.active_fake = 0U;
    sys->stats.free_count = FCASING89_MAX_CASINGS;
    sys->stats.emitted_total = 0UL;
    sys->stats.recycled_total = 0UL;
    sys->stats.dropped_total = 0UL;
    sys->stats.budget_fake_total = 0UL;
}

void fcasing89_begin_frame(FCasing89System *sys)
{
    if (sys == 0) {
        return;
    }
    sys->frame_spawned = 0U;
}

void fcasing89_set_camera(FCasing89System *sys, const FCasing89Camera *camera)
{
    if (sys == 0) {
        return;
    }
    if (camera == 0) {
        sys->camera.valid = 0U;
        return;
    }
    sys->camera = *camera;
}

int fcasing89_set_profile(FCasing89System *sys, fc89_u16 profile_id, const FCasing89Profile *profile)
{
    if (sys == 0 || profile == 0) {
        return 0;
    }
    if (profile_id >= FCASING89_MAX_PROFILES) {
        return 0;
    }
    sys->profiles[profile_id] = *profile;
    return 1;
}

static FCasing89ProviderTransformFn fc89_get_transform_fn(
    const FCasing89System *sys,
    fc89_u8 operation,
    fc89_u32 *out_mask)
{
    if (operation == FCASING89_TRANSFORM_MOVE) {
        *out_mask = FCASING89_PROVIDER_MOVE;
        return sys->provider.move;
    }
    if (operation == FCASING89_TRANSFORM_ROTATE) {
        *out_mask = FCASING89_PROVIDER_ROTATE;
        return sys->provider.rotate;
    }
    *out_mask = FCASING89_PROVIDER_SCALE;
    return sys->provider.scale;
}

static FCasing89Vec3 fc89_apply_transform(
    FCasing89System *sys,
    fc89_u16 casing_index,
    fc89_u16 profile_id,
    fc89_u8 state,
    fc89_u8 operation,
    FCasing89Vec3 current,
    FCasing89Vec3 delta,
    FCasing89Vec3 source,
    fc89_u16 dt_ms)
{
    FCasing89ProviderTransformFn fn;
    FCasing89ProviderTransformQuery query;
    FCasing89Vec3 out;
    fc89_u32 mask;

    fn = fc89_get_transform_fn(sys, operation, &mask);
    if ((sys->provider.enabled_mask & mask) != 0UL && fn != 0) {
        query.current = current;
        query.delta = delta;
        query.source = source;
        query.casing_index = casing_index;
        query.profile_id = profile_id;
        query.dt_ms = dt_ms;
        query.operation = operation;
        query.state = state;
        out = current;
        if (fn(sys->provider.user, &query, &out) != 0) {
            return out;
        }
    }

    if (operation == FCASING89_TRANSFORM_SCALE) {
        return fcasing89_vec3_mul_q(current, delta);
    }
    return fcasing89_vec3_add(current, delta);
}

static FCasing89Vec3 fc89_get_acceleration(
    FCasing89System *sys,
    fc89_u16 casing_index,
    const FCasing89Node *n,
    const FCasing89Profile *p,
    fc89_i32 fallback_y,
    fc89_u16 dt_ms)
{
    FCasing89ProviderGravityQuery query;
    FCasing89Vec3 acceleration;

    acceleration = fcasing89_vec3(0L, fallback_y, 0L);
    if ((sys->provider.enabled_mask & FCASING89_PROVIDER_GRAVITY) != 0UL
        && sys->provider.gravity != 0) {
        query.position = n->pos;
        query.velocity = n->vel;
        query.fallback_acceleration = acceleration;
        query.casing_index = casing_index;
        query.profile_id = n->profile_id;
        query.dt_ms = dt_ms;
        query.state = n->state;
        query.bounce_count = n->bounce_count;
        if (sys->provider.gravity(
                sys->provider.user,
                &query,
                &acceleration) == 0) {
            acceleration = query.fallback_acceleration;
        }
    }
    (void)p;
    return acceleration;
}

static FCasing89Vec3 fc89_step_vec(FCasing89Vec3 per_second, fc89_u16 dt_ms)
{
    FCasing89Vec3 out;
    out.x = fc89_time_step(per_second.x, dt_ms);
    out.y = fc89_time_step(per_second.y, dt_ms);
    out.z = fc89_time_step(per_second.z, dt_ms);
    return out;
}

static void fc89_emit_bounce_if_allowed(
    FCasing89System *sys,
    fc89_u16 idx,
    FCasing89Node *n,
    const FCasing89Profile *p,
    fc89_u8 collision_flags)
{
    fc89_i32 audio_dist;
    if ((collision_flags & FCASING89_COLLISION_NO_BOUNCE_EVENT) != 0U) {
        return;
    }
    if ((n->flags & FCASING89_FLAG_NO_BOUNCE_SOUND) != 0U
        || (n->flags & FCASING89_FLAG_SUPPRESSED_SOUND) != 0U) {
        return;
    }
    if (sys->camera.valid != 0U) {
        audio_dist = fcasing89_manhattan_units(sys->camera.pos, n->pos);
        if (audio_dist > (sys->cfg.audio_distance >> FCASING89_FIX_SHIFT)) {
            return;
        }
    }
    fc89_push_event(
        sys,
        FCASING89_EVENT_BOUNCE,
        p->audio_id,
        idx,
        n->pos,
        n->bounce_count);
}

static void fc89_sleep_node(FCasing89Node *n)
{
    n->state = FCASING89_STATE_SLEEP;
    n->vel.x = 0L;
    n->vel.y = 0L;
    n->vel.z = 0L;
    n->spin.x = n->spin.x / 5L;
    n->spin.y = n->spin.y / 5L;
    n->spin.z = n->spin.z / 5L;
}

static void fc89_resolve_collision(
    FCasing89System *sys,
    fc89_u16 idx,
    FCasing89Node *n,
    const FCasing89Profile *p,
    FCasing89ProviderCollisionResult *result)
{
    FCasing89Vec3 normal;
    FCasing89Vec3 normal_part;
    FCasing89Vec3 tangent_part;
    fc89_i32 impact_speed;
    fc89_i32 dot;

    n->pos = result->position;
    normal = result->normal;
    if (normal.x == 0L && normal.y == 0L && normal.z == 0L) {
        normal = fcasing89_vec3(0L, FCASING89_FIX_ONE, 0L);
    }

    dot = fcasing89_vec3_dot_q(n->vel, normal);
    impact_speed = fcasing89_abs_i32(dot);

    if ((result->flags & FCASING89_COLLISION_SLEEP) != 0U
        || n->bounce_count >= p->max_bounces
        || impact_speed < (fc89_i32)p->min_bounce_speed) {
        fc89_sleep_node(n);
        return;
    }

    if ((result->flags & FCASING89_COLLISION_VELOCITY_VALID) != 0U) {
        n->vel = result->velocity;
    } else {
        normal_part = fcasing89_vec3_scale_q(normal, dot);
        tangent_part.x = n->vel.x - normal_part.x;
        tangent_part.y = n->vel.y - normal_part.y;
        tangent_part.z = n->vel.z - normal_part.z;
        tangent_part = fcasing89_vec3_scale_q(tangent_part, p->friction_q);
        normal_part = fcasing89_vec3_scale_q(
            normal,
            fcasing89_fix_mul(-dot, p->bounce_q));
        n->vel = fcasing89_vec3_add(tangent_part, normal_part);
    }

    n->spin = fcasing89_vec3_scale_q(n->spin, p->friction_q);
    n->bounce_count++;
    fc89_emit_bounce_if_allowed(sys, idx, n, p, result->flags);
}

static int fc89_external_collision(
    FCasing89System *sys,
    fc89_u16 idx,
    FCasing89Node *n,
    const FCasing89Profile *p,
    FCasing89Vec3 old_position,
    fc89_u16 dt_ms)
{
    FCasing89ProviderCollisionQuery query;
    FCasing89ProviderCollisionResult result;

    if ((sys->provider.enabled_mask & FCASING89_PROVIDER_COLLISION) == 0UL
        || sys->provider.collision == 0) {
        return 0;
    }

    query.old_position = old_position;
    query.proposed_position = n->pos;
    query.velocity = n->vel;
    query.radius = p->radius;
    query.casing_index = idx;
    query.profile_id = n->profile_id;
    query.dt_ms = dt_ms;
    query.state = n->state;
    query.bounce_count = n->bounce_count;

    result.position = n->pos;
    result.normal = fcasing89_vec3(0L, 0L, 0L);
    result.velocity = n->vel;
    result.flags = 0U;
    result.reserved0 = 0U;
    result.reserved1 = 0U;

    if (sys->provider.collision(
            sys->provider.user,
            &query,
            &result) == 0) {
        return 0;
    }

    n->pos = result.position;
    if ((result.flags & FCASING89_COLLISION_HIT) != 0U) {
        fc89_resolve_collision(sys, idx, n, p, &result);
    }
    return 1;
}

static void fc89_make_velocity(FCasing89System *sys, const FCasing89Spawn *spawn, const FCasing89Profile *p, FCasing89Vec3 *out_vel, FCasing89Vec3 *out_spin)
{
    fc89_i32 side;
    fc89_i32 up;
    fc89_i32 back;
    fc89_i32 spin;
    FCasing89Vec3 v;
    side = fc89_rand_range(sys, p->side_min, p->side_max) + spawn->local_side_bias;
    up = fc89_rand_range(sys, p->up_min, p->up_max) + spawn->local_up_bias;
    back = fc89_rand_range(sys, p->back_min, p->back_max) + spawn->local_back_bias;
    if ((fc89_rng_next(sys) & 1UL) != 0UL) {
        side = -side;
    }
    v = fcasing89_vec3(0L, 0L, 0L);
    v = fcasing89_vec3_add_scaled_q(v, spawn->right, side);
    v = fcasing89_vec3_add_scaled_q(v, spawn->up, up);
    v = fcasing89_vec3_add_scaled_q(v, spawn->forward, -back);
    *out_vel = v;

    spin = fc89_rand_range(sys, p->spin_min, p->spin_max);
    out_spin->x = spin;
    out_spin->y = fc89_rand_range(sys, -spin, spin);
    out_spin->z = fc89_rand_range(sys, -spin, spin);
}

static void fc89_spawn_one(FCasing89System *sys, const FCasing89Spawn *spawn, fc89_u8 state)
{
    FCasing89Node *n;
    const FCasing89Profile *p;
    fc89_u16 idx;
    if (spawn->profile_id >= FCASING89_MAX_PROFILES) {
        return;
    }
    p = &sys->profiles[spawn->profile_id];
    idx = fc89_alloc_node(sys);
    if (idx == FCASING89_NULL_INDEX) {
        return;
    }
    n = &sys->nodes[idx];
    n->state = state;
    n->flags = spawn->flags;
    n->bounce_count = 0U;
    n->alpha = 255U;
    n->profile_id = spawn->profile_id;
    n->age_ms = 0U;
    n->sleep_age_ms = 0U;
    n->order = sys->order_counter++;
    n->importance = spawn->importance;
    n->reserved0 = 0U;
    n->pos = spawn->origin;
    n->rot.x = (fc89_i32)(fc89_rng_next(sys) & 1023UL) << FCASING89_FIX_SHIFT;
    n->rot.y = (fc89_i32)(fc89_rng_next(sys) & 1023UL) << FCASING89_FIX_SHIFT;
    n->rot.z = (fc89_i32)(fc89_rng_next(sys) & 1023UL) << FCASING89_FIX_SHIFT;
    if ((spawn->flags & FCASING89_FLAG_CUSTOM_SCALE) != 0U) {
        n->scale = fc89_apply_transform(
            sys,
            idx,
            spawn->profile_id,
            state,
            FCASING89_TRANSFORM_SCALE,
            fcasing89_vec3(
                FCASING89_FIX_ONE,
                FCASING89_FIX_ONE,
                FCASING89_FIX_ONE),
            spawn->scale,
            spawn->scale,
            0U);
    } else {
        n->scale = fc89_apply_transform(
            sys,
            idx,
            spawn->profile_id,
            state,
            FCASING89_TRANSFORM_SCALE,
            fcasing89_vec3(
                FCASING89_FIX_ONE,
                FCASING89_FIX_ONE,
                FCASING89_FIX_ONE),
            fcasing89_vec3(
                FCASING89_FIX_ONE,
                FCASING89_FIX_ONE,
                FCASING89_FIX_ONE),
            fcasing89_vec3(
                FCASING89_FIX_ONE,
                FCASING89_FIX_ONE,
                FCASING89_FIX_ONE),
            0U);
    }
    fc89_make_velocity(sys, spawn, p, &n->vel, &n->spin);
    if (state == FCASING89_STATE_FAKE) {
        n->vel.x = (n->vel.x * 70L) / 100L;
        n->vel.y = (n->vel.y * 70L) / 100L;
        n->vel.z = (n->vel.z * 70L) / 100L;
    }
    sys->stats.emitted_total++;
}

int fcasing89_emit(FCasing89System *sys, const FCasing89Spawn *spawn)
{
    const FCasing89Profile *p;
    fc89_u8 count;
    fc89_u8 state;
    fc89_u8 emitted;
    if (sys == 0 || spawn == 0) {
        return 0;
    }
    if (spawn->profile_id >= FCASING89_MAX_PROFILES) {
        return 0;
    }
    p = &sys->profiles[spawn->profile_id];
    state = fc89_choose_state(sys, spawn, p);
    if (state == FCASING89_STATE_FREE) {
        sys->stats.dropped_total++;
        return 0;
    }
    count = spawn->count;
    if (count == 0U) {
        count = p->count_per_emit;
    }
    if (count == 0U) {
        count = 1U;
    }
    if (count > sys->cfg.max_spawn_per_emit) {
        count = sys->cfg.max_spawn_per_emit;
    }
    emitted = 0U;
    while (count > 0U) {
        if (sys->frame_spawned >= sys->cfg.max_spawn_per_frame) {
            sys->stats.dropped_total++;
            fc89_push_event(sys, FCASING89_EVENT_BUDGET, 0U, FCASING89_NULL_INDEX, spawn->origin, 0U);
            break;
        }
        fc89_spawn_one(sys, spawn, state);
        sys->frame_spawned++;
        emitted++;
        count--;
    }
    return (int)emitted;
}

static void fc89_apply_alpha(FCasing89Node *n, const FCasing89Profile *p)
{
    fc89_u16 fade_begin;
    fc89_u16 fade_left;
    fc89_u16 fade_age;
    if (p->fade_ms == 0U || p->lifetime_ms <= p->fade_ms) {
        n->alpha = 255U;
        return;
    }
    fade_begin = (fc89_u16)(p->lifetime_ms - p->fade_ms);
    if (n->age_ms <= fade_begin) {
        n->alpha = 255U;
        return;
    }
    fade_age = (fc89_u16)(n->age_ms - fade_begin);
    if (fade_age >= p->fade_ms) {
        n->alpha = 0U;
        return;
    }
    fade_left = (fc89_u16)(p->fade_ms - fade_age);
    n->alpha = (fc89_u8)((255U * fade_left) / p->fade_ms);
}

static void fc89_update_motion(FCasing89System *sys, fc89_u16 idx, fc89_u16 dt_ms)
{
    FCasing89Node *n;
    const FCasing89Profile *p;
    FCasing89Vec3 acceleration;
    FCasing89Vec3 delta;
    FCasing89Vec3 old_position;
    FCasing89ProviderCollisionResult floor_result;
    fc89_i32 floor_y;
    fc89_i32 old_vy;

    n = &sys->nodes[idx];
    p = &sys->profiles[n->profile_id];
    n->age_ms = (fc89_u16)(n->age_ms + dt_ms);
    if (n->age_ms >= p->lifetime_ms) {
        fc89_release_node(sys, idx, FCASING89_EVENT_DESPAWN);
        return;
    }
    fc89_apply_alpha(n, p);

    delta = fc89_step_vec(n->spin, dt_ms);
    n->rot = fc89_apply_transform(
        sys,
        idx,
        n->profile_id,
        n->state,
        FCASING89_TRANSFORM_ROTATE,
        n->rot,
        delta,
        n->spin,
        dt_ms);

    if (n->state == FCASING89_STATE_SLEEP) {
        n->sleep_age_ms = (fc89_u16)(n->sleep_age_ms + dt_ms);
        if (n->sleep_age_ms >= p->sleep_ms) {
            fc89_release_node(sys, idx, FCASING89_EVENT_DESPAWN);
        }
        return;
    }

    if (n->state == FCASING89_STATE_FAKE) {
        acceleration = fc89_get_acceleration(
            sys,
            idx,
            n,
            p,
            p->gravity / 2L,
            dt_ms);
        delta = fc89_step_vec(acceleration, dt_ms);
        n->vel = fcasing89_vec3_add(n->vel, delta);
        delta = fc89_step_vec(n->vel, dt_ms);
        n->pos = fc89_apply_transform(
            sys,
            idx,
            n->profile_id,
            n->state,
            FCASING89_TRANSFORM_MOVE,
            n->pos,
            delta,
            n->vel,
            dt_ms);
        return;
    }

    old_vy = n->vel.y;
    acceleration = fc89_get_acceleration(
        sys,
        idx,
        n,
        p,
        p->gravity,
        dt_ms);
    delta = fc89_step_vec(acceleration, dt_ms);
    n->vel = fcasing89_vec3_add(n->vel, delta);

    old_position = n->pos;
    delta = fc89_step_vec(n->vel, dt_ms);
    n->pos = fc89_apply_transform(
        sys,
        idx,
        n->profile_id,
        n->state,
        FCASING89_TRANSFORM_MOVE,
        n->pos,
        delta,
        n->vel,
        dt_ms);

    if (fc89_external_collision(sys, idx, n, p, old_position, dt_ms) != 0) {
        return;
    }

    floor_y = p->floor_y;
    if (floor_y == 0L) {
        floor_y = sys->cfg.global_floor_y;
    }
    floor_y += p->radius;

    if (n->pos.y <= floor_y && old_vy <= 0L) {
        floor_result.position = n->pos;
        floor_result.position.y = floor_y;
        floor_result.normal = fcasing89_vec3(0L, FCASING89_FIX_ONE, 0L);
        floor_result.velocity = n->vel;
        floor_result.flags = FCASING89_COLLISION_HIT;
        floor_result.reserved0 = 0U;
        floor_result.reserved1 = 0U;
        fc89_resolve_collision(sys, idx, n, p, &floor_result);
    }
}

void fcasing89_update(FCasing89System *sys, fc89_u16 dt_ms)
{
    int i;
    fc89_u16 sim;
    fc89_u16 fake;
    fc89_u16 free_count;
    if (sys == 0) {
        return;
    }
    if (dt_ms == 0U) {
        return;
    }
    for (i = 0; i < FCASING89_MAX_CASINGS; i++) {
        if (sys->nodes[i].state != FCASING89_STATE_FREE) {
            fc89_update_motion(sys, (fc89_u16)i, dt_ms);
        }
    }
    sim = 0U;
    fake = 0U;
    free_count = 0U;
    for (i = 0; i < FCASING89_MAX_CASINGS; i++) {
        if (sys->nodes[i].state == FCASING89_STATE_FREE) {
            free_count++;
        } else if (sys->nodes[i].state == FCASING89_STATE_FAKE) {
            fake++;
        } else {
            sim++;
        }
    }
    sys->stats.active_sim = sim;
    sys->stats.active_fake = fake;
    sys->stats.active_total = (fc89_u16)(sim + fake);
    sys->stats.free_count = free_count;
}

int fcasing89_collect_render_items(const FCasing89System *sys, FCasing89RenderItem *out_items, int max_items)
{
    int i;
    int out_count;
    const FCasing89Node *n;
    const FCasing89Profile *p;
    if (sys == 0 || out_items == 0 || max_items <= 0) {
        return 0;
    }
    out_count = 0;
    for (i = 0; i < FCASING89_MAX_CASINGS; i++) {
        if (sys->nodes[i].state != FCASING89_STATE_FREE) {
            if (out_count >= max_items) {
                break;
            }
            n = &sys->nodes[i];
            p = &sys->profiles[n->profile_id];
            out_items[out_count].pos = n->pos;
            out_items[out_count].rot = n->rot;
            out_items[out_count].profile_id = n->profile_id;
            out_items[out_count].render_model_id = p->render_model_id;
            out_items[out_count].alpha = n->alpha;
            out_items[out_count].state = n->state;
            out_items[out_count].bounce_count = n->bounce_count;
            out_items[out_count].age_ms = n->age_ms;
            out_items[out_count].scale = n->scale;
            out_count++;
        }
    }
    return out_count;
}

int fcasing89_pop_event(FCasing89System *sys, FCasing89Event *out_event)
{
    if (sys == 0 || out_event == 0) {
        return 0;
    }
    if (sys->event_head == sys->event_tail) {
        out_event->type = FCASING89_EVENT_NONE;
        return 0;
    }
    *out_event = sys->events[sys->event_head];
    sys->events[sys->event_head].type = FCASING89_EVENT_NONE;
    sys->event_head = (fc89_u16)((sys->event_head + 1U) % FCASING89_MAX_EVENTS);
    return 1;
}

void fcasing89_get_stats(const FCasing89System *sys, FCasing89Stats *out_stats)
{
    if (sys == 0 || out_stats == 0) {
        return;
    }
    *out_stats = sys->stats;
}
