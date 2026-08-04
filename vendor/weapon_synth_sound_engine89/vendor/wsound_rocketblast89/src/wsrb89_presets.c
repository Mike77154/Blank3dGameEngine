#include "wsound_rocketblast89.h"

static void wsrb89_params_clear(wsrb89_params *p)
{
    unsigned char *bytes;
    wsrb89_u32 i;
    bytes = (unsigned char *)p;
    i = 0U;
    while (i < (wsrb89_u32)sizeof(wsrb89_params)) {
        bytes[i] = 0U;
        i++;
    }
}

static void wsrb89_preset_common(wsrb89_params *p)
{
    wsrb89_params_clear(p);
    p->sub_start_hz_q8 = 16744U;
    p->sub_end_hz_q8 = 8372U;
    p->sub_pitch_drop_ms = 430U;
    p->attack_ms = 1U;
    p->decay_ms = 430U;
    p->sustain_level_q15 = 1200U;
    p->sustain_ms = 35U;
    p->release_ms = 1250U;
    p->crack_decay_ms = 34U;
    p->debris_decay_ms = 700U;
    p->rumble_decay_ms = 1650U;
    p->crackle_decay_ms = 900U;
    p->svf_cutoff_hz = 3300U;
    p->svf_damp_q15 = 25200U;
    p->svf_mode = WSRB89_SVF_LOW;
    p->rumble_cutoff_hz = 145U;
    p->rumble_damp_q15 = 27000U;
    p->crackle_cutoff_hz = 3600U;
    p->crackle_damp_q15 = 24800U;
    p->motion_lfo_rate_millihz = 820U;
    p->motion_lfo_depth_q15 = 3400U;
    p->drive_q12 = 7200U;
    p->chorus_mix_q15 = 300U;
    p->chorus_base_ms = 29U;
    p->chorus_depth_ms = 1U;
    p->chorus_rate_millihz = 70U;
    p->reverb_mix_q15 = 11500U;
    p->reverb_feedback_q15 = 26000U;
    p->reverb_damp_q15 = 25000U;
    p->output_gain_q12 = 5200U;
}

