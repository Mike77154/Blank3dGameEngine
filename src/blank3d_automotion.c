#include "blank3d_automotion.h"

#include <ctype.h>
#include <limits.h>
#include <string.h>

static GMoveFx89 b3d_q12_to_q16(g3d_fix value)
{
    long v;
    v = (long)value;
    if (v > GMOVE89_FX_MAX / 16L) return GMOVE89_FX_MAX;
    if (v < GMOVE89_FX_MIN / 16L) return GMOVE89_FX_MIN;
    return (GMoveFx89)(v * 16L);
}

static g3d_fix b3d_q16_to_q12(GMoveFx89 value)
{
    long v;
    v = value / 16L;
    if (v > (long)INT_MAX) v = (long)INT_MAX;
    if (v < (long)INT_MIN) v = (long)INT_MIN;
    return (g3d_fix)v;
}

static GMoveVec3_89 b3d_vec_to_gmove(const Vec3 *value)
{
    if (!value) return gmove89_vec3_zero();
    return gmove89_vec3(b3d_q12_to_q16(value->x),
                        b3d_q12_to_q16(value->y),
                        b3d_q12_to_q16(value->z));
}

static Vec3 b3d_vec_from_gmove(GMoveVec3_89 value)
{
    return gamlib_vec3(b3d_q16_to_q12(value.x),
                       b3d_q16_to_q12(value.y),
                       b3d_q16_to_q12(value.z));
}

static int b3d_text_equal(const char *left, const char *right)
{
    unsigned char a;
    unsigned char b;
    if (!left || !right) return 0;
    while (*left && *right) {
        a = (unsigned char)*left;
        b = (unsigned char)*right;
        if (a == '-' || a == ' ') a = '_';
        if (b == '-' || b == ' ') b = '_';
        if (tolower(a) != tolower(b)) return 0;
        ++left;
        ++right;
    }
    return *left == '\0' && *right == '\0';
}

static int b3d_style_from_text(const char *style)
{
    if (b3d_text_equal(style, "zigzag") ||
        b3d_text_equal(style, "zig_zag"))
        return GMOVEPATTERN89_ZIGZAG_TO;
    if (b3d_text_equal(style, "helix") ||
        b3d_text_equal(style, "spiral"))
        return GMOVEPATTERN89_HELIX_TO;
    if (b3d_text_equal(style, "pingpong") ||
        b3d_text_equal(style, "ping_pong") ||
        b3d_text_equal(style, "strafe"))
        return GMOVEPATTERN89_PINGPONG;
    if (b3d_text_equal(style, "orbit") ||
        b3d_text_equal(style, "circle"))
        return GMOVEPATTERN89_ORBIT;
    if (b3d_text_equal(style, "sine") ||
        b3d_text_equal(style, "sine_to") ||
        b3d_text_equal(style, "wave"))
        return GMOVEPATTERN89_SINE_TO;
    return 0;
}

void blank3d_automotion_init(Blank3DAutomotion *motion)
{
    if (!motion) return;
    memset(motion, 0, sizeof(*motion));
    gmovepattern89_config_default(&motion->pattern_config);
    gmovepattern89_reset(&motion->pattern_state);
    gautmove89_config_default(&motion->directed_config);
    motion->pattern_config.type = GMOVEPATTERN89_SINE_TO;
    motion->pattern_config.amplitude = GMOVE89_FX_ONE * 3L / 4L;
    motion->pattern_config.radius = GMOVE89_FX_ONE;
    motion->pattern_config.frequency = GMOVE89_FX_ONE * 5L / 4L;
    motion->pattern_config.stop_distance = GMOVE89_FX_ONE / 2L;
    motion->directed_config.stop_distance = GMOVE89_FX_ONE / 2L;
    motion->directed_config.slowdown_distance = GMOVE89_FX_FROM_INT(3);
    motion->directed_config.speed_mode = GAUTMOVE89_SPEED_SLOWDOWN;
    motion->safe_distance = G3D_FIX_FROM_INT(6);
    motion->enabled = 1;
}

