#include <string.h>
#include "gcrosshair_anim89.h"

static unsigned long gc89a_abs_long(long v)
{
    return v < 0 ? (unsigned long)(-v) : (unsigned long)v;
}

static GC89_Fixed gc89a_mul_fx(GC89_Fixed a, GC89_Fixed b)
{
    int neg;
    unsigned long ua, ub, ah, al, bh, bl, out;
    neg = ((a < 0) != (b < 0));
    ua = gc89a_abs_long(a); ub = gc89a_abs_long(b);
    ah = ua >> 16; al = ua & 65535UL;
    bh = ub >> 16; bl = ub & 65535UL;
    out = ah * ub + al * bh + ((al * bl) >> 16);
    return neg ? -(GC89_Fixed)out : (GC89_Fixed)out;
}

static GC89_Fixed gc89a_clamp01(GC89_Fixed v)
{
    if (v < 0) return 0;
    if (v > GC89_FX_ONE) return GC89_FX_ONE;
    return v;
}

static GC89_Fixed gc89a_curve(int curve, GC89_Fixed t)
{
    GC89_Fixed one_minus, tt, a;
    t = gc89a_clamp01(t);
    if (curve == GCB89_ANIM_CURVE_SNAP) return t >= GC89_FX_ONE ? GC89_FX_ONE : 0;
    if (curve == GCB89_ANIM_CURVE_EASE_IN) return gc89a_mul_fx(t, t);
    if (curve == GCB89_ANIM_CURVE_EASE_OUT) {
        one_minus = GC89_FX_ONE - t;
        return GC89_FX_ONE - gc89a_mul_fx(one_minus, one_minus);
    }
    if (curve == GCB89_ANIM_CURVE_SMOOTHSTEP) {
        tt = gc89a_mul_fx(t, t);
        a = 3L * GC89_FX_ONE - 2L * t;
        return gc89a_mul_fx(tt, a);
    }
    return t;
}

static GC89_Fixed gc89a_lerp(GC89_Fixed a, GC89_Fixed b, GC89_Fixed t)
{
    return a + gc89a_mul_fx(b - a, t);
}

static void gc89a_identity(GC89A_Modifier *m, GC89_Fixed neutral)
{
    memset(m, 0, sizeof(*m));
    m->scale_fx = neutral;
    m->alpha_fx = GC89_FX_ONE;
    m->thickness_scale_fx = GC89_FX_ONE;
    m->dot_scale_fx = GC89_FX_ONE;
}

static void gc89a_build_target(const GC89A_Animator *anim,
                               const GCB89_AnimationEventRecipe *e,
                               GC89A_Modifier *m)
{
    gc89a_identity(m, anim->legacy.neutral_scale_fx);
    if (e->scale_target == GCB89_ANIM_SCALE_MICRO) m->scale_fx = anim->legacy.micro_scale_fx;
    else if (e->scale_target == GCB89_ANIM_SCALE_MAXI) m->scale_fx = anim->legacy.maxi_scale_fx;
    else if (e->scale_target == GCB89_ANIM_SCALE_CUSTOM) m->scale_fx = e->scale_fx;
    else if (e->scale_target == GCB89_ANIM_SCALE_CURRENT) m->scale_fx = anim->current.scale_fx;
    m->rotation_deg_fx = e->rotation_deg_fx;
    m->offset_x_fx = e->offset_x_fx;
    m->offset_y_fx = e->offset_y_fx;
    m->alpha_fx = e->alpha_fx;
    m->thickness_scale_fx = e->thickness_scale_fx;
    m->dot_scale_fx = e->dot_scale_fx;
}

static void gc89a_lerp_modifier(GC89A_Modifier *out,
                                const GC89A_Modifier *a,
                                const GC89A_Modifier *b,
                                GC89_Fixed t)
{
    out->scale_fx = gc89a_lerp(a->scale_fx, b->scale_fx, t);
    out->rotation_deg_fx = gc89a_lerp(a->rotation_deg_fx, b->rotation_deg_fx, t);
    out->offset_x_fx = gc89a_lerp(a->offset_x_fx, b->offset_x_fx, t);
    out->offset_y_fx = gc89a_lerp(a->offset_y_fx, b->offset_y_fx, t);
    out->alpha_fx = gc89a_lerp(a->alpha_fx, b->alpha_fx, t);
    out->thickness_scale_fx = gc89a_lerp(a->thickness_scale_fx, b->thickness_scale_fx, t);
    out->dot_scale_fx = gc89a_lerp(a->dot_scale_fx, b->dot_scale_fx, t);
}

