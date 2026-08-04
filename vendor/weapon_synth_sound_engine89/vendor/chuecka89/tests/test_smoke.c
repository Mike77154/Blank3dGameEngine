#include "chuecka89.h"

#define TEST_FRAMES 96000U
static ch89_i16 g_test_audio[TEST_FRAMES];

int main(void)
{
    ch89_context ctx;
    ch89_gesture g;
    ch89_u32 written;
    ch89_u16 i;
    ch89_i32 activity;

    if (ch89_init(&ctx, 48000U, 89U) != CH89_OK) return 1;
    for (i = 0U; i < (ch89_u16)CH89_PRESET_COUNT; ++i) {
        if (ch89_make_preset((ch89_preset)i, 0U, CH89_Q15_ONE, &g) != CH89_OK) return 2;
        if (ch89_render(&ctx, &g, g_test_audio, TEST_FRAMES, &written) != CH89_OK) return 3;
        if (written == 0U) return 4;
        activity = 0;
        activity += g_test_audio[written / 4U];
        activity += g_test_audio[written / 2U];
        activity += g_test_audio[(written * 3U) / 4U];
        if (activity == 0) {
            ch89_u32 j;
            for (j = 0U; j < written; ++j) activity |= g_test_audio[j];
            if (activity == 0) return 5;
        }
    }
    return 0;
}