void blank3d_automotion_init_archetype(Blank3DAutomotion *motion,
                                       const char *archetype)
{
    blank3d_automotion_init(motion);
    if (!motion || !archetype) return;
    if (strstr(archetype, "hopper") != 0) {
        motion->pattern_config.type = GMOVEPATTERN89_ZIGZAG_TO;
        motion->pattern_config.amplitude = GMOVE89_FX_ONE * 2L / 3L;
        motion->pattern_config.frequency = GMOVE89_FX_ONE * 7L / 4L;
        motion->pattern_config.stop_distance = GMOVE89_FX_ONE;
    } else if (strstr(archetype, "dive") != 0 ||
               strstr(archetype, "bomber") != 0) {
        motion->pattern_config.type = GMOVEPATTERN89_SINE_TO;
        motion->pattern_config.amplitude = GMOVE89_FX_ONE / 2L;
        motion->pattern_config.frequency = GMOVE89_FX_ONE;
        motion->pattern_config.stop_distance = GMOVE89_FX_ONE / 4L;
    } else if (strstr(archetype, "gunner") != 0 ||
               strstr(archetype, "machinegun") != 0) {
        motion->pattern_config.type = GMOVEPATTERN89_PINGPONG;
        motion->pattern_config.amplitude = GMOVE89_FX_ONE;
        motion->pattern_config.frequency = GMOVE89_FX_ONE / 2L;
    }
}

void blank3d_automotion_reset(Blank3DAutomotion *motion)
{
    if (!motion) return;
    gmovepattern89_reset(&motion->pattern_state);
}

int blank3d_automotion_set_style(Blank3DAutomotion *motion,
                                 const char *style)
{
    int type;
    if (!motion || !style) return 0;
    type = b3d_style_from_text(style);
    if (type == 0) return 0;
    if (motion->pattern_config.type != type) {
        motion->pattern_config.type = type;
        blank3d_automotion_reset(motion);
    }
    return 1;
}

const char *blank3d_automotion_style_name(const Blank3DAutomotion *motion)
{
    if (!motion) return "none";
    switch (motion->pattern_config.type) {
    case GMOVEPATTERN89_ZIGZAG_TO: return "zigzag";
    case GMOVEPATTERN89_HELIX_TO: return "helix";
    case GMOVEPATTERN89_PINGPONG: return "pingpong";
    case GMOVEPATTERN89_ORBIT: return "orbit";
    case GMOVEPATTERN89_SINE_TO: return "sine";
    default: return "unknown";
    }
}

void blank3d_automotion_set_enabled(Blank3DAutomotion *motion, int enabled)
{
    if (!motion) return;
    motion->enabled = enabled ? 1 : 0;
    if (!motion->enabled) blank3d_automotion_reset(motion);
}

void blank3d_automotion_set_amplitude(Blank3DAutomotion *motion,
                                      g3d_fix amplitude)
{
    if (!motion) return;
    if (amplitude < 0) amplitude = -amplitude;
    motion->pattern_config.amplitude = b3d_q12_to_q16(amplitude);
}

void blank3d_automotion_set_radius(Blank3DAutomotion *motion,
                                   g3d_fix radius)
{
    if (!motion) return;
    if (radius < 0) radius = -radius;
    motion->pattern_config.radius = b3d_q12_to_q16(radius);
}

void blank3d_automotion_set_frequency(Blank3DAutomotion *motion,
                                      g3d_fix frequency)
{
    if (!motion) return;
    if (frequency < 0) frequency = -frequency;
    motion->pattern_config.frequency = b3d_q12_to_q16(frequency);
}

void blank3d_automotion_set_stop_distance(Blank3DAutomotion *motion,
                                          g3d_fix stop_distance)
{
    GMoveFx89 value;
    if (!motion) return;
    if (stop_distance < 0) stop_distance = -stop_distance;
    value = b3d_q12_to_q16(stop_distance);
    motion->pattern_config.stop_distance = value;
    motion->directed_config.stop_distance = value;
}

void blank3d_automotion_set_safe_distance(Blank3DAutomotion *motion,
                                          g3d_fix safe_distance)
{
    if (!motion) return;
    if (safe_distance < 0) safe_distance = -safe_distance;
    motion->safe_distance = safe_distance;
}

static void b3d_flatten_pair(GMoveVec3_89 *current,
                             GMoveVec3_89 *target,
                             GMoveFx89 *saved_y)
{
    *saved_y = current->y;
    current->y = 0L;
    target->y = 0L;
}

