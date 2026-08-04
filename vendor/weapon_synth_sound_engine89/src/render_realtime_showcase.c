#include <stdio.h>
#include <string.h>

#include "gpaah89.h"
#include "gweaponbody89.h"
#include "gmuzzlegas89.h"
#include "gballisticcrack89.h"
#include "glatetail89.h"
#include "gcinemathump89.h"
#include "chuecka89.h"
#include "gweaponfoley89.h"
#include "gshotgunsequence89.h"
#include "gweaponvoice89.h"
#include "gsynthsoundengine89.h"
#include "wsound_rocketblast89.h"

#define SHOW_RATE 44100U
#define SHOW_LOGICAL 256U
#define SHOW_PHYSICAL 64U
#define SHOW_VARIANTS 4U
#define SHOW_WEAPONS 4U
#define SHOW_MAX_SHOT_FRAMES 132300U
#define SHOW_MECH_COUNT 4U
#define SHOW_MAX_MECH_FRAMES 66150U
#define SHOW_MAX_EVENTS 768U
#define SHOW_MAX_CASING_EVENTS 640U
#define SHOW_MAX_FIRE_EVENTS 24U
#define SHOW_MAX_ROCKET_BLAST_FRAMES 220500U
#define SHOW_CASING_POOL 128U
#define SHOW_FIRE_POOL 12U
#define SHOW_MAX_SCENE_FRAMES 882000U
#define SHOW_Q15 32767

#define WEAPON_PISTOL 0U
#define WEAPON_MACHINE 1U
#define WEAPON_SHOTGUN 2U
#define WEAPON_ROCKET 3U

#define MECH_SHOTGUN_PUMP 0U
#define MECH_SHOTGUN_INSERT 1U
#define MECH_PISTOL_SLIDE 2U
#define MECH_MACHINE_FEED 3U

typedef signed short show_s16;
typedef signed int show_s32;
typedef unsigned int show_u32;
typedef unsigned short show_u16;
typedef unsigned char show_u8;

typedef struct show_bank_s {
    show_s16 *pcm;
    show_u32 frames;
    show_u16 nominal_level_q15;
} show_bank;

typedef struct show_source_s {
    const show_s16 *pcm;
    show_u32 frames;
    show_u32 frame_pos;
    show_u16 frac_q16;
    show_u32 step_q16;
    show_s32 lowpass;
    show_u8 lowpass_shift;
    show_s16 source_gain_q15;
} show_source;

typedef struct show_event_s {
    show_u32 start_frame;
    show_source source;
    gwv89_event_class event_class;
    show_s16 gain_q15;
    show_s16 pan_q15;
    show_u16 distance_q15;
    show_u16 occlusion_q15;
    show_u16 focus_q15;
    show_s16 priority_bias;
    show_u32 instance_key;
    show_u16 instance_limit;
    show_u16 attack_ms;
    show_u16 release_ms;
    show_u8 started;
} show_event;

typedef struct show_casing_event_s {
    show_u32 start_frame;
    gsse89_casing_params params;
    show_u32 seed;
    show_u8 started;
} show_casing_event;

typedef struct show_fire_event_s {
    show_u32 start_frame;
    gsse89_fire_params params;
    show_u32 seed;
    show_u8 started;
} show_fire_event;

static show_s16 shot_storage[SHOW_WEAPONS][SHOW_VARIANTS][SHOW_MAX_SHOT_FRAMES];
static show_s16 mech_storage[SHOW_MECH_COUNT][SHOW_MAX_MECH_FRAMES];
static show_s16 rocket_blast_storage[SHOW_MAX_ROCKET_BLAST_FRAMES];
static show_bank shot_bank[SHOW_WEAPONS][SHOW_VARIANTS];
static show_bank mech_bank[SHOW_MECH_COUNT];
static show_bank rocket_blast_bank;
static show_s16 scratch_a[SHOW_MAX_MECH_FRAMES];
static show_s16 shotgun_texture0[GSS89_RECOMMENDED_TEXTURE_FRAMES];
static show_s16 shotgun_texture1[GSS89_RECOMMENDED_TEXTURE_FRAMES];
static show_event events[SHOW_MAX_EVENTS];
static show_u16 event_count;
static show_casing_event casing_events[SHOW_MAX_CASING_EVENTS];
static show_u16 casing_event_count;
static show_fire_event fire_events[SHOW_MAX_FIRE_EVENTS];
static show_u16 fire_event_count;
static gsse89_casing_voice casing_voice_storage[SHOW_CASING_POOL];
static gsse89_fire_voice fire_voice_storage[SHOW_FIRE_POOL];
static gv89_voice voice_storage[SHOW_LOGICAL];
static show_s16 scene_output[SHOW_MAX_SCENE_FRAMES * 2U];

static show_s16 show_sat16(show_s32 value)
{
    if (value > 32767) return 32767;
    if (value < -32768) return -32768;
    return (show_s16)value;
}

static show_s32 show_abs32(show_s32 value)
{
    if (value < 0) return -value;
    return value;
}

static show_u32 show_ms(show_u32 milliseconds)
{
    return (milliseconds * SHOW_RATE) / 1000U;
}

static void show_zero_s16(show_s16 *pcm, show_u32 frames)
{
    show_u32 i;
    for (i = 0U; i < frames; ++i) pcm[i] = 0;
}

static void show_normalize(show_s16 *pcm, show_u32 frames, show_s16 target)
{
    show_u32 i;
    show_s32 peak;
    show_s32 value;
    show_s32 scale_q15;
    peak = 1;
    for (i = 0U; i < frames; ++i) {
        value = show_abs32((show_s32)pcm[i]);
        if (value > peak) peak = value;
    }
    scale_q15 = ((show_s32)target * 32767) / peak;
    if (scale_q15 > 49152) scale_q15 = 49152;
    for (i = 0U; i < frames; ++i) {
        value = ((show_s32)pcm[i] * scale_q15) >> 15;
        pcm[i] = show_sat16(value);
    }
}

static int show_write_u16(FILE *file, show_u16 value)
{
    if (fputc((int)(value & 255U), file) == EOF) return 0;
    if (fputc((int)((value >> 8) & 255U), file) == EOF) return 0;
    return 1;
}

static int show_write_u32(FILE *file, show_u32 value)
{
    if (fputc((int)(value & 255U), file) == EOF) return 0;
    if (fputc((int)((value >> 8) & 255U), file) == EOF) return 0;
    if (fputc((int)((value >> 16) & 255U), file) == EOF) return 0;
    if (fputc((int)((value >> 24) & 255U), file) == EOF) return 0;
    return 1;
}

static int show_write_wav(const char *path, const show_s16 *pcm,
                          show_u32 frames)
{
    FILE *file;
    show_u32 samples;
    show_u32 bytes;
    show_u32 i;
    file = fopen(path, "wb");
    if (file == 0) return 0;
    samples = frames * 2U;
    bytes = samples * 2U;
    if (fwrite("RIFF", 1U, 4U, file) != 4U) return 0;
    if (!show_write_u32(file, 36U + bytes)) return 0;
    if (fwrite("WAVEfmt ", 1U, 8U, file) != 8U) return 0;
    if (!show_write_u32(file, 16U)) return 0;
    if (!show_write_u16(file, 1U)) return 0;
    if (!show_write_u16(file, 2U)) return 0;
    if (!show_write_u32(file, SHOW_RATE)) return 0;
    if (!show_write_u32(file, SHOW_RATE * 4U)) return 0;
    if (!show_write_u16(file, 4U)) return 0;
    if (!show_write_u16(file, 16U)) return 0;
    if (fwrite("data", 1U, 4U, file) != 4U) return 0;
    if (!show_write_u32(file, bytes)) return 0;
    for (i = 0U; i < samples; ++i) {
        if (!show_write_u16(file, (show_u16)pcm[i])) return 0;
    }
    if (fclose(file) != 0) return 0;
    return 1;
}

