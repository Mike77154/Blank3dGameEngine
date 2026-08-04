#include "boca_exprosiv3d.h"

static const short bex3d_sine_table[256] =
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


static int bex3d_clamp_int(int value, int minimum, int maximum)
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

static BEX3D_Fixed bex3d_clamp_fixed(BEX3D_Fixed value,
                                     BEX3D_Fixed minimum,
                                     BEX3D_Fixed maximum)
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

BEX3D_Fixed bex3d_mul(BEX3D_Fixed a, BEX3D_Fixed b)
{
    return (a * b) >> BEX3D_FP_SHIFT;
}

BEX3D_Fixed bex3d_div(BEX3D_Fixed a, BEX3D_Fixed b)
{
    if (b == 0)
    {
        return 0;
    }
    return (a << BEX3D_FP_SHIFT) / b;
}

BEX3D_Fixed bex3d_sin(int angle)
{
    return (BEX3D_Fixed)bex3d_sine_table[angle & 255];
}

BEX3D_Fixed bex3d_cos(int angle)
{
    return (BEX3D_Fixed)bex3d_sine_table[(angle + 64) & 255];
}

static unsigned long bex3d_next_random(BEX3D_State *state)
{
    state->rng_state =
        (state->rng_state * 1664525UL + 1013904223UL) & 0xFFFFFFFFUL;
    return state->rng_state;
}

static int bex3d_rand_range(BEX3D_State *state, int minimum, int maximum)
{
    unsigned long value;
    unsigned long span;

    if (maximum <= minimum)
    {
        return minimum;
    }

    value = bex3d_next_random(state);
    span = (unsigned long)(maximum - minimum + 1);
    return minimum + (int)((value >> 16) % span);
}

static BEX3D_Fixed bex3d_rand_fixed(BEX3D_State *state,
                                    BEX3D_Fixed minimum,
                                    BEX3D_Fixed maximum)
{
    unsigned long value;
    unsigned long span;

    if (maximum <= minimum)
    {
        return minimum;
    }

    value = bex3d_next_random(state);
    span = (unsigned long)(maximum - minimum + 1L);
    return minimum + (BEX3D_Fixed)((value >> 8) % span);
}

static BEX3D_Fixed bex3d_lerp_fixed(BEX3D_Fixed a,
                                    BEX3D_Fixed b,
                                    BEX3D_Fixed t)
{
    return a + bex3d_mul(b - a, t);
}

static int bex3d_lerp_int(int a, int b, BEX3D_Fixed t)
{
    return a + (int)(((long)(b - a) * t) >> BEX3D_FP_SHIFT);
}

static BEX3D_Color bex3d_color(unsigned char r,
                               unsigned char g,
                               unsigned char b,
                               unsigned char a)
{
    BEX3D_Color color;

    color.r = r;
    color.g = g;
    color.b = b;
    color.a = a;
    return color;
}

static void bex3d_clear_primitive(BEX3D_Primitive *primitive)
{
    primitive->type = 0;
    primitive->role = 0;
    primitive->sides = 0;
    primitive->stacks = 0;
    primitive->flags = 0UL;
    primitive->start_us = 0UL;
    primitive->duration_us = 0UL;
    primitive->x0 = 0;
    primitive->y0 = 0;
    primitive->z0 = 0;
    primitive->x1 = 0;
    primitive->y1 = 0;
    primitive->z1 = 0;
    primitive->radius_a0 = 0;
    primitive->radius_a1 = 0;
    primitive->radius_b0 = 0;
    primitive->radius_b1 = 0;
    primitive->length0 = 0;
    primitive->length1 = 0;
    primitive->yaw = 0;
    primitive->pitch = 0;
    primitive->roll0 = 0;
    primitive->roll1 = 0;
    primitive->hot_color = bex3d_color(255, 255, 255, 255);
    primitive->cool_color = bex3d_color(255, 160, 32, 0);
    primitive->alpha0 = 255;
    primitive->alpha1 = 0;
}

static void bex3d_validate_profile(BEX3D_Profile *profile)
{
    profile->muzzle_pressure =
        bex3d_clamp_fixed(profile->muzzle_pressure, 0, BEX3D_INT(3));
    profile->gas_mass =
        bex3d_clamp_fixed(profile->gas_mass, 0, BEX3D_INT(3));
    profile->residual_fuel =
        bex3d_clamp_fixed(profile->residual_fuel, 0, BEX3D_INT(3));
    profile->turbulence =
        bex3d_clamp_fixed(profile->turbulence, 0, BEX3D_INT(3));
    profile->barrel_efficiency =
        bex3d_clamp_fixed(profile->barrel_efficiency, 0, BEX3D_INT(2));
    profile->radial_spread =
        bex3d_clamp_fixed(profile->radial_spread, BEX3D_FP_HALF,
                          BEX3D_INT(3));
    profile->axial_bias =
        bex3d_clamp_fixed(profile->axial_bias, BEX3D_FP_HALF,
                          BEX3D_INT(3));
    profile->bore_radius =
        bex3d_clamp_fixed(profile->bore_radius, BEX3D_MM(1),
                          BEX3D_CM(8));

    profile->minimum_shock_cells =
        bex3d_clamp_int(profile->minimum_shock_cells, 0, 4);
    profile->maximum_shock_cells =
        bex3d_clamp_int(profile->maximum_shock_cells,
                        profile->minimum_shock_cells, 4);
    profile->minimum_vortex_rings =
        bex3d_clamp_int(profile->minimum_vortex_rings, 0, 3);
    profile->maximum_vortex_rings =
        bex3d_clamp_int(profile->maximum_vortex_rings,
                        profile->minimum_vortex_rings, 3);
    profile->gas_cloud_count =
        bex3d_clamp_int(profile->gas_cloud_count, 0, 4);
    profile->secondary_flash_chance =
        bex3d_clamp_int(profile->secondary_flash_chance, 0, 256);
    profile->primitive_budget =
        bex3d_clamp_int(profile->primitive_budget, 1,
                        BEX3D_MAX_PRIMITIVES);
    profile->device =
        bex3d_clamp_int(profile->device, BEX3D_DEVICE_NONE,
                        BEX3D_DEVICE_REVOLVER_GAP);

    if (profile->event_duration_us < 500UL)
    {
        profile->event_duration_us = 500UL;
    }
    if (profile->event_duration_us > 12000UL)
    {
        profile->event_duration_us = 12000UL;
    }
}

