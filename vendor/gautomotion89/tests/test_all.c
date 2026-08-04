#include <stdio.h>

#include "gmovesequence89.h"

static int failures = 0;

static void expect_true(int condition, const char *name)
{
    if (!condition) {
        printf("FAIL: %s\n", name);
        failures += 1;
    }
}

static void test_fixed_math(void)
{
    GMoveFx89 a;
    GMoveFx89 b;
    GMoveFx89 c;

    a = GMOVE89_FX_FROM_INT(3);
    b = GMOVE89_FX_FROM_INT(2);
    c = gmove89_fx_mul(a, b);
    expect_true(GMOVE89_FX_TO_INT(c) == 6L, "fx_mul 3*2");

    c = gmove89_fx_div(a, b);
    expect_true(c > GMOVE89_FX_ONE && c < GMOVE89_FX_FROM_INT(2),
        "fx_div 3/2");
}

static void test_move_to(void)
{
    GAutMoveConfig89 config;
    GMoveMotion89 motion;
    GMoveVec3_89 current;
    GMoveVec3_89 target;

    gautmove89_config_default(&config);
    config.speed = GMOVE89_FX_FROM_INT(10);

    current = gmove89_vec3_zero();
    target = gmove89_vec3(GMOVE89_FX_FROM_INT(5), 0L, 0L);

    gautmove89_step_to_point(
        current,
        target,
        &config,
        GMOVE89_FX_ONE,
        &motion
    );

    expect_true(motion.reached, "move_to reaches");
    expect_true(
        GMOVE89_FX_TO_INT(motion.target_position.x) >= 4L,
        "move_to target position"
    );
}

static void test_pingpong(void)
{
    GMovePatternConfig89 config;
    GMovePatternState89 state;
    GMoveMotion89 motion;
    GMoveVec3_89 current;

    current = gmove89_vec3_zero();
    gmovepattern89_config_default(&config);
    config.type = GMOVEPATTERN89_PINGPONG;
    config.amplitude = GMOVE89_FX_FROM_INT(2);
    config.frequency = GMOVE89_FX_ONE;
    config.axis_a = gmove89_vec3(GMOVE89_FX_ONE, 0L, 0L);

    gmovepattern89_begin(&state, current);
    gmovepattern89_step(
        &state,
        &config,
        current,
        gmove89_vec3_zero(),
        GMOVE89_FX_QUARTER,
        &motion
    );

    expect_true(
        motion.target_position.x > GMOVE89_FX_FROM_INT(1),
        "pingpong quarter turn"
    );
}

int main(void)
{
    test_fixed_math();
    test_move_to();
    test_pingpong();

    if (failures != 0) {
        printf("%d test(s) failed\n", failures);
        return 1;
    }

    puts("all tests passed");
    return 0;
}
