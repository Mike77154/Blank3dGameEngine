#include "renlist89.h"
#include <stdio.h>
#include <string.h>
int main(void)
{
    static const char text[] =
        "image hero idle:\n"
        "  \"idle0\"\n"
        "  hold\n"
        "image hero run:\n"
        "  speed_hint 2x\n"
        "  \"run0\"\n"
        "  \"run1\"\n"
        "  pingpong\n";
    RenList89 d;
    rl89_id idle;
    rl89_id run;
    if (!rl89_parse(&d, text, (rl89_u32)(sizeof(text)-1U))) return 2;
    idle = rl89_find_animation(&d, "hero", "idle");
    run = rl89_find_animation(&d, "hero", "run");
    if (idle == RL89_INVALID_ID || run == RL89_INVALID_ID) return 3;
    if (d.animations[idle].loop_mode != RL89_LOOP_HOLD) return 4;
    if (d.animations[run].loop_mode != RL89_LOOP_PINGPONG) return 5;
    if (!rl89_property(&d, run, "speed_hint") || strcmp(rl89_property(&d, run, "speed_hint"), "2x") != 0) return 6;
    if (d.animations[run].frame_count != 2U) return 7;
    printf("RenList89 multi PASS animations=%u\n", (unsigned)d.animation_count);
    return 0;
}
