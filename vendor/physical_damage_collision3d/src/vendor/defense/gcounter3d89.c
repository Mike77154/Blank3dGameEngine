#include "gcounter3d89.h"

GCTR_Fix gctr_fix_mul(GCTR_Fix a, GCTR_Fix b)
{
    return (GCTR_Fix)((a * b) / GCTR_FIX_ONE);
}

int gctr_vec3_len_sq_fix(const GCTR_Vec3 *v)
{
    GCTR_Fix r;
    r = gctr_fix_mul(v->x, v->x) + gctr_fix_mul(v->y, v->y) + gctr_fix_mul(v->z, v->z);
    return (int)r;
}

void gctr_default_profile(GCTR_Profile *p)
{
    if (!p) return;
    p->open_after_frames = 0;
    p->valid_frames = 18;
    p->cooldown_frames = 30;
    p->token_life_frames = 24;
    p->max_tokens_per_actor = 2;
    p->stamina_cost = 80;
    p->posture_cost = 0;
    p->require_perfect = 0;
    p->consume_on_fail = 0;
    p->range_sq = 4096;
    p->allowed_sources = GCTR_SRC_BLOCK | GCTR_SRC_GUARD | GCTR_SRC_PARRY | GCTR_SRC_SHELL;
    p->counter_kind = GCTR_COUNTER_LIGHT;
    p->script_code = 0;
}

void gctr_init(GCTR_Context *ctx)
{
    int i;
    if (!ctx) return;
    ctx->event_fn = 0;
    ctx->counter_fn = 0;
    ctx->event_user = 0;
    ctx->counter_user = 0;
    for (i = 0; i < GCOUNTER3D89_MAX_ACTORS; ++i) {
        ctx->actors[i].used = 0;
        ctx->actors[i].actor_id = 0;
        ctx->actors[i].cooldown_timer = 0;
        gctr_default_profile(&ctx->actors[i].profile);
    }
    for (i = 0; i < GCOUNTER3D89_MAX_TOKENS; ++i) {
        ctx->tokens[i].used = 0;
        ctx->tokens[i].owner_id = 0;
        ctx->tokens[i].target_id = 0;
        ctx->tokens[i].born_frame = 0;
        ctx->tokens[i].open_frame = 0;
        ctx->tokens[i].expire_frame = 0;
        ctx->tokens[i].source_flags = 0;
        ctx->tokens[i].defense_result_flags = 0;
        ctx->tokens[i].attacker_rel_pos.x = 0;
        ctx->tokens[i].attacker_rel_pos.y = 0;
        ctx->tokens[i].attacker_rel_pos.z = 0;
    }
}

void gctr_set_event_callback(GCTR_Context *ctx, GCTR_EventFn fn, void *user)
{
    if (!ctx) return;
    ctx->event_fn = fn;
    ctx->event_user = user;
}

void gctr_set_counter_callback(GCTR_Context *ctx, GCTR_CounterFn fn, void *user)
{
    if (!ctx) return;
    ctx->counter_fn = fn;
    ctx->counter_user = user;
}

static void gctr_emit(GCTR_Context *ctx, int actor_id, int other_id, int event_code, int amount)
{
    if (ctx && ctx->event_fn) ctx->event_fn(actor_id, other_id, event_code, amount, ctx->event_user);
}

static GCTR_Actor *gctr_find(GCTR_Context *ctx, int actor_id)
{
    int i;
    if (!ctx) return 0;
    for (i = 0; i < GCOUNTER3D89_MAX_ACTORS; ++i) {
        if (ctx->actors[i].used && ctx->actors[i].actor_id == actor_id) return &ctx->actors[i];
    }
    return 0;
}

