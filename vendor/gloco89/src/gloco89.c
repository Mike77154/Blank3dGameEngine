#include "gloco89.h"

static GLOCO_FX gloco_ms_to_fx(GLOCO_U16 ms)
{
    return (GLOCO_FX)(((unsigned long)ms * (unsigned long)GLOCO_FX_ONE) / 1000ul);
}

GLOCO_FX gloco_fx_from_int(int v)
{
    return ((GLOCO_FX)v) * GLOCO_FX_ONE;
}

int gloco_fx_to_int(GLOCO_FX v)
{
    if (v >= 0) return (int)((v + GLOCO_FX_HALF) / GLOCO_FX_ONE);
    return (int)((v - GLOCO_FX_HALF) / GLOCO_FX_ONE);
}

GLOCO_FX gloco_fx_mul(GLOCO_FX a, GLOCO_FX b)
{
    /* Q8.8 multiply. No long long: keep gameplay units modest. */
    return (GLOCO_FX)((a * b) / GLOCO_FX_ONE);
}

GLOCO_FX gloco_fx_div(GLOCO_FX a, GLOCO_FX b)
{
    if (b == 0) return 0;
    return (GLOCO_FX)((a * GLOCO_FX_ONE) / b);
}

GLOCO_FX gloco_fx_abs(GLOCO_FX v)
{
    return (v < 0) ? -v : v;
}

