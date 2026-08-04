#ifndef GBAR89_BIGHUD_H
#define GBAR89_BIGHUD_H

#include "gbar89.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GBAR89_BIGHUD_UNKNOWN 0
#define GBAR89_BIGHUD_APPLIED 1
#define GBAR89_BIGHUD_BAD_VALUE (-1)

/*
 * Applies one textual BigVaderHudder property directly to a GBar89 meter.
 * The bridge intentionally mirrors every public field in GBar89_Meter and
 * GBar89_Style so presentation can live in a .bighud file instead of C.
 */
int gbar89_bighud_apply_property(GBar89_Meter *meter,
                                 const char *key,
                                 const char *raw_value);

#ifdef __cplusplus
}
#endif

#endif
