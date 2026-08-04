#include <stdio.h>

#include "fmuzzle89.h"

static int test_failures = 0;

static void test_expect(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL: %s\n", message);
        test_failures += 1;
    }
}

static void test_exact_five_and_cone_repeaters(void)
{
    FM89_State state;
    FM89_Config config;
    int i;
    int cyl_repeat_angle;
    int cone_repeat_angle;
    int cone_primary_index;
    int cone_repeat_index;

    fm89_init(&state, 0x12345678UL);
    fm89_default_config(&config);
    config.minimum_primary_fins = 5;
    config.maximum_primary_fins = 5;
    config.allowed_shapes = FM89_SHAPE_MASK_TRIANGLE;
    config.minimum_fin_length = FM89_CM(5);
    config.maximum_fin_length = FM89_CM(5);
    config.minimum_fin_width = FM89_MM(25);
    config.maximum_fin_width = FM89_MM(25);
    config.repeater_enabled = 1;
    config.cone_bank_enabled = 1;
    config.cone_repeater_enabled = 1;
    config.repeat_scale_numerator = 1;
    config.repeat_scale_denominator = 2;
    config.cone_scale_numerator = 1;
    config.cone_scale_denominator = 2;
    fm89_set_config(&state, &config);
    fm89_fire(&state);

    test_expect(state.primary_count == 5,
                "the primary star must have five fins");
    test_expect(state.total_count == 20,
                "base star + repeater + cone star + cone repeater must yield twenty fins");
    test_expect(state.config.cylinder_length == FM89_CM(2),
                "the cylinder must measure 2 cm");
    test_expect(state.config.cone_length == FM89_CM(18),
                "the cone must measure 18 cm");
    test_expect(fm89_core_total_length(&state) == FM89_CM(20),
                "the combined core must measure 20 cm");
    test_expect(state.config.core_radius * 2L == FM89_MM(25),
                "the cylinder and cone base diameter must be 2.5 cm");
    test_expect(fm89_fin_anchor_z(&state) == 0,
                "base star must start at the rear base ring of the cylinder");
    test_expect(fm89_cone_base_anchor_z(&state) == FM89_CM(2),
                "the cone star must start at the cone base");

    for (i = 0; i < 5; ++i)
    {
        cyl_repeat_angle = ((2 * i + 1) * 128) / 5;
        cone_repeat_angle = cyl_repeat_angle;
        cone_primary_index = state.cone_primary_offset + i;
        cone_repeat_index = state.cone_repeat_offset + i;

        test_expect(state.fins[state.cylinder_primary_offset + i].tier == FM89_FIN_CYL_PRIMARY,
                    "first bank must contain cylinder-base primaries");
        test_expect(state.fins[state.cylinder_repeat_offset + i].tier == FM89_FIN_CYL_REPEAT,
                    "second bank must contain cylinder-base repeaters");
        test_expect(state.fins[cone_primary_index].tier == FM89_FIN_CONE_PRIMARY,
                    "third bank must contain cone-base primaries");
        test_expect(state.fins[cone_repeat_index].tier == FM89_FIN_CONE_REPEAT,
                    "fourth bank must contain cone-base repeaters");

        test_expect(state.fins[state.cylinder_repeat_offset + i].angle == cyl_repeat_angle,
                    "cylinder repeater fins must occupy angular midpoints");
        test_expect(state.fins[cone_repeat_index].angle == cone_repeat_angle,
                    "cone repeater fins must occupy angular midpoints");

        test_expect(state.fins[state.cylinder_primary_offset + i].length == FM89_CM(5),
                    "base primary length must be exactly 5 cm");
        test_expect(state.fins[state.cylinder_repeat_offset + i].length == FM89_MM(25),
                    "base repeated length must be exactly 2.5 cm");
        test_expect(state.fins[cone_primary_index].length == FM89_MM(25),
                    "cone primary length must be exactly 2.5 cm");
        test_expect(state.fins[cone_repeat_index].length == FM89_CM(5) / 4L,
                    "cone repeated length must be exactly 1.25 cm");

        test_expect(state.fins[cone_primary_index].anchor_z == FM89_CM(2),
                    "cone primaries must start on the cone base");
        test_expect(state.fins[cone_repeat_index].anchor_z == FM89_CM(2),
                    "cone repeaters must start on the cone base");
        test_expect(state.fins[state.cylinder_primary_offset + i].anchor_z == 0,
                    "cylinder primaries must remain on the cylinder base");
        test_expect(state.fins[state.cylinder_repeat_offset + i].anchor_z == 0,
                    "cylinder repeaters must remain on the cylinder base");
    }
}

static void test_randomized_generation(void)
{
    FM89_State state;
    FM89_Config config;
    int shot;
    int i;

    fm89_init(&state, 0x47494646UL);
    fm89_default_config(&config);
    config.minimum_primary_fins = 3;
    config.maximum_primary_fins = 8;
    config.allowed_shapes = FM89_SHAPE_MASK_ENABLED;
    config.mixed_shapes = 1;
    config.repeater_enabled = 1;
    config.cone_bank_enabled = 1;
    config.cone_repeater_enabled = 1;
    fm89_set_config(&state, &config);

    for (shot = 0; shot < 512; ++shot)
    {
        fm89_fire(&state);
        test_expect(state.primary_count >= 3 && state.primary_count <= 8,
                    "primary count must remain inside configured bounds");
        test_expect(state.total_count == state.primary_count * 4,
                    "four active banks must produce four times the primary count");
        test_expect(fm89_core_total_length(&state) == FM89_CM(20),
                    "core length must remain exactly 20 cm");

        for (i = 0; i < state.total_count; ++i)
        {
            test_expect(state.fins[i].shape != FM89_SHAPE_BOX,
                        "generated fins must never use disabled square sheets");
        }
    }
}

int main(void)
{
    test_exact_five_and_cone_repeaters();
    test_randomized_generation();

    if (test_failures != 0)
    {
        fprintf(stderr, "%d test assertion(s) failed.\n", test_failures);
        return 1;
    }

    printf("FMUZZLE89 v9: all fixed-point geometry tests passed.\n");
    return 0;
}