static int show_render_report(show_u16 weapon, show_u32 seed,
                              show_s16 *dst, show_u32 capacity,
                              show_u32 *written)
{
    gpaah89_preset report_preset;
    gpaah89_state report;
    gwb89_preset body_preset;
    gwb89_context body;
    gmg89_preset gas_preset;
    gmg89_context gas;
    gbc89_preset crack_preset;
    gbc89_context crack;
    glt89_preset tail_preset;
    glt89_context tail;
    gct89_preset thump_preset;
    gct89_context thump;
    gpaah89_preset_id report_id;
    gwb89_preset_id body_id;
    gmg89_preset_id gas_id;
    gbc89_preset_id crack_id;
    glt89_preset_id tail_id;
    gct89_preset_id thump_id;
    show_u32 frames;
    show_u32 i;
    show_s16 dry;
    show_s16 body_sample;
    show_s16 gas_sample;
    show_s16 crack_sample;
    show_s16 thump_sample;
    show_s16 tail_sample;
    show_s32 mix;
    show_s32 input_tail;
    show_s16 target;

    if (weapon == WEAPON_PISTOL) {
        report_id = GPAAH89_PRESET_PISTOL;
        body_id = GWB89_PRESET_PISTOL;
        gas_id = GMG89_PRESET_PISTOL;
        crack_id = GBC89_PRESET_MEDIUM;
        tail_id = GLT89_PRESET_SMALL_ROOM;
        thump_id = GCT89_PRESET_SUBTLE;
        frames = show_ms(1250U);
        target = 24500;
    } else if (weapon == WEAPON_MACHINE) {
        report_id = GPAAH89_PRESET_METRALLA;
        body_id = GWB89_PRESET_RIFLE;
        gas_id = GMG89_PRESET_RIFLE;
        crack_id = GBC89_PRESET_RIFLE_PASS;
        tail_id = GLT89_PRESET_CORRIDOR;
        thump_id = GCT89_PRESET_ACTION;
        frames = show_ms(1100U);
        target = 22500;
    } else if (weapon == WEAPON_SHOTGUN) {
        report_id = GPAAH89_PRESET_SHOTGUN;
        body_id = GWB89_PRESET_SHOTGUN;
        gas_id = GMG89_PRESET_SHOTGUN;
        crack_id = GBC89_PRESET_NEAR;
        tail_id = GLT89_PRESET_WAREHOUSE;
        thump_id = GCT89_PRESET_SHOTGUN;
        frames = show_ms(2100U);
        target = 28500;
    } else {
        report_id = GPAAH89_PRESET_ROCKET_LAUNCHER;
        body_id = GWB89_PRESET_LAUNCHER;
        gas_id = GMG89_PRESET_LAUNCHER;
        crack_id = GBC89_PRESET_FAR;
        tail_id = GLT89_PRESET_EXTERIOR;
        thump_id = GCT89_PRESET_LAUNCHER;
        frames = show_ms(2600U);
        target = 30000;
    }
    if (frames > capacity) return 0;
    if (!gpaah89_get_preset(report_id, &report_preset)) return 0;
    if (!gwb89_get_preset(body_id, &body_preset)) return 0;
    if (!gmg89_get_preset(gas_id, &gas_preset)) return 0;
    if (!gbc89_get_preset(crack_id, &crack_preset)) return 0;
    if (!glt89_get_preset(tail_id, &tail_preset)) return 0;
    if (!gct89_get_preset(thump_id, &thump_preset)) return 0;

    report_preset.reverb_mix_q15 = (show_s16)(report_preset.reverb_mix_q15 * 3 / 5);
    report_preset.output_gain_q15 = (show_s16)(report_preset.output_gain_q15 * 4 / 5);
    tail_preset.wet_q15 = (show_s16)(tail_preset.wet_q15 * 3 / 4);
    tail_preset.dry_q15 = 16384;

    if (!gpaah89_init(&report, SHOW_RATE, &report_preset, seed)) return 0;
    if (!gwb89_init(&body, SHOW_RATE, &body_preset, seed + 11U)) return 0;
    if (!gmg89_init(&gas, SHOW_RATE, &gas_preset, seed + 23U)) return 0;
    if (!gbc89_init(&crack, SHOW_RATE, &crack_preset, seed + 37U)) return 0;
    if (!glt89_init(&tail, SHOW_RATE, &tail_preset)) return 0;
    if (!gct89_init(&thump, SHOW_RATE, &thump_preset, seed + 41U)) return 0;

    gpaah89_trigger(&report, seed + 101U);
    gwb89_trigger(&body, 29000, seed + 103U);
    gmg89_trigger(&gas, 28500, seed + 107U);
    if (weapon == WEAPON_SHOTGUN) {
        gbc89_trigger(&crack, 15U, 13000, seed + 109U);
    } else if (weapon == WEAPON_ROCKET) {
        gbc89_trigger(&crack, 4U, 10500, seed + 109U);
    } else {
        gbc89_trigger(&crack, 8U, 24500, seed + 109U);
    }
    if (weapon == WEAPON_PISTOL) {
        gct89_trigger(&thump, 18500, seed + 113U);
    } else if (weapon == WEAPON_ROCKET) {
        gct89_trigger(&thump, 30500, seed + 113U);
    } else {
        gct89_trigger(&thump, 27000, seed + 113U);
    }

    for (i = 0U; i < frames; ++i) {
        dry = 0;
        gpaah89_render_mono(&report, &dry, 1U);
        body_sample = gwb89_process_sample(&body, dry);
        gas_sample = gmg89_process_sample(&gas);
        crack_sample = gbc89_process_sample(&crack);
        thump_sample = gct89_process_sample(&thump);

        if (weapon == WEAPON_PISTOL) {
            mix = ((show_s32)dry * 18) / 32;
            mix += ((show_s32)body_sample * 15) / 32;
            mix += ((show_s32)gas_sample * 8) / 32;
            mix += ((show_s32)crack_sample * 7) / 32;
            mix += ((show_s32)thump_sample * 5) / 32;
        } else if (weapon == WEAPON_MACHINE) {
            mix = ((show_s32)dry * 17) / 32;
            mix += ((show_s32)body_sample * 16) / 32;
            mix += ((show_s32)gas_sample * 9) / 32;
            mix += ((show_s32)crack_sample * 9) / 32;
            mix += ((show_s32)thump_sample * 4) / 32;
        } else if (weapon == WEAPON_SHOTGUN) {
            mix = ((show_s32)dry * 17) / 32;
            mix += ((show_s32)body_sample * 17) / 32;
            mix += ((show_s32)gas_sample * 10) / 32;
            mix += ((show_s32)crack_sample * 3) / 32;
            mix += ((show_s32)thump_sample * 9) / 32;
        } else {
            mix = ((show_s32)dry * 19) / 32;
            mix += ((show_s32)body_sample * 18) / 32;
            mix += ((show_s32)gas_sample * 17) / 32;
            mix += ((show_s32)crack_sample * 2) / 32;
            mix += ((show_s32)thump_sample * 11) / 32;
        }
        input_tail = mix / 2;
        tail_sample = glt89_process_sample(&tail, show_sat16(input_tail));
        mix += ((show_s32)tail_sample - input_tail) * 13 / 32;
        dst[i] = show_sat16(mix * 3 / 5);
    }
    show_normalize(dst, frames, target);
    *written = frames;
    return 1;
}

