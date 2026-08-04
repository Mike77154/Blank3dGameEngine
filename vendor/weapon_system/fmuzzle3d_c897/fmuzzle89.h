#ifndef FMUZZLE89_H
#define FMUZZLE89_H

/*
 * FMUZZLE89 v9
 * Procedural 3D muzzle-flash geometry.
 *
 * C89 only.
 * Q10 fixed point only.
 * Fixed-size storage only.
 * No dynamic allocation.
 */

#define FM89_FP_SHIFT 10
#define FM89_FP_ONE   (1L << FM89_FP_SHIFT)

#define FM89_CM(value) ((FM89_Fixed)(value) * FM89_FP_ONE)
#define FM89_MM(value) (((FM89_Fixed)(value) * FM89_FP_ONE) / 10L)

#define FM89_MAX_PRIMARY_FINS 8
#define FM89_MAX_TOTAL_FINS   (FM89_MAX_PRIMARY_FINS * 4)
#define FM89_DEFAULT_LIFETIME 4

#define FM89_SHAPE_TRIANGLE 0
#define FM89_SHAPE_LEAF     1
#define FM89_SHAPE_BOX      2 /* Reserved; generation disabled. */

#define FM89_SHAPE_MASK_TRIANGLE 1UL
#define FM89_SHAPE_MASK_LEAF     2UL
#define FM89_SHAPE_MASK_BOX      4UL /* Reserved; ignored. */
#define FM89_SHAPE_MASK_ENABLED  (FM89_SHAPE_MASK_TRIANGLE | \
                                  FM89_SHAPE_MASK_LEAF)
#define FM89_SHAPE_MASK_ALL      FM89_SHAPE_MASK_ENABLED

#define FM89_FIN_CYL_PRIMARY 0
#define FM89_FIN_CYL_REPEAT  1
#define FM89_FIN_CONE_PRIMARY 2
#define FM89_FIN_CONE_REPEAT  3

typedef long FM89_Fixed;

typedef struct FM89_Config
{
    int minimum_primary_fins;
    int maximum_primary_fins;
    unsigned long allowed_shapes;
    int mixed_shapes;
    int repeater_enabled;
    int cone_bank_enabled;
    int cone_repeater_enabled;
    int lifetime;

    /*
     * Axial core geometry:
     * 2 cm cylinder + 18 cm cone = 20 cm total.
     * Shared cylinder/cone base diameter: 2.5 cm.
     */
    FM89_Fixed cylinder_length;
    FM89_Fixed cone_length;
    FM89_Fixed core_radius;
    FM89_Fixed cone_tip_radius;

    /* Full primary fin dimensions. */
    FM89_Fixed minimum_fin_length;
    FM89_Fixed maximum_fin_length;
    FM89_Fixed minimum_fin_width;
    FM89_Fixed maximum_fin_width;

    /* Repeater scale as integer fractions. */
    int repeat_scale_numerator;
    int repeat_scale_denominator;
    int cone_scale_numerator;
    int cone_scale_denominator;

    /* Alternating roll used by the cone-base crown. */
    int cone_roll_angle;
} FM89_Config;

typedef struct FM89_Fin
{
    int angle;
    int shape;
    int roll;
    int tier;
    int parent_index;
    FM89_Fixed anchor_z;
    FM89_Fixed anchor_radius;
    FM89_Fixed length;
    FM89_Fixed half_width;
    FM89_Fixed outer_half_width;
} FM89_Fin;

typedef struct FM89_State
{
    unsigned long rng_state;
    FM89_Config config;

    int active;
    int age;
    int lifetime;
    int primary_count;
    int total_count;
    int family_shape;
    int global_angle;
    int pitch;
    int yaw;

    int cylinder_primary_offset;
    int cylinder_repeat_offset;
    int cone_primary_offset;
    int cone_repeat_offset;

    FM89_Fixed center_x;
    FM89_Fixed center_y;
    FM89_Fixed center_z;
    FM89_Fixed scale;
    int brightness;

    FM89_Fin fins[FM89_MAX_TOTAL_FINS];
} FM89_State;

void fm89_default_config(FM89_Config *config);
void fm89_init(FM89_State *state, unsigned long seed);
void fm89_set_config(FM89_State *state, const FM89_Config *config);
void fm89_fire(FM89_State *state);
void fm89_update(FM89_State *state);

FM89_Fixed fm89_mul(FM89_Fixed a, FM89_Fixed b);
FM89_Fixed fm89_sin(int angle);
FM89_Fixed fm89_cos(int angle);

FM89_Fixed fm89_core_total_length(const FM89_State *state);
FM89_Fixed fm89_fin_anchor_z(const FM89_State *state);
FM89_Fixed fm89_fin_anchor_radius(const FM89_State *state);
FM89_Fixed fm89_cone_base_anchor_z(const FM89_State *state);
FM89_Fixed fm89_cone_base_anchor_radius(const FM89_State *state);
FM89_Fixed fm89_core_radius_at_z(const FM89_State *state, FM89_Fixed z);

#endif
