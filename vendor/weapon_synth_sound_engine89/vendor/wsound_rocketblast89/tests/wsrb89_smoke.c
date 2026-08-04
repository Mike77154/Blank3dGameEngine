#include <stdio.h>
#include "wsound_rocketblast89.h"

#define TEST_CHUNK 257U

static wsrb89_workspace test_workspace;
static wsrb89_context test_context;
static wsrb89_s16 test_left[TEST_CHUNK];
static wsrb89_s16 test_right[TEST_CHUNK];

static int test_one(wsrb89_u32 sample_rate, wsrb89_u16 preset)
{
    wsrb89_params params;
    wsrb89_u32 frames;
    wsrb89_u32 done;
    wsrb89_u32 count;
    wsrb89_u32 i;
    wsrb89_u32 nonzero;
    wsrb89_u32 checksum;

    wsrb89_init(&test_context, &test_workspace, sample_rate,
                 0x31415926U + preset + sample_rate);
    wsrb89_get_preset(&params, preset);
    wsrb89_set_params(&test_context, &params);
    wsrb89_trigger(&test_context, 30000U);

    frames = sample_rate * 4U;
    done = 0U;
    nonzero = 0U;
    checksum = 2166136261U;
    while (done < frames) {
        count = frames - done;
        if (count > TEST_CHUNK) {
            count = TEST_CHUNK;
        }
        wsrb89_render_stereo(&test_context, test_left, test_right, count);
        i = 0U;
        while (i < count) {
            if ((test_left[i] != 0) || (test_right[i] != 0)) {
                nonzero++;
            }
            checksum ^= (wsrb89_u16)test_left[i];
            checksum *= 16777619U;
            checksum ^= (wsrb89_u16)test_right[i];
            checksum *= 16777619U;
            i++;
        }
        done += count;
    }
    if (nonzero == 0U) {
        fprintf(stderr, "silent render: sr=%u preset=%u\n",
                (unsigned int)sample_rate, (unsigned int)preset);
        return 0;
    }
    printf("ok sr=%u preset=%u checksum=%08x nonzero=%u\n",
           (unsigned int)sample_rate, (unsigned int)preset,
           (unsigned int)checksum, (unsigned int)nonzero);
    return 1;
}


static wsrb89_u32 test_render_checksum(const wsrb89_params *params,
                                      wsrb89_u32 seed)
{
    wsrb89_u32 frames;
    wsrb89_u32 done;
    wsrb89_u32 count;
    wsrb89_u32 i;
    wsrb89_u32 checksum;

    wsrb89_init(&test_context, &test_workspace, 44100U, seed);
    wsrb89_set_params(&test_context, params);
    wsrb89_trigger(&test_context, 32767U);
    frames = 44100U;
    done = 0U;
    checksum = 2166136261U;
    while (done < frames) {
        count = frames - done;
        if (count > TEST_CHUNK) {
            count = TEST_CHUNK;
        }
        wsrb89_render_stereo(&test_context, test_left, test_right, count);
        i = 0U;
        while (i < count) {
            checksum ^= (wsrb89_u16)test_left[i];
            checksum *= 16777619U;
            checksum ^= (wsrb89_u16)test_right[i];
            checksum *= 16777619U;
            i++;
        }
        done += count;
    }
    return checksum;
}

static int test_octave_companions(void)
{
    wsrb89_params three_noise;
    wsrb89_params five_noise;
    wsrb89_u32 checksum_three;
    wsrb89_u32 checksum_five;

    wsrb89_get_preset(&five_noise, WSRB89_PRESET_HEAVY_IMPACT);
    three_noise = five_noise;
    three_noise.noise_level_q15[WSRB89_NOISE_BODY_LOW_OCTAVE] = 0;
    three_noise.noise_level_q15[WSRB89_NOISE_BODY_HIGH_OCTAVE] = 0;
    checksum_three = test_render_checksum(&three_noise, 0x51525354U);
    checksum_five = test_render_checksum(&five_noise, 0x51525354U);
    if (checksum_three == checksum_five) {
        fprintf(stderr, "octave companions produced no output difference\n");
        return 0;
    }
    printf("ok octave companions checksum3=%08x checksum5=%08x\n",
           (unsigned int)checksum_three, (unsigned int)checksum_five);
    return 1;
}


static int test_rumble_crackle_and_lfo(void)
{
    wsrb89_params full;
    wsrb89_params old_stack;
    wsrb89_params no_lfo;
    wsrb89_u32 checksum_full;
    wsrb89_u32 checksum_old;
    wsrb89_u32 checksum_no_lfo;

    wsrb89_get_preset(&full, WSRB89_PRESET_HEAVY_IMPACT);
    old_stack = full;
    old_stack.noise_level_q15[WSRB89_NOISE_RUMBLE] = 0;
    old_stack.noise_level_q15[WSRB89_NOISE_CRACKLE] = 0;
    old_stack.motion_lfo_depth_q15 = 0U;
    no_lfo = full;
    no_lfo.motion_lfo_depth_q15 = 0U;

    checksum_old = test_render_checksum(&old_stack, 0x71727374U);
    checksum_no_lfo = test_render_checksum(&no_lfo, 0x71727374U);
    checksum_full = test_render_checksum(&full, 0x71727374U);
    if (checksum_old == checksum_full) {
        fprintf(stderr, "rumble/crackle produced no output difference\n");
        return 0;
    }
    if (checksum_no_lfo == checksum_full) {
        fprintf(stderr, "motion LFO produced no output difference\n");
        return 0;
    }
    printf("ok rumble/crackle old=%08x no_lfo=%08x full=%08x\n",
           (unsigned int)checksum_old,
           (unsigned int)checksum_no_lfo,
           (unsigned int)checksum_full);
    return 1;
}

int main(void)
{
    static const wsrb89_u32 rates[4] = {8000U, 22050U, 44100U, 48000U};
    wsrb89_u16 preset;
    wsrb89_u16 rate_index;

    rate_index = 0U;
    while (rate_index < 4U) {
        preset = 0U;
        while (preset < WSRB89_PRESET_COUNT) {
            if (!test_one(rates[rate_index], preset)) {
                return 1;
            }
            preset++;
        }
        rate_index++;
    }
    if (!test_octave_companions()) {
        return 1;
    }
    if (!test_rumble_crackle_and_lfo()) {
        return 1;
    }
    return 0;
}
