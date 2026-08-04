#include "blank3d_ecg_vitals.h"

#include <stdio.h>
#include <string.h>

static int expect_state(Blank3DEcgVitals *vitals,
                        int health,
                        const char *expected)
{
    const char *actual;
    blank3d_ecg_vitals_update(vitals, health, 0, 0u, 16u);
    actual = ecg_hud_config_state_name(&vitals->config);
    if (!actual || strcmp(actual, expected) != 0) {
        fprintf(stderr, "health %d: expected %s, got %s\n",
                health, expected, actual ? actual : "(null)");
        return 0;
    }
    return 1;
}

int main(void)
{
    Blank3DEcgVitals vitals;
    unsigned int calm_bpm;
    unsigned int threat_bpm;
    const ECG_Surface *surface;

    blank3d_ecg_vitals_init(&vitals);
    if (!expect_state(&vitals, 100, "FINE")) return 1;
    if (!expect_state(&vitals, 74, "CAUTION")) return 2;
    if (!expect_state(&vitals, 49, "ORANGE")) return 3;
    if (!expect_state(&vitals, 24, "DANGER")) return 4;
    if (!expect_state(&vitals, 0, "FLATLINE")) return 5;

    blank3d_ecg_vitals_update(&vitals, 100, 0, 0u, 16u);
    calm_bpm = blank3d_ecg_vitals_bpm(&vitals);
    blank3d_ecg_vitals_update(&vitals, 100, 100, 0u, 16u);
    threat_bpm = blank3d_ecg_vitals_bpm(&vitals);
    if (threat_bpm <= calm_bpm) {
        fprintf(stderr, "threat did not accelerate pulse\n");
        return 6;
    }

    blank3d_ecg_vitals_update(&vitals, 80, 50, 350u, 16u);
    if ((vitals.config.draw_flags & ECG_HUD_DRAW_OVERLAY_BOX) == 0ul) {
        fprintf(stderr, "damage overlay did not arm\n");
        return 7;
    }

    surface = blank3d_ecg_vitals_surface(&vitals);
    if (!surface || surface->width != B3D_ECG_WIDTH ||
        surface->height != B3D_ECG_HEIGHT) {
        fprintf(stderr, "invalid ECG surface\n");
        return 8;
    }

    printf("ECG vitals binding OK: calm=%u threat=%u\n",
           calm_bpm, threat_bpm);
    return 0;
}