void bex3d_default_profile(BEX3D_Profile *profile, int profile_id)
{
    profile->muzzle_pressure = BEX3D_FP_ONE;
    profile->gas_mass = BEX3D_FP_ONE;
    profile->residual_fuel = BEX3D_FP_ONE;
    profile->turbulence = BEX3D_FP_ONE;
    profile->barrel_efficiency = BEX3D_FP_ONE;
    profile->radial_spread = BEX3D_FP_ONE;
    profile->axial_bias = BEX3D_FP_ONE;
    profile->bore_radius = BEX3D_MM(5);
    profile->minimum_shock_cells = 1;
    profile->maximum_shock_cells = 2;
    profile->minimum_vortex_rings = 1;
    profile->maximum_vortex_rings = 2;
    profile->gas_cloud_count = 1;
    profile->secondary_flash_chance = 160;
    profile->primitive_budget = 24;
    profile->device = BEX3D_DEVICE_NONE;
    profile->event_duration_us = 3600UL;

    switch (profile_id)
    {
        case BEX3D_PROFILE_PISTOL:
            profile->muzzle_pressure = BEX3D_INT(9) / 10L;
            profile->gas_mass = BEX3D_INT(7) / 10L;
            profile->residual_fuel = BEX3D_INT(9) / 10L;
            profile->turbulence = BEX3D_INT(9) / 10L;
            profile->barrel_efficiency = BEX3D_INT(8) / 10L;
            profile->radial_spread = BEX3D_INT(11) / 10L;
            profile->axial_bias = BEX3D_INT(9) / 10L;
            profile->bore_radius = BEX3D_MM(5);
            profile->minimum_shock_cells = 1;
            profile->maximum_shock_cells = 2;
            profile->minimum_vortex_rings = 1;
            profile->maximum_vortex_rings = 1;
            profile->gas_cloud_count = 1;
            profile->secondary_flash_chance = 150;
            profile->primitive_budget = 18;
            profile->event_duration_us = 3200UL;
            break;

        case BEX3D_PROFILE_MACHINE_GUN:
            profile->muzzle_pressure = BEX3D_INT(13) / 10L;
            profile->gas_mass = BEX3D_INT(11) / 10L;
            profile->residual_fuel = BEX3D_INT(8) / 10L;
            profile->turbulence = BEX3D_INT(12) / 10L;
            profile->barrel_efficiency = BEX3D_INT(11) / 10L;
            profile->radial_spread = BEX3D_INT(8) / 10L;
            profile->axial_bias = BEX3D_INT(13) / 10L;
            profile->bore_radius = BEX3D_MM(4);
            profile->minimum_shock_cells = 2;
            profile->maximum_shock_cells = 3;
            profile->minimum_vortex_rings = 1;
            profile->maximum_vortex_rings = 2;
            profile->gas_cloud_count = 1;
            profile->secondary_flash_chance = 128;
            profile->primitive_budget = 24;
            profile->device = BEX3D_DEVICE_FLASH_HIDER;
            profile->event_duration_us = 3000UL;
            break;

        case BEX3D_PROFILE_SHOTGUN:
            profile->muzzle_pressure = BEX3D_INT(10) / 10L;
            profile->gas_mass = BEX3D_INT(14) / 10L;
            profile->residual_fuel = BEX3D_INT(12) / 10L;
            profile->turbulence = BEX3D_INT(15) / 10L;
            profile->barrel_efficiency = BEX3D_INT(10) / 10L;
            profile->radial_spread = BEX3D_INT(18) / 10L;
            profile->axial_bias = BEX3D_INT(7) / 10L;
            profile->bore_radius = BEX3D_MM(9);
            profile->minimum_shock_cells = 1;
            profile->maximum_shock_cells = 2;
            profile->minimum_vortex_rings = 1;
            profile->maximum_vortex_rings = 2;
            profile->gas_cloud_count = 3;
            profile->secondary_flash_chance = 190;
            profile->primitive_budget = 28;
            profile->event_duration_us = 4800UL;
            break;

        case BEX3D_PROFILE_MAGNUM_AUTO:
            profile->muzzle_pressure = BEX3D_INT(15) / 10L;
            profile->gas_mass = BEX3D_INT(13) / 10L;
            profile->residual_fuel = BEX3D_INT(16) / 10L;
            profile->turbulence = BEX3D_INT(12) / 10L;
            profile->barrel_efficiency = BEX3D_INT(8) / 10L;
            profile->radial_spread = BEX3D_INT(13) / 10L;
            profile->axial_bias = BEX3D_INT(13) / 10L;
            profile->bore_radius = BEX3D_MM(7);
            profile->minimum_shock_cells = 2;
            profile->maximum_shock_cells = 3;
            profile->minimum_vortex_rings = 1;
            profile->maximum_vortex_rings = 2;
            profile->gas_cloud_count = 2;
            profile->secondary_flash_chance = 240;
            profile->primitive_budget = 30;
            profile->event_duration_us = 5200UL;
            break;

        case BEX3D_PROFILE_MAGNUM_REVOLVER:
            bex3d_default_profile(profile, BEX3D_PROFILE_MAGNUM_AUTO);
            profile->device = BEX3D_DEVICE_REVOLVER_GAP;
            profile->turbulence = BEX3D_INT(14) / 10L;
            profile->primitive_budget = 34;
            break;

        case BEX3D_PROFILE_PRECISION_RIFLE:
            profile->muzzle_pressure = BEX3D_INT(16) / 10L;
            profile->gas_mass = BEX3D_INT(14) / 10L;
            profile->residual_fuel = BEX3D_INT(7) / 10L;
            profile->turbulence = BEX3D_INT(8) / 10L;
            profile->barrel_efficiency = BEX3D_INT(15) / 10L;
            profile->radial_spread = BEX3D_INT(7) / 10L;
            profile->axial_bias = BEX3D_INT(18) / 10L;
            profile->bore_radius = BEX3D_MM(5);
            profile->minimum_shock_cells = 2;
            profile->maximum_shock_cells = 4;
            profile->minimum_vortex_rings = 1;
            profile->maximum_vortex_rings = 2;
            profile->gas_cloud_count = 1;
            profile->secondary_flash_chance = 120;
            profile->primitive_budget = 30;
            profile->device = BEX3D_DEVICE_FLASH_HIDER;
            profile->event_duration_us = 4200UL;
            break;

        case BEX3D_PROFILE_PRECISION_BRAKE:
            bex3d_default_profile(profile, BEX3D_PROFILE_PRECISION_RIFLE);
            profile->residual_fuel = BEX3D_INT(11) / 10L;
            profile->turbulence = BEX3D_INT(14) / 10L;
            profile->radial_spread = BEX3D_INT(13) / 10L;
            profile->device = BEX3D_DEVICE_BRAKE_HORIZONTAL;
            profile->secondary_flash_chance = 185;
            profile->primitive_budget = 36;
            profile->event_duration_us = 5000UL;
            break;

        case BEX3D_PROFILE_SUPPRESSED:
            profile->muzzle_pressure = BEX3D_INT(4) / 10L;
            profile->gas_mass = BEX3D_INT(5) / 10L;
            profile->residual_fuel = BEX3D_INT(2) / 10L;
            profile->turbulence = BEX3D_INT(4) / 10L;
            profile->barrel_efficiency = BEX3D_INT(18) / 10L;
            profile->radial_spread = BEX3D_INT(7) / 10L;
            profile->axial_bias = BEX3D_INT(7) / 10L;
            profile->bore_radius = BEX3D_MM(4);
            profile->minimum_shock_cells = 0;
            profile->maximum_shock_cells = 1;
            profile->minimum_vortex_rings = 0;
            profile->maximum_vortex_rings = 1;
            profile->gas_cloud_count = 1;
            profile->secondary_flash_chance = 24;
            profile->primitive_budget = 10;
            profile->device = BEX3D_DEVICE_SUPPRESSOR;
            profile->event_duration_us = 4200UL;
            break;

        default:
            break;
    }

    bex3d_validate_profile(profile);
}

void bex3d_init(BEX3D_State *state, unsigned long seed)
{
    int i;

    state->rng_state = seed & 0xFFFFFFFFUL;
    bex3d_default_profile(&state->profile, BEX3D_PROFILE_PISTOL);
    state->active = 0;
    state->age_us = 0UL;
    state->duration_us = state->profile.event_duration_us;
    state->primitive_count = 0;
    state->origin_x = 0;
    state->origin_y = 0;
    state->origin_z = 0;
    state->right_x = BEX3D_FP_ONE;
    state->right_y = 0;
    state->right_z = 0;
    state->up_x = 0;
    state->up_y = BEX3D_FP_ONE;
    state->up_z = 0;
    state->forward_x = 0;
    state->forward_y = 0;
    state->forward_z = BEX3D_FP_ONE;

    for (i = 0; i < BEX3D_MAX_PRIMITIVES; ++i)
    {
        bex3d_clear_primitive(&state->primitives[i]);
    }
}

void bex3d_reset(BEX3D_State *state)
{
    int i;

    state->active = 0;
    state->age_us = 0UL;
    state->duration_us = state->profile.event_duration_us;
    state->primitive_count = 0;

    for (i = 0; i < BEX3D_MAX_PRIMITIVES; ++i)
    {
        bex3d_clear_primitive(&state->primitives[i]);
    }
}

