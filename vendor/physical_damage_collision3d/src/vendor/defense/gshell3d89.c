#include "gshell3d89.h"

GSHL_Fix gshl_fix_mul(GSHL_Fix a, GSHL_Fix b)
{
    return (GSHL_Fix)((a * b) / GSHL_FIX_ONE);
}

int gshl_vec3_dot_fix(const GSHL_Vec3 *a, const GSHL_Vec3 *b)
{
    GSHL_Fix r;
    r = gshl_fix_mul(a->x, b->x) + gshl_fix_mul(a->y, b->y) + gshl_fix_mul(a->z, b->z);
    return (int)r;
}

void gshl_default_layer_profile(GSHL_LayerProfile *p)
{
    if (!p) return;
    p->max_hp = 300;
    p->absorb_mul = 1024;
    p->leak_mul = 0;
    p->recharge_delay_frames = 90;
    p->recharge_per_frame = 2;
    p->break_recovery_frames = 120;
    p->coverage_cos = -1024;
    p->damage_mask = GSHL_DMG_BLUNT | GSHL_DMG_SLASH | GSHL_DMG_PIERCE | GSHL_DMG_FIRE | GSHL_DMG_ENERGY;
}

void gshl_init(GSHL_Context *ctx)
{
    int i;
    int j;
    if (!ctx) return;
    ctx->event_fn = 0;
    ctx->event_user = 0;
    for (i = 0; i < GSHELL3D89_MAX_ACTORS; ++i) {
        ctx->actors[i].used = 0;
        ctx->actors[i].actor_id = 0;
        ctx->actors[i].facing_dir.x = GSHL_FIX_ONE;
        ctx->actors[i].facing_dir.y = 0;
        ctx->actors[i].facing_dir.z = 0;
        for (j = 0; j < GSHELL3D89_MAX_LAYERS; ++j) {
            ctx->actors[i].layers[j].used = 0;
            ctx->actors[i].layers[j].hp = 0;
            ctx->actors[i].layers[j].recharge_timer = 0;
            ctx->actors[i].layers[j].broken_timer = 0;
            gshl_default_layer_profile(&ctx->actors[i].layers[j].profile);
        }
    }
}

void gshl_set_event_callback(GSHL_Context *ctx, GSHL_EventFn fn, void *user)
{
    if (!ctx) return;
    ctx->event_fn = fn;
    ctx->event_user = user;
}

static void gshl_emit(GSHL_Context *ctx, int actor_id, int other_id, int event_code, int amount)
{
    if (ctx && ctx->event_fn) ctx->event_fn(actor_id, other_id, event_code, amount, ctx->event_user);
}

static GSHL_Actor *gshl_find(GSHL_Context *ctx, int actor_id)
{
    int i;
    if (!ctx) return 0;
    for (i = 0; i < GSHELL3D89_MAX_ACTORS; ++i) {
        if (ctx->actors[i].used && ctx->actors[i].actor_id == actor_id) return &ctx->actors[i];
    }
    return 0;
}

static const GSHL_Actor *gshl_find_const(const GSHL_Context *ctx, int actor_id)
{
    int i;
    if (!ctx) return 0;
    for (i = 0; i < GSHELL3D89_MAX_ACTORS; ++i) {
        if (ctx->actors[i].used && ctx->actors[i].actor_id == actor_id) return &ctx->actors[i];
    }
    return 0;
}

int gshl_create_actor(GSHL_Context *ctx, int actor_id)
{
    int i;
    int j;
    if (!ctx) return GSHL_FALSE;
    if (gshl_find(ctx, actor_id)) return GSHL_TRUE;
    for (i = 0; i < GSHELL3D89_MAX_ACTORS; ++i) {
        if (!ctx->actors[i].used) {
            ctx->actors[i].used = 1;
            ctx->actors[i].actor_id = actor_id;
            ctx->actors[i].facing_dir.x = GSHL_FIX_ONE;
            ctx->actors[i].facing_dir.y = 0;
            ctx->actors[i].facing_dir.z = 0;
            for (j = 0; j < GSHELL3D89_MAX_LAYERS; ++j) {
                ctx->actors[i].layers[j].used = 0;
                ctx->actors[i].layers[j].hp = 0;
                ctx->actors[i].layers[j].recharge_timer = 0;
                ctx->actors[i].layers[j].broken_timer = 0;
            }
            return GSHL_TRUE;
        }
    }
    return GSHL_FALSE;
}

