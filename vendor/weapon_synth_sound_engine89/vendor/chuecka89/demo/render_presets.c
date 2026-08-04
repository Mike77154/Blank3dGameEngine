#include <stdio.h>
#include "chuecka89.h"
#include "chuecka89_wav.h"

#define DEMO_RATE 44100U
#define DEMO_MAX_FRAMES (DEMO_RATE * 3U)

static ch89_i16 g_audio[DEMO_MAX_FRAMES];

int main(void)
{
    ch89_context ctx;
    ch89_gesture gesture;
    ch89_u32 frames;
    ch89_u16 i;
    ch89_u16 reps;
    char path[160];
    ch89_result r;

    r = ch89_init(&ctx, DEMO_RATE, 0x89C0FFEEU);
    if (r != CH89_OK) {
        fprintf(stderr, "ch89_init failed: %d\n", (int)r);
        return 1;
    }

    for (i = 0U; i < (ch89_u16)CH89_PRESET_COUNT; ++i) {
        reps = 0U;
        if (i == (ch89_u16)CH89_PRESET_SHOTGUN_MULTI_INSERT) reps = 7U;
        if (i == (ch89_u16)CH89_PRESET_SMG_FEED_BURST) reps = 12U;
        if (i == (ch89_u16)CH89_PRESET_MACHINE_GUN_FEED_BURST) reps = 10U;
        r = ch89_make_preset((ch89_preset)i, reps, CH89_Q15_ONE, &gesture);
        if (r != CH89_OK) {
            fprintf(stderr, "preset failed: %u (%d)\n", (unsigned)i, (int)r);
            return 2;
        }
        frames = ch89_required_frames(&gesture, DEMO_RATE);
        if (frames > DEMO_MAX_FRAMES) {
            fprintf(stderr, "preset too long: %u\n", (unsigned)i);
            return 3;
        }
        r = ch89_render(&ctx, &gesture, g_audio, DEMO_MAX_FRAMES, &frames);
        if (r != CH89_OK) {
            fprintf(stderr, "render failed: %u (%d)\n", (unsigned)i, (int)r);
            return 4;
        }
        sprintf(path, "wav/%02u_%s.wav", (unsigned)i, ch89_preset_name((ch89_preset)i));
        if (!ch89_write_wav_mono16(path, g_audio, frames, DEMO_RATE)) {
            fprintf(stderr, "wav write failed: %s\n", path);
            return 5;
        }
        printf("wrote %s (%lu frames)\n", path, (unsigned long)frames);
    }
    return 0;
}
