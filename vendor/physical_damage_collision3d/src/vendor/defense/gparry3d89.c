#include "gparry3d89.h"

GPRY_Fix gpry_fix_mul(GPRY_Fix a, GPRY_Fix b)
{
    return (GPRY_Fix)((a * b) / GPRY_FIX_ONE);
}

int gpry_vec3_dot_fix(const GPRY_Vec3 *a, const GPRY_Vec3 *b)
{
    GPRY_Fix r;
    r = gpry_fix_mul(a->x, b->x) + gpry_fix_mul(a->y, b->y) + gpry_fix_mul(a->z, b->z);
    return (int)r;
}

void gpry_default_profile(GPRY_Profile *p)
{
    if (!p) return;
    p->startup_frames = 1;
    p->perfect_frames = 2;
    p->normal_frames = 4;
    p->late_frames = 3;
    p->recovery_frames = 14;
    p->cooldown_frames = 4;
    p->coverage_cos = 128;
    p->posture_reward_perfect = 120;
    p->posture_reward_normal = 70;
    p->attacker_stagger_perfect = 24;
    p->attacker_stagger_normal = 12;
    p->anti_spam_limit = 3;
    p->anti_spam_extra_recovery = 8;
    p->parry_mask = GPRY_FLAG_THRUST | GPRY_FLAG_PROJECTILE;
}

void gpry_init(GPRY_Context *ctx)
{
    int i;
    if (!ctx) return;
    ctx->event_fn = 0;
    ctx->event_user = 0;
    for (i = 0; i < GPARRY3D89_MAX_ACTORS; ++i) {
        ctx->actors[i].used = 0;
        ctx->actors[i].actor_id = 0;
        ctx->actors[i].state = GPRY_STATE_IDLE;
        ctx->actors[i].timer = 0;
        ctx->actors[i].age_frames = 0;
        ctx->actors[i].spam_count = 0;
        ctx->actors[i].since_last_press = 999;
        ctx->actors[i].facing_dir.x = GPRY_FIX_ONE;
        ctx->actors[i].facing_dir.y = 0;
        ctx->actors[i].facing_dir.z = 0;
        gpry_default_profile(&ctx->actors[i].profile);
    }
}

void gpry_set_event_callback(GPRY_Context *ctx, GPRY_EventFn fn, void *user)
{
    if (!ctx) return;
    ctx->event_fn = fn;
    ctx->event_user = user;
}

static void gpry_emit(GPRY_Context *ctx, int actor_id, int other_id, int event_code, int amount)
{
    if (ctx && ctx->event_fn) ctx->event_fn(actor_id, other_id, event_code, amount, ctx->event_user);
}

static GPRY_Actor *gpry_find(GPRY_Context *ctx, int actor_id)
{
    int i;
    if (!ctx) return 0;
    for (i = 0; i < GPARRY3D89_MAX_ACTORS; ++i) {
        if (ctx->actors[i].used && ctx->actors[i].actor_id == actor_id) return &ctx->actors[i];
    }
    return 0;
}

static const GPRY_Actor *gpry_find_const(const GPRY_Context *ctx, int actor_id)
{
    int i;
    if (!ctx) return 0;
    for (i = 0; i < GPARRY3D89_MAX_ACTORS; ++i) {
        if (ctx->actors[i].used && ctx->actors[i].actor_id == actor_id) return &ctx->actors[i];
    }
    return 0;
}

int gpry_create_actor(GPRY_Context *ctx, int actor_id, const GPRY_Profile *profile)
{
    int i;
    GPRY_Profile defp;
    if (!ctx) return GPRY_FALSE;
    if (gpry_find(ctx, actor_id)) return GPRY_TRUE;
    gpry_default_profile(&defp);
    for (i = 0; i < GPARRY3D89_MAX_ACTORS; ++i) {
        if (!ctx->actors[i].used) {
            ctx->actors[i].used = 1;
            ctx->actors[i].actor_id = actor_id;
            ctx->actors[i].state = GPRY_STATE_IDLE;
            ctx->actors[i].timer = 0;
            ctx->actors[i].age_frames = 0;
            ctx->actors[i].spam_count = 0;
            ctx->actors[i].since_last_press = 999;
            ctx->actors[i].facing_dir.x = GPRY_FIX_ONE;
            ctx->actors[i].facing_dir.y = 0;
            ctx->actors[i].facing_dir.z = 0;
            ctx->actors[i].profile = profile ? *profile : defp;
            return GPRY_TRUE;
        }
    }
    return GPRY_FALSE;
}

