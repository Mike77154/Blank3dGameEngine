#include <stdio.h>
#include "grecoil89.h"
#include "grecoil89_profiles.h"

static void print_fp(grec_fp v)
{
    int whole;
    int frac;
    if (v < 0) {
        putchar('-');
        v = -v;
    }
    whole = (int)(v >> GREC_FP_SHIFT);
    frac = (int)(((v & (GREC_FP_ONE - 1)) * 1000) >> GREC_FP_SHIFT);
    printf("%d.%03d", whole, frac);
}

static void print_output(int tick, const GRecOutput *out)
{
    printf("%03d aim(p/y)=", tick);
    print_fp(out->aim_angles.pitch);
    printf("/");
    print_fp(out->aim_angles.yaw);
    printf(" cam(p/y)=");
    print_fp(out->camera_angles.pitch);
    printf("/");
    print_fp(out->camera_angles.yaw);
    printf(" weapon(back/up)=");
    print_fp(out->weapon_offset.back);
    printf("/");
    print_fp(out->weapon_offset.up);
    printf(" spread=");
    print_fp(out->spread);
    printf(" shot=%u\n", (unsigned)out->shot_index);
}

int main(void)
{
    GRecState st;
    GRecContext ctx;
    GRecOutput out;
    const GRecProfile *profile;
    int tick;

    profile = grec_get_profile(GREC_PROFILE_RIFLE_556);
    if (profile == 0) return 1;

    grec_state_init(&st, 0xCAFE123u);
    grec_context_for_mode(&ctx, GREC_MODE_HIP);

    printf("%s demo: %s\n", grec_version_string(), profile->name);
    printf("Fire ticks: 0, 4, 8, 12, 16, 20, then recovery.\n\n");

    for (tick = 0; tick < 80; ++tick) {
        if (tick == 0 || tick == 4 || tick == 8 || tick == 12 || tick == 16 || tick == 20) {
            grec_fire(&st, profile, &ctx);
        }
        grec_update(&st, profile);
        grec_sample(&st, profile, &ctx, &out);
        if (tick < 28 || (tick % 8) == 0) print_output(tick, &out);
    }

    return 0;
}
