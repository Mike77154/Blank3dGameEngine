#include <stdio.h>

#include "gmovesequence89.h"

static GMoveTarget89 point_target(long x, long y, long z)
{
    GMoveTarget89 target;

    target.type = GMOVE89_TARGET_POINT;
    target.entity_id = 0L;
    target.socket_id = 0L;
    target.point = gmove89_vec3(
        GMOVE89_FX_FROM_INT(x),
        GMOVE89_FX_FROM_INT(y),
        GMOVE89_FX_FROM_INT(z)
    );
    return target;
}

static void clear_step(GMoveSequenceStep89 *step)
{
    step->op = GMOVESEQ89_WAIT;
    step->target = point_target(0L, 0L, 0L);
    gautmove89_config_default(&step->automove);
    gmovepattern89_config_default(&step->pattern);
    step->duration = 0L;
    step->safe_distance = 0L;
    step->turn_fraction_per_second = 0L;
}

int main(void)
{
    GMoveSequenceStep89 steps[5];
    GMoveSequenceState89 sequence;
    GMoveMotion89 motion;
    GMoveVec3_89 position;
    GMoveVec3_89 forward;
    GMoveFx89 dt;
    int frame;
    int i;

    for (i = 0; i < 5; ++i) {
        clear_step(&steps[i]);
    }

    steps[0].op = GMOVESEQ89_MOVE_TO;
    steps[0].target = point_target(8L, 0L, 0L);
    steps[0].automove.speed = GMOVE89_FX_FROM_INT(4);

    steps[1].op = GMOVESEQ89_WAIT;
    steps[1].duration = GMOVE89_FX_ONE;

    steps[2].op = GMOVESEQ89_PATTERN;
    steps[2].target = point_target(18L, 0L, 0L);
    steps[2].pattern.type = GMOVEPATTERN89_HELIX_TO;
    steps[2].pattern.speed = GMOVE89_FX_FROM_INT(3);
    steps[2].pattern.radius = GMOVE89_FX_FROM_INT(2);
    steps[2].pattern.frequency = GMOVE89_FX_ONE;

    steps[3].op = GMOVESEQ89_MOVE_AWAY;
    steps[3].target = point_target(18L, 0L, 0L);
    steps[3].automove.speed = GMOVE89_FX_FROM_INT(5);
    steps[3].safe_distance = GMOVE89_FX_FROM_INT(6);

    steps[4].op = GMOVESEQ89_END;

    position = gmove89_vec3_zero();
    forward = gmove89_vec3(GMOVE89_FX_ONE, 0L, 0L);
    dt = GMOVE89_FX_ONE / 20L;

    gmovesequence89_reset(&sequence);

    for (frame = 0; frame < 400 && !sequence.done; ++frame) {
        if (!gmovesequence89_update(
                &sequence,
                steps,
                5UL,
                0,
                position,
                forward,
                dt,
                &motion)) {
            puts("sequence error");
            return 1;
        }

        if (motion.has_translation) {
            position = motion.target_position;
        }
        if (motion.has_rotation) {
            forward = motion.desired_forward;
        }

        if ((frame % 20) == 0) {
            printf(
                "step=%lu pos=(%ld,%ld,%ld)\n",
                sequence.index,
                GMOVE89_FX_TO_INT(position.x),
                GMOVE89_FX_TO_INT(position.y),
                GMOVE89_FX_TO_INT(position.z)
            );
        }
    }

    printf("done=%d final_step=%lu\n", sequence.done, sequence.index);
    return sequence.done ? 0 : 1;
}
