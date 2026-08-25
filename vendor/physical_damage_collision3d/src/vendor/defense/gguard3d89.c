#include "gguard3d89.h"

static int ggrd_clampi(int v, int lo, int hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

GGRD_Fix ggrd_fix_mul(GGRD_Fix a, GGRD_Fix b)
{
    return (GGRD_Fix)((a * b) / GGRD_FIX_ONE);
}

int ggrd_vec3_dot_fix(const GGRD_Vec3 *a, const GGRD_Vec3 *b)
{
    GGRD_Fix r;
    r = ggrd_fix_mul(a->x, b->x) + ggrd_fix_mul(a->y, b->y) + ggrd_fix_mul(a->z, b->z);
    return (int)r;
}

void ggrd_default_profile(GGRD_Profile *p)
{
    if (!p) return;
    p->raise_frames = 3;
    p->drop_frames = 4;
    p->min_hold_frames = 2;
    p->break_recovery_frames = 45;
    p->max_guard = 1000;
    p->guard_regen_idle = 4;
    p->guard_regen_held = 1;
    p->guard_drain_held = 1;
    p->guard_damage_mul = 1024;
    p->chip_mul = 80;
    p->coverage_cos = 128;
    p->lane_mask = GGRD_FLAG_LOW | GGRD_FLAG_HIGH;
}

void ggrd_init(GGRD_Context *ctx)
{
    int i;
    if (!ctx) return;
    ctx->event_fn = 0;
    ctx->event_user = 0;
    for (i = 0; i < GGUARD3D89_MAX_ACTORS; ++i) {
        ctx->actors[i].used = 0;
        ctx->actors[i].actor_id = 0;
        ctx->actors[i].state = GGRD_STATE_IDLE;
        ctx->actors[i].guard_value = 0;
        ctx->actors[i].state_timer = 0;
        ctx->actors[i].hold_frames = 0;
        ctx->actors[i].facing_dir.x = GGRD_FIX_ONE;
        ctx->actors[i].facing_dir.y = 0;
        ctx->actors[i].facing_dir.z = 0;
        ggrd_default_profile(&ctx->actors[i].profile);
    }
}

void ggrd_set_event_callback(GGRD_Context *ctx, GGRD_EventFn fn, void *user)
{
    if (!ctx) return;
    ctx->event_fn = fn;
    ctx->event_user = user;
}

static void ggrd_emit(GGRD_Context *ctx, int actor_id, int other_id, int event_code, int amount)
{
    if (ctx && ctx->event_fn) ctx->event_fn(actor_id, other_id, event_code, amount, ctx->event_user);
}

static GGRD_Actor *ggrd_find(GGRD_Context *ctx, int actor_id)
{
    int i;
    if (!ctx) return 0;
    for (i = 0; i < GGUARD3D89_MAX_ACTORS; ++i) {
        if (ctx->actors[i].used && ctx->actors[i].actor_id == actor_id) return &ctx->actors[i];
    }
    return 0;
}

static const GGRD_Actor *ggrd_find_const(const GGRD_Context *ctx, int actor_id)
{
    int i;
    if (!ctx) return 0;
    for (i = 0; i < GGUARD3D89_MAX_ACTORS; ++i) {
        if (ctx->actors[i].used && ctx->actors[i].actor_id == actor_id) return &ctx->actors[i];
    }
    return 0;
}

int ggrd_create_actor(GGRD_Context *ctx, int actor_id, const GGRD_Profile *profile)
{
    int i;
    GGRD_Profile defp;
    if (!ctx) return GGRD_FALSE;
    if (ggrd_find(ctx, actor_id)) return GGRD_TRUE;
    ggrd_default_profile(&defp);
    for (i = 0; i < GGUARD3D89_MAX_ACTORS; ++i) {
        if (!ctx->actors[i].used) {
            ctx->actors[i].used = 1;
            ctx->actors[i].actor_id = actor_id;
            ctx->actors[i].profile = profile ? *profile : defp;
            ctx->actors[i].state = GGRD_STATE_IDLE;
            ctx->actors[i].guard_value = ctx->actors[i].profile.max_guard;
            ctx->actors[i].state_timer = 0;
            ctx->actors[i].hold_frames = 0;
            ctx->actors[i].facing_dir.x = GGRD_FIX_ONE;
            ctx->actors[i].facing_dir.y = 0;
            ctx->actors[i].facing_dir.z = 0;
            return GGRD_TRUE;
        }
    }
    return GGRD_FALSE;
}

int ggrd_remove_actor(GGRD_Context *ctx, int actor_id)
{
    GGRD_Actor *a;
    a = ggrd_find(ctx, actor_id);
    if (!a) return GGRD_FALSE;
    a->used = 0;
    return GGRD_TRUE;
}

int ggrd_set_facing(GGRD_Context *ctx, int actor_id, GGRD_Fix x, GGRD_Fix y, GGRD_Fix z)
{
    GGRD_Actor *a;
    a = ggrd_find(ctx, actor_id);
    if (!a) return GGRD_FALSE;
    a->facing_dir.x = x;
    a->facing_dir.y = y;
    a->facing_dir.z = z;
    return GGRD_TRUE;
}

int ggrd_set_guard_value(GGRD_Context *ctx, int actor_id, int value)
{
    GGRD_Actor *a;
    a = ggrd_find(ctx, actor_id);
    if (!a) return GGRD_FALSE;
    a->guard_value = ggrd_clampi(value, 0, a->profile.max_guard);
    return GGRD_TRUE;
}

static void ggrd_step_actor(GGRD_Context *ctx, GGRD_Actor *a, int frames, int hold_input)
{
    int was_state;
    if (!ctx || !a || frames <= 0) return;
    was_state = a->state;

    if (a->state == GGRD_STATE_BROKEN) {
        a->state_timer -= frames;
        if (a->state_timer <= 0) {
            a->state = GGRD_STATE_IDLE;
            a->state_timer = 0;
        }
        return;
    }

    if (hold_input) {
        if (a->state == GGRD_STATE_IDLE || a->state == GGRD_STATE_DROP) {
            a->state = GGRD_STATE_RAISE;
            a->state_timer = a->profile.raise_frames;
            ggrd_emit(ctx, a->actor_id, 0, GGRD_EVENT_RAISE, 0);
        } else if (a->state == GGRD_STATE_RAISE) {
            a->state_timer -= frames;
            if (a->state_timer <= 0) {
                a->state = GGRD_STATE_HELD;
                a->state_timer = 0;
                ggrd_emit(ctx, a->actor_id, 0, GGRD_EVENT_HELD, 0);
            }
        } else if (a->state == GGRD_STATE_HELD) {
            a->hold_frames += frames;
        }
        if (a->state == GGRD_STATE_HELD || a->state == GGRD_STATE_RAISE) {
            a->guard_value -= a->profile.guard_drain_held * frames;
            a->guard_value += a->profile.guard_regen_held * frames;
        }
    } else {
        if (a->state == GGRD_STATE_HELD || a->state == GGRD_STATE_RAISE) {
            if (a->hold_frames >= a->profile.min_hold_frames || a->state == GGRD_STATE_RAISE) {
                a->state = GGRD_STATE_DROP;
                a->state_timer = a->profile.drop_frames;
                a->hold_frames = 0;
                ggrd_emit(ctx, a->actor_id, 0, GGRD_EVENT_DROP, 0);
            }
        } else if (a->state == GGRD_STATE_DROP) {
            a->state_timer -= frames;
            if (a->state_timer <= 0) {
                a->state = GGRD_STATE_IDLE;
                a->state_timer = 0;
            }
        }
        if (a->state == GGRD_STATE_IDLE || a->state == GGRD_STATE_DROP) {
            a->guard_value += a->profile.guard_regen_idle * frames;
        }
    }

    a->guard_value = ggrd_clampi(a->guard_value, 0, a->profile.max_guard);
    if (a->guard_value <= 0 && was_state != GGRD_STATE_BROKEN) {
        a->state = GGRD_STATE_BROKEN;
        a->state_timer = a->profile.break_recovery_frames;
        ggrd_emit(ctx, a->actor_id, 0, GGRD_EVENT_BREAK, 0);
    }
}

void ggrd_update(GGRD_Context *ctx, int frames, int actor_id, int hold_input)
{
    GGRD_Actor *a;
    a = ggrd_find(ctx, actor_id);
    if (!a) return;
    ggrd_step_actor(ctx, a, frames, hold_input);
}

void ggrd_update_all(GGRD_Context *ctx, int frames)
{
    int i;
    if (!ctx || frames <= 0) return;
    for (i = 0; i < GGUARD3D89_MAX_ACTORS; ++i) {
        if (ctx->actors[i].used) ggrd_step_actor(ctx, &ctx->actors[i], frames, 0);
    }
}

static int ggrd_lane_ok(const GGRD_Actor *a, const GGRD_Attack *atk)
{
    unsigned int lane;
    lane = atk->flags & (GGRD_FLAG_LOW | GGRD_FLAG_HIGH);
    if (lane == 0) return 1;
    return ((a->profile.lane_mask & lane) != 0) ? 1 : 0;
}

int ggrd_try_guard(GGRD_Context *ctx, int actor_id, const GGRD_Attack *atk, GGRD_Result *out_result)
{
    GGRD_Actor *a;
    int dot;
    int dir_ok;
    int gdmg;
    int chip;
    if (out_result) {
        out_result->accepted = 0;
        out_result->guarded = 0;
        out_result->guard_broken = 0;
        out_result->damage_to_apply = atk ? atk->damage : 0;
        out_result->guard_damage_taken = 0;
        out_result->blockstun_frames = 0;
        out_result->state_after = GGRD_STATE_IDLE;
        out_result->dot = 0;
    }
    if (!ctx || !atk) return GGRD_FALSE;
    a = ggrd_find(ctx, actor_id);
    if (!a) return GGRD_FALSE;

    dot = ggrd_vec3_dot_fix(&a->facing_dir, &atk->source_dir);
    if (out_result) out_result->dot = dot;

    dir_ok = dot >= a->profile.coverage_cos;
    if (a->state != GGRD_STATE_HELD || !dir_ok || !ggrd_lane_ok(a, atk) || (atk->flags & GGRD_FLAG_UNGUARDABLE) != 0) {
        if (out_result) out_result->state_after = a->state;
        return GGRD_FALSE;
    }

    gdmg = (atk->guard_damage * a->profile.guard_damage_mul) / 1024;
    if ((atk->flags & GGRD_FLAG_HEAVY) != 0) gdmg += atk->guard_damage / 2;
    if (gdmg < 1) gdmg = 1;
    a->guard_value -= gdmg;

    if (a->guard_value <= 0) {
        a->guard_value = 0;
        a->state = GGRD_STATE_BROKEN;
        a->state_timer = a->profile.break_recovery_frames;
        if (out_result) {
            out_result->accepted = 1;
            out_result->guarded = 0;
            out_result->guard_broken = 1;
            out_result->damage_to_apply = atk->damage;
            out_result->guard_damage_taken = gdmg;
            out_result->blockstun_frames = atk->blockstun_frames;
            out_result->state_after = a->state;
        }
        ggrd_emit(ctx, actor_id, atk->attacker_id, GGRD_EVENT_BREAK, gdmg);
        return GGRD_TRUE;
    }

    chip = (atk->damage * a->profile.chip_mul) / 1024;
    if (out_result) {
        out_result->accepted = 1;
        out_result->guarded = 1;
        out_result->guard_broken = 0;
        out_result->damage_to_apply = chip;
        out_result->guard_damage_taken = gdmg;
        out_result->blockstun_frames = atk->blockstun_frames;
        out_result->state_after = a->state;
    }
    ggrd_emit(ctx, actor_id, atk->attacker_id, GGRD_EVENT_ABSORB, gdmg);
    return GGRD_TRUE;
}

int ggrd_get_state(const GGRD_Context *ctx, int actor_id)
{
    const GGRD_Actor *a;
    a = ggrd_find_const(ctx, actor_id);
    if (!a) return GGRD_STATE_IDLE;
    return a->state;
}

int ggrd_get_guard_value(const GGRD_Context *ctx, int actor_id)
{
    const GGRD_Actor *a;
    a = ggrd_find_const(ctx, actor_id);
    if (!a) return 0;
    return a->guard_value;
}