static int show_render_chuecka_foley(ch89_preset ch_preset,
                                     show_u16 repetitions,
                                     gwf89_preset_id foley_preset,
                                     show_u32 seed,
                                     show_s16 *dst,
                                     show_u32 capacity,
                                     show_u32 *written,
                                     show_s16 ch_gain,
                                     show_s16 foley_gain,
                                     show_s16 target)
{
    ch89_context ch_ctx;
    ch89_gesture gesture;
    ch89_u32 ch_frames;
    gwf89_context foley;
    show_u32 i;
    show_u32 frames;
    show_s32 mix;
    if (ch89_init(&ch_ctx, SHOW_RATE, seed) != CH89_OK) return 0;
    if (ch89_make_preset(ch_preset, repetitions, 32767U, &gesture) != CH89_OK) return 0;
    ch_frames = 0U;
    show_zero_s16(scratch_a, SHOW_MAX_MECH_FRAMES);
    if (ch89_render(&ch_ctx, &gesture, scratch_a, SHOW_MAX_MECH_FRAMES,
                    &ch_frames) != CH89_OK) return 0;
    gwf89_init(&foley, seed + 97U);
    gwf89_set_room_send(&foley, 4500);
    gwf89_trigger_ex(&foley, foley_preset, seed + 101U,
                     GWF89_VARIANT_AUTO, GWF89_SPEED_NORMAL);
    frames = ch_frames;
    if (frames < show_ms(450U)) frames = show_ms(450U);
    if (frames > capacity) frames = capacity;
    for (i = 0U; i < frames; ++i) {
        mix = 0;
        if (i < ch_frames) mix += ((show_s32)scratch_a[i] * ch_gain) >> 15;
        mix += ((show_s32)gwf89_process_sample(&foley) * foley_gain) >> 15;
        dst[i] = show_sat16(mix);
    }
    show_normalize(dst, frames, target);
    *written = frames;
    return 1;
}

static int show_render_shotgun_pump(show_s16 *dst, show_u32 capacity,
                                    show_u32 *written)
{
    gss89_context ctx;
    gss89_mix mix;
    show_u32 frames;
    show_u32 i;
    if (gss89_init(&ctx, SHOW_RATE, 0x50554D50U) != GSS89_OK) return 0;
    if (gss89_bind_texture_buffer(&ctx, 0U, shotgun_texture0,
                                  GSS89_RECOMMENDED_TEXTURE_FRAMES) != GSS89_OK) return 0;
    if (gss89_bind_texture_buffer(&ctx, 1U, shotgun_texture1,
                                  GSS89_RECOMMENDED_TEXTURE_FRAMES) != GSS89_OK) return 0;
    gss89_mix_default(&mix);
    mix.report_q15 = 0;
    mix.report_body_q15 = 0;
    mix.report_gas_q15 = 0;
    mix.report_crack_q15 = 0;
    mix.report_thump_q15 = 0;
    mix.report_tail_q15 = 0;
    mix.general_chuecka_q15 = 0;
    mix.foley_q15 = 0;
    mix.master_q15 = 30000;
    gss89_set_mix(&ctx, &mix);
    if (gss89_trigger_pump(&ctx, GPUMP89_PRESET_WITH_SHELL,
                           0x4B4C454BU) != GSS89_OK) return 0;
    frames = show_ms(850U);
    if (frames > capacity) frames = capacity;
    for (i = 0U; i < frames; ++i) dst[i] = gss89_process_sample(&ctx);
    show_normalize(dst, frames, 21500);
    *written = frames;
    return 1;
}

static int show_render_rocket_blast(show_s16 *dst, show_u32 capacity,
                                    show_u32 *written)
{
    wsrb89_context ctx;
    wsrb89_workspace workspace;
    wsrb89_params params;
    wsrb89_s16 left;
    wsrb89_s16 right;
    show_u32 frames;
    show_u32 i;
    show_s32 mono;
    if (dst == 0 || written == 0) return 0;
    frames = show_ms(5000U);
    if (frames > capacity) return 0;
    wsrb89_get_preset(&params, WSRB89_PRESET_HEAVY_IMPACT);
    wsrb89_init(&ctx, &workspace, SHOW_RATE, 0x524F434BU);
    wsrb89_set_params(&ctx, &params);
    wsrb89_trigger(&ctx, 32767U);
    for (i = 0U; i < frames; ++i) {
        left = 0;
        right = 0;
        wsrb89_render_stereo(&ctx, &left, &right, 1U);
        mono = ((show_s32)left + (show_s32)right) / 2;
        dst[i] = show_sat16(mono);
    }
    show_normalize(dst, frames, 30000);
    *written = frames;
    return 1;
}

static int show_build_banks(void)
{
    show_u16 weapon;
    show_u16 variant;
    show_u32 frames;
    show_u32 seed;
    for (weapon = 0U; weapon < SHOW_WEAPONS; ++weapon) {
        for (variant = 0U; variant < SHOW_VARIANTS; ++variant) {
            seed = 1001U + (show_u32)weapon * 10000U +
                   (show_u32)variant * 977U;
            frames = 0U;
            if (!show_render_report(weapon, seed,
                                    shot_storage[weapon][variant],
                                    SHOW_MAX_SHOT_FRAMES, &frames)) return 0;
            shot_bank[weapon][variant].pcm = shot_storage[weapon][variant];
            shot_bank[weapon][variant].frames = frames;
            shot_bank[weapon][variant].nominal_level_q15 = 32767U;
        }
    }

    frames = 0U;
    if (!show_render_rocket_blast(rocket_blast_storage,
                                  SHOW_MAX_ROCKET_BLAST_FRAMES,
                                  &frames)) return 0;
    rocket_blast_bank.pcm = rocket_blast_storage;
    rocket_blast_bank.frames = frames;
    rocket_blast_bank.nominal_level_q15 = 32767U;

    frames = 0U;
    if (!show_render_shotgun_pump(mech_storage[MECH_SHOTGUN_PUMP],
                                  SHOW_MAX_MECH_FRAMES, &frames)) return 0;
    mech_bank[MECH_SHOTGUN_PUMP].pcm = mech_storage[MECH_SHOTGUN_PUMP];
    mech_bank[MECH_SHOTGUN_PUMP].frames = frames;
    mech_bank[MECH_SHOTGUN_PUMP].nominal_level_q15 = 32767U;

    frames = 0U;
    if (!show_render_chuecka_foley(CH89_PRESET_SHOTGUN_INSERT, 1U,
                                   GWF89_SHOTGUN_EMPTY, 2101U,
                                   mech_storage[MECH_SHOTGUN_INSERT],
                                   SHOW_MAX_MECH_FRAMES, &frames,
                                   30000, 7000, 23500)) return 0;
    mech_bank[MECH_SHOTGUN_INSERT].pcm = mech_storage[MECH_SHOTGUN_INSERT];
    mech_bank[MECH_SHOTGUN_INSERT].frames = frames;
    mech_bank[MECH_SHOTGUN_INSERT].nominal_level_q15 = 32767U;

    frames = 0U;
    if (!show_render_chuecka_foley(CH89_PRESET_PISTOL_SLIDE, 1U,
                                   GWF89_PISTOL_HANDLING, 3101U,
                                   mech_storage[MECH_PISTOL_SLIDE],
                                   SHOW_MAX_MECH_FRAMES, &frames,
                                   28000, 11000, 23500)) return 0;
    mech_bank[MECH_PISTOL_SLIDE].pcm = mech_storage[MECH_PISTOL_SLIDE];
    mech_bank[MECH_PISTOL_SLIDE].frames = frames;
    mech_bank[MECH_PISTOL_SLIDE].nominal_level_q15 = 32767U;

    frames = 0U;
    if (!show_render_chuecka_foley(CH89_PRESET_MACHINE_GUN_FEED_BURST, 1U,
                                   GWF89_SMG_SELECTOR, 4101U,
                                   mech_storage[MECH_MACHINE_FEED],
                                   SHOW_MAX_MECH_FRAMES, &frames,
                                   28500, 9000, 22000)) return 0;
    mech_bank[MECH_MACHINE_FEED].pcm = mech_storage[MECH_MACHINE_FEED];
    mech_bank[MECH_MACHINE_FEED].frames = frames;
    mech_bank[MECH_MACHINE_FEED].nominal_level_q15 = 32767U;
    return 1;
}

