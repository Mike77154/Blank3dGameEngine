#include "fmuzzle89.h"

static const short fm89_sine_table[256] =
{
    0, 25, 50, 75, 100, 125, 150, 175,
    200, 224, 249, 273, 297, 321, 345, 369,
    392, 415, 438, 460, 483, 505, 526, 548,
    569, 590, 610, 630, 650, 669, 688, 706,
    724, 742, 759, 775, 792, 807, 822, 837,
    851, 865, 878, 891, 903, 915, 926, 936,
    946, 955, 964, 972, 980, 987, 993, 999,
    1004, 1009, 1013, 1016, 1019, 1021, 1023, 1024,
    1024, 1024, 1023, 1021, 1019, 1016, 1013, 1009,
    1004, 999, 993, 987, 980, 972, 964, 955,
    946, 936, 926, 915, 903, 891, 878, 865,
    851, 837, 822, 807, 792, 775, 759, 742,
    724, 706, 688, 669, 650, 630, 610, 590,
    569, 548, 526, 505, 483, 460, 438, 415,
    392, 369, 345, 321, 297, 273, 249, 224,
    200, 175, 150, 125, 100, 75, 50, 25,
    0, -25, -50, -75, -100, -125, -150, -175,
    -200, -224, -249, -273, -297, -321, -345, -369,
    -392, -415, -438, -460, -483, -505, -526, -548,
    -569, -590, -610, -630, -650, -669, -688, -706,
    -724, -742, -759, -775, -792, -807, -822, -837,
    -851, -865, -878, -891, -903, -915, -926, -936,
    -946, -955, -964, -972, -980, -987, -993, -999,
    -1004, -1009, -1013, -1016, -1019, -1021, -1023, -1024,
    -1024, -1024, -1023, -1021, -1019, -1016, -1013, -1009,
    -1004, -999, -993, -987, -980, -972, -964, -955,
    -946, -936, -926, -915, -903, -891, -878, -865,
    -851, -837, -822, -807, -792, -775, -759, -742,
    -724, -706, -688, -669, -650, -630, -610, -590,
    -569, -548, -526, -505, -483, -460, -438, -415,
    -392, -369, -345, -321, -297, -273, -249, -224,
    -200, -175, -150, -125, -100, -75, -50, -25
};

static unsigned long fm89_next_random(FM89_State *state)
{
    state->rng_state = state->rng_state * 1664525UL + 1013904223UL;
    return state->rng_state;
}

static int fm89_rand_range(FM89_State *state, int minimum, int maximum)
{
    unsigned long value;
    unsigned long span;

    if (maximum <= minimum)
    {
        return minimum;
    }

    value = fm89_next_random(state);
    span = (unsigned long)(maximum - minimum + 1);
    return minimum + (int)((value >> 16) % span);
}

static int fm89_clamp_int(int value, int minimum, int maximum)
{
    if (value < minimum)
    {
        return minimum;
    }
    if (value > maximum)
    {
        return maximum;
    }
    return value;
}

static FM89_Fixed fm89_clamp_fixed(FM89_Fixed value,
                                    FM89_Fixed minimum,
                                    FM89_Fixed maximum)
{
    if (value < minimum)
    {
        return minimum;
    }
    if (value > maximum)
    {
        return maximum;
    }
    return value;
}

static int fm89_pick_shape(FM89_State *state)
{
    int choices[3];
    int count;

    count = 0;
    if ((state->config.allowed_shapes & FM89_SHAPE_MASK_TRIANGLE) != 0UL)
    {
        choices[count++] = FM89_SHAPE_TRIANGLE;
    }
    if ((state->config.allowed_shapes & FM89_SHAPE_MASK_LEAF) != 0UL)
    {
        choices[count++] = FM89_SHAPE_LEAF;
    }
    if (count == 0)
    {
        return FM89_SHAPE_TRIANGLE;
    }
    return choices[fm89_rand_range(state, 0, count - 1)];
}