void gc89a_init(GC89A_Animator *anim,
                const GCB89_AnimationPreset *legacy,
                const GCB89_AnimationRecipe *recipe)
{
    if (!anim) return;
    memset(anim, 0, sizeof(*anim));
    if (legacy) anim->legacy = *legacy;
    else {
        anim->legacy.micro_scale_fx = GC89_FX_ONE;
        anim->legacy.neutral_scale_fx = GC89_FX_ONE;
        anim->legacy.maxi_scale_fx = GC89_FX_ONE;
    }
    if (recipe) anim->recipe = *recipe;
    gc89a_identity(&anim->current, anim->legacy.neutral_scale_fx);
    anim->active_event = -1;
}

void gc89a_reset(GC89A_Animator *anim)
{
    if (!anim) return;
    gc89a_identity(&anim->current, anim->legacy.neutral_scale_fx);
    anim->active_event = -1;
    anim->phase = GC89A_PHASE_IDLE;
    anim->elapsed = 0;
    anim->phase_ticks = 0;
    anim->repeats_left = 0;
    anim->previous_input_flags = 0;
}

static void gc89a_enter_post_attack(GC89A_Animator *anim,
                                    const GCB89_AnimationEventRecipe *e)
{
    anim->current = anim->target;
    anim->elapsed = 0;
    if (e->hold_ticks > 0UL) {
        anim->phase = GC89A_PHASE_HOLD;
        anim->phase_ticks = e->hold_ticks;
        return;
    }
    if (e->return_mode == GCB89_ANIM_RETURN_NONE) {
        anim->phase = GC89A_PHASE_IDLE;
        anim->active_event = -1;
        return;
    }
    anim->phase = GC89A_PHASE_RETURN;
    anim->phase_ticks = e->return_ticks;
}

int gc89a_trigger(GC89A_Animator *anim, int event_id)
{
    const GCB89_AnimationEventRecipe *e;
    if (!anim || event_id < 0 || event_id >= GCB89_ANIM_EVENT_COUNT) return 0;
    e = &anim->recipe.events[event_id];
    if (!e->enabled) return 0;
    if (anim->active_event == event_id && e->retrigger_mode == GCB89_ANIM_RETRIGGER_IGNORE) return 0;
    anim->start = anim->current;
    gc89a_build_target(anim, e, &anim->target);
    if (e->return_mode == GCB89_ANIM_RETURN_NEUTRAL)
        gc89a_identity(&anim->return_target, anim->legacy.neutral_scale_fx);
    else anim->return_target = anim->start;
    anim->active_event = event_id;
    anim->phase = GC89A_PHASE_ATTACK;
    anim->elapsed = 0;
    anim->phase_ticks = e->attack_ticks;
    anim->repeats_left = e->repeat_count < 1 ? 1 : e->repeat_count;
    if (anim->phase_ticks == 0UL) gc89a_enter_post_attack(anim, e);
    return 1;
}

void gc89a_feed_input(GC89A_Animator *anim, const GC89_InputState *input)
{
    int now, before, changed;
    if (!anim || !input || !anim->recipe.auto_input_events) return;
    now = input->flags; before = anim->previous_input_flags; changed = now ^ before;
    if ((changed & GC89_STATE_DISABLED) != 0) {
        if ((now & GC89_STATE_DISABLED) != 0) gc89a_trigger(anim, GCB89_ANIM_EVENT_DISABLED);
        else gc89a_trigger(anim, GCB89_ANIM_EVENT_ENABLED);
    }
    if ((changed & GC89_STATE_AIM) != 0) {
        if ((now & GC89_STATE_AIM) != 0) gc89a_trigger(anim, GCB89_ANIM_EVENT_AIM_ENTER);
        else gc89a_trigger(anim, GCB89_ANIM_EVENT_AIM_EXIT);
    }
    if ((changed & GC89_STATE_FIRE) != 0 && (now & GC89_STATE_FIRE) != 0)
        gc89a_trigger(anim, GCB89_ANIM_EVENT_FIRE);
    if ((changed & GC89_STATE_HIT) != 0 && (now & GC89_STATE_HIT) != 0)
        gc89a_trigger(anim, GCB89_ANIM_EVENT_HIT);
    anim->previous_input_flags = now;
}