GLOCO_FX gloco_fx_clamp(GLOCO_FX v, GLOCO_FX lo, GLOCO_FX hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

GLOCO_Vec3 gloco_v3(GLOCO_FX x, GLOCO_FX y, GLOCO_FX z)
{
    GLOCO_Vec3 v;
    v.x = x;
    v.y = y;
    v.z = z;
    return v;
}

GLOCO_Vec3 gloco_v3_add(GLOCO_Vec3 a, GLOCO_Vec3 b)
{
    GLOCO_Vec3 r;
    r.x = a.x + b.x;
    r.y = a.y + b.y;
    r.z = a.z + b.z;
    return r;
}

GLOCO_Vec3 gloco_v3_sub(GLOCO_Vec3 a, GLOCO_Vec3 b)
{
    GLOCO_Vec3 r;
    r.x = a.x - b.x;
    r.y = a.y - b.y;
    r.z = a.z - b.z;
    return r;
}

GLOCO_Vec3 gloco_v3_scale(GLOCO_Vec3 a, GLOCO_FX s)
{
    GLOCO_Vec3 r;
    r.x = gloco_fx_mul(a.x, s);
    r.y = gloco_fx_mul(a.y, s);
    r.z = gloco_fx_mul(a.z, s);
    return r;
}

GLOCO_FX gloco_v3_dot(GLOCO_Vec3 a, GLOCO_Vec3 b)
{
    return gloco_fx_mul(a.x, b.x) + gloco_fx_mul(a.y, b.y) + gloco_fx_mul(a.z, b.z);
}

static void gloco_sort3(GLOCO_FX *a, GLOCO_FX *b, GLOCO_FX *c)
{
    GLOCO_FX t;
    if (*a < *b) { t = *a; *a = *b; *b = t; }
    if (*b < *c) { t = *b; *b = *c; *c = t; }
    if (*a < *b) { t = *a; *a = *b; *b = t; }
}

GLOCO_FX gloco_v3_len_approx(GLOCO_Vec3 a)
{
    GLOCO_FX x;
    GLOCO_FX y;
    GLOCO_FX z;
    x = gloco_fx_abs(a.x);
    y = gloco_fx_abs(a.y);
    z = gloco_fx_abs(a.z);
    gloco_sort3(&x, &y, &z);
    return x + (y / 2L) + (z / 4L);
}

GLOCO_Vec3 gloco_v3_norm_approx(GLOCO_Vec3 a)
{
    GLOCO_FX len;
    len = gloco_v3_len_approx(a);
    if (len <= 2L) return gloco_v3(0, 0, 0);
    return gloco_v3(gloco_fx_div(a.x, len), gloco_fx_div(a.y, len), gloco_fx_div(a.z, len));
}

GLOCO_Vec3 gloco_v3_move_towards(GLOCO_Vec3 cur, GLOCO_Vec3 target, GLOCO_FX max_delta)
{
    GLOCO_Vec3 d;
    GLOCO_FX len;
    if (max_delta <= 0) return cur;
    d = gloco_v3_sub(target, cur);
    len = gloco_v3_len_approx(d);
    if (len <= max_delta || len <= 0) return target;
    d = gloco_v3_scale(gloco_v3_norm_approx(d), max_delta);
    return gloco_v3_add(cur, d);
}

static void gloco_zero_actor(GLOCO_Actor *a)
{
    a->alive = 0;
    a->profile_id = 0;
    a->state = GLOCO_STATE_IDLE;
    a->prev_state = GLOCO_STATE_IDLE;
    a->flags = 0;
    a->pos = gloco_v3(0, 0, 0);
    a->vel = gloco_v3(0, 0, 0);
    a->facing = gloco_v3(0, 0, GLOCO_FX_ONE);
    a->ground_normal = gloco_v3(0, GLOCO_FX_ONE, 0);
    a->evade_timer_ms = 0;
    a->evade_recover_ms = 0;
    a->slide_timer_ms = 0;
    a->evade_consumed = 0;
    a->step_side = 0;
    a->hard_stop_latched = 0;
    a->stamina = 0;
    a->stride_phase = 0;
    a->last_ground_y = 0;
}

void gloco_movement_provider_init(GLOCO_MovementProvider *provider)
{
    if (!provider) return;
    provider->user = 0;
    provider->horizontal = 0;
    provider->vertical = 0;
    provider->integrate = 0;
    provider->impulse = 0;
    provider->stop = 0;
}

void gloco_physics_provider_init(GLOCO_PhysicsProvider *provider)
{
    if (!provider) return;
    provider->user = 0;
    provider->move = 0;
    provider->actor_create = 0;
    provider->actor_destroy = 0;
    provider->teleport = 0;
    provider->post_move = 0;
}

void gloco_stats_provider_init(GLOCO_StatsProvider *provider)
{
    if (!provider) return;
    provider->user = 0;
    provider->get_stamina = 0;
    provider->set_stamina = 0;
    provider->consume_stamina = 0;
}

void gloco_init(GLOCO_Context *ctx)
{
    int i;
    if (!ctx) return;
    for (i = 0; i < GLOCO_MAX_ACTORS; ++i) gloco_zero_actor(&ctx->actors[i]);
    for (i = 0; i < GLOCO_MAX_PROFILES; ++i) {
        ctx->profile_alive[i] = 0;
        gloco_profile_defaults(&ctx->profiles[i], GLOCO_PROFILE_DEFAULT);
    }
    ctx->world_probe = 0;
    ctx->event_fn = 0;
    ctx->user = 0;
    gloco_movement_provider_init(&ctx->movement_provider);
    gloco_physics_provider_init(&ctx->physics_provider);
    gloco_stats_provider_init(&ctx->stats_provider);
    gloco_profile_defaults(&ctx->profiles[0], GLOCO_PROFILE_DEFAULT);
    ctx->profile_alive[0] = 1;
}

void gloco_set_callbacks(GLOCO_Context *ctx, GLOCO_WorldProbeFn probe, GLOCO_EventFn event_fn, void *user)
{
    if (!ctx) return;
    ctx->world_probe = probe;
    ctx->event_fn = event_fn;
    ctx->user = user;
}

void gloco_set_movement_provider(GLOCO_Context *ctx, const GLOCO_MovementProvider *provider)
{
    if (!ctx) return;
    if (provider) ctx->movement_provider = *provider;
    else gloco_movement_provider_init(&ctx->movement_provider);
}

void gloco_set_physics_provider(GLOCO_Context *ctx, const GLOCO_PhysicsProvider *provider)
{
    int i;
    if (!ctx) return;

    if (ctx->physics_provider.actor_destroy) {
        for (i = 0; i < GLOCO_MAX_ACTORS; ++i) {
            if (ctx->actors[i].alive) {
                ctx->physics_provider.actor_destroy(ctx->physics_provider.user,
                                                    i,
                                                    &ctx->actors[i],
                                                    &ctx->profiles[ctx->actors[i].profile_id]);
            }
        }
    }

    if (provider) ctx->physics_provider = *provider;
    else gloco_physics_provider_init(&ctx->physics_provider);

    if (ctx->physics_provider.actor_create) {
        for (i = 0; i < GLOCO_MAX_ACTORS; ++i) {
            if (ctx->actors[i].alive) {
                ctx->physics_provider.actor_create(ctx->physics_provider.user,
                                                   i,
                                                   &ctx->actors[i],
                                                   &ctx->profiles[ctx->actors[i].profile_id]);
            }
        }
    }
}

void gloco_set_stats_provider(GLOCO_Context *ctx, const GLOCO_StatsProvider *provider)
{
    int i;
    GLOCO_FX value;
    if (!ctx) return;
    if (provider) ctx->stats_provider = *provider;
    else gloco_stats_provider_init(&ctx->stats_provider);
    if (ctx->stats_provider.get_stamina) {
        for (i = 0; i < GLOCO_MAX_ACTORS; ++i) {
            if (!ctx->actors[i].alive) continue;
            value = ctx->actors[i].stamina;
            if (ctx->stats_provider.get_stamina(ctx->stats_provider.user,
                    i, &value) == GLOCO_PROVIDER_HANDLED)
                ctx->actors[i].stamina = value;
        }
    }
}

static GLOCO_FX gloco_stats_get_stamina(GLOCO_Context *ctx, int actor_id,
                                         GLOCO_Actor *actor)
{
    GLOCO_FX value;
    if (!ctx || !actor) return 0;
    value = actor->stamina;
    if (ctx->stats_provider.get_stamina &&
        ctx->stats_provider.get_stamina(ctx->stats_provider.user,
            actor_id, &value) == GLOCO_PROVIDER_HANDLED)
        actor->stamina = value;
    return actor->stamina;
}

static void gloco_stats_set_stamina(GLOCO_Context *ctx, int actor_id,
                                     GLOCO_Actor *actor, GLOCO_FX value)
{
    if (!ctx || !actor) return;
    actor->stamina = value;
    if (ctx->stats_provider.set_stamina &&
        ctx->stats_provider.set_stamina(ctx->stats_provider.user,
            actor_id, value) == GLOCO_PROVIDER_HANDLED)
        (void)gloco_stats_get_stamina(ctx, actor_id, actor);
}

static void gloco_stats_consume_stamina(GLOCO_Context *ctx, int actor_id,
                                         GLOCO_Actor *actor, GLOCO_FX amount)
{
    GLOCO_FX value;
    if (!ctx || !actor || amount <= 0) return;
    value = actor->stamina - amount;
    if (value < 0) value = 0;
    if (ctx->stats_provider.consume_stamina &&
        ctx->stats_provider.consume_stamina(ctx->stats_provider.user,
            actor_id, amount, &value) == GLOCO_PROVIDER_HANDLED) {
        actor->stamina = value;
        return;
    }
    gloco_stats_set_stamina(ctx, actor_id, actor, value);
}

static GLOCO_FX fxi(int v)
{
    return gloco_fx_from_int(v);
}

void gloco_profile_defaults(GLOCO_Profile *p, int preset)
{
    if (!p) return;

    p->walk_speed = fxi(2);
    p->run_speed = fxi(5);
    p->sprint_speed = fxi(7);
    p->aim_speed = fxi(2);
    p->crouch_speed = fxi(1);

    p->acceleration = fxi(18);
    p->sprint_acceleration = fxi(12);
    p->braking = fxi(20);
    p->hard_braking = fxi(34);
    p->ground_friction = fxi(6);
    p->air_control = GLOCO_FX_ONE / 4L;

    p->side_scale = (GLOCO_FX)(GLOCO_FX_ONE * 82L / 100L);
    p->back_scale = (GLOCO_FX)(GLOCO_FX_ONE * 65L / 100L);
    p->aim_side_scale = (GLOCO_FX)(GLOCO_FX_ONE * 72L / 100L);

    p->turn_rate = fxi(8);
    p->yaw_lag = GLOCO_FX_ONE / 3L;

    p->gravity = fxi(24);
    p->terminal_fall = -fxi(42);
    p->ground_snap = GLOCO_FX_ONE / 3L;
    p->max_slope_dot = (GLOCO_FX)(GLOCO_FX_ONE * 70L / 100L);
    p->capsule_radius = GLOCO_FX_ONE / 2L;
    p->capsule_height = fxi(2);

    p->evade_speed = fxi(9);
    p->evade_active_ms = 230;
    p->evade_recover_ms = 170;
    p->evade_friction = fxi(5);
    p->evade_control = GLOCO_FX_ONE / 8L;
    p->evade_stamina_cost = fxi(16);

    p->slide_min_speed = fxi(6);
    p->slide_friction = fxi(4);
    p->slide_ms = 420;

    p->stamina_max = fxi(100);
    p->stamina_sprint_drain = fxi(18);
    p->stamina_recover = fxi(24);
    p->stamina_min_sprint = fxi(12);

    p->stride_walk = fxi(1);
    p->stride_run = fxi(3);
    p->stride_sprint = fxi(4);

    if (preset == GLOCO_PROFILE_TACTICAL) {
        p->run_speed = fxi(4);
        p->sprint_speed = fxi(6);
        p->acceleration = fxi(14);
        p->braking = fxi(23);
        p->ground_friction = fxi(7);
        p->side_scale = (GLOCO_FX)(GLOCO_FX_ONE * 78L / 100L);
        p->back_scale = (GLOCO_FX)(GLOCO_FX_ONE * 58L / 100L);
        p->evade_speed = fxi(8);
        p->evade_active_ms = 210;
    } else if (preset == GLOCO_PROFILE_ARCADE) {
        p->run_speed = fxi(6);
        p->sprint_speed = fxi(9);
        p->acceleration = fxi(28);
        p->sprint_acceleration = fxi(22);
        p->braking = fxi(30);
        p->ground_friction = fxi(4);
        p->side_scale = GLOCO_FX_ONE;
        p->back_scale = (GLOCO_FX)(GLOCO_FX_ONE * 80L / 100L);
        p->evade_speed = fxi(11);
        p->evade_active_ms = 190;
    } else if (preset == GLOCO_PROFILE_HEAVY) {
        p->walk_speed = fxi(1);
        p->run_speed = fxi(4);
        p->sprint_speed = fxi(5);
        p->acceleration = fxi(9);
        p->sprint_acceleration = fxi(8);
        p->braking = fxi(16);
        p->ground_friction = fxi(9);
        p->side_scale = (GLOCO_FX)(GLOCO_FX_ONE * 68L / 100L);
        p->back_scale = (GLOCO_FX)(GLOCO_FX_ONE * 50L / 100L);
        p->evade_speed = fxi(6);
        p->evade_active_ms = 270;
        p->evade_recover_ms = 260;
    }
}

int gloco_set_profile(GLOCO_Context *ctx, int profile_id, const GLOCO_Profile *profile)
{
    if (!ctx || !profile) return GLOCO_ERR_BAD_PROFILE;
    if (profile_id < 0 || profile_id >= GLOCO_MAX_PROFILES) return GLOCO_ERR_BAD_PROFILE;
    ctx->profiles[profile_id] = *profile;
    ctx->profile_alive[profile_id] = 1;
    return GLOCO_OK;
}

int gloco_actor_create(GLOCO_Context *ctx, int profile_id, const GLOCO_Vec3 *pos, const GLOCO_Vec3 *facing)
{
    int i;
    if (!ctx) return GLOCO_ERR_FULL;
    if (profile_id < 0 || profile_id >= GLOCO_MAX_PROFILES || !ctx->profile_alive[profile_id]) return GLOCO_ERR_BAD_PROFILE;
    for (i = 0; i < GLOCO_MAX_ACTORS; ++i) {
        if (!ctx->actors[i].alive) {
            gloco_zero_actor(&ctx->actors[i]);
            ctx->actors[i].alive = 1;
            ctx->actors[i].profile_id = (GLOCO_U8)profile_id;
            ctx->actors[i].pos = pos ? *pos : gloco_v3(0, 0, 0);
            ctx->actors[i].facing = facing ? gloco_v3_norm_approx(*facing) : gloco_v3(0, 0, GLOCO_FX_ONE);
            if (gloco_v3_len_approx(ctx->actors[i].facing) <= 0) ctx->actors[i].facing = gloco_v3(0, 0, GLOCO_FX_ONE);
            ctx->actors[i].ground_normal = gloco_v3(0, GLOCO_FX_ONE, 0);
            ctx->actors[i].stamina = ctx->profiles[profile_id].stamina_max;
            gloco_stats_set_stamina(ctx, i, &ctx->actors[i],
                                    ctx->actors[i].stamina);
            if (ctx->physics_provider.actor_create) {
                ctx->physics_provider.actor_create(ctx->physics_provider.user,
                                                   i,
                                                   &ctx->actors[i],
                                                   &ctx->profiles[profile_id]);
            }
            return i;
        }
    }
    return GLOCO_ERR_FULL;
}

int gloco_actor_destroy(GLOCO_Context *ctx, int actor_id)
{
    GLOCO_Actor *a;
    if (!ctx || actor_id < 0 || actor_id >= GLOCO_MAX_ACTORS) return GLOCO_ERR_BAD_ID;
    a = &ctx->actors[actor_id];
    if (a->alive && ctx->physics_provider.actor_destroy) {
        ctx->physics_provider.actor_destroy(ctx->physics_provider.user,
                                            actor_id,
                                            a,
                                            &ctx->profiles[a->profile_id]);
    }
    gloco_zero_actor(a);
    return GLOCO_OK;
}

GLOCO_Actor *gloco_actor_get(GLOCO_Context *ctx, int actor_id)
{
    if (!ctx || actor_id < 0 || actor_id >= GLOCO_MAX_ACTORS) return 0;
    if (!ctx->actors[actor_id].alive) return 0;
    return &ctx->actors[actor_id];
}

const GLOCO_Actor *gloco_actor_get_const(const GLOCO_Context *ctx, int actor_id)
{
    if (!ctx || actor_id < 0 || actor_id >= GLOCO_MAX_ACTORS) return 0;
    if (!ctx->actors[actor_id].alive) return 0;
    return &ctx->actors[actor_id];
}

int gloco_actor_set_profile(GLOCO_Context *ctx, int actor_id, int profile_id)
{
    GLOCO_Actor *a;
    if (!ctx || profile_id < 0 || profile_id >= GLOCO_MAX_PROFILES || !ctx->profile_alive[profile_id]) return GLOCO_ERR_BAD_PROFILE;
    a = gloco_actor_get(ctx, actor_id);
    if (!a) return GLOCO_ERR_BAD_ID;
    a->profile_id = (GLOCO_U8)profile_id;
    if (gloco_stats_get_stamina(ctx, actor_id, a) >
        ctx->profiles[profile_id].stamina_max)
        gloco_stats_set_stamina(ctx, actor_id, a,
            ctx->profiles[profile_id].stamina_max);
    return GLOCO_OK;
}

int gloco_actor_teleport(GLOCO_Context *ctx, int actor_id, const GLOCO_Vec3 *pos)
{
    GLOCO_Actor *a;
    a = gloco_actor_get(ctx, actor_id);
    if (!a || !pos) return GLOCO_ERR_BAD_ID;
    a->pos = *pos;
    a->vel = gloco_v3(0, 0, 0);
    if (ctx->physics_provider.teleport) {
        ctx->physics_provider.teleport(ctx->physics_provider.user,
                                       actor_id,
                                       a,
                                       pos);
    }
    return GLOCO_OK;
}

int gloco_actor_stop(GLOCO_Context *ctx, int actor_id)
{
    GLOCO_Actor *a;
    a = gloco_actor_get(ctx, actor_id);
    if (!a) return GLOCO_ERR_BAD_ID;
    a->vel = gloco_v3(0, 0, 0);
    a->state = GLOCO_STATE_IDLE;
    if (ctx->movement_provider.stop) {
        ctx->movement_provider.stop(ctx->movement_provider.user, actor_id, a);
    }
    return GLOCO_OK;
}

static void gloco_emit(GLOCO_Context *ctx, int actor_id, int event_id, int value)
{
    if (ctx && ctx->event_fn) ctx->event_fn(ctx->user, actor_id, event_id, value);
}

static GLOCO_Vec3 gloco_flatten_norm(GLOCO_Vec3 v)
{
    v.y = 0;
    return gloco_v3_norm_approx(v);
}

static GLOCO_Vec3 gloco_make_desired_dir(const GLOCO_Input *in)
{
    GLOCO_Vec3 f;
    GLOCO_Vec3 r;
    GLOCO_Vec3 d;
    GLOCO_FX sx;
    GLOCO_FX sz;
    if (!in) return gloco_v3(0, 0, 0);
    f = gloco_flatten_norm(in->basis_fwd);
    r = gloco_flatten_norm(in->basis_right);
    sx = (GLOCO_FX)in->move_x;
    sz = (GLOCO_FX)in->move_z;
    d = gloco_v3_add(gloco_v3_scale(r, sx), gloco_v3_scale(f, sz));
    return gloco_flatten_norm(d);
}

static GLOCO_Vec3 gloco_make_evade_dir(const GLOCO_Actor *a, const GLOCO_Input *in)
{
    GLOCO_Vec3 f;
    GLOCO_Vec3 r;
    GLOCO_Vec3 d;
    GLOCO_FX sx;
    GLOCO_FX sz;
    if (!in) return a->facing;
    f = gloco_flatten_norm(in->basis_fwd);
    r = gloco_flatten_norm(in->basis_right);
    sx = (GLOCO_FX)in->evade_x;
    sz = (GLOCO_FX)in->evade_z;
    if (sx == 0 && sz == 0) {
        sx = (GLOCO_FX)in->move_x;
        sz = (GLOCO_FX)in->move_z;
    }
    d = gloco_v3_add(gloco_v3_scale(r, sx), gloco_v3_scale(f, sz));
    d = gloco_flatten_norm(d);
    if (gloco_v3_len_approx(d) <= 0) d = a->facing;
    return gloco_flatten_norm(d);
}

static GLOCO_FX gloco_axis_scale(const GLOCO_Profile *p, const GLOCO_Input *in, GLOCO_U16 buttons)
{
    GLOCO_FX scale;
    GLOCO_FX absx;
    GLOCO_FX absz;
    scale = GLOCO_FX_ONE;
    absx = gloco_fx_abs((GLOCO_FX)in->move_x);
    absz = gloco_fx_abs((GLOCO_FX)in->move_z);
    if (in->move_z < 0) scale = gloco_fx_mul(scale, p->back_scale);
    if (absx > absz) scale = gloco_fx_mul(scale, p->side_scale);
    if ((buttons & GLOCO_INPUT_AIM) != 0u) scale = gloco_fx_mul(scale, p->aim_side_scale);
    return scale;
}

static GLOCO_FX gloco_pick_speed(const GLOCO_Profile *p, GLOCO_Actor *a, const GLOCO_Input *in)
{
    GLOCO_U16 b;
    GLOCO_FX s;
    b = in ? in->buttons : 0u;
    if ((b & GLOCO_INPUT_CROUCH) != 0u) s = p->crouch_speed;
    else if ((b & GLOCO_INPUT_AIM) != 0u) s = p->aim_speed;
    else if ((b & GLOCO_INPUT_SPRINT) != 0u && a->stamina >= p->stamina_min_sprint && in && in->move_z > 0) s = p->sprint_speed;
    else if ((b & GLOCO_INPUT_WALK) != 0u) s = p->walk_speed;
    else if ((b & GLOCO_INPUT_RUN) != 0u) s = p->run_speed;
    else s = p->run_speed;
    if (in && in->speed_override > 0) s = in->speed_override;
    if (in) s = gloco_fx_mul(s, gloco_axis_scale(p, in, b));
    return s;
}

static void gloco_apply_friction(GLOCO_Actor *a, const GLOCO_Profile *p, GLOCO_FX dt, GLOCO_FX friction)
{
    GLOCO_FX damp;
    GLOCO_Vec3 h;
    damp = gloco_fx_mul(friction, dt);
    damp = gloco_fx_clamp(damp, 0, GLOCO_FX_ONE);
    h = gloco_v3(a->vel.x, 0, a->vel.z);
    h = gloco_v3_scale(h, GLOCO_FX_ONE - damp);
    a->vel.x = h.x;
    a->vel.z = h.z;
    (void)p;
}

static void gloco_apply_stamina(GLOCO_Context *ctx, int actor_id,
                                 GLOCO_Actor *a, const GLOCO_Profile *p,
                                 GLOCO_U16 buttons, GLOCO_FX dt,
                                 int has_input)
{
    GLOCO_FX delta;
    GLOCO_FX value;
    value = gloco_stats_get_stamina(ctx, actor_id, a);
    if ((buttons & GLOCO_INPUT_SPRINT) != 0u && has_input &&
        a->state == GLOCO_STATE_SPRINT) {
        delta = gloco_fx_mul(p->stamina_sprint_drain, dt);
        gloco_stats_consume_stamina(ctx, actor_id, a, delta);
    } else {
        delta = gloco_fx_mul(p->stamina_recover, dt);
        value += delta;
        if (value > p->stamina_max) value = p->stamina_max;
        gloco_stats_set_stamina(ctx, actor_id, a, value);
    }
}

static void gloco_step_foot_cycle(GLOCO_Context *ctx, int id, GLOCO_Actor *a, const GLOCO_Profile *p, GLOCO_FX dt)
{
    GLOCO_FX hspeed;
    GLOCO_FX add;
    GLOCO_FX stride;
    GLOCO_FX old;
    hspeed = gloco_v3_len_approx(gloco_v3(a->vel.x, 0, a->vel.z));
    if (hspeed < GLOCO_FX_ONE / 5L || (a->flags & GLOCO_FLAG_GROUNDED) == 0u) return;
    if (a->state == GLOCO_STATE_SPRINT) stride = p->stride_sprint;
    else if (a->state == GLOCO_STATE_RUN) stride = p->stride_run;
    else stride = p->stride_walk;
    add = gloco_fx_mul(gloco_fx_mul(hspeed, stride), dt);
    old = a->stride_phase;
    a->stride_phase += add;
    while (a->stride_phase >= GLOCO_FX_ONE) a->stride_phase -= GLOCO_FX_ONE;
    if ((old < GLOCO_FX_HALF && a->stride_phase >= GLOCO_FX_HALF) || a->stride_phase < old) {
        if (a->step_side == 0) {
            a->flags |= GLOCO_FLAG_STEP_LEFT;
            gloco_emit(ctx, id, GLOCO_EVENT_STEP_L, 0);
            a->step_side = 1;
        } else {
            a->flags |= GLOCO_FLAG_STEP_RIGHT;
            gloco_emit(ctx, id, GLOCO_EVENT_STEP_R, 0);
            a->step_side = 0;
        }
    }
}

static void gloco_default_probe(const GLOCO_Vec3 *from, const GLOCO_Vec3 *to, GLOCO_ProbeResult *out_result)
{
    (void)from;
    out_result->corrected_pos = *to;
    out_result->ground_normal = gloco_v3(0, GLOCO_FX_ONE, 0);
    out_result->flags = 0;
    if (out_result->corrected_pos.y <= 0) {
        out_result->corrected_pos.y = 0;
        out_result->flags |= GLOCO_FLAG_GROUNDED;
    }
}

static void gloco_change_state(GLOCO_Context *ctx, int id, GLOCO_Actor *a, int new_state)
{
    if (a->state != (GLOCO_U8)new_state) {
        a->prev_state = a->state;
        a->state = (GLOCO_U8)new_state;
        gloco_emit(ctx, id, GLOCO_EVENT_STATE, new_state);
    }
}

static void gloco_tick_timers(GLOCO_Context *ctx, int id, GLOCO_Actor *a, GLOCO_U16 dt_ms)
{
    if (a->evade_timer_ms > 0) {
        if (dt_ms >= a->evade_timer_ms) {
            a->evade_timer_ms = 0;
            a->evade_recover_ms = 1;
            gloco_emit(ctx, id, GLOCO_EVENT_EVADE_OFF, 0);
        } else {
            a->evade_timer_ms = (GLOCO_U16)(a->evade_timer_ms - dt_ms);
        }
    } else if (a->evade_recover_ms > 0) {
        if (dt_ms >= a->evade_recover_ms) a->evade_recover_ms = 0;
        else a->evade_recover_ms = (GLOCO_U16)(a->evade_recover_ms - dt_ms);
    }

    if (a->slide_timer_ms > 0) {
        if (dt_ms >= a->slide_timer_ms) a->slide_timer_ms = 0;
        else a->slide_timer_ms = (GLOCO_U16)(a->slide_timer_ms - dt_ms);
    }
}

static int gloco_can_start_evade(const GLOCO_Actor *a, const GLOCO_Profile *p, GLOCO_U16 buttons)
{
    if ((buttons & GLOCO_INPUT_EVADE) == 0u) return 0;
    if (a->evade_timer_ms > 0 || a->evade_recover_ms > 0) return 0;
    if (a->stamina < p->evade_stamina_cost) return 0;
    if ((a->flags & GLOCO_FLAG_GROUNDED) == 0u) return 0;
    return 1;
}

static void gloco_start_evade(GLOCO_Context *ctx, int id, GLOCO_Actor *a, const GLOCO_Profile *p, const GLOCO_Input *in)
{
    GLOCO_Vec3 dir;
    GLOCO_Vec3 builtin_velocity;
    int handled;
    dir = gloco_make_evade_dir(a, in);
    builtin_velocity = a->vel;
    builtin_velocity.x = gloco_fx_mul(dir.x, p->evade_speed);
    builtin_velocity.z = gloco_fx_mul(dir.z, p->evade_speed);
    handled = GLOCO_PROVIDER_FALLBACK;
    if (ctx->movement_provider.impulse) {
        handled = ctx->movement_provider.impulse(ctx->movement_provider.user,
                                                 id,
                                                 a,
                                                 GLOCO_IMPULSE_EVADE,
                                                 &builtin_velocity);
    }
    if (handled != GLOCO_PROVIDER_HANDLED) {
        a->vel.x = builtin_velocity.x;
        a->vel.z = builtin_velocity.z;
    }
    a->evade_timer_ms = p->evade_active_ms;
    a->evade_recover_ms = p->evade_recover_ms;
    gloco_stats_consume_stamina(ctx, id, a, p->evade_stamina_cost);
    gloco_change_state(ctx, id, a, GLOCO_STATE_EVADE);
    a->flags |= GLOCO_FLAG_EVADING;
    gloco_emit(ctx, id, GLOCO_EVENT_EVADE_ON, 0);
}

static void gloco_maybe_start_slide(GLOCO_Context *ctx, int id, GLOCO_Actor *a, const GLOCO_Profile *p, GLOCO_U16 buttons)
{
    GLOCO_FX hspeed;
    if ((buttons & GLOCO_INPUT_SLIDE) == 0u) return;
    if (a->slide_timer_ms > 0) return;
    if ((a->flags & GLOCO_FLAG_GROUNDED) == 0u) return;
    hspeed = gloco_v3_len_approx(gloco_v3(a->vel.x, 0, a->vel.z));
    if (hspeed >= p->slide_min_speed) {
        a->slide_timer_ms = p->slide_ms;
        gloco_change_state(ctx, id, a, GLOCO_STATE_SLIDE);
        a->flags |= GLOCO_FLAG_SLIDING;
    }
}

static void gloco_build_motion_intent(const GLOCO_Actor *a,
                                      const GLOCO_Profile *p,
                                      const GLOCO_Input *input,
                                      GLOCO_U16 buttons,
                                      int has_input,
                                      GLOCO_Vec3 desired_dir,
                                      GLOCO_MotionIntent *intent)
{
    GLOCO_FX speed;
    GLOCO_FX accel;
    intent->desired_dir = desired_dir;
    intent->target_velocity = gloco_v3(0, a->vel.y, 0);
    intent->target_speed = 0;
    intent->acceleration = 0;
    intent->friction = 0;
    intent->turn_rate = p->turn_rate;
    intent->buttons = buttons;
    intent->has_input = (GLOCO_U8)(has_input ? 1 : 0);
    intent->mode = GLOCO_MOTION_MODE_MOVE;

    if (a->evade_timer_ms > 0) {
        intent->mode = GLOCO_MOTION_MODE_EVADE;
        intent->friction = p->evade_friction;
        if (has_input) {
            speed = gloco_fx_mul(gloco_pick_speed(p, (GLOCO_Actor *)a, input), p->evade_control);
            intent->target_speed = speed;
            intent->target_velocity = gloco_v3_scale(desired_dir, speed);
            intent->target_velocity.y = a->vel.y;
            intent->acceleration = gloco_fx_mul(p->acceleration, p->evade_control);
        }
    } else if (a->slide_timer_ms > 0) {
        intent->mode = GLOCO_MOTION_MODE_SLIDE;
        intent->friction = p->slide_friction;
        intent->target_velocity = a->vel;
    } else if (has_input) {
        speed = gloco_pick_speed(p, (GLOCO_Actor *)a, input);
        accel = ((buttons & GLOCO_INPUT_SPRINT) != 0u) ? p->sprint_acceleration : p->acceleration;
        if ((a->flags & GLOCO_FLAG_GROUNDED) == 0u) accel = gloco_fx_mul(accel, p->air_control);
        intent->mode = GLOCO_MOTION_MODE_MOVE;
        intent->target_speed = speed;
        intent->target_velocity = gloco_v3_scale(desired_dir, speed);
        intent->target_velocity.y = a->vel.y;
        intent->acceleration = accel;
    } else if ((buttons & GLOCO_INPUT_HARD_STOP) != 0u) {
        intent->mode = GLOCO_MOTION_MODE_HARD_STOP;
        intent->acceleration = p->hard_braking;
    } else {
        intent->mode = GLOCO_MOTION_MODE_BRAKE;
        intent->acceleration = p->braking;
        intent->friction = p->ground_friction;
    }
}

static void gloco_apply_provider_motion_state(GLOCO_Context *ctx,
                                               int actor_id,
                                               GLOCO_Actor *a,
                                               const GLOCO_Input *input,
                                               GLOCO_U16 buttons,
                                               int has_input)
{
    if (a->evade_timer_ms > 0) {
        a->flags |= GLOCO_FLAG_EVADING;
        gloco_change_state(ctx, actor_id, a, GLOCO_STATE_EVADE);
    } else if (a->slide_timer_ms > 0) {
        a->flags |= GLOCO_FLAG_SLIDING;
        gloco_change_state(ctx, actor_id, a, GLOCO_STATE_SLIDE);
    } else if (has_input) {
        if ((buttons & GLOCO_INPUT_AIM) != 0u) {
            a->flags |= GLOCO_FLAG_AIMING;
            gloco_change_state(ctx, actor_id, a, GLOCO_STATE_AIM_MOVE);
        } else if ((buttons & GLOCO_INPUT_SPRINT) != 0u && a->stamina > 0 && input && input->move_z > 0) {
            a->flags |= GLOCO_FLAG_SPRINTING;
            gloco_change_state(ctx, actor_id, a, GLOCO_STATE_SPRINT);
        } else if ((buttons & GLOCO_INPUT_WALK) != 0u) {
            gloco_change_state(ctx, actor_id, a, GLOCO_STATE_WALK);
        } else {
            gloco_change_state(ctx, actor_id, a, GLOCO_STATE_RUN);
        }
    } else {
        if (gloco_v3_len_approx(gloco_v3(a->vel.x, 0, a->vel.z)) < GLOCO_FX_ONE / 8L) {
            a->vel.x = 0;
            a->vel.z = 0;
            gloco_change_state(ctx, actor_id, a, GLOCO_STATE_IDLE);
        }
    }
}

static void gloco_update_hard_stop_latch(GLOCO_Context *ctx, int actor_id,
                                         GLOCO_Actor *actor,
                                         GLOCO_U16 buttons)
{
    int pressed;
    pressed = (buttons & GLOCO_INPUT_HARD_STOP) != 0u;
    if (pressed && !actor->hard_stop_latched)
        gloco_emit(ctx, actor_id, GLOCO_EVENT_HARD_STOP, 0);
    actor->hard_stop_latched = (GLOCO_U8)(pressed ? 1 : 0);
}

int gloco_update_actor(GLOCO_Context *ctx, int actor_id, const GLOCO_Input *input, GLOCO_U16 dt_ms)
{
    GLOCO_Actor *a;
    const GLOCO_Profile *p;
    GLOCO_FX dt;
    GLOCO_Vec3 desired_dir;
    GLOCO_Vec3 target;
    GLOCO_Vec3 from;
    GLOCO_Vec3 to;
    GLOCO_ProbeResult pr;
    GLOCO_FX speed;
    GLOCO_FX accel;
    GLOCO_FX max_delta;
    GLOCO_U16 buttons;
    GLOCO_MotionIntent intent;
    int has_input;
    int was_grounded;
    int handled;

    if (!ctx || actor_id < 0 || actor_id >= GLOCO_MAX_ACTORS) return GLOCO_ERR_BAD_ID;
    a = &ctx->actors[actor_id];
    if (!a->alive) return GLOCO_ERR_BAD_ID;
    p = &ctx->profiles[a->profile_id];
    dt = gloco_ms_to_fx(dt_ms);
    buttons = input ? input->buttons : 0u;
    (void)gloco_stats_get_stamina(ctx, actor_id, a);
    gloco_update_hard_stop_latch(ctx, actor_id, a, buttons);
    has_input = (input && (input->move_x != 0 || input->move_z != 0));
    was_grounded = ((a->flags & GLOCO_FLAG_GROUNDED) != 0u);

    a->flags &= (GLOCO_U16)~(GLOCO_FLAG_STEP_LEFT | GLOCO_FLAG_STEP_RIGHT | GLOCO_FLAG_BLOCKED | GLOCO_FLAG_STEEP | GLOCO_FLAG_SPRINTING | GLOCO_FLAG_AIMING | GLOCO_FLAG_EVADING | GLOCO_FLAG_SLIDING);

    gloco_tick_timers(ctx, actor_id, a, dt_ms);

    if (gloco_can_start_evade(a, p, buttons)) {
        gloco_start_evade(ctx, actor_id, a, p, input);
    }

    gloco_maybe_start_slide(ctx, actor_id, a, p, buttons);

    desired_dir = gloco_make_desired_dir(input);
    gloco_build_motion_intent(a, p, input, buttons, has_input, desired_dir, &intent);

    handled = GLOCO_PROVIDER_FALLBACK;
    if (ctx->movement_provider.horizontal) {
        handled = ctx->movement_provider.horizontal(ctx->movement_provider.user,
                                                    actor_id,
                                                    a,
                                                    p,
                                                    input,
                                                    &intent,
                                                    dt_ms);
    }

    if (handled == GLOCO_PROVIDER_HANDLED) {
        gloco_apply_provider_motion_state(ctx, actor_id, a, input, buttons, has_input);
    } else {
        if (a->evade_timer_ms > 0) {
            gloco_apply_friction(a, p, dt, p->evade_friction);
            a->flags |= GLOCO_FLAG_EVADING;
            gloco_change_state(ctx, actor_id, a, GLOCO_STATE_EVADE);
            if (has_input) {
                target = gloco_v3_scale(desired_dir, gloco_fx_mul(gloco_pick_speed(p, a, input), p->evade_control));
                max_delta = gloco_fx_mul(gloco_fx_mul(p->acceleration, p->evade_control), dt);
                a->vel = gloco_v3_move_towards(a->vel, gloco_v3(target.x, a->vel.y, target.z), max_delta);
            }
        } else if (a->slide_timer_ms > 0) {
            gloco_apply_friction(a, p, dt, p->slide_friction);
            a->flags |= GLOCO_FLAG_SLIDING;
            gloco_change_state(ctx, actor_id, a, GLOCO_STATE_SLIDE);
        } else {
            if (has_input) {
                speed = gloco_pick_speed(p, a, input);
                target = gloco_v3_scale(desired_dir, speed);
                accel = ((buttons & GLOCO_INPUT_SPRINT) != 0u) ? p->sprint_acceleration : p->acceleration;
                if ((a->flags & GLOCO_FLAG_GROUNDED) == 0u) accel = gloco_fx_mul(accel, p->air_control);
                max_delta = gloco_fx_mul(accel, dt);
                a->vel = gloco_v3_move_towards(a->vel, gloco_v3(target.x, a->vel.y, target.z), max_delta);
                a->facing = gloco_v3_move_towards(a->facing, desired_dir, gloco_fx_mul(p->turn_rate, dt));
                a->facing = gloco_flatten_norm(a->facing);
                if ((buttons & GLOCO_INPUT_AIM) != 0u) {
                    a->flags |= GLOCO_FLAG_AIMING;
                    gloco_change_state(ctx, actor_id, a, GLOCO_STATE_AIM_MOVE);
                } else if ((buttons & GLOCO_INPUT_SPRINT) != 0u && a->stamina > 0 && input && input->move_z > 0) {
                    a->flags |= GLOCO_FLAG_SPRINTING;
                    gloco_change_state(ctx, actor_id, a, GLOCO_STATE_SPRINT);
                } else if ((buttons & GLOCO_INPUT_WALK) != 0u) {
                    gloco_change_state(ctx, actor_id, a, GLOCO_STATE_WALK);
                } else {
                    gloco_change_state(ctx, actor_id, a, GLOCO_STATE_RUN);
                }
            } else {
                if ((buttons & GLOCO_INPUT_HARD_STOP) != 0u) {
                    a->vel = gloco_v3_move_towards(a->vel, gloco_v3(0, a->vel.y, 0), gloco_fx_mul(p->hard_braking, dt));
                } else {
                    a->vel = gloco_v3_move_towards(a->vel, gloco_v3(0, a->vel.y, 0), gloco_fx_mul(p->braking, dt));
                    gloco_apply_friction(a, p, dt, p->ground_friction);
                }
                if (gloco_v3_len_approx(gloco_v3(a->vel.x, 0, a->vel.z)) < GLOCO_FX_ONE / 8L) {
                    a->vel.x = 0;
                    a->vel.z = 0;
                    gloco_change_state(ctx, actor_id, a, GLOCO_STATE_IDLE);
                }
            }
        }

    }

    handled = GLOCO_PROVIDER_FALLBACK;
    if (ctx->movement_provider.vertical) {
        handled = ctx->movement_provider.vertical(ctx->movement_provider.user,
                                                  actor_id,
                                                  a,
                                                  p,
                                                  dt_ms);
    }
    if (handled != GLOCO_PROVIDER_HANDLED) {
        if ((a->flags & GLOCO_FLAG_GROUNDED) == 0u) {
            a->vel.y -= gloco_fx_mul(p->gravity, dt);
            if (a->vel.y < p->terminal_fall) a->vel.y = p->terminal_fall;
        } else {
            if (a->vel.y < 0) a->vel.y = 0;
        }
    }

    from = a->pos;
    to = gloco_v3_add(a->pos, gloco_v3_scale(a->vel, dt));
    if (ctx->movement_provider.integrate) {
        GLOCO_Vec3 provider_to;
        provider_to = to;
        handled = ctx->movement_provider.integrate(ctx->movement_provider.user,
                                                   actor_id,
                                                   a,
                                                   dt_ms,
                                                   &from,
                                                   &to,
                                                   &provider_to);
        if (handled == GLOCO_PROVIDER_HANDLED) to = provider_to;
    }
    pr.corrected_pos = to;
    pr.ground_normal = a->ground_normal;
    pr.flags = 0;
    handled = GLOCO_PROVIDER_FALLBACK;
    if (ctx->physics_provider.move) {
        handled = ctx->physics_provider.move(ctx->physics_provider.user,
                                             actor_id,
                                             a,
                                             p,
                                             dt_ms,
                                             &from,
                                             &to,
                                             &pr);
    }
    if (handled != GLOCO_PROVIDER_HANDLED) {
        if (ctx->world_probe) {
            ctx->world_probe(ctx->user, &from, &to, p->capsule_radius, p->capsule_height, &pr);
        } else {
            gloco_default_probe(&from, &to, &pr);
        }
    }

    a->pos = pr.corrected_pos;
    a->ground_normal = gloco_v3_norm_approx(pr.ground_normal);
    if (gloco_v3_len_approx(a->ground_normal) <= 0) a->ground_normal = gloco_v3(0, GLOCO_FX_ONE, 0);

    if ((pr.flags & GLOCO_FLAG_GROUNDED) != 0u) {
        a->flags |= GLOCO_FLAG_GROUNDED;
        if (!was_grounded) gloco_emit(ctx, actor_id, GLOCO_EVENT_LAND, 0);
        a->last_ground_y = a->pos.y;
    } else {
        a->flags &= (GLOCO_U16)~GLOCO_FLAG_GROUNDED;
        if (a->state != GLOCO_STATE_EVADE && a->state != GLOCO_STATE_SLIDE) gloco_change_state(ctx, actor_id, a, GLOCO_STATE_AIR);
    }
    if ((pr.flags & GLOCO_FLAG_BLOCKED) != 0u) a->flags |= GLOCO_FLAG_BLOCKED;
    if ((pr.flags & GLOCO_FLAG_STEEP) != 0u) a->flags |= GLOCO_FLAG_STEEP;

    if (ctx->physics_provider.post_move) {
        ctx->physics_provider.post_move(ctx->physics_provider.user,
                                        actor_id,
                                        a,
                                        p,
                                        &pr);
    }

    gloco_apply_stamina(ctx, actor_id, a, p, buttons, dt, has_input);
    gloco_step_foot_cycle(ctx, actor_id, a, p, dt);
    return GLOCO_OK;
}

void gloco_update_all(GLOCO_Context *ctx, const GLOCO_Input *inputs, GLOCO_U16 dt_ms)
{
    int i;
    if (!ctx) return;
    for (i = 0; i < GLOCO_MAX_ACTORS; ++i) {
        if (ctx->actors[i].alive) gloco_update_actor(ctx, i, inputs ? &inputs[i] : 0, dt_ms);
    }
}

const char *gloco_state_name(int state_id)
{
    switch (state_id) {
    case GLOCO_STATE_IDLE: return "idle";
    case GLOCO_STATE_WALK: return "walk";
    case GLOCO_STATE_RUN: return "run";
    case GLOCO_STATE_SPRINT: return "sprint";
    case GLOCO_STATE_AIM_MOVE: return "aim_move";
    case GLOCO_STATE_EVADE: return "evade";
    case GLOCO_STATE_SLIDE: return "slide";
    case GLOCO_STATE_AIR: return "air";
    default: return "unknown";
    }
}