static FM89_Fixed fm89_scale_fraction(FM89_Fixed value,
                                       int numerator,
                                       int denominator)
{
    if (denominator <= 0)
    {
        denominator = 1;
    }
    return (value * (FM89_Fixed)numerator) / (FM89_Fixed)denominator;
}

static void fm89_clear_fin(FM89_Fin *fin)
{
    fin->angle = 0;
    fin->shape = FM89_SHAPE_TRIANGLE;
    fin->roll = 0;
    fin->tier = FM89_FIN_CYL_PRIMARY;
    fin->parent_index = -1;
    fin->anchor_z = 0;
    fin->anchor_radius = 0;
    fin->length = 0;
    fin->half_width = 0;
    fin->outer_half_width = 0;
}

FM89_Fixed fm89_mul(FM89_Fixed a, FM89_Fixed b)
{
    return (a * b) >> FM89_FP_SHIFT;
}

FM89_Fixed fm89_sin(int angle)
{
    return (FM89_Fixed)fm89_sine_table[angle & 255];
}

FM89_Fixed fm89_cos(int angle)
{
    return (FM89_Fixed)fm89_sine_table[(angle + 64) & 255];
}

void fm89_default_config(FM89_Config *config)
{
    config->minimum_primary_fins = 3;
    config->maximum_primary_fins = 8;
    config->allowed_shapes = FM89_SHAPE_MASK_ENABLED;
    config->mixed_shapes = 0;
    config->repeater_enabled = 1;
    config->cone_bank_enabled = 1;
    config->cone_repeater_enabled = 1;
    config->lifetime = FM89_DEFAULT_LIFETIME;

    config->cylinder_length = FM89_CM(2);
    config->cone_length = FM89_CM(18);
    config->core_radius = FM89_MM(25) / 2L;
    config->cone_tip_radius = FM89_MM(2);

    config->minimum_fin_length = FM89_MM(32);
    config->maximum_fin_length = FM89_CM(5);
    config->minimum_fin_width = FM89_MM(16);
    config->maximum_fin_width = FM89_MM(25);

    config->repeat_scale_numerator = 1;
    config->repeat_scale_denominator = 2;
    config->cone_scale_numerator = 1;
    config->cone_scale_denominator = 2;
    config->cone_roll_angle = 24;
}

void fm89_init(FM89_State *state, unsigned long seed)
{
    int i;

    state->rng_state = seed;
    fm89_default_config(&state->config);
    state->active = 0;
    state->age = 0;
    state->lifetime = FM89_DEFAULT_LIFETIME;
    state->primary_count = 0;
    state->total_count = 0;
    state->family_shape = FM89_SHAPE_TRIANGLE;
    state->global_angle = 0;
    state->pitch = 0;
    state->yaw = 0;
    state->cylinder_primary_offset = 0;
    state->cylinder_repeat_offset = 0;
    state->cone_primary_offset = 0;
    state->cone_repeat_offset = 0;
    state->center_x = 0;
    state->center_y = 0;
    state->center_z = FM89_CM(50);
    state->scale = 0;
    state->brightness = 0;

    for (i = 0; i < FM89_MAX_TOTAL_FINS; ++i)
    {
        fm89_clear_fin(&state->fins[i]);
    }
}