static void show_source_step_one(show_source *source)
{
    show_u32 fraction;
    show_u32 whole;
    if (source->frame_pos >= source->frames) return;
    whole = source->step_q16 >> 16;
    fraction = (show_u32)source->frac_q16
             + (source->step_q16 & 65535U);
    source->frame_pos += whole + (fraction >> 16);
    source->frac_q16 = (show_u16)(fraction & 65535U);
    if (source->frame_pos >= source->frames) {
        source->frame_pos = source->frames;
        source->frac_q16 = 0U;
    }
}

static show_s16 show_source_process(void *user)
{
    show_source *source;
    show_s32 sample;
    source = (show_source *)user;
    if (source->frame_pos >= source->frames) return 0;
    sample = source->pcm[source->frame_pos];
    if (source->lowpass_shift != 0U) {
        source->lowpass += (sample - source->lowpass) >> source->lowpass_shift;
        sample = source->lowpass;
    }
    sample = (sample * source->source_gain_q15) >> 15;
    show_source_step_one(source);
    return show_sat16(sample);
}

static int show_source_active(const void *user)
{
    const show_source *source;
    source = (const show_source *)user;
    return source->frame_pos < source->frames;
}

static void show_source_stop(void *user)
{
    show_source *source;
    source = (show_source *)user;
    source->frame_pos = source->frames;
    source->frac_q16 = 0U;
}

static void show_source_advance(void *user, show_u32 frames)
{
    show_source *source;
    source = (show_source *)user;
    while (frames > 0U && source->frame_pos < source->frames) {
        show_source_step_one(source);
        frames--;
    }
}

static void show_source_restart(void *user)
{
    show_source *source;
    source = (show_source *)user;
    source->frame_pos = 0U;
    source->frac_q16 = 0U;
    source->lowpass = 0;
}

static show_u16 show_source_level(const void *user)
{
    const show_source *source;
    show_u32 remaining;
    source = (const show_source *)user;
    if (source->frame_pos >= source->frames) return 0U;
    remaining = source->frames - source->frame_pos;
    if (remaining > SHOW_RATE / 3U) return 32767U;
    return (show_u16)((remaining * 32767U) / (SHOW_RATE / 3U));
}

static void show_clear_events(void)
{
    event_count = 0U;
    casing_event_count = 0U;
    fire_event_count = 0U;
}

static int show_add_event(show_u32 start_frame, const show_bank *bank,
                          gwv89_event_class event_class,
                          show_s16 gain_q15, show_s16 pan_q15,
                          show_u16 distance_q15, show_s16 priority_bias,
                          show_u32 pitch_q16, show_u32 instance_key,
                          show_u16 instance_limit)
{
    show_event *event;
    if (event_count >= SHOW_MAX_EVENTS || bank == 0 || bank->pcm == 0) return 0;
    event = &events[event_count++];
    memset(event, 0, sizeof(*event));
    event->start_frame = start_frame;
    event->source.pcm = bank->pcm;
    event->source.frames = bank->frames;
    event->source.frame_pos = 0U;
    event->source.frac_q16 = 0U;
    event->source.step_q16 = pitch_q16;
    event->source.lowpass = 0;
    if (distance_q15 > 24500U) event->source.lowpass_shift = 4U;
    else if (distance_q15 > 15000U) event->source.lowpass_shift = 3U;
    else if (distance_q15 > 7000U) event->source.lowpass_shift = 2U;
    else event->source.lowpass_shift = 0U;
    event->source.source_gain_q15 = 32767;
    event->event_class = event_class;
    event->gain_q15 = gain_q15;
    event->pan_q15 = pan_q15;
    event->distance_q15 = distance_q15;
    event->occlusion_q15 = 32767U;
    event->focus_q15 = 32767U;
    event->priority_bias = priority_bias;
    event->instance_key = instance_key;
    event->instance_limit = instance_limit;
    event->attack_ms = event_class == GWV89_EVENT_REPORT ? 0U : 1U;
    event->release_ms = event_class == GWV89_EVENT_REPORT ? 12U : 8U;
    event->started = 0U;
    return 1;
}

static int show_add_casing_event(show_u32 start_frame,
                                  gt89_shell_type shell,
                                  gt89_surface_type surface,
                                  show_u8 velocity,
                                  show_u8 spin,
                                  show_s16 pan_q15,
                                  show_u16 distance_q15,
                                  show_s16 gain_q15,
                                  show_u32 seed,
                                  show_u32 instance_key)
{
    show_casing_event *event;
    if (casing_event_count >= SHOW_MAX_CASING_EVENTS) return 0;
    event = &casing_events[casing_event_count++];
    gsse89_casing_defaults(&event->params);
    event->start_frame = start_frame;
    event->params.shell = shell;
    event->params.surface = surface;
    event->params.velocity = velocity;
    event->params.angular_velocity = spin;
    event->params.variation = (gt89_u8)(seed & 255U);
    event->params.pan_q15 = pan_q15;
    event->params.distance_q15 = distance_q15;
    event->params.gain_q15 = gain_q15;
    event->params.instance_key = instance_key;
    event->params.instance_limit = 24U;
    event->seed = seed;
    event->started = 0U;
    return 1;
}

static int show_add_fire_event(show_u32 start_frame,
                                gsse89_fire_role role,
                                show_s32 preset_id,
                                show_u32 duration_ms,
                                show_s16 pan_q15,
                                show_u16 distance_q15,
                                show_s16 gain_q15,
                                show_s16 priority_bias,
                                show_u32 seed,
                                show_u32 instance_key)
{
    show_fire_event *event;
    if (fire_event_count >= SHOW_MAX_FIRE_EVENTS) return 0;
    event = &fire_events[fire_event_count++];
    gsse89_fire_defaults(&event->params, role);
    event->start_frame = start_frame;
    event->params.preset_id = preset_id;
    if (preset_id == GFIRE89_PRESET_FLAMETHROWER) {
        event->params.intensity_q15 = 30500;
        event->params.airflow_q15 = 22000;
        event->params.crackle_q15 = 27000;
        event->params.size_q15 = 30000;
        event->params.pressure_q15 = 31500;
        event->params.drive_q15 = 22500;
        event->params.brightness_q15 = 15500;
        event->params.output_gain_q15 = 26000;
        event->params.release_ms = 150U;
    } else if (preset_id == GFIRE89_PRESET_BONFIRE) {
        event->params.intensity_q15 = 28500;
        event->params.airflow_q15 = 17500;
        event->params.crackle_q15 = 30000;
        event->params.size_q15 = 28500;
        event->params.pressure_q15 = 20500;
        event->params.drive_q15 = 12500;
        event->params.brightness_q15 = 18000;
    } else if (preset_id == GFIRE89_PRESET_DEBRIS) {
        event->params.intensity_q15 = 20500;
        event->params.airflow_q15 = 12000;
        event->params.crackle_q15 = 31000;
        event->params.size_q15 = 15000;
        event->params.pressure_q15 = 13500;
        event->params.drive_q15 = 8000;
        event->params.brightness_q15 = 20500;
    }
    event->params.duration_ms = duration_ms;
    event->params.pan_q15 = pan_q15;
    event->params.distance_q15 = distance_q15;
    event->params.gain_q15 = gain_q15;
    event->params.priority_bias = priority_bias;
    event->params.instance_key = instance_key;
    event->seed = seed;
    event->started = 0U;
    return 1;
}

static void show_sort_casing_events(void)
{
    show_u16 i;
    show_u16 j;
    show_casing_event key;
    for (i = 1U; i < casing_event_count; ++i) {
        key = casing_events[i];
        j = i;
        while (j > 0U && casing_events[j - 1U].start_frame > key.start_frame) {
            casing_events[j] = casing_events[j - 1U];
            --j;
        }
        casing_events[j] = key;
    }
}

