#ifndef BLANK3D_AUTOMOTION_H
#define BLANK3D_AUTOMOTION_H

#include "../vendor/gamlib3d/gamlib3d_transform.h"
#include "gautmove89.h"
#include "gmovepattern89.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Blank3DAutomotionTag {
    GMovePatternConfig89 pattern_config;
    GMovePatternState89 pattern_state;
    GAutMoveConfig89 directed_config;
    g3d_fix safe_distance;
    int enabled;
} Blank3DAutomotion;

void blank3d_automotion_init(Blank3DAutomotion *motion);
void blank3d_automotion_init_archetype(Blank3DAutomotion *motion,
                                       const char *archetype);
void blank3d_automotion_reset(Blank3DAutomotion *motion);
int blank3d_automotion_set_style(Blank3DAutomotion *motion,
                                 const char *style);
const char *blank3d_automotion_style_name(const Blank3DAutomotion *motion);
void blank3d_automotion_set_enabled(Blank3DAutomotion *motion, int enabled);
void blank3d_automotion_set_amplitude(Blank3DAutomotion *motion,
                                      g3d_fix amplitude);
void blank3d_automotion_set_radius(Blank3DAutomotion *motion,
                                   g3d_fix radius);
void blank3d_automotion_set_frequency(Blank3DAutomotion *motion,
                                      g3d_fix frequency);
void blank3d_automotion_set_stop_distance(Blank3DAutomotion *motion,
                                          g3d_fix stop_distance);
void blank3d_automotion_set_safe_distance(Blank3DAutomotion *motion,
                                          g3d_fix safe_distance);

int blank3d_automotion_step_pattern(Blank3DAutomotion *motion,
                                    const Vec3 *current,
                                    const Vec3 *target,
                                    g3d_fix speed,
                                    g3d_fix dt,
                                    int flat_only,
                                    Vec3 *out_position,
                                    int *out_reached);
int blank3d_automotion_step_direct(Blank3DAutomotion *motion,
                                   const Vec3 *current,
                                   const Vec3 *target,
                                   g3d_fix speed,
                                   g3d_fix dt,
                                   int flat_only,
                                   Vec3 *out_position,
                                   int *out_reached);
int blank3d_automotion_step_away(Blank3DAutomotion *motion,
                                 const Vec3 *current,
                                 const Vec3 *threat,
                                 const Vec3 *fallback_direction,
                                 g3d_fix speed,
                                 g3d_fix safe_distance,
                                 g3d_fix dt,
                                 int flat_only,
                                 Vec3 *out_position,
                                 int *out_reached);

#ifdef __cplusplus
}
#endif

#endif
