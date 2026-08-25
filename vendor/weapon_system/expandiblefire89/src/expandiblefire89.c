#include "expandiblefire89.h"

#include <string.h>

static ef89_fx ef89_abs(ef89_fx value)
{
    return value < 0L ? -value : value;
}

static ef89_fx ef89_max3(ef89_fx a, ef89_fx b, ef89_fx c)
{
    ef89_fx m;
    m = a > b ? a : b;
    return m > c ? m : c;
}

static unsigned long ef89_isqrt(unsigned long value)
{
    unsigned long result;
    unsigned long bit;
    result = 0UL;
    bit = 1UL << 30;
    while (bit > value) bit >>= 2;
    while (bit != 0UL) {
        if (value >= result + bit) {
            value -= result + bit;
            result = (result >> 1) + bit;
        } else {
            result >>= 1;
        }
        bit >>= 2;
    }
    return result;
}

/* Safe Q20.12 vector length for Win32 long. Components are first normalized
 * against their largest axis, avoiding squaring world-coordinate magnitudes. */
static ef89_fx ef89_vec_length(ef89_vec3 value)
{
    ef89_fx m;
    ef89_fx sx;
    ef89_fx sy;
    ef89_fx sz;
    unsigned long sum;
    unsigned long root;

    m = ef89_max3(ef89_abs(value.x), ef89_abs(value.y), ef89_abs(value.z));
    if (m <= 0L) return 0L;
    sx = (value.x * EF89_ONE) / m;
    sy = (value.y * EF89_ONE) / m;
    sz = (value.z * EF89_ONE) / m;
    sum = (unsigned long)(sx * sx) +
          (unsigned long)(sy * sy) +
          (unsigned long)(sz * sz);
    root = ef89_isqrt(sum);
    if (root == 0UL) return 0L;
    return (m * (ef89_fx)root) / EF89_ONE;
}

static ef89_fx ef89_clamp01(ef89_fx value)
{
    if (value < 0L) return 0L;
    if (value > EF89_ONE) return EF89_ONE;
    return value;
}

static ef89_fx ef89_lerp(ef89_fx a, ef89_fx b, ef89_fx t)
{
    t = ef89_clamp01(t);
    return a + ((b - a) * t) / EF89_ONE;
}

void expandiblefire89_config_default(ef89_config *config)
{
    if (!config) return;
    config->scale_start_fx = EF89_ONE;
    config->scale_end_fx = EF89_ONE;
    config->growth_distance_fx = EF89_ONE;
    config->kill_distance_fx = 0L;
}

void expandiblefire89_init(ef89_state *state, ef89_vec3 spawn_position,
                           const ef89_config *config)
{
    ef89_config fallback;
    if (!state) return;
    if (!config) {
        expandiblefire89_config_default(&fallback);
        config = &fallback;
    }
    memset(state, 0, sizeof(*state));
    state->active = 1;
    state->spawn_position = spawn_position;
    state->previous_position = spawn_position;
    state->distance_travelled_fx = 0L;
    state->scale_fx = config->scale_start_fx > 0L
        ? config->scale_start_fx : EF89_ONE;
}

int expandiblefire89_step(ef89_state *state, const ef89_config *config,
                          ef89_vec3 current_position, ef89_result *result)
{
    ef89_vec3 delta;
    ef89_fx segment;
    ef89_fx progress;
    ef89_fx start_scale;
    ef89_fx end_scale;
    ef89_fx growth_distance;

    if (!state || !config || !result) return 0;
    memset(result, 0, sizeof(*result));
    if (!state->active) return 1;

    delta.x = current_position.x - state->previous_position.x;
    delta.y = current_position.y - state->previous_position.y;
    delta.z = current_position.z - state->previous_position.z;
    segment = ef89_vec_length(delta);
    if (segment > 0L)
        state->distance_travelled_fx += segment;
    state->previous_position = current_position;

    start_scale = config->scale_start_fx > 0L
        ? config->scale_start_fx : EF89_ONE;
    end_scale = config->scale_end_fx > 0L
        ? config->scale_end_fx : start_scale;
    growth_distance = config->growth_distance_fx;
    if (growth_distance <= 0L)
        progress = EF89_ONE;
    else
        progress = (state->distance_travelled_fx * EF89_ONE) /
                   growth_distance;
    progress = ef89_clamp01(progress);
    state->scale_fx = ef89_lerp(start_scale, end_scale, progress);

    if (config->kill_distance_fx > 0L &&
        state->distance_travelled_fx >= config->kill_distance_fx)
        state->active = 0;

    result->alive = state->active;
    result->distance_travelled_fx = state->distance_travelled_fx;
    result->progress_fx = progress;
    result->scale_fx = state->scale_fx;
    return 1;
}

ef89_fx expandiblefire89_apply_scale(ef89_fx base_value,
                                     ef89_fx scale_fx)
{
    if (base_value <= 0L) return 0L;
    if (scale_fx <= 0L) return base_value;
    return (base_value * scale_fx) / EF89_ONE;
}