static void show_sort_fire_events(void)
{
    show_u16 i;
    show_u16 j;
    show_fire_event key;
    for (i = 1U; i < fire_event_count; ++i) {
        key = fire_events[i];
        j = i;
        while (j > 0U && fire_events[j - 1U].start_frame > key.start_frame) {
            fire_events[j] = fire_events[j - 1U];
            --j;
        }
        fire_events[j] = key;
    }
}

static void show_sort_events(void)
{
    show_u16 i;
    show_u16 j;
    show_event key;
    for (i = 1U; i < event_count; ++i) {
        key = events[i];
        j = i;
        while (j > 0U && events[j - 1U].start_frame > key.start_frame) {
            events[j] = events[j - 1U];
            --j;
        }
        events[j] = key;
    }
}

static gv89_result show_start_event(gwv89_context *handler,
                                    show_event *event)
{
    gv89_provider_ex provider;
    gwv89_event_desc desc;
    gv89_handle handle;
    memset(&provider, 0, sizeof(provider));
    provider.base.user = &event->source;
    provider.base.process_mono = show_source_process;
    provider.base.is_active = show_source_active;
    provider.base.stop = show_source_stop;
    provider.advance_frames = show_source_advance;
    provider.restart = show_source_restart;
    provider.estimated_level_q15 = show_source_level;
    provider.physical_state_changed = 0;
    gwv89_event_default(&desc, event->event_class);
    desc.gain_q15 = event->gain_q15;
    desc.screen_x_q15 = event->pan_q15;
    desc.distance_q15 = event->distance_q15;
    desc.occlusion_q15 = event->occlusion_q15;
    desc.focus_q15 = event->focus_q15;
    desc.priority_bias = event->priority_bias;
    desc.instance_key = event->instance_key;
    desc.instance_limit = event->instance_limit;
    desc.attack_ms = event->attack_ms;
    desc.release_ms = event->release_ms;
    desc.allow_higher_priority_steal = 1U;
    desc.allow_protected_steal = event->event_class == GWV89_EVENT_REPORT ? 1U : 0U;
    return gwv89_play_ex(handler, &desc, &provider, &handle);
}

static int show_render_scene(const char *path, show_u32 frames,
                             show_s32 output_gain_q15,
                             gv89_stats *out_stats)
{
    gwv89_context handler;
    gsse89_context extras;
    show_u32 frame;
    show_u16 next_event;
    show_u16 next_casing;
    show_u16 next_fire;
    show_s16 left;
    show_s16 right;
    show_s16 raw_left;
    show_s16 raw_right;
    show_s32 dc_left;
    show_s32 dc_right;
    show_s32 prev_in_left;
    show_s32 prev_in_right;
    show_s32 prev_out_left;
    show_s32 prev_out_right;
    gv89_result result;
    if (frames > SHOW_MAX_SCENE_FRAMES) return 0;
    if (gwv89_init_ex(&handler, voice_storage, SHOW_LOGICAL,
                      SHOW_PHYSICAL, SHOW_RATE) != GV89_OK) return 0;
    if (!gsse89_init(&extras, casing_voice_storage, SHOW_CASING_POOL,
                     fire_voice_storage, SHOW_FIRE_POOL, SHOW_RATE)) return 0;
    gv89_set_master(&handler.voices, 28500, 30000, 5U);
    gv89_set_virtualization(&handler.voices, 1500U, 2200U, 128U);
    show_sort_events();
    show_sort_casing_events();
    show_sort_fire_events();
    next_event = 0U;
    next_casing = 0U;
    next_fire = 0U;
    prev_in_left = 0;
    prev_in_right = 0;
    prev_out_left = 0;
    prev_out_right = 0;
    for (frame = 0U; frame < frames; ++frame) {
        if ((next_event < event_count && events[next_event].start_frame == frame) ||
            (next_casing < casing_event_count && casing_events[next_casing].start_frame == frame) ||
            (next_fire < fire_event_count && fire_events[next_fire].start_frame == frame)) {
            gwv89_begin_batch(&handler);
            while (next_event < event_count &&
                   events[next_event].start_frame == frame) {
                result = show_start_event(&handler, &events[next_event]);
                if (result == GV89_OK) events[next_event].started = 1U;
                ++next_event;
            }
            while (next_casing < casing_event_count &&
                   casing_events[next_casing].start_frame == frame) {
                result = gsse89_play_casing(&extras, &handler,
                         &casing_events[next_casing].params,
                         casing_events[next_casing].seed, 0);
                if (result == GV89_OK) casing_events[next_casing].started = 1U;
                ++next_casing;
            }
            while (next_fire < fire_event_count &&
                   fire_events[next_fire].start_frame == frame) {
                result = gsse89_play_fire(&extras, &handler,
                         &fire_events[next_fire].params,
                         fire_events[next_fire].seed, 0);
                if (result == GV89_OK) fire_events[next_fire].started = 1U;
                ++next_fire;
            }
            result = gwv89_end_batch(&handler);
            if (result != GV89_OK) return 0;
        }
        left = 0;
        right = 0;
        gwv89_process_stereo_sample(&handler, &left, &right);
        raw_left = left;
        raw_right = right;
        dc_left = (show_s32)raw_left - prev_in_left +
                  ((prev_out_left * 32604) >> 15);
        dc_right = (show_s32)raw_right - prev_in_right +
                   ((prev_out_right * 32604) >> 15);
        prev_in_left = raw_left;
        prev_in_right = raw_right;
        prev_out_left = dc_left;
        prev_out_right = dc_right;
        dc_left = (dc_left * output_gain_q15) >> 15;
        dc_right = (dc_right * output_gain_q15) >> 15;
        scene_output[frame * 2U] = show_sat16(dc_left);
        scene_output[frame * 2U + 1U] = show_sat16(dc_right);
    }
    if (out_stats != 0) gwv89_get_stats(&handler, out_stats);
    return show_write_wav(path, scene_output, frames);
}

static show_u32 show_pitch(show_s32 permille)
{
    show_s32 value;
    value = 65536 + (65536 * permille) / 1000;
    if (value < 56000) value = 56000;
    if (value > 76000) value = 76000;
    return (show_u32)value;
}