static void gc89a_finish_return(GC89A_Animator *anim,
                                const GCB89_AnimationEventRecipe *e)
{
    anim->current = anim->return_target;
    --anim->repeats_left;
    if (anim->repeats_left > 0) {
        anim->start = anim->current;
        gc89a_build_target(anim, e, &anim->target);
        anim->phase = GC89A_PHASE_ATTACK;
        anim->phase_ticks = e->attack_ticks;
        anim->elapsed = 0;
        if (anim->phase_ticks == 0UL) gc89a_enter_post_attack(anim, e);
    } else {
        anim->phase = GC89A_PHASE_IDLE;
        anim->active_event = -1;
    }
}

void gc89a_update(GC89A_Animator *anim, unsigned long ticks)
{
    const GCB89_AnimationEventRecipe *e;
    unsigned long step, left;
    GC89_Fixed t;
    while (anim && ticks > 0UL && anim->active_event >= 0) {
        e = &anim->recipe.events[anim->active_event];
        if (anim->phase == GC89A_PHASE_ATTACK) {
            left = anim->phase_ticks - anim->elapsed;
            step = ticks < left ? ticks : left;
            anim->elapsed += step; ticks -= step;
            t = (GC89_Fixed)((anim->elapsed * 65536UL) / anim->phase_ticks);
            t = gc89a_curve(e->attack_curve, t);
            gc89a_lerp_modifier(&anim->current, &anim->start, &anim->target, t);
            if (anim->elapsed >= anim->phase_ticks) gc89a_enter_post_attack(anim, e);
        } else if (anim->phase == GC89A_PHASE_HOLD) {
            left = anim->phase_ticks - anim->elapsed;
            step = ticks < left ? ticks : left;
            anim->elapsed += step; ticks -= step;
            if (anim->elapsed >= anim->phase_ticks) {
                anim->elapsed = 0;
                if (e->return_mode == GCB89_ANIM_RETURN_NONE) {
                    anim->phase = GC89A_PHASE_IDLE; anim->active_event = -1;
                } else {
                    anim->phase = GC89A_PHASE_RETURN; anim->phase_ticks = e->return_ticks;
                    if (anim->phase_ticks == 0UL) gc89a_finish_return(anim, e);
                }
            }
        } else if (anim->phase == GC89A_PHASE_RETURN) {
            if (anim->phase_ticks == 0UL) { gc89a_finish_return(anim, e); continue; }
            left = anim->phase_ticks - anim->elapsed;
            step = ticks < left ? ticks : left;
            anim->elapsed += step; ticks -= step;
            t = (GC89_Fixed)((anim->elapsed * 65536UL) / anim->phase_ticks);
            t = gc89a_curve(e->return_curve, t);
            gc89a_lerp_modifier(&anim->current, &anim->target, &anim->return_target, t);
            if (anim->elapsed >= anim->phase_ticks) gc89a_finish_return(anim, e);
        } else break;
    }
}

void gc89a_apply_core(const GC89A_Animator *anim, GC89_Core *core)
{
    if (!anim || !core) return;
    gc89_core_set_scale_extended(core, anim->current.scale_fx);
}

static unsigned long gc89a_alpha_color(unsigned long c, GC89_Fixed alpha_fx)
{
    unsigned long a, scaled;
    if (alpha_fx >= GC89_FX_ONE) return c;
    if (alpha_fx <= 0) return c & 0xFFFFFF00UL;
    a = c & 255UL;
    scaled = (a * (unsigned long)alpha_fx) / 65536UL;
    if (scaled > 255UL) scaled = 255UL;
    return (c & 0xFFFFFF00UL) | scaled;
}

void gc89a_apply_draw_spec(const GC89A_Animator *anim, GC89_DrawSpec *spec)
{
    if (!anim || !spec) return;
    spec->center_offset_x_fx += anim->current.offset_x_fx;
    spec->center_offset_y_fx += anim->current.offset_y_fx;
    spec->shape_rotation_deg_fx += anim->current.rotation_deg_fx;
    spec->thickness_fx = gc89a_mul_fx(spec->thickness_fx, anim->current.thickness_scale_fx);
    spec->dot_size_fx = gc89a_mul_fx(spec->dot_size_fx, anim->current.dot_scale_fx);
    spec->outline_width_fx = gc89a_mul_fx(spec->outline_width_fx, anim->current.thickness_scale_fx);
    spec->color_rgba = gc89a_alpha_color(spec->color_rgba, anim->current.alpha_fx);
    spec->image_tint_rgba = gc89a_alpha_color(spec->image_tint_rgba, anim->current.alpha_fx);
    spec->outline_color_rgba = gc89a_alpha_color(spec->outline_color_rgba, anim->current.alpha_fx);
}

const GC89A_Modifier *gc89a_modifier(const GC89A_Animator *anim)
{
    return anim ? &anim->current : 0;
}