void bex3d_set_profile(BEX3D_State *state, const BEX3D_Profile *profile)
{
    state->profile = *profile;
    bex3d_validate_profile(&state->profile);
    state->duration_us = state->profile.event_duration_us;
}

void bex3d_set_origin(BEX3D_State *state,
                      BEX3D_Fixed x,
                      BEX3D_Fixed y,
                      BEX3D_Fixed z)
{
    state->origin_x = x;
    state->origin_y = y;
    state->origin_z = z;
}

void bex3d_set_basis(BEX3D_State *state,
                     BEX3D_Fixed right_x,
                     BEX3D_Fixed right_y,
                     BEX3D_Fixed right_z,
                     BEX3D_Fixed up_x,
                     BEX3D_Fixed up_y,
                     BEX3D_Fixed up_z,
                     BEX3D_Fixed forward_x,
                     BEX3D_Fixed forward_y,
                     BEX3D_Fixed forward_z)
{
    state->right_x = right_x;
    state->right_y = right_y;
    state->right_z = right_z;
    state->up_x = up_x;
    state->up_y = up_y;
    state->up_z = up_z;
    state->forward_x = forward_x;
    state->forward_y = forward_y;
    state->forward_z = forward_z;
}

static BEX3D_Primitive *bex3d_add_primitive(BEX3D_State *state,
                                             int type,
                                             int role)
{
    BEX3D_Primitive *primitive;

    if (state->primitive_count >= state->profile.primitive_budget ||
        state->primitive_count >= BEX3D_MAX_PRIMITIVES)
    {
        return (BEX3D_Primitive *)0;
    }

    primitive = &state->primitives[state->primitive_count++];
    bex3d_clear_primitive(primitive);
    primitive->type = type;
    primitive->role = role;
    return primitive;
}

static void bex3d_set_motion(BEX3D_Primitive *primitive,
                             BEX3D_Fixed x0,
                             BEX3D_Fixed y0,
                             BEX3D_Fixed z0,
                             BEX3D_Fixed x1,
                             BEX3D_Fixed y1,
                             BEX3D_Fixed z1)
{
    primitive->x0 = x0;
    primitive->y0 = y0;
    primitive->z0 = z0;
    primitive->x1 = x1;
    primitive->y1 = y1;
    primitive->z1 = z1;
}

static void bex3d_set_life(BEX3D_Primitive *primitive,
                           unsigned long start_us,
                           unsigned long duration_us)
{
    primitive->start_us = start_us;
    primitive->duration_us = duration_us;
}

static void bex3d_set_fade(BEX3D_Primitive *primitive,
                           int alpha0,
                           int alpha1)
{
    primitive->alpha0 = bex3d_clamp_int(alpha0, 0, 255);
    primitive->alpha1 = bex3d_clamp_int(alpha1, 0, 255);
}

static void bex3d_set_colors(BEX3D_Primitive *primitive,
                             BEX3D_Color hot,
                             BEX3D_Color cool)
{
    primitive->hot_color = hot;
    primitive->cool_color = cool;
}

static void bex3d_add_primary_jet(BEX3D_State *state,
                                  BEX3D_Fixed jet_length,
                                  BEX3D_Fixed jet_radius)
{
    BEX3D_Primitive *primitive;
    BEX3D_Fixed drift;

    primitive = bex3d_add_primitive(state, BEX3D_PRIM_FRUSTUM,
                                    BEX3D_ROLE_PRIMARY_JET);
    if (primitive == (BEX3D_Primitive *)0)
    {
        return;
    }

    drift = jet_length / 5L;
    primitive->sides = 8;
    primitive->flags = BEX3D_CAP_START;
    bex3d_set_life(primitive, 0UL, 1450UL);
    bex3d_set_motion(primitive,
                     0, 0, jet_length / 2L,
                     0, 0, jet_length / 2L + drift);
    primitive->radius_a0 = state->profile.bore_radius;
    primitive->radius_a1 = state->profile.bore_radius / 2L;
    primitive->radius_b0 = jet_radius;
    primitive->radius_b1 = jet_radius + jet_radius / 3L;
    primitive->length0 = jet_length;
    primitive->length1 = jet_length + jet_length / 3L;
    primitive->roll0 = bex3d_rand_range(state, 0, 255);
    primitive->roll1 = primitive->roll0 + bex3d_rand_range(state, -16, 16);
    bex3d_set_colors(primitive,
                     bex3d_color(255, 255, 235, 255),
                     bex3d_color(255, 118, 18, 24));
    bex3d_set_fade(primitive, 255, 0);
}

static void bex3d_add_shock_cells(BEX3D_State *state,
                                  BEX3D_Fixed jet_length,
                                  BEX3D_Fixed jet_radius,
                                  int count)
{
    int i;
    BEX3D_Fixed cell_length;
    BEX3D_Fixed center;
    BEX3D_Fixed radius;
    BEX3D_Primitive *first;
    BEX3D_Primitive *second;
    BEX3D_Primitive *disk;

    if (count <= 0)
    {
        return;
    }

    cell_length = jet_length / (BEX3D_Fixed)(count + 2);
    center = jet_length / 3L;

    for (i = 0; i < count; ++i)
    {
        radius = jet_radius -
                 (jet_radius * (BEX3D_Fixed)i) /
                 (BEX3D_Fixed)(count + 2);
        if (radius < state->profile.bore_radius)
        {
            radius = state->profile.bore_radius;
        }

        first = bex3d_add_primitive(state, BEX3D_PRIM_FRUSTUM,
                                    BEX3D_ROLE_SHOCK_CELL);
        if (first == (BEX3D_Primitive *)0)
        {
            return;
        }
        first->sides = 8;
        bex3d_set_life(first, 100UL + (unsigned long)(i * 80), 1350UL);
        bex3d_set_motion(first,
                         0, 0, center,
                         0, 0, center + cell_length / 10L);
        first->radius_a0 = radius / 3L;
        first->radius_a1 = radius / 5L;
        first->radius_b0 = radius;
        first->radius_b1 = radius + radius / 6L;
        first->length0 = cell_length / 2L;
        first->length1 = cell_length / 2L + cell_length / 8L;
        first->roll0 = bex3d_rand_range(state, 0, 255);
        first->roll1 = first->roll0 + 8;
        bex3d_set_colors(first,
                         bex3d_color(255, 246, 194, 220),
                         bex3d_color(255, 128, 20, 16));
        bex3d_set_fade(first, 220, 0);

        second = bex3d_add_primitive(state, BEX3D_PRIM_FRUSTUM,
                                     BEX3D_ROLE_SHOCK_CELL);
        if (second == (BEX3D_Primitive *)0)
        {
            return;
        }
        second->sides = 8;
        bex3d_set_life(second, 120UL + (unsigned long)(i * 80), 1300UL);
        bex3d_set_motion(second,
                         0, 0, center + cell_length / 2L,
                         0, 0, center + cell_length / 2L +
                               cell_length / 10L);
        second->radius_a0 = radius;
        second->radius_a1 = radius + radius / 8L;
        second->radius_b0 = radius / 4L;
        second->radius_b1 = radius / 6L;
        second->length0 = cell_length / 2L;
        second->length1 = cell_length / 2L + cell_length / 8L;
        second->roll0 = first->roll0;
        second->roll1 = first->roll1;
        bex3d_set_colors(second,
                         bex3d_color(255, 222, 132, 205),
                         bex3d_color(255, 92, 8, 8));
        bex3d_set_fade(second, 210, 0);

        if (i == 0 || (i == count - 1 && count > 2))
        {
            disk = bex3d_add_primitive(state, BEX3D_PRIM_FRUSTUM,
                                       BEX3D_ROLE_MACH_DISK);
            if (disk == (BEX3D_Primitive *)0)
            {
                return;
            }
            disk->sides = 10;
            disk->flags = BEX3D_CAP_START | BEX3D_CAP_END;
            bex3d_set_life(disk, 180UL + (unsigned long)(i * 100), 900UL);
            bex3d_set_motion(disk,
                             0, 0, center + cell_length / 2L,
                             0, 0, center + cell_length / 2L +
                                   cell_length / 12L);
            disk->radius_a0 = radius / 2L;
            disk->radius_a1 = radius / 3L;
            disk->radius_b0 = radius / 2L;
            disk->radius_b1 = radius / 3L;
            disk->length0 = BEX3D_MM(2);
            disk->length1 = BEX3D_MM(1);
            disk->roll0 = bex3d_rand_range(state, 0, 255);
            disk->roll1 = disk->roll0;
            bex3d_set_colors(disk,
                             bex3d_color(255, 255, 235, 255),
                             bex3d_color(255, 192, 84, 80));
            bex3d_set_fade(disk, 255, 0);
        }

        center += cell_length;
    }
}