int gctr_create_actor(GCTR_Context *ctx, int actor_id, const GCTR_Profile *profile)
{
    int i;
    GCTR_Profile defp;
    if (!ctx) return GCTR_FALSE;
    if (gctr_find(ctx, actor_id)) return GCTR_TRUE;
    gctr_default_profile(&defp);
    for (i = 0; i < GCOUNTER3D89_MAX_ACTORS; ++i) {
        if (!ctx->actors[i].used) {
            ctx->actors[i].used = 1;
            ctx->actors[i].actor_id = actor_id;
            ctx->actors[i].cooldown_timer = 0;
            ctx->actors[i].profile = profile ? *profile : defp;
            return GCTR_TRUE;
        }
    }
    return GCTR_FALSE;
}

int gctr_remove_actor(GCTR_Context *ctx, int actor_id)
{
    GCTR_Actor *a;
    int i;
    a = gctr_find(ctx, actor_id);
    if (!a) return GCTR_FALSE;
    a->used = 0;
    for (i = 0; i < GCOUNTER3D89_MAX_TOKENS; ++i) {
        if (ctx->tokens[i].used && ctx->tokens[i].owner_id == actor_id) ctx->tokens[i].used = 0;
    }
    return GCTR_TRUE;
}

int gctr_count_tokens(const GCTR_Context *ctx, int actor_id)
{
    int i;
    int count;
    if (!ctx) return 0;
    count = 0;
    for (i = 0; i < GCOUNTER3D89_MAX_TOKENS; ++i) {
        if (ctx->tokens[i].used && ctx->tokens[i].owner_id == actor_id) count++;
    }
    return count;
}

void gctr_update(GCTR_Context *ctx, int frames, int current_frame)
{
    int i;
    GCTR_Actor *a;
    if (!ctx || frames <= 0) return;
    for (i = 0; i < GCOUNTER3D89_MAX_ACTORS; ++i) {
        a = &ctx->actors[i];
        if (!a->used) continue;
        if (a->cooldown_timer > 0) {
            a->cooldown_timer -= frames;
            if (a->cooldown_timer < 0) a->cooldown_timer = 0;
        }
    }
    for (i = 0; i < GCOUNTER3D89_MAX_TOKENS; ++i) {
        if (ctx->tokens[i].used && current_frame > ctx->tokens[i].expire_frame) {
            gctr_emit(ctx, ctx->tokens[i].owner_id, ctx->tokens[i].target_id, GCTR_EVENT_EXPIRE, 0);
            ctx->tokens[i].used = 0;
        }
    }
}

static int gctr_find_open_token(GCTR_Context *ctx)
{
    int i;
    if (!ctx) return -1;
    for (i = 0; i < GCOUNTER3D89_MAX_TOKENS; ++i) {
        if (!ctx->tokens[i].used) return i;
    }
    return -1;
}

int gctr_feed_defense_event(GCTR_Context *ctx, const GCTR_DefenseEvent *ev)
{
    GCTR_Actor *a;
    int idx;
    int count;
    if (!ctx || !ev) return GCTR_FALSE;
    a = gctr_find(ctx, ev->defender_id);
    if (!a) return GCTR_FALSE;
    if ((ev->source_flags & a->profile.allowed_sources) == 0) return GCTR_FALSE;
    if (a->profile.require_perfect && ((ev->defense_result_flags & GCTR_DEF_PERFECT) == 0)) return GCTR_FALSE;
    if ((ev->defense_result_flags & GCTR_DEF_BROKEN) != 0) return GCTR_FALSE;

    count = gctr_count_tokens(ctx, ev->defender_id);
    if (count >= a->profile.max_tokens_per_actor) return GCTR_FALSE;
    idx = gctr_find_open_token(ctx);
    if (idx < 0) return GCTR_FALSE;

    ctx->tokens[idx].used = 1;
    ctx->tokens[idx].owner_id = ev->defender_id;
    ctx->tokens[idx].target_id = ev->attacker_id;
    ctx->tokens[idx].born_frame = ev->event_frame;
    ctx->tokens[idx].open_frame = ev->event_frame + a->profile.open_after_frames;
    ctx->tokens[idx].expire_frame = ev->event_frame + a->profile.token_life_frames;
    if (ctx->tokens[idx].expire_frame < ctx->tokens[idx].open_frame + a->profile.valid_frames) {
        ctx->tokens[idx].expire_frame = ctx->tokens[idx].open_frame + a->profile.valid_frames;
    }
    ctx->tokens[idx].source_flags = ev->source_flags;
    ctx->tokens[idx].defense_result_flags = ev->defense_result_flags;
    ctx->tokens[idx].attacker_rel_pos = ev->attacker_rel_pos;
    gctr_emit(ctx, ev->defender_id, ev->attacker_id, GCTR_EVENT_TOKEN, idx);
    return GCTR_TRUE;
}

