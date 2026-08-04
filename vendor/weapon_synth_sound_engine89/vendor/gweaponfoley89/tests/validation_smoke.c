#include <stdio.h>
#include "gweaponfoley89.h"

#define TEST_SAMPLES 22050UL

int main(void)
{
    gwf89_context a;
    gwf89_context b;
    gwf89_context c;
    gwf89_s16 sa;
    gwf89_s16 sb;
    gwf89_s16 sc;
    gwf89_s16 dry;
    gwf89_s16 room;
    unsigned long i;
    unsigned long differences;

    gwf89_init(&a, 1UL);
    gwf89_init(&b, 1UL);
    gwf89_trigger_ex(&a, GWF89_SNIPER_BOLT_DRY, 0x12345678UL,
                     1U, GWF89_SPEED_NORMAL);
    gwf89_trigger_ex(&b, GWF89_SNIPER_BOLT_DRY, 0x12345678UL,
                     1U, GWF89_SPEED_NORMAL);
    for (i = 0UL; i < TEST_SAMPLES; ++i) {
        sa = gwf89_process_sample(&a);
        sb = gwf89_process_sample(&b);
        if (sa != sb) {
            fprintf(stderr, "determinism failed at %lu\n", i);
            return 1;
        }
    }

    gwf89_init(&a, 1UL);
    gwf89_init(&c, 1UL);
    gwf89_trigger_ex(&a, GWF89_SNIPER_BOLT_DRY, 0x12345678UL,
                     0U, GWF89_SPEED_NORMAL);
    gwf89_trigger_ex(&c, GWF89_SNIPER_BOLT_DRY, 0x12345678UL,
                     2U, GWF89_SPEED_NORMAL);
    differences = 0UL;
    for (i = 0UL; i < TEST_SAMPLES; ++i) {
        sa = gwf89_process_sample(&a);
        sc = gwf89_process_sample(&c);
        if (sa != sc) ++differences;
    }
    if (differences == 0UL) {
        fprintf(stderr, "variants are identical\n");
        return 1;
    }

    gwf89_init(&a, 1UL);
    gwf89_trigger_ex(&a, GWF89_PISTOL_EMPTY, 0x89ABCDEFUL,
                     1U, GWF89_SPEED_NORMAL);
    gwf89_set_room_send(&a, 0);
    for (i = 0UL; i < TEST_SAMPLES; ++i) {
        sa = gwf89_process_sample_stems(&a, &dry, &room);
        if (sa != dry) {
            fprintf(stderr, "dry stem mismatch at %lu\n", i);
            return 1;
        }
    }

    printf("ok differences=%lu context=%lu\n",
           differences, gwf89_context_bytes());
    return 0;
}
