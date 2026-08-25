#include <string.h>
#include "gcrosshair_runtime89.h"

static int gc89r_load_preset(GC89R_Runtime *runtime, int preset_id)
{
    if (!runtime) return 0;
    if (!gcb89_preset_is_valid(preset_id)) return 0;
    if (!gcb89_make_preset(preset_id, &runtime->style)) return 0;
    if (!gcb89_make_preset_animation(preset_id, &runtime->animation)) return 0;
    if (!gcb89_make_preset_animation_recipe(preset_id, &runtime->animation_recipe)) return 0;
    runtime->preset_id = preset_id;
    runtime->preset_loaded = 1;
    gc89_core_set_ranges(&runtime->core,
                         runtime->animation.micro_scale_fx,
                         runtime->animation.neutral_scale_fx,
                         runtime->animation.maxi_scale_fx,
                         runtime->animation.speed_fx_per_tick);
    gc89_core_set_scale(&runtime->core, runtime->animation.neutral_scale_fx);
    gc89a_init(&runtime->animator, &runtime->animation, &runtime->animation_recipe);
    gc89a_apply_core(&runtime->animator, &runtime->core);
    return 1;
}

int gc89r_init(GC89R_Runtime *runtime, const char *root_ini)
{
    if (!runtime) return 0;
    memset(runtime, 0, sizeof(*runtime));
    gc89_core_init(&runtime->core);
    gc89p_runtime_init(&runtime->providers);
    if (root_ini && root_ini[0]) {
        if (!gcb89_recipe_load_root(root_ini)) return 0;
    } else {
        if (gcb89_preset_count() <= 0) return 0;
    }
    return gc89r_load_preset(runtime, GCB89_PRESET_BLANK3D_DEFAULT);
}

int gc89r_set_preset(GC89R_Runtime *runtime, int preset_id)
{
    return gc89r_load_preset(runtime, preset_id);
}

int gc89r_preset_id(const GC89R_Runtime *runtime)
{
    if (!runtime || !runtime->preset_loaded) return -1;
    return runtime->preset_id;
}

const char *gc89r_preset_name(const GC89R_Runtime *runtime)
{
    if (!runtime || !runtime->preset_loaded) return "invalid";
    return gcb89_preset_name(runtime->preset_id);
}

GC89P_Runtime *gc89r_providers(GC89R_Runtime *runtime)
{
    if (!runtime) return 0;
    return &runtime->providers;
}

GC89_Core *gc89r_core(GC89R_Runtime *runtime)
{
    if (!runtime) return 0;
    return &runtime->core;
}

void gc89r_update(GC89R_Runtime *runtime, unsigned long ticks)
{
    if (!runtime) return;
    if (runtime->animator.active_event >= 0) {
        gc89a_update(&runtime->animator, ticks);
        gc89a_apply_core(&runtime->animator, &runtime->core);
    } else {
        gc89_core_update(&runtime->core, ticks);
    }
}

void gc89r_process_input(GC89R_Runtime *runtime, const GC89_InputState *input)
{
    if (!runtime || !input) return;
    gc89a_feed_input(&runtime->animator, input);
    gc89a_apply_core(&runtime->animator, &runtime->core);
}

int gc89r_trigger_event(GC89R_Runtime *runtime, int event_id)
{
    int r;
    if (!runtime) return 0;
    r = gc89a_trigger(&runtime->animator, event_id);
    gc89a_apply_core(&runtime->animator, &runtime->core);
    return r;
}

const GC89A_Modifier *gc89r_animation_modifier(const GC89R_Runtime *runtime)
{
    if (!runtime) return 0;
    return gc89a_modifier(&runtime->animator);
}

static int gc89r_draw_with_providers(GC89R_Runtime *runtime,
                                     const GC89P_Runtime *providers,
                                     int screen_width,
                                     int screen_height,
                                     const GC89_InputState *input,
                                     GC89P_DrawReport *report)
{
    GC89_DrawSpec spec;
    GC89P_DrawMeta meta;
    if (!runtime || !providers || !runtime->preset_loaded) return 0;
    if (input) gc89r_process_input(runtime, input);
    gcp89_resolve(&runtime->style, input, &spec);
    gc89a_apply_draw_spec(&runtime->animator, &spec);
    meta.semantic_id = runtime->preset_id;
    meta.semantic_name = gcb89_preset_name(runtime->preset_id);
    meta.semantic_category = gcb89_preset_category(runtime->preset_id);
    return gc89p_draw_ex(providers, &runtime->core,
                         screen_width, screen_height,
                         &spec, &meta, report);
}

int gc89r_draw(GC89R_Runtime *runtime,
               int screen_width,
               int screen_height,
               const GC89_InputState *input,
               GC89P_DrawReport *report)
{
    if (!runtime) return 0;
    return gc89r_draw_with_providers(runtime, &runtime->providers,
                                     screen_width, screen_height,
                                     input, report);
}

int gc89r_draw_rgba(GC89R_Runtime *runtime,
                    const GC89_InputState *input,
                    GC89P_SoftwareSurface *surface,
                    GC89P_DrawReport *report)
{
    GC89P_Runtime local;
    GC89_DrawCallbacks software;
    if (!runtime || !surface) return 0;
    local = runtime->providers;
    gc89p_make_software_primitives(&software);
    gc89p_runtime_set_primitive_fallback(&local, &software, surface);
    gc89p_runtime_enable_primitive_fallback(&local, 1);
    return gc89r_draw_with_providers(runtime, &local,
                                     surface->width, surface->height,
                                     input, report);
}