static void bex3d_add_vortex_rings(BEX3D_State *state,
                                   BEX3D_Fixed jet_length,
                                   BEX3D_Fixed jet_radius,
                                   int count)
{
    int i;
    BEX3D_Primitive *ring;
    BEX3D_Fixed base_z;
    BEX3D_Fixed radius;

    base_z = (jet_length * 3L) / 5L;
    for (i = 0; i < count; ++i)
    {
        ring = bex3d_add_primitive(state, BEX3D_PRIM_RING,
                                   BEX3D_ROLE_VORTEX_RING);
        if (ring == (BEX3D_Primitive *)0)
        {
            return;
        }
        radius = jet_radius + (BEX3D_Fixed)i * (jet_radius / 3L);
        ring->sides = 10;
        ring->stacks = 4;
        bex3d_set_life(ring, 450UL + (unsigned long)(i * 220),
                       1800UL + (unsigned long)(i * 180));
        bex3d_set_motion(ring,
                         0, 0, base_z + (BEX3D_Fixed)i * BEX3D_CM(2),
                         0, 0, base_z + (BEX3D_Fixed)i * BEX3D_CM(5));
        ring->radius_a0 = radius;
        ring->radius_a1 = radius + radius / 2L;
        ring->radius_b0 = radius / 5L;
        ring->radius_b1 = radius / 9L;
        ring->length0 = 0;
        ring->length1 = 0;
        ring->roll0 = bex3d_rand_range(state, 0, 255);
        ring->roll1 = ring->roll0 + bex3d_rand_range(state, -24, 24);
        bex3d_set_colors(ring,
                         bex3d_color(255, 190, 68, 165),
                         bex3d_color(255, 78, 10, 0));
        bex3d_set_fade(ring, 160, 0);
    }
}

static void bex3d_add_secondary_flash(BEX3D_State *state,
                                      BEX3D_Fixed jet_length,
                                      BEX3D_Fixed jet_radius,
                                      BEX3D_Fixed effective_fuel)
{
    BEX3D_Primitive *sphere;
    BEX3D_Fixed radius;
    BEX3D_Fixed offset;
    BEX3D_Fixed jitter;
    int extra_lobes;
    int i;

    if (bex3d_rand_range(state, 0, 255) >=
        state->profile.secondary_flash_chance)
    {
        return;
    }

    radius = BEX3D_CM(2) + jet_radius / 2L +
             bex3d_mul(state->profile.gas_mass, BEX3D_CM(2)) +
             bex3d_mul(effective_fuel, BEX3D_CM(2));
    radius = bex3d_mul(radius, state->profile.radial_spread);
    offset = jet_length +
             bex3d_mul(state->profile.muzzle_pressure, BEX3D_CM(4));
    jitter = bex3d_mul(state->profile.turbulence, BEX3D_MM(8));

    sphere = bex3d_add_primitive(state, BEX3D_PRIM_SPHERE,
                                 BEX3D_ROLE_SECONDARY_FLASH);
    if (sphere == (BEX3D_Primitive *)0)
    {
        return;
    }
    sphere->sides = 16;
    sphere->stacks = 8;
    bex3d_set_life(sphere, 500UL, 2700UL);
    bex3d_set_motion(sphere,
                     bex3d_rand_fixed(state, -jitter, jitter),
                     bex3d_rand_fixed(state, -jitter, jitter),
                     offset,
                     bex3d_rand_fixed(state, -jitter * 2L, jitter * 2L),
                     bex3d_rand_fixed(state, -jitter * 2L, jitter * 2L),
                     offset + BEX3D_CM(5));
    sphere->radius_a0 = radius / 3L;
    sphere->radius_a1 = radius;
    sphere->radius_b0 = radius / 3L;
    sphere->radius_b1 = radius;
    sphere->length0 = radius / 2L;
    sphere->length1 = radius;
    sphere->roll0 = bex3d_rand_range(state, 0, 255);
    sphere->roll1 = sphere->roll0 + bex3d_rand_range(state, -32, 32);
    bex3d_set_colors(sphere,
                     bex3d_color(255, 255, 222, 245),
                     bex3d_color(255, 96, 8, 8));
    bex3d_set_fade(sphere, 245, 0);

    extra_lobes = 0;
    if (state->profile.turbulence > BEX3D_INT(13) / 10L)
    {
        extra_lobes = 2;
    }
    else if (state->profile.turbulence > BEX3D_FP_ONE)
    {
        extra_lobes = 1;
    }

    for (i = 0; i < extra_lobes; ++i)
    {
        sphere = bex3d_add_primitive(state, BEX3D_PRIM_SPHERE,
                                     BEX3D_ROLE_SECONDARY_FLASH);
        if (sphere == (BEX3D_Primitive *)0)
        {
            return;
        }
        sphere->sides = 12;
        sphere->stacks = 6;
        bex3d_set_life(sphere, 650UL + (unsigned long)(i * 100), 2200UL);
        bex3d_set_motion(sphere,
                         bex3d_rand_fixed(state, -radius, radius),
                         bex3d_rand_fixed(state, -radius, radius),
                         offset + bex3d_rand_fixed(state,
                                                   -radius / 2L,
                                                   radius / 2L),
                         bex3d_rand_fixed(state, -radius * 2L, radius * 2L),
                         bex3d_rand_fixed(state, -radius * 2L, radius * 2L),
                         offset + BEX3D_CM(4));
        sphere->radius_a0 = radius / 5L;
        sphere->radius_a1 = radius / 2L;
        sphere->radius_b0 = radius / 5L;
        sphere->radius_b1 = radius / 2L;
        sphere->length0 = radius / 4L;
        sphere->length1 = radius / 2L;
        sphere->roll0 = bex3d_rand_range(state, 0, 255);
        sphere->roll1 = sphere->roll0 + bex3d_rand_range(state, -48, 48);
        bex3d_set_colors(sphere,
                         bex3d_color(255, 224, 154, 190),
                         bex3d_color(255, 70, 4, 0));
        bex3d_set_fade(sphere, 180, 0);
    }
}

