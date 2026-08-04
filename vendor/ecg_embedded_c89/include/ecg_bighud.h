#ifndef ECG_BIGHUD_H
#define ECG_BIGHUD_H

#include "ecg_hud.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ECG_BIGHUD_UNKNOWN 0
#define ECG_BIGHUD_APPLIED 1
#define ECG_BIGHUD_BAD_VALUE (-1)

/* Applies one BigVaderHudder property to every public ECG HUD/render field. */
int ecg_bighud_apply_property(ECG_HudConfig *config,
                              const char *key,
                              const char *raw_value);

#ifdef __cplusplus
}
#endif

#endif
