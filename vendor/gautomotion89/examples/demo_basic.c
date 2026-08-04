#include <stdio.h>

#include "gautmove89.h"
#include "gmovepattern89.h"

static void print_vec(const char *label, GMoveVec3_89 value)
{
    printf(
        "%s (%ld, %ld, %ld)\n",
        label,
        GMOVE89_FX_TO_INT(value.x),
        GMOVE89_FX_TO_INT(value.y),
        GMOVE89_FX_TO_INT(value.z)
    );
}

int main(void)
{
    GMoveVec3_89 position;
    GMoveVec3_89 target;
    GMoveMotion89 motion;
    GAutMoveConfig89 move_config;
    GMovePatternConfig89 pattern_config;
    GMovePatternState89 pattern_state;
    GMoveFx89 dt;
    int frame;

    position = gmove89_vec3(0L, 0L, 0L);
    target = gmove89_vec3(
        GMOVE89_FX_FROM_INT(20),
        0L,
        GMOVE89_FX_FROM_INT(10)
    );
    dt = GMOVE89_FX_ONE / 10L;

    gautmove89_config_default(&move_config);
    move_config.speed = GMOVE89_FX_FROM_INT(5);
    move_config.stop_distance = GMOVE89_FX_FROM_INT(1);
    move_config.speed_mode = GAUTMOVE89_SPEED_SLOWDOWN;
    move_config.slowdown_distance = GMOVE89_FX_FROM_INT(4);

    puts("AUTOMOVE");
    for (frame = 0; frame < 60; ++frame) {
        gautmove89_step_to_point(
            position,
            target,
            &move_config,
            dt,
            &motion
        );
        position = motion.target_position;
        if ((frame % 10) == 0 || motion.reached) {
            print_vec("position", position);
        }
        if (motion.reached) {
            break;
        }
    }

    puts("\nZIGZAG PATTERN");
    position = gmove89_vec3(0L, 0L, 0L);
    target = gmove89_vec3(GMOVE89_FX_FROM_INT(20), 0L, 0L);

    gmovepattern89_config_default(&pattern_config);
    pattern_config.type = GMOVEPATTERN89_ZIGZAG_TO;
    pattern_config.speed = GMOVE89_FX_FROM_INT(4);
    pattern_config.amplitude = GMOVE89_FX_FROM_INT(2);
    pattern_config.frequency = GMOVE89_FX_ONE * 2L;

    gmovepattern89_begin(&pattern_state, position);

    for (frame = 0; frame < 80; ++frame) {
        gmovepattern89_step(
            &pattern_state,
            &pattern_config,
            position,
            target,
            dt,
            &motion
        );
        position = motion.target_position;
        if ((frame % 8) == 0 || motion.reached) {
            print_vec("position", position);
        }
        if (motion.reached) {
            break;
        }
    }

    return 0;
}