static int show_scene_shotgun(gv89_stats *stats)
{
    show_u32 t;
    show_clear_events();
    show_add_event(show_ms(250U), &mech_bank[MECH_SHOTGUN_INSERT],
                   GWV89_EVENT_MECHANISM, 24500, -1800, 1000U, 25,
                   show_pitch(-10), 1001U, 2U);
    show_add_event(show_ms(690U), &mech_bank[MECH_SHOTGUN_INSERT],
                   GWV89_EVENT_MECHANISM, 24000, 1200, 1200U, 20,
                   show_pitch(8), 1001U, 2U);
    show_add_event(show_ms(1120U), &mech_bank[MECH_SHOTGUN_INSERT],
                   GWV89_EVENT_MECHANISM, 23800, -800, 1400U, 20,
                   show_pitch(-4), 1001U, 2U);
    show_add_event(show_ms(1560U), &mech_bank[MECH_SHOTGUN_PUMP],
                   GWV89_EVENT_MECHANISM, 28600, 0, 500U, 80,
                   show_pitch(0), 2001U, 1U);
    t = 2260U;
    show_add_event(show_ms(t), &shot_bank[WEAPON_SHOTGUN][0],
                   GWV89_EVENT_REPORT, 30200, -1200, 800U, 170,
                   show_pitch(-7), 3001U, 4U);
    show_add_event(show_ms(t + 910U), &mech_bank[MECH_SHOTGUN_PUMP],
                   GWV89_EVENT_MECHANISM, 28400, -500, 700U, 90,
                   show_pitch(5), 2001U, 1U);
    show_add_event(show_ms(t + 1600U), &shot_bank[WEAPON_SHOTGUN][1],
                   GWV89_EVENT_REPORT, 30000, 900, 1000U, 170,
                   show_pitch(8), 3001U, 4U);
    show_add_event(show_ms(t + 2520U), &mech_bank[MECH_SHOTGUN_PUMP],
                   GWV89_EVENT_MECHANISM, 28200, 500, 900U, 90,
                   show_pitch(-3), 2001U, 1U);
    show_add_event(show_ms(t + 3220U), &shot_bank[WEAPON_SHOTGUN][2],
                   GWV89_EVENT_REPORT, 29800, -700, 1200U, 170,
                   show_pitch(-2), 3001U, 4U);
    show_add_event(show_ms(t + 4140U), &mech_bank[MECH_SHOTGUN_PUMP],
                   GWV89_EVENT_MECHANISM, 28000, 0, 1000U, 90,
                   show_pitch(4), 2001U, 1U);
    show_add_casing_event(show_ms(t + 1060U), GT89_SHELL_SHOTGUN_PLASTIC,
                          GT89_SURFACE_CONCRETE, 214U, 224U, -1800, 1600U,
                          25000, 8101U, 8100U);
    show_add_casing_event(show_ms(t + 2670U), GT89_SHELL_SHOTGUN_PLASTIC,
                          GT89_SURFACE_CONCRETE, 205U, 210U, 1400, 1800U,
                          24600, 8102U, 8100U);
    show_add_casing_event(show_ms(t + 4290U), GT89_SHELL_SHOTGUN_PLASTIC,
                          GT89_SURFACE_CONCRETE, 198U, 235U, -500, 1900U,
                          24200, 8103U, 8100U);
    return show_render_scene("audio/01_shotgun_sequence_plus_gtinkle.wav",
                             show_ms(8200U), 44000, stats);
}

static int show_add_machine_burst(show_u32 start_ms, show_u16 shots,
                                  show_u16 interval_ms, show_s16 pan_start,
                                  show_s16 pan_step, show_u16 distance,
                                  show_u16 variant_base, show_u32 key)
{
    show_u16 i;
    show_s16 pan;
    show_s32 pitch_delta;
    pan = pan_start;
    for (i = 0U; i < shots; ++i) {
        pitch_delta = (show_s32)((i * 17U + variant_base * 11U) % 43U) - 21;
        if (!show_add_event(show_ms(start_ms + (show_u32)i * interval_ms),
                            &shot_bank[WEAPON_MACHINE][(i + variant_base) & 3U],
                            GWV89_EVENT_REPORT,
                            (show_s16)(26600 - (i & 3U) * 420), pan,
                            (show_u16)(distance + (i & 3U) * 320U), 135,
                            show_pitch(pitch_delta), key, 40U)) return 0;
        if (!show_add_casing_event(
                show_ms(start_ms + (show_u32)i * interval_ms + 22U),
                GT89_SHELL_RIFLE_BRASS, GT89_SURFACE_CONCRETE,
                (show_u8)(188U + (i & 15U)),
                (show_u8)(210U + (i & 31U)),
                (show_s16)(pan + 1200),
                (show_u16)(distance + 650U),
                (show_s16)(18800 - (i & 3U) * 500),
                9000U + key + i, key)) return 0;
        pan = (show_s16)(pan + pan_step);
    }
    return 1;
}

static int show_scene_machine(gv89_stats *stats)
{
    show_clear_events();
    show_add_event(show_ms(250U), &mech_bank[MECH_MACHINE_FEED],
                   GWV89_EVENT_MECHANISM, 22000, -9000, 2000U, 30,
                   show_pitch(-8), 4100U, 2U);
    if (!show_add_machine_burst(1050U, 5U, 92U, -12500, 1150,
                                1200U, 0U, 4200U)) return 0;
    show_add_event(show_ms(2050U), &mech_bank[MECH_MACHINE_FEED],
                   GWV89_EVENT_MECHANISM, 20500, -4500, 3500U, 25,
                   show_pitch(10), 4100U, 2U);
    if (!show_add_machine_burst(2670U, 11U, 74U, -6500, 950,
                                2200U, 1U, 4200U)) return 0;
    if (!show_add_machine_burst(4550U, 24U, 55U, 10500, -720,
                                3600U, 2U, 4200U)) return 0;
    show_add_event(show_ms(6300U), &mech_bank[MECH_MACHINE_FEED],
                   GWV89_EVENT_MECHANISM, 21000, 5000, 5000U, 35,
                   show_pitch(-4), 4100U, 2U);
    if (!show_add_machine_burst(6800U, 8U, 68U, 3500, -600,
                                5200U, 3U, 4200U)) return 0;
    return show_render_scene("audio/02_machinegun_plus_gtinkle.wav",
                             show_ms(9200U), 90000, stats);
}

static int show_scene_pistols(gv89_stats *stats)
{
    static const show_u16 left_times[] = {900U, 1370U, 1910U, 2790U, 3220U,
                                          3660U, 5280U, 5710U, 6190U};
    static const show_u16 right_times[] = {1120U, 1580U, 2310U, 3010U, 3490U,
                                           4050U, 5480U, 5980U, 6500U};
    show_u16 i;
    show_s32 pitch_delta;
    show_clear_events();
    show_add_event(show_ms(250U), &mech_bank[MECH_PISTOL_SLIDE],
                   GWV89_EVENT_MECHANISM, 23000, -15500, 1200U, 35,
                   show_pitch(-10), 5001U, 2U);
    show_add_event(show_ms(480U), &mech_bank[MECH_PISTOL_SLIDE],
                   GWV89_EVENT_MECHANISM, 22500, 15000, 1600U, 35,
                   show_pitch(12), 5002U, 2U);
    for (i = 0U; i < (show_u16)(sizeof(left_times) / sizeof(left_times[0])); ++i) {
        pitch_delta = (show_s32)((i * 13U) % 29U) - 14;
        show_add_event(show_ms(left_times[i]),
                       &shot_bank[WEAPON_PISTOL][i & 3U],
                       GWV89_EVENT_REPORT, 28200, -14500,
                       (show_u16)(800U + i * 180U), 125,
                       show_pitch(pitch_delta), 5101U, 8U);
        show_add_casing_event(show_ms((show_u32)left_times[i] + 34U),
                              GT89_SHELL_PISTOL_BRASS, GT89_SURFACE_TILE,
                              (show_u8)(172U + (i & 15U)),
                              (show_u8)(190U + (i * 7U) % 55U),
                              -13200, (show_u16)(1300U + i * 210U),
                              20500, 10000U + i, 5101U);
    }
    for (i = 0U; i < (show_u16)(sizeof(right_times) / sizeof(right_times[0])); ++i) {
        pitch_delta = (show_s32)((i * 19U) % 31U) - 15;
        show_add_event(show_ms(right_times[i]),
                       &shot_bank[WEAPON_PISTOL][(i + 2U) & 3U],
                       GWV89_EVENT_REPORT, 27900, 14500,
                       (show_u16)(1000U + i * 190U), 125,
                       show_pitch(pitch_delta), 5102U, 8U);
        show_add_casing_event(show_ms((show_u32)right_times[i] + 36U),
                              GT89_SHELL_PISTOL_BRASS, GT89_SURFACE_TILE,
                              (show_u8)(168U + (i & 19U)),
                              (show_u8)(185U + (i * 9U) % 60U),
                              13200, (show_u16)(1500U + i * 220U),
                              20200, 10100U + i, 5102U);
    }
    show_add_event(show_ms(4420U), &mech_bank[MECH_PISTOL_SLIDE],
                   GWV89_EVENT_MECHANISM, 22000, -14500, 2400U, 40,
                   show_pitch(4), 5001U, 2U);
    show_add_event(show_ms(4670U), &mech_bank[MECH_PISTOL_SLIDE],
                   GWV89_EVENT_MECHANISM, 21500, 14500, 3000U, 40,
                   show_pitch(-6), 5002U, 2U);
    return show_render_scene("audio/03_dual_pistols_plus_gtinkle.wav",
                             show_ms(8500U), 93000, stats);
}