static void b3d_finish_position(const GMoveMotion89 *result,
                                int flat_only,
                                GMoveFx89 saved_y,
                                Vec3 *out_position,
                                int *out_reached)
{
    GMoveVec3_89 position;
    position = result->target_position;
    if (flat_only) position.y = saved_y;
    if (out_position) *out_position = b3d_vec_from_gmove(position);
    if (out_reached) *out_reached = result->reached;
}

int blank3d_automotion_step_pattern(Blank3DAutomotion *motion,
                                    const Vec3 *current,
                                    const Vec3 *target,
                                    g3d_fix speed,
                                    g3d_fix dt,
                                    int flat_only,
                                    Vec3 *out_position,
                                    int *out_reached)
{
    GMoveVec3_89 current89;
    GMoveVec3_89 target89;
    GMoveFx89 saved_y;
    GMoveMotion89 result;
    if (!motion || !current || !target || !out_position ||
        !motion->enabled) return 0;
    current89 = b3d_vec_to_gmove(current);
    target89 = b3d_vec_to_gmove(target);
    saved_y = current89.y;
    if (flat_only) b3d_flatten_pair(&current89, &target89, &saved_y);
    motion->pattern_config.speed = b3d_q12_to_q16(speed);
    if (motion->pattern_state.completed)
        gmovepattern89_reset(&motion->pattern_state);
    gmovepattern89_step(&motion->pattern_state,
                        &motion->pattern_config,
                        current89, target89,
                        b3d_q12_to_q16(dt), &result);
    if (!result.valid || !result.has_translation) return 0;
    b3d_finish_position(&result, flat_only, saved_y,
                        out_position, out_reached);
    return 1;
}

int blank3d_automotion_step_direct(Blank3DAutomotion *motion,
                                   const Vec3 *current,
                                   const Vec3 *target,
                                   g3d_fix speed,
                                   g3d_fix dt,
                                   int flat_only,
                                   Vec3 *out_position,
                                   int *out_reached)
{
    GMoveVec3_89 current89;
    GMoveVec3_89 target89;
    GMoveFx89 saved_y;
    GMoveMotion89 result;
    if (!motion || !current || !target || !out_position) return 0;
    current89 = b3d_vec_to_gmove(current);
    target89 = b3d_vec_to_gmove(target);
    saved_y = current89.y;
    if (flat_only) b3d_flatten_pair(&current89, &target89, &saved_y);
    motion->directed_config.speed = b3d_q12_to_q16(speed);
    gautmove89_step_to_point(current89, target89,
                             &motion->directed_config,
                             b3d_q12_to_q16(dt), &result);
    if (!result.valid || !result.has_translation) return 0;
    b3d_finish_position(&result, flat_only, saved_y,
                        out_position, out_reached);
    return 1;
}

int blank3d_automotion_step_away(Blank3DAutomotion *motion,
                                 const Vec3 *current,
                                 const Vec3 *threat,
                                 const Vec3 *fallback_direction,
                                 g3d_fix speed,
                                 g3d_fix safe_distance,
                                 g3d_fix dt,
                                 int flat_only,
                                 Vec3 *out_position,
                                 int *out_reached)
{
    GMoveVec3_89 current89;
    GMoveVec3_89 threat89;
    GMoveVec3_89 fallback89;
    GMoveFx89 saved_y;
    GMoveMotion89 result;
    if (!motion || !current || !threat || !fallback_direction ||
        !out_position) return 0;
    current89 = b3d_vec_to_gmove(current);
    threat89 = b3d_vec_to_gmove(threat);
    fallback89 = b3d_vec_to_gmove(fallback_direction);
    saved_y = current89.y;
    if (flat_only) {
        b3d_flatten_pair(&current89, &threat89, &saved_y);
        fallback89.y = 0L;
    }
    motion->directed_config.speed = b3d_q12_to_q16(speed);
    gautmove89_step_away_from_point(current89, threat89, fallback89,
                                    &motion->directed_config,
                                    b3d_q12_to_q16(safe_distance),
                                    b3d_q12_to_q16(dt), &result);
    if (!result.valid || !result.has_translation) return 0;
    b3d_finish_position(&result, flat_only, saved_y,
                        out_position, out_reached);
    return 1;
}
