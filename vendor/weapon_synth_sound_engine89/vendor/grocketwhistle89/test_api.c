#include "grocketwhistle89.h"

#define TEST_RATE 44100
#define TEST_BLOCK 128

static gwh89_s32 block_energy(const gwh89_s16 *block, gwh89_s32 samples)
{
    gwh89_s32 i;
    gwh89_s32 value;
    gwh89_s32 energy;
    energy = 0;
    for (i = 0; i < samples; ++i) {
        value = block[i];
        if (value < 0) {
            value = -value;
        }
        energy += value >> 6;
    }
    return energy;
}

int main(void)
{
    gwh89_state state;
    gwh89_state flat_state;
    gwh89_s16 block[TEST_BLOCK * 2];
    gwh89_s16 flat_block[TEST_BLOCK * 2];
    gwh89_s32 i;
    gwh89_s32 nonzero;
    gwh89_s32 guard;
    gwh89_s32 frequency;
    gwh89_s32 shaped_energy;
    gwh89_s32 flat_energy;
    gwh89_s32 saw_tail;
    gwh89_s32 saw_stereo_air;

    gwh89_init(&state, TEST_RATE, 0x13579BDFU);
    gwh89_trigger_preset(&state, GWH89_PRESET_RPG7_SUSTAINED);
    gwh89_set_auto_hold_ms(&state, 0);

    nonzero = 0;
    saw_stereo_air = 0;
    for (i = 0; i < 400; ++i) {
        gwh89_set_motion(&state, -40 + (i / 5), 28000);
        gwh89_render_stereo(&state, block, TEST_BLOCK);
        if (block[0] != 0 || block[1] != 0) {
            nonzero = 1;
        }
        if (block[0] != block[1]) {
            saw_stereo_air = 1;
        }
    }

    if (!nonzero) {
        return 1;
    }
    if (!gwh89_is_active(&state)) {
        return 2;
    }
    if (!saw_stereo_air) {
        return 10;
    }

    frequency = gwh89_get_current_frequency_hz(&state);
    if (frequency < 100 || frequency > 10000) {
        return 3;
    }

    gwh89_release(&state);
    guard = 0;
    saw_tail = 0;
    while (gwh89_is_active(&state) && guard < 1500) {
        gwh89_render_stereo(&state, block, TEST_BLOCK);
        if (state.stage == GWH89_STAGE_REVERB_TAIL &&
            block_energy(block, TEST_BLOCK * 2) > 0) {
            saw_tail = 1;
        }
        guard += 1;
    }

    if (gwh89_is_active(&state)) {
        return 4;
    }
    if (!saw_tail) {
        return 5;
    }

    gwh89_init(&state, TEST_RATE, 0x2468ACE0U);
    gwh89_init(&flat_state, TEST_RATE, 0x2468ACE0U);
    gwh89_trigger_preset(&state, GWH89_PRESET_AT4_FAST);
    gwh89_trigger_preset(&flat_state, GWH89_PRESET_AT4_FAST);
    gwh89_set_reverb_enabled(&state, 0);
    gwh89_set_reverb_enabled(&flat_state, 0);
    gwh89_set_eq_flat(&flat_state);
    gwh89_set_output_eq_flat(&flat_state);

    for (i = 0; i < 20; ++i) {
        gwh89_render_stereo(&state, block, TEST_BLOCK);
        gwh89_render_stereo(&flat_state, flat_block, TEST_BLOCK);
    }
    shaped_energy = block_energy(block, TEST_BLOCK * 2);
    flat_energy = block_energy(flat_block, TEST_BLOCK * 2);
    if (shaped_energy == flat_energy) {
        return 6;
    }

    if (gwh89_get_eq_crossover_hz(3) != 1800) {
        return 7;
    }
    if (gwh89_get_eq_band_name(GWH89_EQ_FUNDAMENTAL_900_1800) == 0) {
        return 8;
    }
    if (gwh89_get_output_eq_crossover_hz(3) != 2400) {
        return 11;
    }
    if (gwh89_get_output_eq_band_name(
            GWH89_OUTPUT_EQ_AIR_ABOVE_6000) == 0) {
        return 12;
    }
    if (gwh89_get_preset(GWH89_PRESET_CHIFLADORA_AIR_REF) == 0) {
        return 13;
    }

    gwh89_trigger_preset(&state, GWH89_PRESET_JAVELIN_TWO_STAGE);
    if (gwh89_get_estimated_total_samples(&state) <= 0) {
        return 9;
    }

    return 0;
}