static void bex3d_add_gas_clouds(BEX3D_State *state,
                                 BEX3D_Fixed jet_length,
                                 BEX3D_Fixed jet_radius,
                                 int count)
{
    int i;
    BEX3D_Primitive *cloud;
    BEX3D_Fixed radius;
    BEX3D_Fixed spread;

    radius = jet_radius + bex3d_mul(state->profile.gas_mass, BEX3D_CM(1));
    spread = bex3d_mul(state->profile.radial_spread, BEX3D_CM(2));

    for (i = 0; i < count; ++i)
    {
        cloud = bex3d_add_primitive(state, BEX3D_PRIM_SPHERE,
                                    BEX3D_ROLE_GAS_CLOUD);
        if (cloud == (BEX3D_Primitive *)0)
        {
            return;
        }
        cloud->sides = 10;
        cloud->stacks = 5;
        bex3d_set_life(cloud, 1200UL + (unsigned long)(i * 260),
                       2500UL + (unsigned long)(i * 300));
        bex3d_set_motion(cloud,
                         bex3d_rand_fixed(state, -spread, spread),
                         bex3d_rand_fixed(state, -spread, spread),
                         jet_length / 2L + (BEX3D_Fixed)i * BEX3D_CM(2),
                         bex3d_rand_fixed(state, -spread * 2L, spread * 2L),
                         bex3d_rand_fixed(state, 0, spread * 2L),
                         jet_length + (BEX3D_Fixed)i * BEX3D_CM(4));
        cloud->radius_a0 = radius / 4L;
        cloud->radius_a1 = radius + (BEX3D_Fixed)i * BEX3D_CM(1);
        cloud->radius_b0 = radius / 4L;
        cloud->radius_b1 = radius + radius / 2L;
        cloud->length0 = radius / 3L;
        cloud->length1 = radius;
        cloud->roll0 = bex3d_rand_range(state, 0, 255);
        cloud->roll1 = cloud->roll0 + bex3d_rand_range(state, -40, 40);
        bex3d_set_colors(cloud,
                         bex3d_color(255, 132, 46, 90),
                         bex3d_color(72, 66, 60, 0));
        bex3d_set_fade(cloud, 85, 0);
    }
}

static void bex3d_add_device_jet(BEX3D_State *state,
                                 int role,
                                 int yaw,
                                 int pitch,
                                 BEX3D_Fixed center_z,
                                 BEX3D_Fixed length,
                                 BEX3D_Fixed radius,
                                 int alpha)
{
    BEX3D_Primitive *jet;
    BEX3D_Fixed sy;
    BEX3D_Fixed cy;
    BEX3D_Fixed sp;
    BEX3D_Fixed cp;
    BEX3D_Fixed fx;
    BEX3D_Fixed fy;
    BEX3D_Fixed fz;
    BEX3D_Fixed expanded_length;

    jet = bex3d_add_primitive(state, BEX3D_PRIM_FRUSTUM, role);
    if (jet == (BEX3D_Primitive *)0)
    {
        return;
    }

    sy = bex3d_sin(yaw);
    cy = bex3d_cos(yaw);
    sp = bex3d_sin(pitch);
    cp = bex3d_cos(pitch);
    fx = bex3d_mul(sy, cp);
    fy = -sp;
    fz = bex3d_mul(cy, cp);
    expanded_length = length + length / 3L;

    jet->sides = 7;
    jet->flags = BEX3D_CAP_START;
    bex3d_set_life(jet, 80UL, 1500UL);
    bex3d_set_motion(jet,
                     bex3d_mul(fx, length / 2L),
                     bex3d_mul(fy, length / 2L),
                     center_z + bex3d_mul(fz, length / 2L),
                     bex3d_mul(fx, expanded_length / 2L),
                     bex3d_mul(fy, expanded_length / 2L),
                     center_z + bex3d_mul(fz, expanded_length / 2L));
    jet->radius_a0 = radius / 3L;
    jet->radius_a1 = radius / 5L;
    jet->radius_b0 = radius;
    jet->radius_b1 = radius + radius / 2L;
    jet->length0 = length;
    jet->length1 = length + length / 3L;
    jet->yaw = yaw;
    jet->pitch = pitch;
    jet->roll0 = bex3d_rand_range(state, 0, 255);
    jet->roll1 = jet->roll0 + bex3d_rand_range(state, -20, 20);
    bex3d_set_colors(jet,
                     bex3d_color(255, 245, 205, 235),
                     bex3d_color(255, 78, 6, 0));
    bex3d_set_fade(jet, alpha, 0);
}

static void bex3d_add_device_effects(BEX3D_State *state,
                                     BEX3D_Fixed jet_length,
                                     BEX3D_Fixed jet_radius)
{
    BEX3D_Fixed side_length;
    BEX3D_Fixed side_radius;
    int i;
    int yaw;
    int pitch;

    side_length = jet_length / 2L;
    side_radius = jet_radius;

    switch (state->profile.device)
    {
        case BEX3D_DEVICE_FLASH_HIDER:
            for (i = 0; i < 3; ++i)
            {
                yaw = bex3d_rand_range(state, -10, 10);
                pitch = bex3d_rand_range(state, -10, 10);
                bex3d_add_device_jet(state, BEX3D_ROLE_DEVICE_JET,
                                     yaw, pitch, BEX3D_CM(1),
                                     side_length / 2L,
                                     side_radius / 2L, 125);
            }
            break;

        case BEX3D_DEVICE_BRAKE_HORIZONTAL:
            bex3d_add_device_jet(state, BEX3D_ROLE_DEVICE_JET,
                                 64, 0, 0,
                                 side_length, side_radius, 220);
            bex3d_add_device_jet(state, BEX3D_ROLE_DEVICE_JET,
                                 192, 0, 0,
                                 side_length, side_radius, 220);
            break;

        case BEX3D_DEVICE_BRAKE_RADIAL:
            bex3d_add_device_jet(state, BEX3D_ROLE_DEVICE_JET,
                                 64, 0, 0,
                                 side_length, side_radius, 205);
            bex3d_add_device_jet(state, BEX3D_ROLE_DEVICE_JET,
                                 192, 0, 0,
                                 side_length, side_radius, 205);
            bex3d_add_device_jet(state, BEX3D_ROLE_DEVICE_JET,
                                 0, -64, 0,
                                 side_length, side_radius, 205);
            bex3d_add_device_jet(state, BEX3D_ROLE_DEVICE_JET,
                                 0, 64, 0,
                                 side_length, side_radius, 205);
            break;

        case BEX3D_DEVICE_SUPPRESSOR:
            bex3d_add_device_jet(state, BEX3D_ROLE_DEVICE_JET,
                                 0, 0, BEX3D_CM(1),
                                 side_length / 2L,
                                 side_radius / 3L, 70);
            break;

        case BEX3D_DEVICE_REVOLVER_GAP:
            bex3d_add_device_jet(state, BEX3D_ROLE_CYLINDER_GAP,
                                 64, 0, -BEX3D_CM(2),
                                 side_length / 2L,
                                 side_radius / 2L, 190);
            bex3d_add_device_jet(state, BEX3D_ROLE_CYLINDER_GAP,
                                 192, 0, -BEX3D_CM(2),
                                 side_length / 2L,
                                 side_radius / 2L, 190);
            break;

        default:
            break;
    }
}