static GCTR_Token *gctr_find_best_token(GCTR_Context *ctx, const GCTR_Request *req, const GCTR_Profile *p)
{
    int i;
    int age;
    GCTR_Token *best;
    int best_age;
    if (!ctx || !req || !p) return 0;
    best = 0;
    best_age = 999999;
    for (i = 0; i < GCOUNTER3D89_MAX_TOKENS; ++i) {
        if (!ctx->tokens[i].used) continue;
        if (ctx->tokens[i].owner_id != req->actor_id) continue;
        if (req->target_id != 0 && ctx->tokens[i].target_id != req->target_id) continue;
        if (req->current_frame < ctx->tokens[i].open_frame) continue;
        if (req->current_frame > ctx->tokens[i].open_frame + p->valid_frames) continue;
        if (gctr_vec3_len_sq_fix(&req->target_rel_pos) > p->range_sq) continue;
        age = req->current_frame - ctx->tokens[i].born_frame;
        if (age < best_age) {
            best = &ctx->tokens[i];
            best_age = age;
        }
    }
    return best;
}

int gctr_try_counter(GCTR_Context *ctx, const GCTR_Request *req, GCTR_Result *out_result)
{
    GCTR_Actor *a;
    GCTR_Token *t;
    int token_age;
    if (out_result) {
        out_result->accepted = 0;
        out_result->denied_reason = 0;
        out_result->counter_kind = 0;
        out_result->script_code = 0;
        out_result->target_id = 0;
        out_result->token_age = 0;
        out_result->stamina_cost = 0;
        out_result->posture_cost = 0;
    }
    if (!ctx || !req) return GCTR_FALSE;
    a = gctr_find(ctx, req->actor_id);
    if (!a) return GCTR_FALSE;
    if (a->cooldown_timer > 0) {
        if (out_result) out_result->denied_reason = GCTR_EVENT_COOLDOWN;
        gctr_emit(ctx, req->actor_id, req->target_id, GCTR_EVENT_DENIED, GCTR_EVENT_COOLDOWN);
        return GCTR_FALSE;
    }
    if (req->stamina_available < a->profile.stamina_cost || req->posture_available < a->profile.posture_cost) {
        if (out_result) out_result->denied_reason = 2;
        gctr_emit(ctx, req->actor_id, req->target_id, GCTR_EVENT_DENIED, 2);
        return GCTR_FALSE;
    }

    t = gctr_find_best_token(ctx, req, &a->profile);
    if (!t) {
        if (out_result) out_result->denied_reason = 3;
        gctr_emit(ctx, req->actor_id, req->target_id, GCTR_EVENT_DENIED, 3);
        return GCTR_FALSE;
    }

    token_age = req->current_frame - t->born_frame;
    if (out_result) {
        out_result->accepted = 1;
        out_result->denied_reason = 0;
        out_result->counter_kind = a->profile.counter_kind;
        out_result->script_code = a->profile.script_code;
        out_result->target_id = t->target_id;
        out_result->token_age = token_age;
        out_result->stamina_cost = a->profile.stamina_cost;
        out_result->posture_cost = a->profile.posture_cost;
    }

    if (ctx->counter_fn) ctx->counter_fn(req->actor_id, t->target_id, a->profile.counter_kind, a->profile.script_code, ctx->counter_user);
    gctr_emit(ctx, req->actor_id, t->target_id, GCTR_EVENT_COUNTER, a->profile.counter_kind);
    a->cooldown_timer = a->profile.cooldown_frames;
    t->used = 0;
    return GCTR_TRUE;
}
