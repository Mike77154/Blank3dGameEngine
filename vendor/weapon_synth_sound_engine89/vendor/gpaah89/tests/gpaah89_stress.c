#include <stdio.h>
#include "gpaah89.h"

#define TEST_BLOCK 257U

int main(void)
{
    static const gpaah89_u32 rates[4] = { 8000U, 22050U, 44100U, 48000U };
    gpaah89_state state;
    gpaah89_preset preset;
    gpaah89_s16 out[TEST_BLOCK * 2U];
    gpaah89_u32 checksum;
    gpaah89_u32 rendered;
    gpaah89_u32 frame;
    gpaah89_u32 n;
    int preset_id;
    int rate_id;
    gpaah89_u32 i;

    checksum = 2166136261U;
    for (preset_id = 0; preset_id < (int)GPAAH89_PRESET_COUNT; ++preset_id) {
        if (!gpaah89_get_preset((gpaah89_preset_id)preset_id, &preset))
            return 1;
        for (rate_id = 0; rate_id < 4; ++rate_id) {
            if (!gpaah89_init(&state, rates[rate_id], &preset,
                              1000U + (gpaah89_u32)preset_id))
                return 2;
            rendered = rates[rate_id] * 2U;
            frame = 0U;
            while (frame < rendered) {
                n = rendered - frame;
                if (n > TEST_BLOCK) n = TEST_BLOCK;
                if (frame != 0U && (frame % (rates[rate_id] / 5U)) < n)
                    gpaah89_trigger(&state, frame + 9001U);
                if (gpaah89_render_stereo(&state, out, n) != n)
                    return 3;
                for (i = 0U; i < n * 2U; ++i) {
                    checksum ^= (gpaah89_u16)out[i];
                    checksum *= 16777619U;
                }
                frame += n;
            }
        }
    }

    printf("gpaah89 stress checksum: %lu\n", (unsigned long)checksum);
    return 0;
}