int gshl_remove_actor(GSHL_Context *ctx, int actor_id)
{
    GSHL_Actor *a;
    a = gshl_find(ctx, actor_id);
    if (!a) return GSHL_FALSE;
    a->used = 0;
    return GSHL_TRUE;
}

int gshl_set_facing(GSHL_Context *ctx, int actor_id, GSHL_Fix x, GSHL_Fix y, GSHL_Fix z)
{
    GSHL_Actor *a;
    a = gshl_find(ctx, actor_id);
    if (!a) return GSHL_FALSE;
    a->facing_dir.x = x;
    a->facing_dir.y = y;
    a->facing_dir.z = z;
    return GSHL_TRUE;
}

int gshl_set_layer(GSHL_Context *ctx, int actor_id, int layer_index, const GSHL_LayerProfile *profile, int start_full)
{
    GSHL_Actor *a;
    GSHL_LayerProfile defp;
    if (layer_index < 0 || layer_index >= GSHELL3D89_MAX_LAYERS) return GSHL_FALSE;
    a = gshl_find(ctx, actor_id);
    if (!a) return GSHL_FALSE;
    gshl_default_layer_profile(&defp);
    a->layers[layer_index].used = 1;
    a->layers[layer_index].profile = profile ? *profile : defp;
    a->layers[layer_index].hp = start_full ? a->layers[layer_index].profile.max_hp : 0;
    a->layers[layer_index].recharge_timer = 0;
    a->layers[layer_index].broken_timer = 0;
    return GSHL_TRUE;
}

int gshl_clear_layer(GSHL_Context *ctx, int actor_id, int layer_index)
{
    GSHL_Actor *a;
    if (layer_index < 0 || layer_index >= GSHELL3D89_MAX_LAYERS) return GSHL_FALSE;
    a = gshl_find(ctx, actor_id);
    if (!a) return GSHL_FALSE;
    a->layers[layer_index].used = 0;
    a->layers[layer_index].hp = 0;
    return GSHL_TRUE;
}

void gshl_update(GSHL_Context *ctx, int frames)
{
    int i;
    int j;
    GSHL_Actor *a;
    GSHL_Layer *l;
    if (!ctx || frames <= 0) return;
    for (i = 0; i < GSHELL3D89_MAX_ACTORS; ++i) {
        a = &ctx->actors[i];
        if (!a->used) continue;
        for (j = 0; j < GSHELL3D89_MAX_LAYERS; ++j) {
            l = &a->layers[j];
            if (!l->used) continue;
            if (l->broken_timer > 0) {
                l->broken_timer -= frames;
                if (l->broken_timer < 0) l->broken_timer = 0;
                continue;
            }
            if (l->recharge_timer > 0) {
                l->recharge_timer -= frames;
                if (l->recharge_timer < 0) l->recharge_timer = 0;
                continue;
            }
            if (l->hp < l->profile.max_hp) {
                l->hp += l->profile.recharge_per_frame * frames;
                if (l->hp > l->profile.max_hp) l->hp = l->profile.max_hp;
                gshl_emit(ctx, a->actor_id, 0, GSHL_EVENT_RECHARGE, l->hp);
            }
        }
    }
}