void bex3d_fire(BEX3D_State *state)
{
    BEX3D_Fixed efficiency_loss;
    BEX3D_Fixed effective_fuel;
    BEX3D_Fixed jet_length;
    BEX3D_Fixed jet_radius;
    BEX3D_Fixed random_scale;
    int shock_cells;
    int vortex_rings;

    bex3d_reset(state);
    state->active = 1;
    state->duration_us = state->profile.event_duration_us;

    efficiency_loss = BEX3D_INT(2) - state->profile.barrel_efficiency;
    efficiency_loss = bex3d_clamp_fixed(efficiency_loss, 0, BEX3D_INT(2));
    effective_fuel = state->profile.residual_fuel + efficiency_loss / 3L;
    effective_fuel = bex3d_clamp_fixed(effective_fuel, 0, BEX3D_INT(3));

    random_scale = bex3d_rand_fixed(state,
                                    BEX3D_INT(85) / 100L,
                                    BEX3D_INT(115) / 100L);

    jet_length = BEX3D_CM(5) +
                 bex3d_mul(state->profile.muzzle_pressure, BEX3D_CM(8)) +
                 bex3d_mul(state->profile.gas_mass, BEX3D_CM(3));
    jet_length = bex3d_mul(jet_length, state->profile.axial_bias);
    jet_length = bex3d_mul(jet_length, random_scale);

    jet_radius = state->profile.bore_radius +
                 bex3d_mul(state->profile.gas_mass, BEX3D_CM(1)) +
                 bex3d_mul(state->profile.turbulence, BEX3D_MM(5));
    jet_radius = bex3d_mul(jet_radius, state->profile.radial_spread);
    jet_radius = bex3d_mul(jet_radius, random_scale);

    if (state->profile.device == BEX3D_DEVICE_SUPPRESSOR)
    {
        jet_length = jet_length / 2L;
        jet_radius = jet_radius / 2L;
    }

    shock_cells = bex3d_rand_range(state,
                                   state->profile.minimum_shock_cells,
                                   state->profile.maximum_shock_cells);
    vortex_rings = bex3d_rand_range(state,
                                    state->profile.minimum_vortex_rings,
                                    state->profile.maximum_vortex_rings);

    bex3d_add_primary_jet(state, jet_length, jet_radius);
    bex3d_add_shock_cells(state, jet_length, jet_radius, shock_cells);
    bex3d_add_vortex_rings(state, jet_length, jet_radius, vortex_rings);
    bex3d_add_secondary_flash(state, jet_length, jet_radius,
                              effective_fuel);
    bex3d_add_gas_clouds(state, jet_length, jet_radius,
                         state->profile.gas_cloud_count);
    bex3d_add_device_effects(state, jet_length, jet_radius);
}

void bex3d_update_us(BEX3D_State *state, unsigned long delta_us)
{
    if (!state->active)
    {
        return;
    }

    if (delta_us > state->duration_us ||
        state->age_us >= state->duration_us - delta_us)
    {
        state->age_us = state->duration_us;
        state->active = 0;
        return;
    }

    state->age_us += delta_us;
}

static int bex3d_primitive_visible_at(const BEX3D_Primitive *primitive,
                                      unsigned long age_us)
{
    if (primitive->type == 0 || primitive->duration_us == 0UL)
    {
        return 0;
    }
    if (age_us < primitive->start_us)
    {
        return 0;
    }
    if (age_us >= primitive->start_us + primitive->duration_us)
    {
        return 0;
    }
    return 1;
}

int bex3d_visible_primitive_count(const BEX3D_State *state)
{
    int i;
    int count;

    count = 0;
    for (i = 0; i < state->primitive_count; ++i)
    {
        if (bex3d_primitive_visible_at(&state->primitives[i], state->age_us))
        {
            ++count;
        }
    }
    return count;
}

const BEX3D_Primitive *bex3d_get_primitive(const BEX3D_State *state,
                                           int index)
{
    if (index < 0 || index >= state->primitive_count)
    {
        return (const BEX3D_Primitive *)0;
    }
    return &state->primitives[index];
}

static BEX3D_Fixed bex3d_primitive_t(const BEX3D_State *state,
                                     const BEX3D_Primitive *primitive)
{
    unsigned long local_age;

    if (state->age_us <= primitive->start_us)
    {
        return 0;
    }
    local_age = state->age_us - primitive->start_us;
    if (local_age >= primitive->duration_us)
    {
        return BEX3D_FP_ONE;
    }
    return (BEX3D_Fixed)((local_age * (unsigned long)BEX3D_FP_ONE) /
                         primitive->duration_us);
}

static void bex3d_primitive_basis(const BEX3D_Primitive *primitive,
                                  BEX3D_Fixed t,
                                  BEX3D_Fixed *right_x,
                                  BEX3D_Fixed *right_y,
                                  BEX3D_Fixed *right_z,
                                  BEX3D_Fixed *up_x,
                                  BEX3D_Fixed *up_y,
                                  BEX3D_Fixed *up_z,
                                  BEX3D_Fixed *forward_x,
                                  BEX3D_Fixed *forward_y,
                                  BEX3D_Fixed *forward_z)
{
    BEX3D_Fixed sy;
    BEX3D_Fixed cy;
    BEX3D_Fixed sp;
    BEX3D_Fixed cp;
    BEX3D_Fixed sr;
    BEX3D_Fixed cr;
    BEX3D_Fixed base_rx;
    BEX3D_Fixed base_ry;
    BEX3D_Fixed base_rz;
    BEX3D_Fixed base_ux;
    BEX3D_Fixed base_uy;
    BEX3D_Fixed base_uz;
    int roll;

    sy = bex3d_sin(primitive->yaw);
    cy = bex3d_cos(primitive->yaw);
    sp = bex3d_sin(primitive->pitch);
    cp = bex3d_cos(primitive->pitch);

    base_rx = cy;
    base_ry = 0;
    base_rz = -sy;

    base_ux = bex3d_mul(sy, sp);
    base_uy = cp;
    base_uz = bex3d_mul(cy, sp);

    *forward_x = bex3d_mul(sy, cp);
    *forward_y = -sp;
    *forward_z = bex3d_mul(cy, cp);

    roll = bex3d_lerp_int(primitive->roll0, primitive->roll1, t);
    sr = bex3d_sin(roll);
    cr = bex3d_cos(roll);

    *right_x = bex3d_mul(base_rx, cr) + bex3d_mul(base_ux, sr);
    *right_y = bex3d_mul(base_ry, cr) + bex3d_mul(base_uy, sr);
    *right_z = bex3d_mul(base_rz, cr) + bex3d_mul(base_uz, sr);

    *up_x = bex3d_mul(base_ux, cr) - bex3d_mul(base_rx, sr);
    *up_y = bex3d_mul(base_uy, cr) - bex3d_mul(base_ry, sr);
    *up_z = bex3d_mul(base_uz, cr) - bex3d_mul(base_rz, sr);
}

static void bex3d_transform_vertex(const BEX3D_State *state,
                                   const BEX3D_Primitive *primitive,
                                   BEX3D_Fixed t,
                                   BEX3D_Fixed local_x,
                                   BEX3D_Fixed local_y,
                                   BEX3D_Fixed local_z,
                                   BEX3D_Vertex *vertex,
                                   BEX3D_Color color,
                                   int alpha_scale)
{
    BEX3D_Fixed prx;
    BEX3D_Fixed pry;
    BEX3D_Fixed prz;
    BEX3D_Fixed pux;
    BEX3D_Fixed puy;
    BEX3D_Fixed puz;
    BEX3D_Fixed pfx;
    BEX3D_Fixed pfy;
    BEX3D_Fixed pfz;
    BEX3D_Fixed center_x;
    BEX3D_Fixed center_y;
    BEX3D_Fixed center_z;
    BEX3D_Fixed explosion_x;
    BEX3D_Fixed explosion_y;
    BEX3D_Fixed explosion_z;
    int alpha;

    bex3d_primitive_basis(primitive, t,
                          &prx, &pry, &prz,
                          &pux, &puy, &puz,
                          &pfx, &pfy, &pfz);

    center_x = bex3d_lerp_fixed(primitive->x0, primitive->x1, t);
    center_y = bex3d_lerp_fixed(primitive->y0, primitive->y1, t);
    center_z = bex3d_lerp_fixed(primitive->z0, primitive->z1, t);

    explosion_x = center_x +
                  bex3d_mul(prx, local_x) +
                  bex3d_mul(pux, local_y) +
                  bex3d_mul(pfx, local_z);
    explosion_y = center_y +
                  bex3d_mul(pry, local_x) +
                  bex3d_mul(puy, local_y) +
                  bex3d_mul(pfy, local_z);
    explosion_z = center_z +
                  bex3d_mul(prz, local_x) +
                  bex3d_mul(puz, local_y) +
                  bex3d_mul(pfz, local_z);

    vertex->x = state->origin_x +
                bex3d_mul(state->right_x, explosion_x) +
                bex3d_mul(state->up_x, explosion_y) +
                bex3d_mul(state->forward_x, explosion_z);
    vertex->y = state->origin_y +
                bex3d_mul(state->right_y, explosion_x) +
                bex3d_mul(state->up_y, explosion_y) +
                bex3d_mul(state->forward_y, explosion_z);
    vertex->z = state->origin_z +
                bex3d_mul(state->right_z, explosion_x) +
                bex3d_mul(state->up_z, explosion_y) +
                bex3d_mul(state->forward_z, explosion_z);

    alpha = ((int)color.a * alpha_scale) / 255;
    vertex->r = color.r;
    vertex->g = color.g;
    vertex->b = color.b;
    vertex->a = (unsigned char)bex3d_clamp_int(alpha, 0, 255);
}