static int show_scene_firefight(gv89_stats *stats)
{
    show_u32 t;
    show_u16 burst;
    show_u16 i;
    show_s16 pan;
    show_u16 distance;
    show_s32 pitch_delta;
    show_clear_events();

    show_add_fire_event(show_ms(0U), GSSE89_FIRE_ROLE_AMBIENCE,
                        GFIRE89_PRESET_DEBRIS, 14800U, -23000, 14500U,
                        10500, -45, 12001U, 12001U);
    show_add_fire_event(show_ms(0U), GSSE89_FIRE_ROLE_AMBIENCE,
                        GFIRE89_PRESET_BONFIRE, 14800U, 23500, 17500U,
                        9500, -55, 12002U, 12002U);
    show_add_fire_event(show_ms(7350U), GSSE89_FIRE_ROLE_FLAMETHROWER,
                        GFIRE89_PRESET_FLAMETHROWER, 1900U, -9000, 4200U,
                        23000, 140, 12100U, 12100U);
    /* Stagger the second burner so the broadband components do not stack
       into a television-like hiss wall. */
    show_add_fire_event(show_ms(9400U), GSSE89_FIRE_ROLE_FLAMETHROWER,
                        GFIRE89_PRESET_FLAMETHROWER, 1250U, 11000, 6500U,
                        18800, 115, 12101U, 12101U);

    show_add_event(show_ms(180U), &mech_bank[MECH_MACHINE_FEED],
                   GWV89_EVENT_MECHANISM, 19000, -18000, 9000U, 15,
                   show_pitch(-5), 6001U, 2U);
    show_add_event(show_ms(320U), &mech_bank[MECH_SHOTGUN_PUMP],
                   GWV89_EVENT_MECHANISM, 22500, 17000, 6500U, 50,
                   show_pitch(4), 6002U, 1U);
    show_add_event(show_ms(520U), &mech_bank[MECH_PISTOL_SLIDE],
                   GWV89_EVENT_MECHANISM, 18500, 0, 10500U, 20,
                   show_pitch(-8), 6003U, 2U);

    t = 900U;
    for (burst = 0U; burst < 8U; ++burst) {
        pan = (burst & 1U) ? 17500 : -18500;
        distance = (show_u16)(6500U + burst * 900U);
        for (i = 0U; i < (show_u16)(7U + (burst & 3U)); ++i) {
            pitch_delta = (show_s32)((burst * 23U + i * 11U) % 39U) - 19;
            show_add_event(show_ms(t + (show_u32)i * (58U + (burst & 1U) * 10U)),
                           &shot_bank[WEAPON_MACHINE][(burst + i) & 3U],
                           GWV89_EVENT_REPORT,
                           (show_s16)(22000 - (burst & 3U) * 500),
                           (show_s16)(pan + (show_s16)i *
                                      ((burst & 1U) ? -430 : 390)),
                           (show_u16)(distance + i * 180U), 110,
                           show_pitch(pitch_delta), 6100U + burst, 20U);
            show_add_casing_event(
                show_ms(t + (show_u32)i * (58U + (burst & 1U) * 10U) + 20U),
                GT89_SHELL_RIFLE_BRASS, GT89_SURFACE_CONCRETE,
                (show_u8)(180U + (i & 31U)), (show_u8)(205U + (i & 31U)),
                (show_s16)(pan + (show_s16)i * ((burst & 1U) ? -430 : 390) + 900),
                (show_u16)(distance + i * 180U + 500U), 15000,
                13000U + burst * 32U + i, 6100U + burst);
        }
        t += 1280U + (show_u32)(burst & 1U) * 260U;
    }

    for (i = 0U; i < 16U; ++i) {
        t = 1250U + (show_u32)i * 710U + (show_u32)(i & 3U) * 90U;
        pan = (show_s16)(-13000 + (show_s16)((i * 5700U) % 26000U));
        distance = (show_u16)(3000U + (i * 1300U) % 15000U);
        pitch_delta = (show_s32)((i * 17U) % 35U) - 17;
        show_add_event(show_ms(t), &shot_bank[WEAPON_PISTOL][i & 3U],
                       GWV89_EVENT_REPORT, 22800, pan, distance, 120,
                       show_pitch(pitch_delta), 6200U + (i & 3U), 10U);
        show_add_casing_event(show_ms(t + 32U), GT89_SHELL_PISTOL_BRASS,
                              GT89_SURFACE_CONCRETE, 170U,
                              (show_u8)(190U + (i & 31U)),
                              (show_s16)(pan + 1000),
                              (show_u16)(distance + 450U), 15400,
                              14000U + i, 6200U + (i & 3U));
    }

    for (i = 0U; i < 6U; ++i) {
        t = 2100U + (show_u32)i * 1900U;
        pan = (i & 1U) ? 21000 : -21000;
        distance = (show_u16)(2500U + i * 1700U);
        show_add_event(show_ms(t), &shot_bank[WEAPON_SHOTGUN][i & 3U],
                       GWV89_EVENT_REPORT, 28400, pan, distance, 175,
                       show_pitch((show_s32)i * 3 - 8), 6300U + i, 3U);
        show_add_event(show_ms(t + 780U), &mech_bank[MECH_SHOTGUN_PUMP],
                       GWV89_EVENT_MECHANISM, 22500, pan,
                       (show_u16)(distance + 700U), 70,
                       show_pitch((show_s32)i * 2 - 5), 6400U + i, 1U);
        show_add_casing_event(show_ms(t + 930U), GT89_SHELL_SHOTGUN_PLASTIC,
                              GT89_SURFACE_CONCRETE, 198U,
                              (show_u8)(220U + (i & 31U)), pan,
                              (show_u16)(distance + 900U), 19500,
                              15000U + i, 6300U + i);
    }

    for (i = 0U; i < 180U; ++i) {
        t = 3600U + (show_u32)(i / 30U) * 1050U +
            (show_u32)(i % 30U) * 18U;
        pan = (show_s16)(-30000 + (show_s16)((i * 1900U) % 60000U));
        distance = (show_u16)(12000U + (i * 337U) % 19000U);
        pitch_delta = (show_s32)((i * 29U) % 51U) - 25;
        show_add_event(show_ms(t), &shot_bank[WEAPON_MACHINE][i & 3U],
                       GWV89_EVENT_REPORT, 15000, pan, distance,
                       (show_s16)(55 + (i & 7U)), show_pitch(pitch_delta),
                       6500U + (i & 15U), 36U);
        if ((i % 3U) == 0U) {
            show_add_casing_event(show_ms(t + 20U), GT89_SHELL_RIFLE_BRASS,
                                  GT89_SURFACE_CONCRETE, 165U,
                                  (show_u8)(190U + (i & 63U)), pan,
                                  (show_u16)(distance + 700U), 9800,
                                  16000U + i, 6500U + (i & 15U));
        }
    }

    /* Rocket launcher is deliberately split into two real game events:
       gpaah89 ignition/report at the muzzle, then wsound_rocketblast89 at
       the remote impact point after a clearly audible flight interval. */
    show_add_event(show_ms(11750U), &shot_bank[WEAPON_ROCKET][0],
                   GWV89_EVENT_REPORT, 28600, -15500, 3600U, 240,
                   show_pitch(-8), 6800U, 2U);
    show_add_event(show_ms(13350U), &rocket_blast_bank,
                   GWV89_EVENT_EXPLOSION, 30400, 12500, 7400U, 310,
                   show_pitch(0), 6801U, 2U);

    /* Final distant crossfire wall: deliberately exceeds the 64-voice DSP
       budget so virtualization and deterministic report stealing are audible
       under a musically controlled, low-gain background layer. */
    for (i = 0U; i < 150U; ++i) {
        t = 11150U + (show_u32)(i % 75U) * 8U +
            (show_u32)(i / 75U) * 35U;
        pan = (show_s16)(-31000 + (show_s16)((i * 2701U) % 62000U));
        distance = (show_u16)(17500U + (i * 503U) % 14500U);
        pitch_delta = (show_s32)((i * 31U) % 61U) - 30;
        show_add_event(show_ms(t), &shot_bank[WEAPON_MACHINE][(i + 1U) & 3U],
                       GWV89_EVENT_REPORT,
                       (show_s16)(12200 + (i & 3U) * 550),
                       pan, distance, (show_s16)(35 + (i & 3U)),
                       show_pitch(pitch_delta), 6700U + (i & 31U), 48U);
    }

    return show_render_scene("audio/04_full_firefight_plus_gtinkle_gfire_rocket.wav",
                             show_ms(19000U), 43000, stats);
}

