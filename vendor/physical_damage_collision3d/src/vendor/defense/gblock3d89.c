#include "gblock3d89.h"

static int gblk_clampi(int v, int lo, int hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

GBLK_Fix gblk_fix_mul(GBLK_Fix a, GBLK_Fix b)
{
    return (GBLK_Fix)((a * b) / GBLK_FIX_ONE);
}

int gblk_vec3_dot_fix(const GBLK_Vec3 *a, const GBLK_Vec3 *b)
{
    GBLK_Fix r;
    r = gblk_fix_mul(a->x, b->x) + gblk_fix_mul(a->y, b->y) + gblk_fix_mul(a->z, b->z);
    return (int)r;
}

void gblk_default_profile(GBLK_Profile *p)
{
    if (!p) return;
    p->coverage_cos = 256;
    p->stamina_cost_mul = 1024;
    p->posture_cost_mul = 1024;
    p->chip_mul = 64;
    p->recoil_frames = 5;
    p->blockstun_bonus = 0;
    p->min_stamina_to_block = 1;
    p->max_stamina = 1000;
    p->stamina_regen_per_frame = 2;
    p->max_posture = 1000;
    p->posture_regen_per_frame = 1;
}

void gblk_init(GBLK_Context *ctx)
{
    int i;
    if (!ctx) return;
    ctx->event_fn = 0;
    ctx->event_user = 0;
    for (i = 0; i < GBLOCK3D89_MAX_ACTORS; ++i) {
        ctx->actors[i].used = 0;
        ctx->actors[i].actor_id = 0;
        ctx->actors[i].active = 0;
        ctx->actors[i].stamina = 0;
        ctx->actors[i].posture = 0;
        ctx->actors[i].cooldown_frames = 0;
        ctx->actors[i].facing_dir.x = GBLK_FIX_ONE;
        ctx->actors[i].facing_dir.y = 0;
        ctx->actors[i].facing_dir.z = 0;
        gblk_default_profile(&ctx->actors[i].profile);
    }
}

void gblk_set_event_callback(GBLK_Context *ctx, GBLK_EventFn fn, void *user)
{
    if (!ctx) return;
    ctx->event_fn = fn;
    ctx->event_user = user;
}

static GBLK_Actor *gblk_find(GBLK_Context *ctx, int actor_id)
{
    int i;
    if (!ctx) return 0;
    for (i = 0; i < GBLOCK3D89_MAX_ACTORS; ++i) {
        if (ctx->actors[i].used && ctx->actors[i].actor_id == actor_id) return &ctx->actors[i];
    }
    return 0;
}

static const GBLK_Actor *gblk_find_const(const GBLK_Context *ctx, int actor_id)
{
    int i;
    if (!ctx) return 0;
    for (i = 0; i < GBLOCK3D89_MAX_ACTORS; ++i) {
        if (ctx->actors[i].used && ctx->actors[i].actor_id == actor_id) return &ctx->actors[i];
    }
    return 0;
}

int gblk_create_actor(GBLK_Context *ctx, int actor_id, const GBLK_Profile *profile)
{
    int i;
    GBLK_Profile defp;
    if (!ctx) return GBLK_FALSE;
    if (gblk_find(ctx, actor_id)) return GBLK_TRUE;
    gblk_default_profile(&defp);
    for (i = 0; i < GBLOCK3D89_MAX_ACTORS; ++i) {
        if (!ctx->actors[i].used) {
            ctx->actors[i].used = 1;
            ctx->actors[i].actor_id = actor_id;
            ctx->actors[i].active = 0;
            ctx->actors[i].profile = profile ? *profile : defp;
            ctx->actors[i].stamina = ctx->actors[i].profile.max_stamina;
            ctx->actors[i].posture = ctx->actors[i].profile.max_posture;
            ctx->actors[i].cooldown_frames = 0;
            ctx->actors[i].facing_dir.x = GBLK_FIX_ONE;
            ctx->actors[i].facing_dir.y = 0;
            ctx->actors[i].facing_dir.z = 0;
            return GBLK_TRUE;
        }
    }
    return GBLK_FALSE;
}

int gblk_remove_actor(GBLK_Context *ctx, int actor_id)
{
    GBLK_Actor *a;
    a = gblk_find(ctx, actor_id);
    if (!a) return GBLK_FALSE;
    a->used = 0;
    return GBLK_TRUE;
}

int gblk_set_active(GBLK_Context *ctx, int actor_id, int active)
{
    GBLK_Actor *a;
    a = gblk_find(ctx, actor_id);
    if (!a) return GBLK_FALSE;
    a->active = active ? 1 : 0;
    return GBLK_TRUE;
}

int gblk_set_facing(GBLK_Context *ctx, int actor_id, GBLK_Fix x, GBLK_Fix y, GBLK_Fix z)
{
    GBLK_Actor *a;
    a = gblk_find(ctx, actor_id);
    if (!a) return GBLK_FALSE;
    a->facing_dir.x = x;
    a->facing_dir.y = y;
    a->facing_dir.z = z;
    return GBLK_TRUE;
}

int gblk_set_resources(GBLK_Context *ctx, int actor_id, int stamina, int posture)
{
    GBLK_Actor *a;
    a = gblk_find(ctx, actor_id);
    if (!a) return GBLK_FALSE;
    a->stamina = gblk_clampi(stamina, 0, a->profile.max_stamina);
    a->posture = gblk_clampi(posture, 0, a->profile.max_posture);
    return GBLK_TRUE;
}

void gblk_update(GBLK_Context *ctx, int frames)
{
    int i;
    int f;
    GBLK_Actor *a;
    if (!ctx || frames <= 0) return;
    for (i = 0; i < GBLOCK3D89_MAX_ACTORS; ++i) {
        a = &ctx->actors[i];
        if (!a->used) continue;
        f = frames;
        if (a->cooldown_frames > 0) {
            if (f >= a->cooldown_frames) a->cooldown_frames = 0;
            else a->cooldown_frames -= f;
        }
        a->stamina = gblk_clampi(a->stamina + a->profile.stamina_regen_per_frame * frames, 0, a->profile.max_stamina);
        a->posture = gblk_clampi(a->posture + a->profile.posture_regen_per_frame * frames, 0, a->profile.max_posture);
    }
}

static void gblk_emit(GBLK_Context *ctx, int actor_id, int other_id, int event_code, int amount)
{
    if (ctx && ctx->event_fn) ctx->event_fn(actor_id, other_id, event_code, amount, ctx->event_user);
}

int gblk_try_block(GBLK_Context *ctx, int actor_id, const GBLK_Attack *atk, GBLK_Result *out_result)
{
    GBLK_Actor *a;
    int dot;
    int stamina_cost;
    int posture_cost;
    int chip;
    int dir_ok;
    if (out_result) {
        out_result->accepted = 0;
        out_result->blocked = 0;
        out_result->broken = 0;
        out_result->damage_to_apply = atk ? atk->damage : 0;
        out_result->stamina_cost = 0;
        out_result->posture_cost = 0;
        out_result->blockstun_frames = 0;
        out_result->recoil_frames = 0;
        out_result->dot = 0;
        out_result->event_code = GBLK_EVENT_PASSED;
    }
    if (!ctx || !atk) return GBLK_FALSE;
    a = gblk_find(ctx, actor_id);
    if (!a) return GBLK_FALSE;

    if (!a->active || a->cooldown_frames > 0) {
        gblk_emit(ctx, actor_id, atk->attacker_id, GBLK_EVENT_PASSED, atk->damage);
        return GBLK_FALSE;
    }
    if ((atk->flags & GBLK_FLAG_UNBLOCKABLE) != 0) {
        gblk_emit(ctx, actor_id, atk->attacker_id, GBLK_EVENT_PASSED, atk->damage);
        return GBLK_FALSE;
    }

    dot = gblk_vec3_dot_fix(&a->facing_dir, &atk->source_dir);
    dir_ok = ((atk->flags & GBLK_FLAG_IGNORE_DIR) != 0) || (dot >= a->profile.coverage_cos);
    if (!dir_ok) {
        if (out_result) out_result->dot = dot;
        gblk_emit(ctx, actor_id, atk->attacker_id, GBLK_EVENT_PASSED, atk->damage);
        return GBLK_FALSE;
    }

    stamina_cost = (atk->stamina_damage * a->profile.stamina_cost_mul) / 1024;
    posture_cost = (atk->posture_damage * a->profile.posture_cost_mul) / 1024;
    if (stamina_cost < a->profile.min_stamina_to_block) stamina_cost = a->profile.min_stamina_to_block;

    a->stamina -= stamina_cost;
    a->posture -= posture_cost;

    if ((atk->flags & GBLK_FLAG_GUARD_BREAKER) != 0) {
        a->posture -= atk->breaker_power;
    }

    if (a->stamina < 0 || a->posture < 0) {
        a->stamina = gblk_clampi(a->stamina, 0, a->profile.max_stamina);
        a->posture = gblk_clampi(a->posture, 0, a->profile.max_posture);
        a->cooldown_frames = a->profile.recoil_frames + atk->blockstun_frames;
        if (out_result) {
            out_result->accepted = 1;
            out_result->blocked = 0;
            out_result->broken = 1;
            out_result->damage_to_apply = atk->damage;
            out_result->stamina_cost = stamina_cost;
            out_result->posture_cost = posture_cost;
            out_result->blockstun_frames = atk->blockstun_frames + a->profile.blockstun_bonus;
            out_result->recoil_frames = a->profile.recoil_frames;
            out_result->dot = dot;
            out_result->event_code = GBLK_EVENT_BROKEN;
        }
        gblk_emit(ctx, actor_id, atk->attacker_id, GBLK_EVENT_BROKEN, atk->damage);
        return GBLK_TRUE;
    }

    chip = (atk->damage * a->profile.chip_mul) / 1024;
    if (out_result) {
        out_result->accepted = 1;
        out_result->blocked = 1;
        out_result->broken = 0;
        out_result->damage_to_apply = chip;
        out_result->stamina_cost = stamina_cost;
        out_result->posture_cost = posture_cost;
        out_result->blockstun_frames = atk->blockstun_frames + a->profile.blockstun_bonus;
        out_result->recoil_frames = a->profile.recoil_frames;
        out_result->dot = dot;
        out_result->event_code = GBLK_EVENT_BLOCKED;
    }
    gblk_emit(ctx, actor_id, atk->attacker_id, GBLK_EVENT_BLOCKED, chip);
    if (chip > 0) gblk_emit(ctx, actor_id, atk->attacker_id, GBLK_EVENT_CHIPPED, chip);
    return GBLK_TRUE;
}

int gblk_get_stamina(const GBLK_Context *ctx, int actor_id)
{
    const GBLK_Actor *a;
    a = gblk_find_const(ctx, actor_id);
    if (!a) return 0;
    return a->stamina;
}

int gblk_get_posture(const GBLK_Context *ctx, int actor_id)
{
    const GBLK_Actor *a;
    a = gblk_find_const(ctx, actor_id);
    if (!a) return 0;
    return a->posture;
}
