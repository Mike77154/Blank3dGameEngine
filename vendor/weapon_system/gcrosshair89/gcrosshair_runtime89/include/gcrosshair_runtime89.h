#ifndef GCROSSHAIR_RUNTIME89_H
#define GCROSSHAIR_RUNTIME89_H

#include "gcrosshair_base89.h"
#include "gcrosshair_recipe89.h"
#include "gcrosshair_params89.h"
#include "gcrosshair_core89.h"
#include "gcrosshair_provider89.h"
#include "gcrosshair_anim89.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GC89R_ABI_VERSION 2

typedef struct GC89R_Runtime {
    GC89_Core core;
    GC89P_Runtime providers;
    GC89_Style style;
    GCB89_AnimationPreset animation;
    GCB89_AnimationRecipe animation_recipe;
    GC89A_Animator animator;
    int preset_id;
    int preset_loaded;
} GC89R_Runtime;

/* root_ini may be NULL to use base89's normal recipes/gcrosshair.ini search. */
int gc89r_init(GC89R_Runtime *runtime, const char *root_ini);
int gc89r_set_preset(GC89R_Runtime *runtime, int preset_id);
int gc89r_preset_id(const GC89R_Runtime *runtime);
const char *gc89r_preset_name(const GC89R_Runtime *runtime);

GC89P_Runtime *gc89r_providers(GC89R_Runtime *runtime);
GC89_Core *gc89r_core(GC89R_Runtime *runtime);

void gc89r_update(GC89R_Runtime *runtime, unsigned long ticks);
void gc89r_process_input(GC89R_Runtime *runtime, const GC89_InputState *input);
int gc89r_trigger_event(GC89R_Runtime *runtime, int event_id);
const GC89A_Modifier *gc89r_animation_modifier(const GC89R_Runtime *runtime);

/* Resolve style/input and route through provider89. */
int gc89r_draw(GC89R_Runtime *runtime,
               int screen_width,
               int screen_height,
               const GC89_InputState *input,
               GC89P_DrawReport *report);

/*
 * Same high-level path, but installs bundled software primitives as the last
 * primitive fallback for this draw. Existing HUD/vector/asset providers still
 * get first chance to handle the request.
 */
int gc89r_draw_rgba(GC89R_Runtime *runtime,
                    const GC89_InputState *input,
                    GC89P_SoftwareSurface *surface,
                    GC89P_DrawReport *report);

#ifdef __cplusplus
}
#endif

#endif