static int gshl_layer_can_take(const GSHL_Actor *a, const GSHL_Layer *l, const GSHL_Damage *dmg, int *dot_out)
{
    int dot;
    if (!l->used || l->hp <= 0 || l->broken_timer > 0) return 0;
    if ((dmg->flags & GSHL_FLAG_BYPASS_SHELL) != 0) return 0;
    if ((l->profile.damage_mask & dmg->damage_type) == 0) return 0;
    dot = gshl_vec3_dot_fix(&a->facing_dir, &dmg->source_dir);
    if (dot_out) *dot_out = dot;
    if (dot < l->profile.coverage_cos) return 0;
    return 1;
}

int gshl_apply_damage(GSHL_Context *ctx, int actor_id, const GSHL_Damage *dmg, GSHL_Result *out_result)
{
    GSHL_Actor *a;
    GSHL_Layer *l;
    int j;
    int dot;
    int absorb_try;
    int leak_min;
    int absorbed;
    int leak;
    if (out_result) {
        out_result->accepted = 0;
        out_result->absorbed = 0;
        out_result->leaked_damage = dmg ? dmg->damage : 0;
        out_result->broken_layer_index = -1;
        out_result->remaining_shell_hp = 0;
        out_result->dot = 0;
    }
    if (!ctx || !dmg) return GSHL_FALSE;
    a = gshl_find(ctx, actor_id);
    if (!a) return GSHL_FALSE;

    for (j = 0; j < GSHELL3D89_MAX_LAYERS; ++j) {
        l = &a->layers[j];
        dot = 0;
        if (!gshl_layer_can_take(a, l, dmg, &dot)) continue;
        absorb_try = (dmg->damage * l->profile.absorb_mul) / 1024;
        leak_min = (dmg->damage * l->profile.leak_mul) / 1024;
        if (absorb_try > dmg->damage) absorb_try = dmg->damage;
        absorbed = absorb_try;
        if (absorbed > l->hp) absorbed = l->hp;
        l->hp -= absorbed;
        leak = dmg->damage - absorbed;
        if (leak < leak_min) leak = leak_min;
        if ((dmg->flags & GSHL_FLAG_NO_RECHARGE_RESET) == 0) l->recharge_timer = l->profile.recharge_delay_frames;

        if (out_result) {
            out_result->accepted = 1;
            out_result->absorbed = absorbed;
            out_result->leaked_damage = leak;
            out_result->remaining_shell_hp = l->hp;
            out_result->dot = dot;
        }
        gshl_emit(ctx, actor_id, dmg->attacker_id, GSHL_EVENT_ABSORB, absorbed);
        if (leak > 0) gshl_emit(ctx, actor_id, dmg->attacker_id, GSHL_EVENT_LEAK, leak);

        if (l->hp <= 0) {
            l->hp = 0;
            l->broken_timer = l->profile.break_recovery_frames;
            if (out_result) out_result->broken_layer_index = j;
            gshl_emit(ctx, actor_id, dmg->attacker_id, GSHL_EVENT_LAYER_BREAK, j);
        }
        return GSHL_TRUE;
    }

    if (out_result) {
        out_result->leaked_damage = dmg->damage;
        out_result->broken_layer_index = -1;
    }
    gshl_emit(ctx, actor_id, dmg->attacker_id, GSHL_EVENT_BREACH, dmg->damage);
    return GSHL_FALSE;
}

int gshl_get_layer_hp(const GSHL_Context *ctx, int actor_id, int layer_index)
{
    const GSHL_Actor *a;
    if (layer_index < 0 || layer_index >= GSHELL3D89_MAX_LAYERS) return 0;
    a = gshl_find_const(ctx, actor_id);
    if (!a) return 0;
    if (!a->layers[layer_index].used) return 0;
    return a->layers[layer_index].hp;
}

int gshl_get_total_hp(const GSHL_Context *ctx, int actor_id)
{
    const GSHL_Actor *a;
    int j;
    int total;
    a = gshl_find_const(ctx, actor_id);
    if (!a) return 0;
    total = 0;
    for (j = 0; j < GSHELL3D89_MAX_LAYERS; ++j) {
        if (a->layers[j].used) total += a->layers[j].hp;
    }
    return total;
}
