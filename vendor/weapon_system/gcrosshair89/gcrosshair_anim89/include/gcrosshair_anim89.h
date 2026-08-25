#ifndef GCROSSHAIR_ANIM89_H
#define GCROSSHAIR_ANIM89_H

#include "gcrosshair_base89.h"
#include "gcrosshair_core89.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GC89A_ABI_VERSION 1
#define GC89A_PHASE_IDLE    0
#define GC89A_PHASE_ATTACK  1
#define GC89A_PHASE_HOLD    2
#define GC89A_PHASE_RETURN  3

typedef struct GC89A_Modifier {
    GC89_Fixed scale_fx;
    GC89_Fixed rotation_deg_fx;
    GC89_Fixed offset_x_fx;
    GC89_Fixed offset_y_fx;
    GC89_Fixed alpha_fx;
    GC89_Fixed thickness_scale_fx;
    GC89_Fixed dot_scale_fx;
} GC89A_Modifier;

typedef struct GC89A_Animator {
    GCB89_AnimationPreset legacy;
    GCB89_AnimationRecipe recipe;
    GC89A_Modifier current;
    GC89A_Modifier start;
    GC89A_Modifier target;
    GC89A_Modifier return_target;
    int active_event;
    int phase;
    unsigned long elapsed;
    unsigned long phase_ticks;
    int repeats_left;
    int previous_input_flags;
} GC89A_Animator;

void gc89a_init(GC89A_Animator *anim,
                const GCB89_AnimationPreset *legacy,
                const GCB89_AnimationRecipe *recipe);
void gc89a_reset(GC89A_Animator *anim);
int gc89a_trigger(GC89A_Animator *anim, int event_id);
void gc89a_feed_input(GC89A_Animator *anim, const GC89_InputState *input);
void gc89a_update(GC89A_Animator *anim, unsigned long ticks);
void gc89a_apply_core(const GC89A_Animator *anim, GC89_Core *core);
void gc89a_apply_draw_spec(const GC89A_Animator *anim, GC89_DrawSpec *spec);
const GC89A_Modifier *gc89a_modifier(const GC89A_Animator *anim);

#ifdef __cplusplus
}
#endif
#endif
