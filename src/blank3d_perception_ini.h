#ifndef BLANK3D_PERCEPTION_INI_H
#define BLANK3D_PERCEPTION_INI_H

#include <stddef.h>

#include "blank3d_perception.h"
#include "blank3d_truth_gate.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Loads only sensory/truth keys from an entity or dedicated perception INI.
   Existing defaults remain active for omitted keys. */
int blank3d_perception_ini_load(const char *path,
                                Blank3DTruthGate *truth_gate,
                                g3d_fix *truth_range,
                                Blank3DPerceptionConfig *perception,
                                char *status,
                                size_t status_capacity);

#ifdef __cplusplus
}
#endif

#endif /* BLANK3D_PERCEPTION_INI_H */