int gpry_remove_actor(GPRY_Context *ctx, int actor_id)
{
    GPRY_Actor *a;
    a = gpry_find(ctx, actor_id);
    if (!a) return GPRY_FALSE;
    a->used = 0;
    return GPRY_TRUE;
}

int gpry_set_facing(GPRY_Context *ctx, int actor_id, GPRY_Fix x, GPRY_Fix y, GPRY_Fix z)
{
    GPRY_Actor *a;
    a = gpry_find(ctx, actor_id);
    if (!a) return GPRY_FALSE;
    a->facing_dir.x = x;
    a->facing_dir.y = y;
    a->facing_dir.z = z;
    return GPRY_TRUE;
}

int gpry_press(GPRY_Context *ctx, int actor_id)
{
    GPRY_Actor *a;
    int extra;
    a = gpry_find(ctx, actor_id);
    if (!a) return GPRY_FALSE;
    if (a->state == GPRY_STATE_RECOVERY || a->state == GPRY_STATE_COOLDOWN) return GPRY_FALSE;

    if (a->since_last_press < 10) a->spam_count++;
    else if (a->spam_count > 0) a->spam_count--;

    extra = 0;
    if (a->spam_count > a->profile.anti_spam_limit) {
        extra = a->profile.anti_spam_extra_recovery;
        gpry_emit(ctx, actor_id, 0, GPRY_EVENT_SPAM_PENALTY, extra);
    }

    a->state = GPRY_STATE_STARTUP;
    a->timer = a->profile.startup_frames + extra;
    a->age_frames = 0;
    a->since_last_press = 0;
    gpry_emit(ctx, actor_id, 0, GPRY_EVENT_PRESS, 0);
    return GPRY_TRUE;
}

void gpry_update(GPRY_Context *ctx, int frames)
{
    int i;
    GPRY_Actor *a;
    if (!ctx || frames <= 0) return;
    for (i = 0; i < GPARRY3D89_MAX_ACTORS; ++i) {
        a = &ctx->actors[i];
        if (!a->used) continue;
        a->since_last_press += frames;
        if (a->since_last_press > 999) a->since_last_press = 999;
        if (a->state == GPRY_STATE_IDLE) continue;
        a->timer -= frames;
        a->age_frames += frames;
        while (a->timer <= 0 && a->state != GPRY_STATE_IDLE) {
            if (a->state == GPRY_STATE_STARTUP) {
                a->state = GPRY_STATE_ACTIVE;
                a->timer += a->profile.perfect_frames + a->profile.normal_frames + a->profile.late_frames;
            } else if (a->state == GPRY_STATE_ACTIVE) {
                a->state = GPRY_STATE_RECOVERY;
                a->timer += a->profile.recovery_frames;
            } else if (a->state == GPRY_STATE_RECOVERY) {
                a->state = GPRY_STATE_COOLDOWN;
                a->timer += a->profile.cooldown_frames;
            } else if (a->state == GPRY_STATE_COOLDOWN) {
                a->state = GPRY_STATE_IDLE;
                a->timer = 0;
            } else {
                a->state = GPRY_STATE_IDLE;
                a->timer = 0;
            }
        }
    }
}

static int gpry_attack_type_ok(const GPRY_Actor *a, const GPRY_Attack *atk)
{
    unsigned int t;
    t = atk->flags & (GPRY_FLAG_THRUST | GPRY_FLAG_PROJECTILE);
    if (t == 0) return 1;
    return ((a->profile.parry_mask & t) != 0) ? 1 : 0;
}