void fm89_set_config(FM89_State *state, const FM89_Config *config)
{
    state->config = *config;

    state->config.minimum_primary_fins =
        fm89_clamp_int(state->config.minimum_primary_fins,
                       1, FM89_MAX_PRIMARY_FINS);
    state->config.maximum_primary_fins =
        fm89_clamp_int(state->config.maximum_primary_fins,
                       state->config.minimum_primary_fins,
                       FM89_MAX_PRIMARY_FINS);
    state->config.lifetime = fm89_clamp_int(state->config.lifetime, 1, 16);

    state->config.allowed_shapes &= FM89_SHAPE_MASK_ENABLED;
    if (state->config.allowed_shapes == 0UL)
    {
        state->config.allowed_shapes = FM89_SHAPE_MASK_TRIANGLE;
    }

    state->config.cylinder_length =
        fm89_clamp_fixed(state->config.cylinder_length,
                         FM89_MM(1), FM89_CM(64));
    state->config.cone_length =
        fm89_clamp_fixed(state->config.cone_length,
                         FM89_MM(1), FM89_CM(64));
    state->config.core_radius =
        fm89_clamp_fixed(state->config.core_radius,
                         FM89_MM(1), FM89_CM(16));
    state->config.cone_tip_radius =
        fm89_clamp_fixed(state->config.cone_tip_radius,
                         0, state->config.core_radius);

    state->config.minimum_fin_length =
        fm89_clamp_fixed(state->config.minimum_fin_length,
                         FM89_MM(1), FM89_CM(32));
    state->config.maximum_fin_length =
        fm89_clamp_fixed(state->config.maximum_fin_length,
                         state->config.minimum_fin_length,
                         FM89_CM(32));
    state->config.minimum_fin_width =
        fm89_clamp_fixed(state->config.minimum_fin_width,
                         FM89_MM(1), FM89_CM(16));
    state->config.maximum_fin_width =
        fm89_clamp_fixed(state->config.maximum_fin_width,
                         state->config.minimum_fin_width,
                         FM89_CM(16));

    state->config.repeat_scale_numerator =
        fm89_clamp_int(state->config.repeat_scale_numerator, 1, 16);
    state->config.repeat_scale_denominator =
        fm89_clamp_int(state->config.repeat_scale_denominator,
                       state->config.repeat_scale_numerator, 32);
    state->config.cone_scale_numerator =
        fm89_clamp_int(state->config.cone_scale_numerator, 1, 16);
    state->config.cone_scale_denominator =
        fm89_clamp_int(state->config.cone_scale_denominator,
                       state->config.cone_scale_numerator, 32);
    state->config.cone_roll_angle =
        fm89_clamp_int(state->config.cone_roll_angle, 0, 64);
}

FM89_Fixed fm89_core_total_length(const FM89_State *state)
{
    return state->config.cylinder_length + state->config.cone_length;
}

FM89_Fixed fm89_fin_anchor_z(const FM89_State *state)
{
    (void)state;
    return 0;
}

FM89_Fixed fm89_fin_anchor_radius(const FM89_State *state)
{
    return state->config.core_radius;
}

FM89_Fixed fm89_cone_base_anchor_z(const FM89_State *state)
{
    return state->config.cylinder_length;
}

FM89_Fixed fm89_cone_base_anchor_radius(const FM89_State *state)
{
    return fm89_core_radius_at_z(state, state->config.cylinder_length);
}

FM89_Fixed fm89_core_radius_at_z(const FM89_State *state, FM89_Fixed z)
{
    FM89_Fixed cone_position;
    FM89_Fixed delta;

    if (z <= 0)
    {
        return state->config.core_radius;
    }
    if (z <= state->config.cylinder_length)
    {
        return state->config.core_radius;
    }
    if (z >= fm89_core_total_length(state))
    {
        return state->config.cone_tip_radius;
    }

    cone_position = z - state->config.cylinder_length;
    delta = state->config.core_radius - state->config.cone_tip_radius;
    return state->config.core_radius
         - (delta * cone_position) / state->config.cone_length;
}

static void fm89_make_fin(FM89_Fin *fin,
                          int angle,
                          int shape,
                          int roll,
                          int tier,
                          int parent_index,
                          FM89_Fixed anchor_z,
                          FM89_Fixed anchor_radius,
                          FM89_Fixed length,
                          FM89_Fixed half_width,
                          FM89_Fixed outer_half_width)
{
    fin->angle = angle;
    fin->shape = shape;
    fin->roll = roll;
    fin->tier = tier;
    fin->parent_index = parent_index;
    fin->anchor_z = anchor_z;
    fin->anchor_radius = anchor_radius;
    fin->length = length;
    fin->half_width = half_width;
    fin->outer_half_width = outer_half_width;
}