static int show_scene_rocket_pair(gv89_stats *stats)
{
    show_clear_events();
    show_add_event(show_ms(650U), &shot_bank[WEAPON_ROCKET][1],
                   GWV89_EVENT_REPORT, 30000, -9000, 1700U, 260,
                   show_pitch(0), 6900U, 1U);
    show_add_event(show_ms(2250U), &rocket_blast_bank,
                   GWV89_EVENT_EXPLOSION, 31200, 10500, 5200U, 320,
                   show_pitch(0), 6901U, 1U);
    return show_render_scene("audio/06_rocket_launcher_launch_to_impact.wav",
                             show_ms(7800U), 40000, stats);
}

static int show_scene_extras(gv89_stats *stats)
{
    show_u16 i;
    show_clear_events();
    show_add_casing_event(show_ms(300U), GT89_SHELL_RIMFIRE_BRASS,
                          GT89_SURFACE_TILE, 145U, 180U, -22000, 900U,
                          25500, 20001U, 20001U);
    show_add_casing_event(show_ms(720U), GT89_SHELL_PISTOL_BRASS,
                          GT89_SURFACE_CONCRETE, 178U, 212U, -10000, 1200U,
                          25500, 20002U, 20002U);
    show_add_casing_event(show_ms(1180U), GT89_SHELL_MAGNUM_BRASS,
                          GT89_SURFACE_METAL, 205U, 235U, 2000, 1500U,
                          25000, 20003U, 20003U);
    show_add_casing_event(show_ms(1660U), GT89_SHELL_RIFLE_BRASS,
                          GT89_SURFACE_CONCRETE, 220U, 245U, 12000, 1900U,
                          24800, 20004U, 20004U);
    show_add_casing_event(show_ms(2160U), GT89_SHELL_SHOTGUN_PLASTIC,
                          GT89_SURFACE_WOOD, 190U, 225U, 22000, 2200U,
                          25000, 20005U, 20005U);

    show_add_fire_event(show_ms(2800U), GSSE89_FIRE_ROLE_AMBIENCE,
                        GFIRE89_PRESET_TORCH, 1600U, -18000, 1800U,
                        20500, 0, 21001U, 21001U);
    show_add_fire_event(show_ms(4700U), GSSE89_FIRE_ROLE_AMBIENCE,
                        GFIRE89_PRESET_CAMPFIRE, 2400U, -6500, 3200U,
                        22000, 0, 21002U, 21002U);
    show_add_fire_event(show_ms(7350U), GSSE89_FIRE_ROLE_AMBIENCE,
                        GFIRE89_PRESET_BONFIRE, 2600U, 9500, 4700U,
                        23500, 10, 21003U, 21003U);
    show_add_fire_event(show_ms(10100U), GSSE89_FIRE_ROLE_FLAMETHROWER,
                        GFIRE89_PRESET_FLAMETHROWER, 1900U, 0, 1800U,
                        24500, 160, 21004U, 21004U);
    for (i = 0U; i < 12U; ++i) {
        show_add_event(show_ms(10400U + (show_u32)i * 95U),
                       &shot_bank[WEAPON_MACHINE][i & 3U],
                       GWV89_EVENT_REPORT, 15000,
                       (show_s16)(-15000 + (show_s16)i * 2700),
                       (show_u16)(4500U + i * 280U), 80,
                       show_pitch((show_s32)(i & 7U) - 4),
                       22000U, 20U);
        show_add_casing_event(show_ms(10425U + (show_u32)i * 95U),
                              GT89_SHELL_RIFLE_BRASS, GT89_SURFACE_METAL,
                              (show_u8)(180U + i), (show_u8)(210U + i),
                              (show_s16)(-13500 + (show_s16)i * 2500),
                              (show_u16)(4800U + i * 300U), 14500,
                              23000U + i, 22000U);
    }
    return show_render_scene("audio/05_gtinkle_gfire_integration_showcase.wav",
                             show_ms(13800U), 65000, stats);
}

static void show_print_stats(const char *name, const gv89_stats *stats)
{
    printf("%s: starts=%lu steals=%lu rejects=%lu peak_logical=%lu peak_physical=%lu virtual_peak_est=%lu limiter_hits=%lu\n",
           name,
           (unsigned long)stats->starts,
           (unsigned long)stats->steals,
           (unsigned long)stats->rejects,
           (unsigned long)stats->peak_logical,
           (unsigned long)stats->peak_physical,
           (unsigned long)(stats->peak_logical > stats->peak_physical ?
                           stats->peak_logical - stats->peak_physical : 0U),
           (unsigned long)stats->limiter_hits);
}

int main(void)
{
    gv89_stats shotgun_stats;
    gv89_stats machine_stats;
    gv89_stats pistol_stats;
    gv89_stats firefight_stats;
    gv89_stats rocket_pair_stats;
    gv89_stats extras_stats;
    if (!show_build_banks()) {
        fprintf(stderr, "failed to build procedural banks\n");
        return 1;
    }
    if (!show_scene_shotgun(&shotgun_stats)) return 2;
    if (!show_scene_machine(&machine_stats)) return 3;
    if (!show_scene_pistols(&pistol_stats)) return 4;
    if (!show_scene_firefight(&firefight_stats)) return 5;
    if (!show_scene_rocket_pair(&rocket_pair_stats)) return 6;
    if (!show_scene_extras(&extras_stats)) return 7;
    show_print_stats("shotgun", &shotgun_stats);
    show_print_stats("machinegun", &machine_stats);
    show_print_stats("pistols", &pistol_stats);
    show_print_stats("firefight+rocket", &firefight_stats);
    show_print_stats("rocket launch+impact", &rocket_pair_stats);
    show_print_stats("gtinkle+gfire", &extras_stats);
    printf("banks: pistol=%lu machine=%lu shotgun=%lu rocket_ignition=%lu rocket_blast=%lu pump=%lu insert=%lu slide=%lu feed=%lu\n",
           (unsigned long)shot_bank[WEAPON_PISTOL][0].frames,
           (unsigned long)shot_bank[WEAPON_MACHINE][0].frames,
           (unsigned long)shot_bank[WEAPON_SHOTGUN][0].frames,
           (unsigned long)shot_bank[WEAPON_ROCKET][0].frames,
           (unsigned long)rocket_blast_bank.frames,
           (unsigned long)mech_bank[MECH_SHOTGUN_PUMP].frames,
           (unsigned long)mech_bank[MECH_SHOTGUN_INSERT].frames,
           (unsigned long)mech_bank[MECH_PISTOL_SLIDE].frames,
           (unsigned long)mech_bank[MECH_MACHINE_FEED].frames);
    return 0;
}
