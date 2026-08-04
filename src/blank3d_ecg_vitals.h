#ifndef BLANK3D_ECG_VITALS_H
#define BLANK3D_ECG_VITALS_H

#include "ecg.h"

#ifdef __cplusplus
extern "C" {
#endif

#define B3D_ECG_WIDTH 360u
#define B3D_ECG_HEIGHT 160u
#define B3D_ECG_PROFILE_COUNT 6u
#define B3D_ECG_PROFILE_FLATLINE 5u

typedef struct Blank3DEcgDynamicsTag {
    int dynamic_state;
    int dynamic_bpm;
    int dynamic_offset;
    int dynamic_text;
    int dynamic_damage_overlay;
    int phase_direction;

    int danger_below;
    int orange_below;
    int caution_below;

    unsigned int bpm_base;
    unsigned int bpm_threat_span;
    unsigned int bpm_injury_span;
    unsigned int bpm_min;
    unsigned int bpm_max;

    unsigned int interval_numerator;
    unsigned int interval_min_ms;
    unsigned int interval_max_ms;
    unsigned int damage_interval_ms;
    unsigned int damage_flash_period_ms;
    unsigned int fixed_offset;

    ECG_Color clear_color;
} Blank3DEcgDynamics;

typedef struct Blank3DEcgVitalsTag {
    ECG_Surface surface;
    ECG_Profile profiles[B3D_ECG_PROFILE_COUNT];
    ECG_Profile render_profiles[B3D_ECG_PROFILE_COUNT];
    ECG_HudConfig config;
    Blank3DEcgDynamics dynamics;
    ECG_Color pixels[B3D_ECG_WIDTH * B3D_ECG_HEIGHT];
    unsigned int phase;
    unsigned int phase_accum_ms;
    unsigned int bpm;
    int health;
    int threat_level;
    int initialized;
} Blank3DEcgVitals;

void blank3d_ecg_vitals_init(Blank3DEcgVitals *vitals);
void blank3d_ecg_vitals_update(Blank3DEcgVitals *vitals,
                               int health,
                               int threat_level,
                               unsigned int damage_flash_ms,
                               unsigned int frame_ms);
int blank3d_ecg_vitals_apply_bighud_property(Blank3DEcgVitals *vitals,
                                             const char *key,
                                             const char *value);
const ECG_Surface *blank3d_ecg_vitals_surface(
                               const Blank3DEcgVitals *vitals);
unsigned int blank3d_ecg_vitals_bpm(const Blank3DEcgVitals *vitals);

#ifdef __cplusplus
}
#endif

#endif