static int gpry_window_code(const GPRY_Actor *a)
{
    int active_age;
    if (a->state != GPRY_STATE_ACTIVE) return GPRY_RESULT_FAIL;
    active_age = a->age_frames - a->profile.startup_frames;
    if (active_age < 0) return GPRY_RESULT_FAIL;
    if (active_age < a->profile.perfect_frames) return GPRY_RESULT_PERFECT;
    active_age -= a->profile.perfect_frames;
    if (active_age < a->profile.normal_frames) return GPRY_RESULT_NORMAL;
    active_age -= a->profile.normal_frames;
    if (active_age < a->profile.late_frames) return GPRY_RESULT_LATE;
    return GPRY_RESULT_FAIL;
}

int gpry_try_parry(GPRY_Context *ctx, int actor_id, const GPRY_Attack *atk, GPRY_Result *out_result)
{
    GPRY_Actor *a;
    int dot;
    int code;
    if (out_result) {
        out_result->accepted = 0;
        out_result->result_code = GPRY_RESULT_FAIL;
        out_result->damage_to_apply = atk ? atk->damage : 0;
        out_result->defender_posture_delta = 0;
        out_result->attacker_stagger_frames = 0;
        out_result->recovery_frames = 0;
        out_result->dot = 0;
    }
    if (!ctx || !atk) return GPRY_FALSE;
    a = gpry_find(ctx, actor_id);
    if (!a) return GPRY_FALSE;
    dot = gpry_vec3_dot_fix(&a->facing_dir, &atk->source_dir);
    if (out_result) out_result->dot = dot;

    if ((atk->flags & GPRY_FLAG_UNPARRYABLE) != 0 || dot < a->profile.coverage_cos || !gpry_attack_type_ok(a, atk)) {
        gpry_emit(ctx, actor_id, atk->attacker_id, GPRY_EVENT_FAIL, atk->damage);
        return GPRY_FALSE;
    }

    code = gpry_window_code(a);
    if (code == GPRY_RESULT_FAIL) {
        gpry_emit(ctx, actor_id, atk->attacker_id, GPRY_EVENT_FAIL, atk->damage);
        return GPRY_FALSE;
    }

    if (out_result) {
        out_result->accepted = 1;
        out_result->result_code = code;
        out_result->dot = dot;
    }

    if (code == GPRY_RESULT_PERFECT) {
        if (out_result) {
            out_result->damage_to_apply = 0;
            out_result->defender_posture_delta = a->profile.posture_reward_perfect;
            out_result->attacker_stagger_frames = a->profile.attacker_stagger_perfect;
            out_result->recovery_frames = 1;
        }
        a->state = GPRY_STATE_COOLDOWN;
        a->timer = a->profile.cooldown_frames;
        gpry_emit(ctx, actor_id, atk->attacker_id, GPRY_EVENT_PERFECT, a->profile.attacker_stagger_perfect);
    } else if (code == GPRY_RESULT_NORMAL) {
        if (out_result) {
            out_result->damage_to_apply = 0;
            out_result->defender_posture_delta = a->profile.posture_reward_normal;
            out_result->attacker_stagger_frames = a->profile.attacker_stagger_normal;
            out_result->recovery_frames = a->profile.recovery_frames / 2;
        }
        a->state = GPRY_STATE_RECOVERY;
        a->timer = a->profile.recovery_frames / 2;
        gpry_emit(ctx, actor_id, atk->attacker_id, GPRY_EVENT_NORMAL, a->profile.attacker_stagger_normal);
    } else {
        if (out_result) {
            out_result->damage_to_apply = atk->damage / 4;
            out_result->defender_posture_delta = 0 - (atk->posture_damage / 2);
            out_result->attacker_stagger_frames = 0;
            out_result->recovery_frames = a->profile.recovery_frames;
        }
        a->state = GPRY_STATE_RECOVERY;
        a->timer = a->profile.recovery_frames;
        gpry_emit(ctx, actor_id, atk->attacker_id, GPRY_EVENT_LATE, atk->damage / 4);
    }
    return GPRY_TRUE;
}

int gpry_get_state(const GPRY_Context *ctx, int actor_id)
{
    const GPRY_Actor *a;
    a = gpry_find_const(ctx, actor_id);
    if (!a) return GPRY_STATE_IDLE;
    return a->state;
}

int gpry_get_age(const GPRY_Context *ctx, int actor_id)
{
    const GPRY_Actor *a;
    a = gpry_find_const(ctx, actor_id);
    if (!a) return 0;
    return a->age_frames;
}