static int bex3d_mesh_has_room(const BEX3D_MeshBuffer *mesh,
                               unsigned long vertices,
                               unsigned long indices)
{
    if (mesh->vertex_count + vertices > mesh->vertex_capacity)
    {
        return 0;
    }
    if (mesh->index_count + indices > mesh->index_capacity)
    {
        return 0;
    }
    return 1;
}

static void bex3d_push_index(BEX3D_MeshBuffer *mesh,
                             unsigned long index)
{
    mesh->indices[mesh->index_count++] = (unsigned short)index;
}

static int bex3d_build_frustum(const BEX3D_State *state,
                               const BEX3D_Primitive *primitive,
                               BEX3D_Fixed t,
                               BEX3D_MeshBuffer *mesh)
{
    int sides;
    int has_start;
    int has_end;
    unsigned long needed_vertices;
    unsigned long needed_indices;
    unsigned long base;
    unsigned long start_center;
    unsigned long end_center;
    BEX3D_Fixed radius_a;
    BEX3D_Fixed radius_b;
    BEX3D_Fixed length;
    BEX3D_Fixed z0;
    BEX3D_Fixed z1;
    BEX3D_Fixed c;
    BEX3D_Fixed s;
    BEX3D_Fixed x;
    BEX3D_Fixed y;
    int alpha;
    int i;
    int next;

    sides = bex3d_clamp_int(primitive->sides, 3, BEX3D_MAX_SIDES);
    has_start = ((primitive->flags & BEX3D_CAP_START) != 0UL);
    has_end = ((primitive->flags & BEX3D_CAP_END) != 0UL);
    needed_vertices = (unsigned long)(sides * 2 + has_start + has_end);
    needed_indices = (unsigned long)(sides * 6 +
                     (has_start ? sides * 3 : 0) +
                     (has_end ? sides * 3 : 0));

    if (!bex3d_mesh_has_room(mesh, needed_vertices, needed_indices))
    {
        mesh->truncated = 1;
        return 0;
    }

    base = mesh->vertex_count;
    radius_a = bex3d_lerp_fixed(primitive->radius_a0,
                                primitive->radius_a1, t);
    radius_b = bex3d_lerp_fixed(primitive->radius_b0,
                                primitive->radius_b1, t);
    length = bex3d_lerp_fixed(primitive->length0,
                              primitive->length1, t);
    z0 = -length / 2L;
    z1 = length / 2L;
    alpha = bex3d_lerp_int(primitive->alpha0, primitive->alpha1, t);

    for (i = 0; i < sides; ++i)
    {
        c = bex3d_cos((i * 256) / sides);
        s = bex3d_sin((i * 256) / sides);
        x = bex3d_mul(c, radius_a);
        y = bex3d_mul(s, radius_a);
        bex3d_transform_vertex(state, primitive, t,
                               x, y, z0,
                               &mesh->vertices[mesh->vertex_count++],
                               primitive->hot_color, alpha);
    }

    for (i = 0; i < sides; ++i)
    {
        c = bex3d_cos((i * 256) / sides);
        s = bex3d_sin((i * 256) / sides);
        x = bex3d_mul(c, radius_b);
        y = bex3d_mul(s, radius_b);
        bex3d_transform_vertex(state, primitive, t,
                               x, y, z1,
                               &mesh->vertices[mesh->vertex_count++],
                               primitive->cool_color, alpha);
    }

    start_center = 0UL;
    end_center = 0UL;
    if (has_start)
    {
        start_center = mesh->vertex_count;
        bex3d_transform_vertex(state, primitive, t,
                               0, 0, z0,
                               &mesh->vertices[mesh->vertex_count++],
                               primitive->hot_color, alpha);
    }
    if (has_end)
    {
        end_center = mesh->vertex_count;
        bex3d_transform_vertex(state, primitive, t,
                               0, 0, z1,
                               &mesh->vertices[mesh->vertex_count++],
                               primitive->cool_color, alpha);
    }

    for (i = 0; i < sides; ++i)
    {
        next = (i + 1) % sides;
        bex3d_push_index(mesh, base + (unsigned long)i);
        bex3d_push_index(mesh, base + (unsigned long)next);
        bex3d_push_index(mesh, base + (unsigned long)sides +
                               (unsigned long)i);

        bex3d_push_index(mesh, base + (unsigned long)next);
        bex3d_push_index(mesh, base + (unsigned long)sides +
                               (unsigned long)next);
        bex3d_push_index(mesh, base + (unsigned long)sides +
                               (unsigned long)i);

        if (has_start)
        {
            bex3d_push_index(mesh, start_center);
            bex3d_push_index(mesh, base + (unsigned long)next);
            bex3d_push_index(mesh, base + (unsigned long)i);
        }
        if (has_end)
        {
            bex3d_push_index(mesh, end_center);
            bex3d_push_index(mesh, base + (unsigned long)sides +
                                   (unsigned long)i);
            bex3d_push_index(mesh, base + (unsigned long)sides +
                                   (unsigned long)next);
        }
    }

    return 1;
}

static BEX3D_Color bex3d_mix_color(BEX3D_Color a,
                                    BEX3D_Color b,
                                    BEX3D_Fixed t)
{
    BEX3D_Color color;

    color.r = (unsigned char)bex3d_clamp_int(
        bex3d_lerp_int((int)a.r, (int)b.r, t), 0, 255);
    color.g = (unsigned char)bex3d_clamp_int(
        bex3d_lerp_int((int)a.g, (int)b.g, t), 0, 255);
    color.b = (unsigned char)bex3d_clamp_int(
        bex3d_lerp_int((int)a.b, (int)b.b, t), 0, 255);
    color.a = (unsigned char)bex3d_clamp_int(
        bex3d_lerp_int((int)a.a, (int)b.a, t), 0, 255);
    return color;
}