void wsrb89_get_preset(wsrb89_params *p, wsrb89_u16 preset_id)
{
    wsrb89_preset_common(p);

    if (preset_id == WSRB89_PRESET_CONCRETE_PAAS) {
        p->noise_level_q15[WSRB89_NOISE_CRACK] = 28600;
        p->noise_level_q15[WSRB89_NOISE_BODY] = 26600;
        p->noise_level_q15[WSRB89_NOISE_DEBRIS] = 20800;
        p->noise_level_q15[WSRB89_NOISE_BODY_LOW_OCTAVE] = 23200;
        p->noise_level_q15[WSRB89_NOISE_BODY_HIGH_OCTAVE] = 5200;
        p->noise_level_q15[WSRB89_NOISE_RUMBLE] = 20500;
        p->noise_level_q15[WSRB89_NOISE_CRACKLE] = 11800;
        p->sine_level_q15 = 9800;
        p->saw_level_q15 = 1200;
        p->shock_level_q15 = 31000;
        p->decay_ms = 390U;
        p->sustain_level_q15 = 1000U;
        p->release_ms = 1100U;
        p->crack_decay_ms = 30U;
        p->debris_decay_ms = 850U;
        p->rumble_decay_ms = 1550U;
        p->crackle_decay_ms = 1100U;
        p->svf_cutoff_hz = 3500U;
        p->rumble_cutoff_hz = 155U;
        p->crackle_cutoff_hz = 3300U;
        p->eq_gain_q14[0] = 6200;
        p->eq_gain_q14[1] = 5000;
        p->eq_gain_q14[2] = 2300;
        p->eq_gain_q14[3] = -1700;
        p->eq_gain_q14[4] = -2800;
        p->eq_gain_q14[5] = -5600;
        p->drive_q12 = 7600U;
        p->reverb_mix_q15 = 11000U;
        p->reverb_feedback_q15 = 25200U;
        p->reverb_damp_q15 = 24800U;
        p->output_gain_q12 = 5200U;
        return;
    }

    if (preset_id == WSRB89_PRESET_METAL_STRIKE) {
        p->noise_level_q15[WSRB89_NOISE_CRACK] = 27600;
        p->noise_level_q15[WSRB89_NOISE_BODY] = 24600;
        p->noise_level_q15[WSRB89_NOISE_DEBRIS] = 24400;
        p->noise_level_q15[WSRB89_NOISE_BODY_LOW_OCTAVE] = 20500;
        p->noise_level_q15[WSRB89_NOISE_BODY_HIGH_OCTAVE] = 6500;
        p->noise_level_q15[WSRB89_NOISE_RUMBLE] = 17200;
        p->noise_level_q15[WSRB89_NOISE_CRACKLE] = 15800;
        p->sine_level_q15 = 9000;
        p->saw_level_q15 = 900;
        p->shock_level_q15 = 30000;
        p->sub_start_hz_q8 = 17408U;
        p->sub_end_hz_q8 = 9216U;
        p->sub_pitch_drop_ms = 320U;
        p->decay_ms = 350U;
        p->sustain_level_q15 = 800U;
        p->release_ms = 1000U;
        p->debris_decay_ms = 900U;
        p->rumble_decay_ms = 1350U;
        p->crackle_decay_ms = 1250U;
        p->svf_cutoff_hz = 4200U;
        p->rumble_cutoff_hz = 170U;
        p->crackle_cutoff_hz = 4300U;
        p->motion_lfo_rate_millihz = 980U;
        p->svf_damp_q15 = 24500U;
        p->eq_gain_q14[0] = 4400;
        p->eq_gain_q14[1] = 3500;
        p->eq_gain_q14[2] = 500;
        p->eq_gain_q14[3] = 1600;
        p->eq_gain_q14[4] = 500;
        p->eq_gain_q14[5] = -3500;
        p->drive_q12 = 7800U;
        p->chorus_mix_q15 = 400U;
        p->reverb_mix_q15 = 10500U;
        p->reverb_feedback_q15 = 25000U;
        p->output_gain_q12 = 5100U;
        return;
    }

    if (preset_id == WSRB89_PRESET_AIRBURST) {
        p->noise_level_q15[WSRB89_NOISE_CRACK] = 30000;
        p->noise_level_q15[WSRB89_NOISE_BODY] = 25200;
        p->noise_level_q15[WSRB89_NOISE_DEBRIS] = 20800;
        p->noise_level_q15[WSRB89_NOISE_BODY_LOW_OCTAVE] = 19500;
        p->noise_level_q15[WSRB89_NOISE_BODY_HIGH_OCTAVE] = 5500;
        p->noise_level_q15[WSRB89_NOISE_RUMBLE] = 13500;
        p->noise_level_q15[WSRB89_NOISE_CRACKLE] = 10500;
        p->sine_level_q15 = 8200;
        p->saw_level_q15 = 900;
        p->shock_level_q15 = 32000;
        p->sub_start_hz_q8 = 15872U;
        p->sub_end_hz_q8 = 8704U;
        p->sub_pitch_drop_ms = 290U;
        p->decay_ms = 330U;
        p->sustain_level_q15 = 700U;
        p->release_ms = 900U;
        p->crack_decay_ms = 24U;
        p->debris_decay_ms = 500U;
        p->rumble_decay_ms = 820U;
        p->crackle_decay_ms = 620U;
        p->svf_cutoff_hz = 4300U;
        p->rumble_cutoff_hz = 190U;
        p->crackle_cutoff_hz = 3900U;
        p->svf_damp_q15 = 25000U;
        p->eq_gain_q14[0] = 3900;
        p->eq_gain_q14[1] = 3000;
        p->eq_gain_q14[2] = 1200;
        p->eq_gain_q14[3] = 600;
        p->eq_gain_q14[4] = -1400;
        p->eq_gain_q14[5] = -3800;
        p->drive_q12 = 7100U;
        p->chorus_mix_q15 = 120U;
        p->reverb_mix_q15 = 8500U;
        p->reverb_feedback_q15 = 24500U;
        p->output_gain_q12 = 5200U;
        return;
    }

    if (preset_id == WSRB89_PRESET_INDOOR_BUNKER) {
        p->noise_level_q15[WSRB89_NOISE_CRACK] = 26800;
        p->noise_level_q15[WSRB89_NOISE_BODY] = 30000;
        p->noise_level_q15[WSRB89_NOISE_DEBRIS] = 15000;
        p->noise_level_q15[WSRB89_NOISE_BODY_LOW_OCTAVE] = 29200;
        p->noise_level_q15[WSRB89_NOISE_BODY_HIGH_OCTAVE] = 1800;
        p->noise_level_q15[WSRB89_NOISE_RUMBLE] = 27800;
        p->noise_level_q15[WSRB89_NOISE_CRACKLE] = 7600;
        p->sine_level_q15 = 13000;
        p->saw_level_q15 = 1200;
        p->shock_level_q15 = 29200;
        p->sub_pitch_drop_ms = 620U;
        p->decay_ms = 600U;
        p->sustain_level_q15 = 2500U;
        p->sustain_ms = 70U;
        p->release_ms = 1900U;
        p->crack_decay_ms = 42U;
        p->debris_decay_ms = 950U;
        p->rumble_decay_ms = 2450U;
        p->crackle_decay_ms = 1050U;
        p->svf_cutoff_hz = 2500U;
        p->rumble_cutoff_hz = 112U;
        p->crackle_cutoff_hz = 2800U;
        p->motion_lfo_rate_millihz = 540U;
        p->motion_lfo_depth_q15 = 4400U;
        p->svf_damp_q15 = 25200U;
        p->eq_gain_q14[0] = 7600;
        p->eq_gain_q14[1] = 6500;
        p->eq_gain_q14[2] = 3800;
        p->eq_gain_q14[3] = -2600;
        p->eq_gain_q14[4] = -4600;
        p->eq_gain_q14[5] = -7600;
        p->drive_q12 = 7600U;
        p->chorus_mix_q15 = 500U;
        p->chorus_base_ms = 33U;
        p->chorus_depth_ms = 1U;
        p->reverb_mix_q15 = 18500U;
        p->reverb_feedback_q15 = 28500U;
        p->reverb_damp_q15 = 26600U;
        p->output_gain_q12 = 4900U;
        return;
    }

    if (preset_id == WSRB89_PRESET_DISTANT_PAAS) {
        p->noise_level_q15[WSRB89_NOISE_CRACK] = 15600;
        p->noise_level_q15[WSRB89_NOISE_BODY] = 30000;
        p->noise_level_q15[WSRB89_NOISE_DEBRIS] = 5200;
        p->noise_level_q15[WSRB89_NOISE_BODY_LOW_OCTAVE] = 29000;
        p->noise_level_q15[WSRB89_NOISE_BODY_HIGH_OCTAVE] = 0;
        p->noise_level_q15[WSRB89_NOISE_RUMBLE] = 25800;
        p->noise_level_q15[WSRB89_NOISE_CRACKLE] = 2800;
        p->sine_level_q15 = 14500;
        p->saw_level_q15 = 500;
        p->shock_level_q15 = 16500;
        p->sub_start_hz_q8 = 14336U;
        p->sub_end_hz_q8 = 7680U;
        p->sub_pitch_drop_ms = 720U;
        p->attack_ms = 4U;
        p->decay_ms = 700U;
        p->sustain_level_q15 = 2100U;
        p->sustain_ms = 80U;
        p->release_ms = 2000U;
        p->crack_decay_ms = 65U;
        p->debris_decay_ms = 950U;
        p->rumble_decay_ms = 2600U;
        p->crackle_decay_ms = 900U;
        p->svf_cutoff_hz = 1250U;
        p->rumble_cutoff_hz = 95U;
        p->crackle_cutoff_hz = 2400U;
        p->motion_lfo_rate_millihz = 380U;
        p->motion_lfo_depth_q15 = 4800U;
        p->svf_damp_q15 = 23000U;
        p->eq_gain_q14[0] = 7600;
        p->eq_gain_q14[1] = 6800;
        p->eq_gain_q14[2] = 3300;
        p->eq_gain_q14[3] = -4200;
        p->eq_gain_q14[4] = -7600;
        p->eq_gain_q14[5] = -9000;
        p->drive_q12 = 6800U;
        p->chorus_mix_q15 = 150U;
        p->reverb_mix_q15 = 17000U;
        p->reverb_feedback_q15 = 27600U;
        p->reverb_damp_q15 = 26800U;
        p->output_gain_q12 = 5400U;
        return;
    }

    if (preset_id == WSRB89_PRESET_COMPACT_RPG) {
        p->noise_level_q15[WSRB89_NOISE_CRACK] = 29400;
        p->noise_level_q15[WSRB89_NOISE_BODY] = 24400;
        p->noise_level_q15[WSRB89_NOISE_DEBRIS] = 16400;
        p->noise_level_q15[WSRB89_NOISE_BODY_LOW_OCTAVE] = 20500;
        p->noise_level_q15[WSRB89_NOISE_BODY_HIGH_OCTAVE] = 5500;
        p->noise_level_q15[WSRB89_NOISE_RUMBLE] = 15500;
        p->noise_level_q15[WSRB89_NOISE_CRACKLE] = 11200;
        p->sine_level_q15 = 9000;
        p->saw_level_q15 = 1000;
        p->shock_level_q15 = 31800;
        p->sub_start_hz_q8 = 17664U;
        p->sub_end_hz_q8 = 9472U;
        p->sub_pitch_drop_ms = 250U;
        p->decay_ms = 260U;
        p->sustain_level_q15 = 500U;
        p->sustain_ms = 15U;
        p->release_ms = 750U;
        p->crack_decay_ms = 22U;
        p->debris_decay_ms = 400U;
        p->rumble_decay_ms = 900U;
        p->crackle_decay_ms = 560U;
        p->svf_cutoff_hz = 3800U;
        p->rumble_cutoff_hz = 175U;
        p->crackle_cutoff_hz = 3800U;
        p->motion_lfo_rate_millihz = 1150U;
        p->eq_gain_q14[0] = 4700;
        p->eq_gain_q14[1] = 3600;
        p->eq_gain_q14[2] = 1200;
        p->eq_gain_q14[3] = -500;
        p->eq_gain_q14[4] = -2200;
        p->eq_gain_q14[5] = -4600;
        p->drive_q12 = 7900U;
        p->chorus_mix_q15 = 150U;
        p->reverb_mix_q15 = 8500U;
        p->reverb_feedback_q15 = 24500U;
        p->output_gain_q12 = 5200U;
        return;
    }

    p->noise_level_q15[WSRB89_NOISE_CRACK] = 28000;
    p->noise_level_q15[WSRB89_NOISE_BODY] = 29000;
    p->noise_level_q15[WSRB89_NOISE_DEBRIS] = 15000;
    p->noise_level_q15[WSRB89_NOISE_BODY_LOW_OCTAVE] = 26000;
    p->noise_level_q15[WSRB89_NOISE_BODY_HIGH_OCTAVE] = 3000;
    p->noise_level_q15[WSRB89_NOISE_RUMBLE] = 22800;
    p->noise_level_q15[WSRB89_NOISE_CRACKLE] = 9800;
    p->sine_level_q15 = 12000;
    p->saw_level_q15 = 1200;
    p->shock_level_q15 = 31000;
    p->sub_pitch_drop_ms = 520U;
    p->decay_ms = 500U;
    p->sustain_level_q15 = 1800U;
    p->sustain_ms = 50U;
    p->release_ms = 1450U;
    p->crack_decay_ms = 36U;
    p->debris_decay_ms = 820U;
    p->rumble_decay_ms = 1900U;
    p->crackle_decay_ms = 980U;
    p->svf_cutoff_hz = 3000U;
    p->rumble_cutoff_hz = 125U;
    p->crackle_cutoff_hz = 3400U;
    p->motion_lfo_rate_millihz = 720U;
    p->motion_lfo_depth_q15 = 3900U;
    p->eq_gain_q14[0] = 7000;
    p->eq_gain_q14[1] = 5600;
    p->eq_gain_q14[2] = 2800;
    p->eq_gain_q14[3] = -2000;
    p->eq_gain_q14[4] = -4600;
    p->eq_gain_q14[5] = -7200;
    p->drive_q12 = 7200U;
    p->reverb_mix_q15 = 13500U;
    p->reverb_feedback_q15 = 27000U;
    p->reverb_damp_q15 = 25500U;
    p->output_gain_q12 = 5300U;
}
