#include "expandiblefire89.h"

#include <stdio.h>

static ef89_fx q(int whole, int quarters)
{
    return (ef89_fx)whole * EF89_ONE +
           ((ef89_fx)quarters * EF89_ONE) / 4L;
}

int main(void)
{
    ef89_config config;
    ef89_state state;
    ef89_result result;
    ef89_vec3 p;
    ef89_fx scaled;

    expandiblefire89_config_default(&config);
    config.scale_start_fx = EF89_ONE / 4L;
    config.scale_end_fx = EF89_ONE * 2L;
    config.growth_distance_fx = EF89_ONE * 4L;
    config.kill_distance_fx = EF89_ONE * 6L;

    p.x = 0L; p.y = 0L; p.z = 0L;
    expandiblefire89_init(&state, p, &config);
    if (!expandiblefire89_step(&state, &config, p, &result) ||
        !result.alive || result.scale_fx != EF89_ONE / 4L)
        return 1;

    p.z = EF89_ONE * 2L;
    if (!expandiblefire89_step(&state, &config, p, &result) ||
        !result.alive || result.progress_fx < (EF89_ONE / 2L - 2L) ||
        result.progress_fx > (EF89_ONE / 2L + 2L))
        return 2;
    if (result.scale_fx <= EF89_ONE / 4L ||
        result.scale_fx >= EF89_ONE * 2L)
        return 3;

    p.z = EF89_ONE * 4L;
    if (!expandiblefire89_step(&state, &config, p, &result) ||
        !result.alive || result.scale_fx != EF89_ONE * 2L)
        return 4;

    p.z = EF89_ONE * 6L;
    if (!expandiblefire89_step(&state, &config, p, &result) ||
        result.alive || state.active)
        return 5;

    scaled = expandiblefire89_apply_scale(q(1, 0), EF89_ONE / 2L);
    if (scaled != EF89_ONE / 2L) return 6;

    puts("expandiblefire89 expanding projectile behavior: PASS");
    return 0;
}