void fm89_fire(FM89_State *state)
{
    int i;
    int index;
    int primary_angle;
    int repeat_angle;
    int shape;
    int base_roll;
    int cone_roll;
    FM89_Fixed full_width;
    FM89_Fixed half_width;
    FM89_Fixed outer_half_width;
    FM89_Fixed length;
    FM89_Fixed cone_length;
    FM89_Fixed cone_half_width;
    FM89_Fixed cone_outer_half_width;
    FM89_Fixed cone_repeat_length;
    FM89_Fixed cone_repeat_half_width;
    FM89_Fixed cone_repeat_outer_half_width;

    state->active = 1;
    state->age = 0;
    state->lifetime = state->config.lifetime;
    state->primary_count = fm89_rand_range(
        state,
        state->config.minimum_primary_fins,
        state->config.maximum_primary_fins);
    state->family_shape = fm89_pick_shape(state);
    state->global_angle = fm89_rand_range(state, 0, 255);
    state->pitch = fm89_rand_range(state, -15, -9);
    state->yaw = fm89_rand_range(state, -42, -34);
    state->center_x = (FM89_Fixed)fm89_rand_range(
        state, -FM89_MM(5), FM89_MM(5));
    state->center_y = (FM89_Fixed)fm89_rand_range(
        state, -FM89_MM(4), FM89_MM(4));
    state->center_z = FM89_CM(50)
                    + (FM89_Fixed)fm89_rand_range(
                        state, -FM89_CM(1), FM89_CM(1));

    for (i = 0; i < FM89_MAX_TOTAL_FINS; ++i)
    {
        fm89_clear_fin(&state->fins[i]);
    }

    index = 0;
    state->cylinder_primary_offset = index;
    index += state->primary_count;

    if (state->config.repeater_enabled)
    {
        state->cylinder_repeat_offset = index;
        index += state->primary_count;
    }
    else
    {
        state->cylinder_repeat_offset = -1;
    }

    if (state->config.cone_bank_enabled)
    {
        state->cone_primary_offset = index;
        index += state->primary_count;
    }
    else
    {
        state->cone_primary_offset = -1;
    }

    if (state->config.cone_bank_enabled && state->config.cone_repeater_enabled)
    {
        state->cone_repeat_offset = index;
        index += state->primary_count;
    }
    else
    {
        state->cone_repeat_offset = -1;
    }

    state->total_count = index;

    for (i = 0; i < state->primary_count; ++i)
    {
        primary_angle = (i * 256) / state->primary_count;
        repeat_angle = ((2 * i + 1) * 128) / state->primary_count;
        shape = state->config.mixed_shapes
              ? fm89_pick_shape(state)
              : state->family_shape;
        base_roll = fm89_rand_range(state, -8, 8);
        full_width = (FM89_Fixed)fm89_rand_range(
            state,
            (int)state->config.minimum_fin_width,
            (int)state->config.maximum_fin_width);
        length = (FM89_Fixed)fm89_rand_range(
            state,
            (int)state->config.minimum_fin_length,
            (int)state->config.maximum_fin_length);
        half_width = full_width / 2L;
        outer_half_width =
            (half_width * (FM89_Fixed)fm89_rand_range(
                state, 65, 100)) / 100L;

        fm89_make_fin(&state->fins[state->cylinder_primary_offset + i],
                      primary_angle,
                      shape,
                      base_roll,
                      FM89_FIN_CYL_PRIMARY,
                      -1,
                      fm89_fin_anchor_z(state),
                      fm89_fin_anchor_radius(state),
                      length,
                      half_width,
                      outer_half_width);

        if (state->cylinder_repeat_offset >= 0)
        {
            fm89_make_fin(&state->fins[state->cylinder_repeat_offset + i],
                          repeat_angle,
                          shape,
                          base_roll,
                          FM89_FIN_CYL_REPEAT,
                          state->cylinder_primary_offset + i,
                          fm89_fin_anchor_z(state),
                          fm89_fin_anchor_radius(state),
                          fm89_scale_fraction(length,
                                              state->config.repeat_scale_numerator,
                                              state->config.repeat_scale_denominator),
                          fm89_scale_fraction(half_width,
                                              state->config.repeat_scale_numerator,
                                              state->config.repeat_scale_denominator),
                          fm89_scale_fraction(outer_half_width,
                                              state->config.repeat_scale_numerator,
                                              state->config.repeat_scale_denominator));
        }

        if (state->cone_primary_offset >= 0)
        {
            cone_roll = (i & 1)
                      ? -state->config.cone_roll_angle
                      : state->config.cone_roll_angle;
            cone_length = fm89_scale_fraction(length,
                                              state->config.cone_scale_numerator,
                                              state->config.cone_scale_denominator);
            cone_half_width = fm89_scale_fraction(half_width,
                                                  state->config.cone_scale_numerator,
                                                  state->config.cone_scale_denominator);
            cone_outer_half_width = fm89_scale_fraction(outer_half_width,
                                                        state->config.cone_scale_numerator,
                                                        state->config.cone_scale_denominator);
            fm89_make_fin(&state->fins[state->cone_primary_offset + i],
                          primary_angle,
                          shape,
                          cone_roll,
                          FM89_FIN_CONE_PRIMARY,
                          state->cylinder_primary_offset + i,
                          fm89_cone_base_anchor_z(state),
                          fm89_cone_base_anchor_radius(state),
                          cone_length,
                          cone_half_width,
                          cone_outer_half_width);

            if (state->cone_repeat_offset >= 0)
            {
                cone_repeat_length = fm89_scale_fraction(cone_length,
                                                         state->config.repeat_scale_numerator,
                                                         state->config.repeat_scale_denominator);
                cone_repeat_half_width = fm89_scale_fraction(cone_half_width,
                                                             state->config.repeat_scale_numerator,
                                                             state->config.repeat_scale_denominator);
                cone_repeat_outer_half_width = fm89_scale_fraction(cone_outer_half_width,
                                                                   state->config.repeat_scale_numerator,
                                                                   state->config.repeat_scale_denominator);
                fm89_make_fin(&state->fins[state->cone_repeat_offset + i],
                              repeat_angle,
                              shape,
                              cone_roll,
                              FM89_FIN_CONE_REPEAT,
                              state->cone_primary_offset + i,
                              fm89_cone_base_anchor_z(state),
                              fm89_cone_base_anchor_radius(state),
                              cone_repeat_length,
                              cone_repeat_half_width,
                              cone_repeat_outer_half_width);
            }
        }
    }
}

void fm89_update(FM89_State *state)
{
    int age;

    if (!state->active)
    {
        return;
    }

    if (state->age >= state->lifetime)
    {
        state->active = 0;
        state->scale = 0;
        state->brightness = 0;
        return;
    }

    age = state->age;
    if (age == 0)
    {
        state->scale = (FM89_FP_ONE * 7L) / 10L;
        state->brightness = FM89_FP_ONE;
    }
    else if (age == 1)
    {
        state->scale = (FM89_FP_ONE * 11L) / 10L;
        state->brightness = FM89_FP_ONE;
    }
    else if (age == 2)
    {
        state->scale = (FM89_FP_ONE * 8L) / 10L;
        state->brightness = (FM89_FP_ONE * 7L) / 10L;
    }
    else
    {
        state->scale = (FM89_FP_ONE * 4L) / 10L;
        state->brightness = (FM89_FP_ONE * 3L) / 10L;
    }

    state->age += 1;
}