static int bex3d_build_sphere(const BEX3D_State *state,
                              const BEX3D_Primitive *primitive,
                              BEX3D_Fixed t,
                              BEX3D_MeshBuffer *mesh)
{
    int sides;
    int stacks;
    unsigned long needed_vertices;
    unsigned long needed_indices;
    unsigned long base;
    BEX3D_Fixed radius_x;
    BEX3D_Fixed radius_y;
    BEX3D_Fixed radius_z;
    BEX3D_Fixed lat_sin;
    BEX3D_Fixed lat_cos;
    BEX3D_Fixed lon_sin;
    BEX3D_Fixed lon_cos;
    BEX3D_Fixed x;
    BEX3D_Fixed y;
    BEX3D_Fixed z;
    BEX3D_Fixed color_t;
    BEX3D_Color color;
    int alpha;
    int stack;
    int side;
    int next_side;
    unsigned long a;
    unsigned long b;
    unsigned long c;
    unsigned long d;

    sides = bex3d_clamp_int(primitive->sides, 4, BEX3D_MAX_SIDES);
    stacks = bex3d_clamp_int(primitive->stacks, 2, BEX3D_MAX_STACKS);
    needed_vertices = (unsigned long)((stacks + 1) * sides);
    needed_indices = (unsigned long)(stacks * sides * 6);

    if (!bex3d_mesh_has_room(mesh, needed_vertices, needed_indices))
    {
        mesh->truncated = 1;
        return 0;
    }

    base = mesh->vertex_count;
    radius_x = bex3d_lerp_fixed(primitive->radius_a0,
                                primitive->radius_a1, t);
    radius_y = bex3d_lerp_fixed(primitive->radius_b0,
                                primitive->radius_b1, t);
    radius_z = bex3d_lerp_fixed(primitive->length0,
                                primitive->length1, t);
    alpha = bex3d_lerp_int(primitive->alpha0, primitive->alpha1, t);

    for (stack = 0; stack <= stacks; ++stack)
    {
        lat_sin = bex3d_sin(-64 + (stack * 128) / stacks);
        lat_cos = bex3d_cos(-64 + (stack * 128) / stacks);
        color_t = (BEX3D_Fixed)((stack * BEX3D_FP_ONE) / stacks);
        color = bex3d_mix_color(primitive->hot_color,
                                primitive->cool_color,
                                color_t);

        for (side = 0; side < sides; ++side)
        {
            lon_sin = bex3d_sin((side * 256) / sides);
            lon_cos = bex3d_cos((side * 256) / sides);
            x = bex3d_mul(bex3d_mul(lat_cos, lon_cos), radius_x);
            y = bex3d_mul(bex3d_mul(lat_cos, lon_sin), radius_y);
            z = bex3d_mul(lat_sin, radius_z);
            bex3d_transform_vertex(state, primitive, t,
                                   x, y, z,
                                   &mesh->vertices[mesh->vertex_count++],
                                   color, alpha);
        }
    }

    for (stack = 0; stack < stacks; ++stack)
    {
        for (side = 0; side < sides; ++side)
        {
            next_side = (side + 1) % sides;
            a = base + (unsigned long)(stack * sides + side);
            b = base + (unsigned long)(stack * sides + next_side);
            c = base + (unsigned long)((stack + 1) * sides + side);
            d = base + (unsigned long)((stack + 1) * sides + next_side);

            bex3d_push_index(mesh, a);
            bex3d_push_index(mesh, b);
            bex3d_push_index(mesh, c);
            bex3d_push_index(mesh, b);
            bex3d_push_index(mesh, d);
            bex3d_push_index(mesh, c);
        }
    }

    return 1;
}

static int bex3d_build_ring(const BEX3D_State *state,
                            const BEX3D_Primitive *primitive,
                            BEX3D_Fixed t,
                            BEX3D_MeshBuffer *mesh)
{
    int major_segments;
    int minor_segments;
    unsigned long needed_vertices;
    unsigned long needed_indices;
    unsigned long base;
    BEX3D_Fixed major_radius;
    BEX3D_Fixed minor_radius;
    BEX3D_Fixed major_sin;
    BEX3D_Fixed major_cos;
    BEX3D_Fixed minor_sin;
    BEX3D_Fixed minor_cos;
    BEX3D_Fixed radial;
    BEX3D_Fixed x;
    BEX3D_Fixed y;
    BEX3D_Fixed z;
    BEX3D_Fixed color_t;
    BEX3D_Color color;
    int alpha;
    int major;
    int minor;
    int next_major;
    int next_minor;
    unsigned long a;
    unsigned long b;
    unsigned long c;
    unsigned long d;

    major_segments = bex3d_clamp_int(primitive->sides, 4, BEX3D_MAX_SIDES);
    minor_segments = bex3d_clamp_int(primitive->stacks, 3, BEX3D_MAX_STACKS);
    needed_vertices = (unsigned long)(major_segments * minor_segments);
    needed_indices = (unsigned long)(major_segments * minor_segments * 6);

    if (!bex3d_mesh_has_room(mesh, needed_vertices, needed_indices))
    {
        mesh->truncated = 1;
        return 0;
    }

    base = mesh->vertex_count;
    major_radius = bex3d_lerp_fixed(primitive->radius_a0,
                                    primitive->radius_a1, t);
    minor_radius = bex3d_lerp_fixed(primitive->radius_b0,
                                    primitive->radius_b1, t);
    alpha = bex3d_lerp_int(primitive->alpha0, primitive->alpha1, t);

    for (major = 0; major < major_segments; ++major)
    {
        major_sin = bex3d_sin((major * 256) / major_segments);
        major_cos = bex3d_cos((major * 256) / major_segments);
        for (minor = 0; minor < minor_segments; ++minor)
        {
            minor_sin = bex3d_sin((minor * 256) / minor_segments);
            minor_cos = bex3d_cos((minor * 256) / minor_segments);
            radial = major_radius + bex3d_mul(minor_radius, minor_cos);
            x = bex3d_mul(radial, major_cos);
            y = bex3d_mul(radial, major_sin);
            z = bex3d_mul(minor_radius, minor_sin);
            color_t = (minor_cos + BEX3D_FP_ONE) / 2L;
            color = bex3d_mix_color(primitive->cool_color,
                                    primitive->hot_color,
                                    color_t);
            bex3d_transform_vertex(state, primitive, t,
                                   x, y, z,
                                   &mesh->vertices[mesh->vertex_count++],
                                   color, alpha);
        }
    }

    for (major = 0; major < major_segments; ++major)
    {
        next_major = (major + 1) % major_segments;
        for (minor = 0; minor < minor_segments; ++minor)
        {
            next_minor = (minor + 1) % minor_segments;
            a = base + (unsigned long)(major * minor_segments + minor);
            b = base + (unsigned long)(next_major * minor_segments + minor);
            c = base + (unsigned long)(major * minor_segments + next_minor);
            d = base +
                (unsigned long)(next_major * minor_segments + next_minor);

            bex3d_push_index(mesh, a);
            bex3d_push_index(mesh, b);
            bex3d_push_index(mesh, c);
            bex3d_push_index(mesh, b);
            bex3d_push_index(mesh, d);
            bex3d_push_index(mesh, c);
        }
    }

    return 1;
}

int bex3d_build_mesh(const BEX3D_State *state, BEX3D_MeshBuffer *mesh)
{
    int i;
    BEX3D_Fixed t;
    const BEX3D_Primitive *primitive;

    if (mesh == (BEX3D_MeshBuffer *)0 ||
        mesh->vertices == (BEX3D_Vertex *)0 ||
        mesh->indices == (unsigned short *)0 ||
        mesh->vertex_capacity == 0UL ||
        mesh->index_capacity == 0UL)
    {
        return BEX3D_BUILD_BAD_BUFFER;
    }

    mesh->vertex_count = 0UL;
    mesh->index_count = 0UL;
    mesh->truncated = 0;

    if (!state->active && state->age_us >= state->duration_us)
    {
        return BEX3D_BUILD_OK;
    }

    for (i = 0; i < state->primitive_count; ++i)
    {
        primitive = &state->primitives[i];
        if (!bex3d_primitive_visible_at(primitive, state->age_us))
        {
            continue;
        }

        t = bex3d_primitive_t(state, primitive);
        switch (primitive->type)
        {
            case BEX3D_PRIM_FRUSTUM:
                if (!bex3d_build_frustum(state, primitive, t, mesh))
                {
                    return BEX3D_BUILD_TRUNCATED;
                }
                break;

            case BEX3D_PRIM_SPHERE:
                if (!bex3d_build_sphere(state, primitive, t, mesh))
                {
                    return BEX3D_BUILD_TRUNCATED;
                }
                break;

            case BEX3D_PRIM_RING:
                if (!bex3d_build_ring(state, primitive, t, mesh))
                {
                    return BEX3D_BUILD_TRUNCATED;
                }
                break;

            default:
                break;
        }
    }

    return mesh->truncated ? BEX3D_BUILD_TRUNCATED : BEX3D_BUILD_OK;
}
