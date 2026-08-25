#ifndef GWEAPONCROSSHAIR89_H
#define GWEAPONCROSSHAIR89_H

#include "gcrosshair_runtime89.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GWC89_DEFAULT_ROOT "config/crosshair/gcrosshair.ini"
#define GWC89_ANIM_TICK_MS 16UL

typedef struct GWeaponCrosshair89Tag {
    GC89R_Runtime runtime;
    GC89_InputState input;
    GC89P_DrawReport report;
    unsigned long animation_ms_accum;
    int initialized;
    int enabled;
    int preset_id;
    int last_aiming;
    int last_firing;
    int last_hit;
} GWeaponCrosshair89;

int gweaponcrosshair89_init(GWeaponCrosshair89 *crosshair,
                           const char *recipe_root);
void gweaponcrosshair89_set_primitive_provider(
    GWeaponCrosshair89 *crosshair,
    const GC89_DrawCallbacks *callbacks,
    void *user);
void gweaponcrosshair89_set_primitive_fallback(
    GWeaponCrosshair89 *crosshair,
    const GC89_DrawCallbacks *callbacks,
    void *user);
GC89P_Runtime *gweaponcrosshair89_providers(GWeaponCrosshair89 *crosshair);
int gweaponcrosshair89_find_preset(const char *preset_name);
int gweaponcrosshair89_set_preset_id(GWeaponCrosshair89 *crosshair,
                                    int preset_id);
int gweaponcrosshair89_set_preset_name(GWeaponCrosshair89 *crosshair,
                                      const char *preset_name);
const char *gweaponcrosshair89_preset_name(const GWeaponCrosshair89 *crosshair);

/* Manual recipe events are useful for weapon-specific HUD feedback beyond
   AIM/FIRE/HIT, e.g. lock-on, reload confirm or custom scripted pulses. */
int gweaponcrosshair89_trigger_event(GWeaponCrosshair89 *crosshair,
                                    int event_id);
const GC89A_Modifier *gweaponcrosshair89_animation_modifier(
    const GWeaponCrosshair89 *crosshair);

int gweaponcrosshair89_draw(GWeaponCrosshair89 *crosshair,
                           int screen_width,
                           int screen_height,
                           int aiming,
                           int firing,
                           int hit,
                           long camera_recoil_x1000,
                           unsigned long frame_ms);

#ifdef __cplusplus
}
#endif

#endif
