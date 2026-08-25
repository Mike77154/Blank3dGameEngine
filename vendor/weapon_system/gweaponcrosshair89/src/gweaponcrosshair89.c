#include "gweaponcrosshair89.h"

#include <string.h>

static int b3d_crosshair_is_disabled_name(const char *name)
{
    if (!name || !name[0]) return 0;
    if (strcmp(name, "none") == 0) return 1;
    if (strcmp(name, "off") == 0) return 1;
    if (strcmp(name, "disabled") == 0) return 1;
    return 0;
}

/* gcrosshair89 animation recipes use compact integer animation ticks.
   The host keeps real-time behavior by accumulating frame_ms and converting
   it to an approximately 60 Hz animation clock. This keeps 2..9 tick recipe
   timings visible and frame-rate independent without floats. */
static unsigned long b3d_crosshair_animation_ticks(
    GWeaponCrosshair89 *crosshair,
    unsigned long frame_ms)
{
    unsigned long ticks;
    if (!crosshair) return 0UL;
    if (frame_ms == 0UL) frame_ms = GWC89_ANIM_TICK_MS;
    if (frame_ms > 1000UL) frame_ms = 1000UL;
    crosshair->animation_ms_accum += frame_ms;
    ticks = crosshair->animation_ms_accum / GWC89_ANIM_TICK_MS;
    crosshair->animation_ms_accum %= GWC89_ANIM_TICK_MS;
    return ticks;
}

int gweaponcrosshair89_init(GWeaponCrosshair89 *crosshair,
                           const char *recipe_root)
{
    const char *root;
    if (!crosshair) return 0;
    memset(crosshair, 0, sizeof(*crosshair));
    root = recipe_root && recipe_root[0]
         ? recipe_root : GWC89_DEFAULT_ROOT;
    if (!gc89r_init(&crosshair->runtime, root)) return 0;
    crosshair->initialized = 1;
    crosshair->enabled = 1;
    crosshair->preset_id = gc89r_preset_id(&crosshair->runtime);
    return 1;
}

void gweaponcrosshair89_set_primitive_provider(
    GWeaponCrosshair89 *crosshair,
    const GC89_DrawCallbacks *callbacks,
    void *user)
{
    GC89P_Runtime *providers;
    if (!crosshair || !crosshair->initialized) return;
    providers = gc89r_providers(&crosshair->runtime);
    if (!providers) return;
    gc89p_runtime_set_primitive_provider(providers, callbacks, user);
}

void gweaponcrosshair89_set_primitive_fallback(
    GWeaponCrosshair89 *crosshair,
    const GC89_DrawCallbacks *callbacks,
    void *user)
{
    GC89P_Runtime *providers;
    if (!crosshair || !crosshair->initialized) return;
    providers = gc89r_providers(&crosshair->runtime);
    if (!providers) return;
    gc89p_runtime_set_primitive_fallback(providers, callbacks, user);
    gc89p_runtime_enable_primitive_fallback(providers, 1);
}

GC89P_Runtime *gweaponcrosshair89_providers(GWeaponCrosshair89 *crosshair)
{
    if (!crosshair || !crosshair->initialized) return 0;
    return gc89r_providers(&crosshair->runtime);
}

int gweaponcrosshair89_find_preset(const char *preset_name)
{
    int i;
    int count;
    const char *name;
    if (!preset_name || !preset_name[0]) return -1;
    count = gcb89_preset_count();
    for (i = 0; i < count; ++i) {
        name = gcb89_preset_name(i);
        if (name && strcmp(name, preset_name) == 0) return i;
    }
    return -1;
}

int gweaponcrosshair89_set_preset_id(GWeaponCrosshair89 *crosshair,
                                    int preset_id)
{
    if (!crosshair || !crosshair->initialized) return 0;
    if (!gc89r_set_preset(&crosshair->runtime, preset_id)) return 0;
    crosshair->preset_id = preset_id;
    crosshair->enabled = 1;
    crosshair->animation_ms_accum = 0UL;
    crosshair->last_aiming = 0;
    crosshair->last_firing = 0;
    crosshair->last_hit = 0;
    return 1;
}

int gweaponcrosshair89_set_preset_name(GWeaponCrosshair89 *crosshair,
                                      const char *preset_name)
{
    int preset_id;
    if (!crosshair || !crosshair->initialized) return 0;
    if (!preset_name || !preset_name[0]) preset_name = "default";
    if (b3d_crosshair_is_disabled_name(preset_name)) {
        crosshair->enabled = 0;
        return 1;
    }
    preset_id = gweaponcrosshair89_find_preset(preset_name);
    if (preset_id < 0) return 0;
    return gweaponcrosshair89_set_preset_id(crosshair, preset_id);
}

const char *gweaponcrosshair89_preset_name(const GWeaponCrosshair89 *crosshair)
{
    if (!crosshair || !crosshair->initialized) return "uninitialized";
    if (!crosshair->enabled) return "none";
    return gc89r_preset_name(&crosshair->runtime);
}

int gweaponcrosshair89_trigger_event(GWeaponCrosshair89 *crosshair,
                                    int event_id)
{
    if (!crosshair || !crosshair->initialized || !crosshair->enabled)
        return 0;
    return gc89r_trigger_event(&crosshair->runtime, event_id);
}

const GC89A_Modifier *gweaponcrosshair89_animation_modifier(
    const GWeaponCrosshair89 *crosshair)
{
    if (!crosshair || !crosshair->initialized) return 0;
    return gc89r_animation_modifier(&crosshair->runtime);
}

int gweaponcrosshair89_draw(GWeaponCrosshair89 *crosshair,
                           int screen_width,
                           int screen_height,
                           int aiming,
                           int firing,
                           int hit,
                           long camera_recoil_x1000,
                           unsigned long frame_ms)
{
    unsigned long animation_ticks;
    if (!crosshair || !crosshair->initialized || !crosshair->enabled)
        return 0;
    if (screen_width <= 0 || screen_height <= 0) return 0;

    gcb89_make_blank3d_input(&crosshair->input,
                             aiming, firing, hit,
                             camera_recoil_x1000);

    /* ABI 2 runtime owns input-edge animation. Feed before update so the
       event can advance during the same rendered frame. gc89r_draw() feeds
       the same state once more, which is edge-safe inside anim89. */
    gc89r_process_input(&crosshair->runtime, &crosshair->input);
    animation_ticks = b3d_crosshair_animation_ticks(crosshair, frame_ms);
    if (animation_ticks > 0UL)
        gc89r_update(&crosshair->runtime, animation_ticks);

    crosshair->last_aiming = aiming ? 1 : 0;
    crosshair->last_firing = firing ? 1 : 0;
    crosshair->last_hit = hit ? 1 : 0;

    return gc89r_draw(&crosshair->runtime,
                      screen_width, screen_height,
                      &crosshair->input,
                      &crosshair->report);
}
