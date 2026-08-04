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
#include "wsound_ggrenadeblast89.h"
#include "wsoundammo89.h"
#include "wsoundbeltfeed89.h"
#include "wsoundreceiver89.h"
#include "wmagazine89.h"
#include "grocketwhistle89.h"

#define SHOW_RATE 44100U
#define SHOW_LOGICAL 256U
#define SHOW_PHYSICAL 64U
#define SHOW_VARIANTS 4U
#define SHOW_WEAPONS 7U
#define SHOW_MAX_SHOT_FRAMES 132300U
#define SHOW_MECH_COUNT 17U
#define SHOW_MAX_MECH_FRAMES 110250U
#define SHOW_MAX_EVENTS 768U
#define SHOW_MAX_CASING_EVENTS 640U
#define SHOW_MAX_FIRE_EVENTS 24U
#define SHOW_MAX_ROCKET_BLAST_FRAMES 220500U
#define SHOW_MAX_GRENADE_BLAST_FRAMES 220500U
#define SHOW_MAX_ROCKET_WHISTLE_FRAMES 110250U
#define SHOW_CASING_POOL 128U
#define SHOW_FIRE_POOL 12U
#define SHOW_MAX_SCENE_FRAMES 6200000U
#define SHOW_Q15 32767

#define WEAPON_PISTOL 0U
#define WEAPON_MACHINE 1U
#define WEAPON_SHOTGUN 2U
#define WEAPON_ROCKET 3U
#define WEAPON_MAGNUM 4U
#define WEAPON_SNIPER 5U
#define WEAPON_GRENADE_LAUNCHER 6U

#define MECH_SHOTGUN_PUMP 0U
#define MECH_SHOTGUN_INSERT 1U
#define MECH_PISTOL_SLIDE 2U
#define MECH_MACHINE_FEED 3U
#define MECH_MAGNUM_ACTION 4U
#define MECH_SNIPER_BOLT 5U
#define MECH_GRENADE_LAUNCHER 6U
#define MECH_ROCKET_LAUNCHER 7U
#define MECH_GRENADE_PIN 8U
#define MECH_PISTOL_MAG 9U
#define MECH_MAGNUM_RELOAD 10U
#define MECH_SHOTGUN_MULTI_INSERT 11U
#define MECH_MACHINE_BELT 12U
#define MECH_SNIPER_MAG 13U
#define MECH_ROCKET_RELOAD 14U
#define MECH_PISTOL_AUTO 15U
#define MECH_MACHINE_CYCLE 16U

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
static show_s16 grenade_blast_storage[SHOW_MAX_GRENADE_BLAST_FRAMES];
static show_s16 rocket_whistle_storage[SHOW_MAX_ROCKET_WHISTLE_FRAMES];
static show_bank shot_bank[SHOW_WEAPONS][SHOW_VARIANTS];
static show_bank mech_bank[SHOW_MECH_COUNT];
static show_bank rocket_blast_bank;
static show_bank grenade_blast_bank;
static show_bank rocket_whistle_bank;
static show_s16 scratch_a[SHOW_MAX_MECH_FRAMES];
static show_s16 scratch_b[SHOW_MAX_MECH_FRAMES];
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
    return (milliseconds / 1000U) * SHOW_RATE
         + ((milliseconds % 1000U) * SHOW_RATE) / 1000U;
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
    wsoundreceiver89_context receiver;
    wsoundreceiver89_preset receiver_id;
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
    show_s16 receiver_sample;
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
        target = 27800;
    } else if (weapon == WEAPON_MACHINE) {
        report_id = GPAAH89_PRESET_METRALLA;
        body_id = GWB89_PRESET_RIFLE;
        gas_id = GMG89_PRESET_RIFLE;
        crack_id = GBC89_PRESET_RIFLE_PASS;
        tail_id = GLT89_PRESET_CORRIDOR;
        thump_id = GCT89_PRESET_ACTION;
        frames = show_ms(1100U);
        target = 25800;
    } else if (weapon == WEAPON_SHOTGUN) {
        report_id = GPAAH89_PRESET_SHOTGUN;
        body_id = GWB89_PRESET_SHOTGUN;
        gas_id = GMG89_PRESET_SHOTGUN;
        crack_id = GBC89_PRESET_NEAR;
        tail_id = GLT89_PRESET_WAREHOUSE;
        thump_id = GCT89_PRESET_SHOTGUN;
        frames = show_ms(2100U);
        target = 31800;
    } else if (weapon == WEAPON_MAGNUM) {
        report_id = GPAAH89_PRESET_MAGNUM;
        body_id = GWB89_PRESET_MAGNUM;
        gas_id = GMG89_PRESET_MAGNUM;
        crack_id = GBC89_PRESET_NEAR;
        tail_id = GLT89_PRESET_SMALL_ROOM;
        thump_id = GCT89_PRESET_ACTION;
        frames = show_ms(1650U);
        target = 30800;
    } else if (weapon == WEAPON_SNIPER) {
        report_id = GPAAH89_PRESET_SNIPER_RIFLE;
        body_id = GWB89_PRESET_SNIPER;
        gas_id = GMG89_PRESET_SNIPER;
        crack_id = GBC89_PRESET_SNIPER_PASS;
        tail_id = GLT89_PRESET_EXTERIOR;
        thump_id = GCT89_PRESET_SNIPER;
        frames = show_ms(2600U);
        target = 32000;
    } else if (weapon == WEAPON_GRENADE_LAUNCHER) {
        report_id = GPAAH89_PRESET_ROCKET_LAUNCHER;
        body_id = GWB89_PRESET_LAUNCHER;
        gas_id = GMG89_PRESET_LAUNCHER;
        crack_id = GBC89_PRESET_FAR;
        tail_id = GLT89_PRESET_SMALL_ROOM;
        thump_id = GCT89_PRESET_LAUNCHER;
        frames = show_ms(1700U);
        target = 29200;
    } else {
        report_id = GPAAH89_PRESET_ROCKET_LAUNCHER;
        body_id = GWB89_PRESET_LAUNCHER;
        gas_id = GMG89_PRESET_LAUNCHER;
        crack_id = GBC89_PRESET_FAR;
        tail_id = GLT89_PRESET_EXTERIOR;
        thump_id = GCT89_PRESET_LAUNCHER;
        frames = show_ms(2600U);
        target = 31000;
    }
    if (weapon == WEAPON_PISTOL) {
        receiver_id = WSOUNDRECEIVER89_PISTOL_STEEL;
    } else if (weapon == WEAPON_SHOTGUN) {
        receiver_id = WSOUNDRECEIVER89_SHOTGUN_WOOD;
    } else if (weapon == WEAPON_ROCKET ||
               weapon == WEAPON_GRENADE_LAUNCHER ||
               weapon == WEAPON_MAGNUM) {
        receiver_id = WSOUNDRECEIVER89_HEAVY;
    } else {
        receiver_id = WSOUNDRECEIVER89_RIFLE_STEEL;
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
    if (wsoundreceiver89_init(&receiver, receiver_id) != WSOUND89_OK) return 0;

    gpaah89_trigger(&report, seed + 101U);
    wsoundreceiver89_excite(&receiver, weapon == WEAPON_PISTOL ? 18000 : 24500);
    gwb89_trigger(&body, 29000, seed + 103U);
    gmg89_trigger(&gas, 28500, seed + 107U);
    if (weapon == WEAPON_SHOTGUN) {
        gbc89_trigger(&crack, 15U, 13000, seed + 109U);
    } else if (weapon == WEAPON_ROCKET || weapon == WEAPON_GRENADE_LAUNCHER) {
        gbc89_trigger(&crack, 4U, 9000, seed + 109U);
    } else if (weapon == WEAPON_SNIPER) {
        gbc89_trigger(&crack, 22U, 31500, seed + 109U);
    } else if (weapon == WEAPON_MAGNUM) {
        gbc89_trigger(&crack, 10U, 27800, seed + 109U);
    } else {
        gbc89_trigger(&crack, 8U, 24500, seed + 109U);
    }
    if (weapon == WEAPON_PISTOL) {
        gct89_trigger(&thump, 20500, seed + 113U);
    } else if (weapon == WEAPON_ROCKET) {
        gct89_trigger(&thump, 31500, seed + 113U);
    } else if (weapon == WEAPON_GRENADE_LAUNCHER) {
        gct89_trigger(&thump, 26500, seed + 113U);
    } else if (weapon == WEAPON_MAGNUM || weapon == WEAPON_SNIPER) {
        gct89_trigger(&thump, 30000, seed + 113U);
    } else {
        gct89_trigger(&thump, 27800, seed + 113U);
    }

    for (i = 0U; i < frames; ++i) {
        dry = 0;
        gpaah89_render_mono(&report, &dry, 1U);
        body_sample = gwb89_process_sample(&body, dry);
        gas_sample = gmg89_process_sample(&gas);
        crack_sample = gbc89_process_sample(&crack);
        thump_sample = gct89_process_sample(&thump);

        receiver_sample = wsoundreceiver89_process_sample(&receiver, dry);

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
            mix = ((show_s32)dry * 19) / 32;
            mix += ((show_s32)body_sample * 19) / 32;
            mix += ((show_s32)gas_sample * 11) / 32;
            mix += ((show_s32)crack_sample * 3) / 32;
            mix += ((show_s32)thump_sample * 11) / 32;
        } else if (weapon == WEAPON_MAGNUM) {
            mix = ((show_s32)dry * 20) / 32;
            mix += ((show_s32)body_sample * 18) / 32;
            mix += ((show_s32)gas_sample * 12) / 32;
            mix += ((show_s32)crack_sample * 8) / 32;
            mix += ((show_s32)thump_sample * 10) / 32;
        } else if (weapon == WEAPON_SNIPER) {
            mix = ((show_s32)dry * 20) / 32;
            mix += ((show_s32)body_sample * 19) / 32;
            mix += ((show_s32)gas_sample * 12) / 32;
            mix += ((show_s32)crack_sample * 12) / 32;
            mix += ((show_s32)thump_sample * 10) / 32;
        } else if (weapon == WEAPON_GRENADE_LAUNCHER) {
            mix = ((show_s32)dry * 17) / 32;
            mix += ((show_s32)body_sample * 17) / 32;
            mix += ((show_s32)gas_sample * 14) / 32;
            mix += ((show_s32)crack_sample * 2) / 32;
            mix += ((show_s32)thump_sample * 9) / 32;
        } else {
            mix = ((show_s32)dry * 20) / 32;
            mix += ((show_s32)body_sample * 19) / 32;
            mix += ((show_s32)gas_sample * 18) / 32;
            mix += ((show_s32)crack_sample * 2) / 32;
            mix += ((show_s32)thump_sample * 12) / 32;
        }
        if (weapon == WEAPON_PISTOL) {
            mix += ((show_s32)receiver_sample * 4) / 32;
        } else if (weapon == WEAPON_MACHINE || weapon == WEAPON_SNIPER) {
            mix += ((show_s32)receiver_sample * 6) / 32;
        } else {
            mix += ((show_s32)receiver_sample * 5) / 32;
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
                                     gwf89_speed foley_speed,
                                     wsoundreceiver89_preset receiver_preset,
                                     show_u16 minimum_ms,
                                     show_u32 seed,
                                     show_s16 *dst,
                                     show_u32 capacity,
                                     show_u32 *written,
                                     show_s16 ch_gain,
                                     show_s16 foley_gain,
                                     show_s16 receiver_gain,
                                     show_s16 target)
{
    ch89_context ch_ctx;
    ch89_gesture gesture;
    ch89_u32 ch_frames;
    gwf89_context foley;
    wsoundreceiver89_context receiver;
    show_u32 i;
    show_u32 frames;
    show_s32 dry;
    show_s32 mix;
    show_s16 resonant;
    if (ch89_init(&ch_ctx, SHOW_RATE, seed) != CH89_OK) return 0;
    if (ch89_make_preset(ch_preset, repetitions, 32767U, &gesture) != CH89_OK) return 0;
    ch_frames = 0U;
    show_zero_s16(scratch_a, SHOW_MAX_MECH_FRAMES);
    if (ch89_render(&ch_ctx, &gesture, scratch_a, SHOW_MAX_MECH_FRAMES,
                    &ch_frames) != CH89_OK) return 0;
    gwf89_init(&foley, seed + 97U);
    gwf89_set_room_send(&foley, 3600);
    gwf89_trigger_ex(&foley, foley_preset, seed + 101U,
                     GWF89_VARIANT_AUTO, foley_speed);
    if (wsoundreceiver89_init(&receiver, receiver_preset) != WSOUND89_OK)
        return 0;
    wsoundreceiver89_excite(&receiver, 19500);
    frames = ch_frames;
    if (frames < show_ms((show_u32)minimum_ms))
        frames = show_ms((show_u32)minimum_ms);
    if (frames > capacity) frames = capacity;
    for (i = 0U; i < frames; ++i) {
        dry = 0;
        if (i < ch_frames) dry += ((show_s32)scratch_a[i] * ch_gain) >> 15;
        dry += ((show_s32)gwf89_process_sample(&foley) * foley_gain) >> 15;
        resonant = wsoundreceiver89_process_sample(&receiver, show_sat16(dry));
        mix = dry + (((show_s32)resonant * receiver_gain) >> 15);
        dst[i] = show_sat16(mix);
    }
    show_normalize(dst, frames, target);
    *written = frames;
    return 1;
}

static int show_render_machine_cycle(show_s16 *dst, show_u32 capacity,
                                     show_u32 *written)
{
    ch89_context ch_ctx;
    ch89_gesture gesture;
    ch89_u32 ch_frames;
    gwf89_context foley;
    wsoundbeltfeed89_context belt;
    wsoundreceiver89_context receiver;
    show_u32 frames;
    show_u32 i;
    show_s32 dry;
    show_s32 mix;
    show_s16 resonant;
    if (dst == 0 || written == 0) return 0;
    if (ch89_init(&ch_ctx, SHOW_RATE, 0x4D434943U) != CH89_OK) return 0;
    if (ch89_make_preset(CH89_PRESET_MACHINE_GUN_FEED_BURST, 1U,
                         32767U, &gesture) != CH89_OK) return 0;
    ch_frames = 0U;
    show_zero_s16(scratch_a, SHOW_MAX_MECH_FRAMES);
    if (ch89_render(&ch_ctx, &gesture, scratch_a, SHOW_MAX_MECH_FRAMES,
                    &ch_frames) != CH89_OK) return 0;
    gwf89_init(&foley, 0x4D434944U);
    gwf89_set_room_send(&foley, 2200);
    gwf89_trigger_ex(&foley, GWF89_SMG_EMPTY, 0x4D434945U,
                     GWF89_VARIANT_AUTO, GWF89_SPEED_FAST);
    if (wsoundbeltfeed89_init(&belt, SHOW_RATE, 0x4D434946U) != WSOUND89_OK)
        return 0;
    if (wsoundbeltfeed89_start(&belt, 650U, 22000U, 24500U) != WSOUND89_OK)
        return 0;
    if (wsoundreceiver89_init(&receiver, WSOUNDRECEIVER89_HEAVY) != WSOUND89_OK)
        return 0;
    wsoundreceiver89_excite(&receiver, 22500);
    frames = show_ms(210U);
    if (frames > capacity) frames = capacity;
    for (i = 0U; i < frames; ++i) {
        if (i == show_ms(105U)) wsoundbeltfeed89_set_gate(&belt, 0);
        dry = 0;
        if (i < ch_frames) dry += ((show_s32)scratch_a[i] * 14500) >> 15;
        dry += ((show_s32)gwf89_process_sample(&foley) * 5200) >> 15;
        dry += ((show_s32)wsoundbeltfeed89_process_sample(&belt) * 12500) >> 15;
        resonant = wsoundreceiver89_process_sample(&receiver, show_sat16(dry));
        mix = dry + (((show_s32)resonant * 7600) >> 15);
        dst[i] = show_sat16(mix);
    }
    show_normalize(dst, frames, 17500);
    *written = frames;
    return 1;
}


static int show_render_reload_layer(ch89_preset ch_preset,
                                    show_u16 repetitions,
                                    gwf89_preset_id foley_preset,
                                    wsoundammo89_type ammo_type,
                                    wsoundreceiver89_preset receiver_preset,
                                    show_u32 seed,
                                    show_u16 duration_ms,
                                    show_s16 ch_gain,
                                    show_s16 foley_gain,
                                    show_s16 ammo_gain,
                                    show_s16 receiver_gain,
                                    show_s16 target,
                                    show_s16 *dst,
                                    show_u32 capacity,
                                    show_u32 *written)
{
    ch89_context ch_ctx;
    ch89_gesture gesture;
    ch89_u32 ch_frames;
    gwf89_context foley;
    wsoundammo89_context ammo;
    wsoundreceiver89_context receiver;
    show_u32 frames;
    show_u32 i;
    show_s32 dry;
    show_s32 mix;
    show_s16 resonant;
    if (dst == 0 || written == 0) return 0;
    if (ch89_init(&ch_ctx, SHOW_RATE, seed) != CH89_OK) return 0;
    if (ch89_make_preset(ch_preset, repetitions, 32767U, &gesture) != CH89_OK) return 0;
    ch_frames = 0U;
    show_zero_s16(scratch_a, SHOW_MAX_MECH_FRAMES);
    if (ch89_render(&ch_ctx, &gesture, scratch_a, SHOW_MAX_MECH_FRAMES,
                    &ch_frames) != CH89_OK) return 0;
    gwf89_init(&foley, seed + 17U);
    gwf89_set_room_send(&foley, 3400);
    gwf89_trigger_ex(&foley, foley_preset, seed + 31U,
                     GWF89_VARIANT_AUTO, GWF89_SPEED_NORMAL);
    if (wsoundammo89_init(&ammo, seed + 47U) != WSOUND89_OK) return 0;
    if (wsoundammo89_trigger(&ammo, ammo_type, 26000U, 28000U) != WSOUND89_OK) return 0;
    if (wsoundreceiver89_init(&receiver, receiver_preset) != WSOUND89_OK) return 0;
    wsoundreceiver89_excite(&receiver, 21000);
    frames = show_ms((show_u32)duration_ms);
    if (frames < ch_frames) frames = ch_frames;
    if (frames > capacity) frames = capacity;
    for (i = 0U; i < frames; ++i) {
        dry = 0;
        if (i < ch_frames) dry += ((show_s32)scratch_a[i] * ch_gain) >> 15;
        dry += ((show_s32)gwf89_process_sample(&foley) * foley_gain) >> 15;
        dry += ((show_s32)wsoundammo89_process_sample(&ammo) * ammo_gain) >> 15;
        resonant = wsoundreceiver89_process_sample(&receiver, show_sat16(dry));
        mix = dry + (((show_s32)resonant * receiver_gain) >> 15);
        dst[i] = show_sat16(mix);
    }
    show_normalize(dst, frames, target);
    *written = frames;
    return 1;
}


static int show_render_magazine_reload(int magazine_preset,
                                       ch89_preset ch_preset,
                                       gwf89_preset_id foley_preset,
                                       wsoundreceiver89_preset receiver_preset,
                                       show_u32 seed,
                                       show_u16 duration_ms,
                                       show_s16 target,
                                       show_s16 *dst,
                                       show_u32 capacity,
                                       show_u32 *written)
{
    WMag89State magazine[4];
    int actions[4];
    int velocities[4];
    show_u32 starts[4];
    show_u32 base_frames;
    show_u32 frames;
    show_u32 i;
    show_u16 j;
    show_s32 magazine_mix;
    show_s32 mix;
    show_s32 base_sample;
    if (dst == 0 || written == 0) return 0;
    show_zero_s16(scratch_b, SHOW_MAX_MECH_FRAMES);
    base_frames = 0U;
    if (!show_render_reload_layer(ch_preset, 2U, foley_preset,
                                  WSOUNDAMMO89_BOX_MAG, receiver_preset,
                                  seed + 17U, duration_ms,
                                  17500, 7800, 12500, 8000, 17000,
                                  scratch_b, SHOW_MAX_MECH_FRAMES,
                                  &base_frames)) return 0;
    actions[0] = WMAG89_ACTION_REMOVE;
    actions[1] = WMAG89_ACTION_INSERT;
    actions[2] = WMAG89_ACTION_SEAT_TAP;
    actions[3] = WMAG89_ACTION_TUG_CHECK;
    velocities[0] = 27000;
    velocities[1] = 31500;
    velocities[2] = 29000;
    velocities[3] = 23500;
    if (magazine_preset == WMAG89_PRESET_SNIPER_BOX) {
        starts[0] = show_ms(70U);
        starts[1] = show_ms(560U);
        starts[2] = show_ms(1080U);
        starts[3] = show_ms(1420U);
    } else {
        starts[0] = show_ms(45U);
        starts[1] = show_ms(430U);
        starts[2] = show_ms(850U);
        starts[3] = show_ms(1120U);
    }
    for (j = 0U; j < 4U; ++j) {
        if (wmag89_init(&magazine[j], (int)SHOW_RATE,
                        seed + 101U + (show_u32)j * 79U) != WMAG89_OK)
            return 0;
        if (wmag89_set_preset(&magazine[j], magazine_preset) != WMAG89_OK)
            return 0;
        wmag89_set_reverb(&magazine[j], 14500, 9000, 2500);
        wmag89_set_output_gain(&magazine[j], 28500);
    }
    frames = show_ms((show_u32)duration_ms);
    if (frames < base_frames) frames = base_frames;
    if (frames > capacity) frames = capacity;
    for (i = 0U; i < frames; ++i) {
        for (j = 0U; j < 4U; ++j) {
            if (i == starts[j]) {
                if (wmag89_trigger(&magazine[j], actions[j],
                                   velocities[j]) != WMAG89_OK)
                    return 0;
            }
        }
        magazine_mix = 0;
        for (j = 0U; j < 4U; ++j) {
            if (wmag89_is_active(&magazine[j]))
                magazine_mix += (show_s32)wmag89_process_sample(&magazine[j]);
        }
        base_sample = i < base_frames ? (show_s32)scratch_b[i] : 0;
        mix = (base_sample * 22500) >> 15;
        mix += (magazine_mix * 23800) >> 15;
        dst[i] = show_sat16(mix);
    }
    show_normalize(dst, frames, target);
    *written = frames;
    return 1;
}

static int show_render_belt_reload(show_s16 *dst, show_u32 capacity,
                                   show_u32 *written)
{
    ch89_context ch_ctx;
    ch89_gesture gesture;
    ch89_u32 ch_frames;
    gwf89_context foley;
    wsoundammo89_context ammo;
    wsoundbeltfeed89_context belt;
    wsoundreceiver89_context receiver;
    show_u32 frames;
    show_u32 i;
    show_s32 dry;
    show_s32 mix;
    show_s16 resonant;
    if (dst == 0 || written == 0) return 0;
    if (ch89_init(&ch_ctx, SHOW_RATE, 0x42454C54U) != CH89_OK) return 0;
    if (ch89_make_preset(CH89_PRESET_MACHINE_GUN_FEED_BURST, 2U,
                         32767U, &gesture) != CH89_OK) return 0;
    ch_frames = 0U;
    show_zero_s16(scratch_a, SHOW_MAX_MECH_FRAMES);
    if (ch89_render(&ch_ctx, &gesture, scratch_a, SHOW_MAX_MECH_FRAMES,
                    &ch_frames) != CH89_OK) return 0;
    gwf89_init(&foley, 0x42454C55U);
    gwf89_set_room_send(&foley, 3600);
    gwf89_trigger_ex(&foley, GWF89_SMG_SELECTOR, 0x42454C56U,
                     GWF89_VARIANT_AUTO, GWF89_SPEED_NORMAL);
    if (wsoundammo89_init(&ammo, 0x42454C57U) != WSOUND89_OK) return 0;
    if (wsoundammo89_trigger(&ammo, WSOUNDAMMO89_BELT_BOX,
                             24000U, 30000U) != WSOUND89_OK) return 0;
    if (wsoundbeltfeed89_init(&belt, SHOW_RATE, 0x42454C58U) != WSOUND89_OK) return 0;
    if (wsoundbeltfeed89_start(&belt, 760U, 28500U, 30500U) != WSOUND89_OK) return 0;
    if (wsoundreceiver89_init(&receiver, WSOUNDRECEIVER89_HEAVY) != WSOUND89_OK) return 0;
    wsoundreceiver89_excite(&receiver, 24500);
    frames = show_ms(1800U);
    if (frames > capacity) frames = capacity;
    for (i = 0U; i < frames; ++i) {
        if (i == show_ms(1050U)) wsoundbeltfeed89_set_gate(&belt, 0);
        dry = 0;
        if (i < ch_frames) dry += ((show_s32)scratch_a[i] * 21000) >> 15;
        dry += ((show_s32)gwf89_process_sample(&foley) * 8500) >> 15;
        dry += ((show_s32)wsoundammo89_process_sample(&ammo) * 15000) >> 15;
        dry += ((show_s32)wsoundbeltfeed89_process_sample(&belt) * 18000) >> 15;
        resonant = wsoundreceiver89_process_sample(&receiver, show_sat16(dry));
        mix = dry + (((show_s32)resonant * 10500) >> 15);
        dst[i] = show_sat16(mix);
    }
    show_normalize(dst, frames, 23000);
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

static int show_render_grenade_blast(show_s16 *dst, show_u32 capacity,
                                     show_u32 *written)
{
    ws_ggb89 synth;
    show_u32 frames;
    show_u32 i;
    if (dst == 0 || written == 0) return 0;
    frames = show_ms(5000U);
    if (frames > capacity) return 0;
    ws_ggb89_init(&synth, SHOW_RATE, 0x4752454EU);
    ws_ggb89_trigger(&synth, WS_GGB89_M67_OPEN, 32767);
    for (i = 0U; i < frames; ++i) dst[i] = ws_ggb89_process(&synth);
    show_normalize(dst, frames, 31800);
    *written = frames;
    return 1;
}


static int show_render_rocket_whistle(show_s16 *dst, show_u32 capacity,
                                      show_u32 *written)
{
    gwh89_state whistle;
    show_u32 maximum;
    show_u32 half;
    show_u32 i;
    show_u32 frames;
    show_s32 radial;
    show_s32 distance;
    if (dst == 0 || written == 0) return 0;
    maximum = show_ms(1500U);
    if (maximum > capacity) maximum = capacity;
    half = maximum / 2U;
    gwh89_init(&whistle, (gwh89_s32)SHOW_RATE, 0x57484953U);
    gwh89_trigger_preset(&whistle, GWH89_PRESET_RPG7_SUSTAINED);
    gwh89_set_auto_hold_ms(&whistle, 950);
    gwh89_set_output_eq_band_gain_q15(&whistle,
        GWH89_OUTPUT_EQ_RUMBLE_120, 1200);
    gwh89_set_output_eq_band_gain_q15(&whistle,
        GWH89_OUTPUT_EQ_LOW_120_300, 9000);
    gwh89_set_output_eq_band_gain_q15(&whistle,
        GWH89_OUTPUT_EQ_CORE_300_900, 25000);
    gwh89_set_output_eq_band_gain_q15(&whistle,
        GWH89_OUTPUT_EQ_WHISTLE_900_2400, 33000);
    gwh89_set_output_eq_band_gain_q15(&whistle,
        GWH89_OUTPUT_EQ_EDGE_2400_6000, 37000);
    gwh89_set_output_eq_band_gain_q15(&whistle,
        GWH89_OUTPUT_EQ_AIR_ABOVE_6000, 34500);
    frames = 0U;
    for (i = 0U; i < maximum; ++i) {
        if ((i & 63U) == 0U) {
            radial = -90 + (show_s32)((210U * i) / maximum);
            if (i < half) {
                distance = 18500 + (show_s32)((11500U * i) / half);
            } else {
                distance = 30000 - (show_s32)((14000U * (i - half)) /
                                               (maximum - half));
            }
            if (distance < 1000) distance = 1000;
            if (distance > 32767) distance = 32767;
            gwh89_set_motion(&whistle, radial, distance);
        }
        dst[i] = (show_s16)gwh89_render_mono_sample(&whistle);
        frames = i + 1U;
        if (!gwh89_is_active(&whistle) && i > show_ms(700U)) break;
    }
    show_normalize(dst, frames, 8200);
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
    if (!show_render_grenade_blast(grenade_blast_storage,
                                   SHOW_MAX_GRENADE_BLAST_FRAMES,
                                   &frames)) return 0;
    grenade_blast_bank.pcm = grenade_blast_storage;
    grenade_blast_bank.frames = frames;
    grenade_blast_bank.nominal_level_q15 = 32767U;

    frames = 0U;
    if (!show_render_rocket_whistle(rocket_whistle_storage,
                                    SHOW_MAX_ROCKET_WHISTLE_FRAMES,
                                    &frames)) return 0;
    rocket_whistle_bank.pcm = rocket_whistle_storage;
    rocket_whistle_bank.frames = frames;
    rocket_whistle_bank.nominal_level_q15 = 32767U;

    frames = 0U;
    if (!show_render_shotgun_pump(mech_storage[MECH_SHOTGUN_PUMP],
                                  SHOW_MAX_MECH_FRAMES, &frames)) return 0;
    mech_bank[MECH_SHOTGUN_PUMP].pcm = mech_storage[MECH_SHOTGUN_PUMP];
    mech_bank[MECH_SHOTGUN_PUMP].frames = frames;
    mech_bank[MECH_SHOTGUN_PUMP].nominal_level_q15 = 32767U;

    frames = 0U;
    if (!show_render_chuecka_foley(CH89_PRESET_SHOTGUN_INSERT, 1U,
                                   GWF89_SHOTGUN_EMPTY, GWF89_SPEED_NORMAL,
                                   WSOUNDRECEIVER89_SHOTGUN_WOOD, 450U,
                                   2101U, mech_storage[MECH_SHOTGUN_INSERT],
                                   SHOW_MAX_MECH_FRAMES, &frames,
                                   30000, 7000, 7000, 23500)) return 0;
    mech_bank[MECH_SHOTGUN_INSERT].pcm = mech_storage[MECH_SHOTGUN_INSERT];
    mech_bank[MECH_SHOTGUN_INSERT].frames = frames;
    mech_bank[MECH_SHOTGUN_INSERT].nominal_level_q15 = 32767U;

    frames = 0U;
    if (!show_render_chuecka_foley(CH89_PRESET_PISTOL_SLIDE, 1U,
                                   GWF89_PISTOL_HANDLING, GWF89_SPEED_NORMAL,
                                   WSOUNDRECEIVER89_PISTOL_STEEL, 430U,
                                   3101U, mech_storage[MECH_PISTOL_SLIDE],
                                   SHOW_MAX_MECH_FRAMES, &frames,
                                   28000, 11000, 7600, 23500)) return 0;
    mech_bank[MECH_PISTOL_SLIDE].pcm = mech_storage[MECH_PISTOL_SLIDE];
    mech_bank[MECH_PISTOL_SLIDE].frames = frames;
    mech_bank[MECH_PISTOL_SLIDE].nominal_level_q15 = 32767U;

    frames = 0U;
    if (!show_render_chuecka_foley(CH89_PRESET_PISTOL_SLIDE, 1U,
                                   GWF89_PISTOL_EMPTY, GWF89_SPEED_FAST,
                                   WSOUNDRECEIVER89_PISTOL_STEEL, 170U,
                                   3151U, mech_storage[MECH_PISTOL_AUTO],
                                   SHOW_MAX_MECH_FRAMES, &frames,
                                   24500, 6200, 6600, 17500)) return 0;
    mech_bank[MECH_PISTOL_AUTO].pcm = mech_storage[MECH_PISTOL_AUTO];
    mech_bank[MECH_PISTOL_AUTO].frames = frames;
    mech_bank[MECH_PISTOL_AUTO].nominal_level_q15 = 32767U;

    frames = 0U;
    if (!show_render_chuecka_foley(CH89_PRESET_MACHINE_GUN_FEED_BURST, 1U,
                                   GWF89_SMG_SELECTOR, GWF89_SPEED_NORMAL,
                                   WSOUNDRECEIVER89_HEAVY, 430U,
                                   4101U, mech_storage[MECH_MACHINE_FEED],
                                   SHOW_MAX_MECH_FRAMES, &frames,
                                   28500, 9000, 7200, 22000)) return 0;
    mech_bank[MECH_MACHINE_FEED].pcm = mech_storage[MECH_MACHINE_FEED];
    mech_bank[MECH_MACHINE_FEED].frames = frames;
    mech_bank[MECH_MACHINE_FEED].nominal_level_q15 = 32767U;

    frames = 0U;
    if (!show_render_machine_cycle(mech_storage[MECH_MACHINE_CYCLE],
                                   SHOW_MAX_MECH_FRAMES, &frames)) return 0;
    mech_bank[MECH_MACHINE_CYCLE].pcm = mech_storage[MECH_MACHINE_CYCLE];
    mech_bank[MECH_MACHINE_CYCLE].frames = frames;
    mech_bank[MECH_MACHINE_CYCLE].nominal_level_q15 = 32767U;

    frames = 0U;
    if (!show_render_chuecka_foley(CH89_PRESET_REVOLVER_CYLINDER, 1U,
                                   GWF89_MAGNUM_EMPTY, GWF89_SPEED_FAST,
                                   WSOUNDRECEIVER89_HEAVY, 230U,
                                   5101U, mech_storage[MECH_MAGNUM_ACTION],
                                   SHOW_MAX_MECH_FRAMES, &frames,
                                   27000, 10500, 6200, 20500)) return 0;
    mech_bank[MECH_MAGNUM_ACTION].pcm = mech_storage[MECH_MAGNUM_ACTION];
    mech_bank[MECH_MAGNUM_ACTION].frames = frames;
    mech_bank[MECH_MAGNUM_ACTION].nominal_level_q15 = 32767U;

    frames = 0U;
    if (!show_render_chuecka_foley(CH89_PRESET_MAGNUM_HEAVY_ACTION, 1U,
                                   GWF89_SNIPER_BOLT_DRY, GWF89_SPEED_NORMAL,
                                   WSOUNDRECEIVER89_RIFLE_STEEL, 760U,
                                   6101U, mech_storage[MECH_SNIPER_BOLT],
                                   SHOW_MAX_MECH_FRAMES, &frames,
                                   22500, 16500, 9000, 23200)) return 0;
    mech_bank[MECH_SNIPER_BOLT].pcm = mech_storage[MECH_SNIPER_BOLT];
    mech_bank[MECH_SNIPER_BOLT].frames = frames;
    mech_bank[MECH_SNIPER_BOLT].nominal_level_q15 = 32767U;

    frames = 0U;
    if (!show_render_chuecka_foley(CH89_PRESET_GRENADE_LAUNCHER, 1U,
                                   GWF89_LAUNCHER_LATCH, GWF89_SPEED_NORMAL,
                                   WSOUNDRECEIVER89_HEAVY, 430U,
                                   7101U, mech_storage[MECH_GRENADE_LAUNCHER],
                                   SHOW_MAX_MECH_FRAMES, &frames,
                                   25500, 13500, 7200, 21500)) return 0;
    mech_bank[MECH_GRENADE_LAUNCHER].pcm = mech_storage[MECH_GRENADE_LAUNCHER];
    mech_bank[MECH_GRENADE_LAUNCHER].frames = frames;
    mech_bank[MECH_GRENADE_LAUNCHER].nominal_level_q15 = 32767U;

    frames = 0U;
    if (!show_render_chuecka_foley(CH89_PRESET_ROCKET_LAUNCHER, 1U,
                                   GWF89_LAUNCHER_LATCH, GWF89_SPEED_NORMAL,
                                   WSOUNDRECEIVER89_HEAVY, 430U,
                                   8101U, mech_storage[MECH_ROCKET_LAUNCHER],
                                   SHOW_MAX_MECH_FRAMES, &frames,
                                   25000, 12500, 7600, 20500)) return 0;
    mech_bank[MECH_ROCKET_LAUNCHER].pcm = mech_storage[MECH_ROCKET_LAUNCHER];
    mech_bank[MECH_ROCKET_LAUNCHER].frames = frames;
    mech_bank[MECH_ROCKET_LAUNCHER].nominal_level_q15 = 32767U;

    frames = 0U;
    if (!show_render_chuecka_foley(CH89_PRESET_ATOM_ECKT, 1U,
                                   GWF89_LAUNCHER_LATCH, GWF89_SPEED_NORMAL,
                                   WSOUNDRECEIVER89_HEAVY, 430U,
                                   9101U, mech_storage[MECH_GRENADE_PIN],
                                   SHOW_MAX_MECH_FRAMES, &frames,
                                   19000, 15000, 5200, 18000)) return 0;
    mech_bank[MECH_GRENADE_PIN].pcm = mech_storage[MECH_GRENADE_PIN];
    mech_bank[MECH_GRENADE_PIN].frames = frames;
    mech_bank[MECH_GRENADE_PIN].nominal_level_q15 = 32767U;

    frames = 0U;
    if (!show_render_magazine_reload(WMAG89_PRESET_PISTOL_POLYMER,
                                     CH89_PRESET_PISTOL_SLIDE,
                                     GWF89_PISTOL_HANDLING,
                                     WSOUNDRECEIVER89_PISTOL_STEEL,
                                     10101U, 1500U, 22000,
                                     mech_storage[MECH_PISTOL_MAG],
                                     SHOW_MAX_MECH_FRAMES, &frames)) return 0;
    mech_bank[MECH_PISTOL_MAG].pcm = mech_storage[MECH_PISTOL_MAG];
    mech_bank[MECH_PISTOL_MAG].frames = frames;
    mech_bank[MECH_PISTOL_MAG].nominal_level_q15 = 32767U;

    frames = 0U;
    if (!show_render_reload_layer(CH89_PRESET_REVOLVER_CYLINDER, 2U,
                                  GWF89_MAGNUM_LATCH,
                                  WSOUNDAMMO89_LOOSE_SHELLS,
                                  WSOUNDRECEIVER89_HEAVY,
                                  11101U, 1500U,
                                  22000, 11500, 18500, 10500, 22000,
                                  mech_storage[MECH_MAGNUM_RELOAD],
                                  SHOW_MAX_MECH_FRAMES, &frames)) return 0;
    mech_bank[MECH_MAGNUM_RELOAD].pcm = mech_storage[MECH_MAGNUM_RELOAD];
    mech_bank[MECH_MAGNUM_RELOAD].frames = frames;
    mech_bank[MECH_MAGNUM_RELOAD].nominal_level_q15 = 32767U;

    frames = 0U;
    if (!show_render_reload_layer(CH89_PRESET_SHOTGUN_MULTI_INSERT, 5U,
                                  GWF89_SHOTGUN_EMPTY,
                                  WSOUNDAMMO89_LOOSE_SHELLS,
                                  WSOUNDRECEIVER89_SHOTGUN_WOOD,
                                  12101U, 2200U,
                                  25000, 7000, 16000, 9000, 22500,
                                  mech_storage[MECH_SHOTGUN_MULTI_INSERT],
                                  SHOW_MAX_MECH_FRAMES, &frames)) return 0;
    mech_bank[MECH_SHOTGUN_MULTI_INSERT].pcm =
        mech_storage[MECH_SHOTGUN_MULTI_INSERT];
    mech_bank[MECH_SHOTGUN_MULTI_INSERT].frames = frames;
    mech_bank[MECH_SHOTGUN_MULTI_INSERT].nominal_level_q15 = 32767U;

    frames = 0U;
    if (!show_render_belt_reload(mech_storage[MECH_MACHINE_BELT],
                                 SHOW_MAX_MECH_FRAMES, &frames)) return 0;
    mech_bank[MECH_MACHINE_BELT].pcm = mech_storage[MECH_MACHINE_BELT];
    mech_bank[MECH_MACHINE_BELT].frames = frames;
    mech_bank[MECH_MACHINE_BELT].nominal_level_q15 = 32767U;

    frames = 0U;
    if (!show_render_magazine_reload(WMAG89_PRESET_SNIPER_BOX,
                                     CH89_PRESET_MAGNUM_HEAVY_ACTION,
                                     GWF89_SNIPER_BOLT_DRY,
                                     WSOUNDRECEIVER89_RIFLE_STEEL,
                                     13101U, 1850U, 22500,
                                     mech_storage[MECH_SNIPER_MAG],
                                     SHOW_MAX_MECH_FRAMES, &frames)) return 0;
    mech_bank[MECH_SNIPER_MAG].pcm = mech_storage[MECH_SNIPER_MAG];
    mech_bank[MECH_SNIPER_MAG].frames = frames;
    mech_bank[MECH_SNIPER_MAG].nominal_level_q15 = 32767U;

    frames = 0U;
    if (!show_render_reload_layer(CH89_PRESET_ROCKET_LAUNCHER, 2U,
                                  GWF89_LAUNCHER_LATCH,
                                  WSOUNDAMMO89_LOOSE_SHELLS,
                                  WSOUNDRECEIVER89_HEAVY,
                                  14101U, 1700U,
                                  22000, 12500, 14500, 12000, 22000,
                                  mech_storage[MECH_ROCKET_RELOAD],
                                  SHOW_MAX_MECH_FRAMES, &frames)) return 0;
    mech_bank[MECH_ROCKET_RELOAD].pcm = mech_storage[MECH_ROCKET_RELOAD];
    mech_bank[MECH_ROCKET_RELOAD].frames = frames;
    mech_bank[MECH_ROCKET_RELOAD].nominal_level_q15 = 32767U;
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
    if (gwv89_apply_mix_profile(&handler, GWV89_MIX_HYBRID) != GV89_OK) return 0;
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
    show_add_event(show_ms(11850U), &rocket_whistle_bank,
                   GWV89_EVENT_AMBIENCE, 12000, -1000, 5200U, 90,
                   show_pitch(-4), 6802U, 2U);
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
    show_add_event(show_ms(750U), &rocket_whistle_bank,
                   GWV89_EVENT_AMBIENCE, 13000, 500, 3500U, 95,
                   show_pitch(0), 6902U, 1U);
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

static void show_schedule_pistol(show_u32 base)
{
    show_add_event(base + show_ms(180U), &mech_bank[MECH_PISTOL_SLIDE],
                   GWV89_EVENT_MECHANISM, 15800, -900, 700U, 40,
                   show_pitch(-6), 30001U, 1U);
    show_add_event(base + show_ms(900U), &shot_bank[WEAPON_PISTOL][0],
                   GWV89_EVENT_REPORT, 32767, 0, 900U, 260,
                   show_pitch(0), 30002U, 2U);
    show_add_event(base + show_ms(914U), &mech_bank[MECH_PISTOL_AUTO],
                   GWV89_EVENT_MECHANISM, 8800, 800, 1050U, 30,
                   show_pitch(18), 30003U, 1U);
    show_add_casing_event(base + show_ms(942U), GT89_SHELL_PISTOL_BRASS,
                          GT89_SURFACE_CONCRETE, 175U, 218U, 5200,
                          1250U, 13200, 30004U, 30004U);
}

static void show_schedule_magnum(show_u32 base)
{
    show_add_event(base + show_ms(200U), &mech_bank[MECH_MAGNUM_ACTION],
                   GWV89_EVENT_MECHANISM, 15500, -1100, 750U, 42,
                   show_pitch(-12), 31001U, 1U);
    show_add_event(base + show_ms(920U), &shot_bank[WEAPON_MAGNUM][1],
                   GWV89_EVENT_REPORT, 32767, 0, 950U, 285,
                   show_pitch(-3), 31002U, 1U);
    show_add_event(base + show_ms(1580U), &mech_bank[MECH_MAGNUM_ACTION],
                   GWV89_EVENT_MECHANISM, 14500, 1200, 850U, 38,
                   show_pitch(8), 31003U, 1U);
    show_add_casing_event(base + show_ms(2010U), GT89_SHELL_MAGNUM_BRASS,
                          GT89_SURFACE_METAL, 105U, 160U, 4200,
                          900U, 10500, 31004U, 31004U);
}

static void show_schedule_shotgun(show_u32 base)
{
    show_add_event(base + show_ms(170U), &mech_bank[MECH_SHOTGUN_INSERT],
                   GWV89_EVENT_MECHANISM, 14500, -900, 700U, 30,
                   show_pitch(-5), 32001U, 2U);
    show_add_event(base + show_ms(500U), &mech_bank[MECH_SHOTGUN_PUMP],
                   GWV89_EVENT_MECHANISM, 17500, 0, 800U, 55,
                   show_pitch(-8), 32002U, 1U);
    show_add_event(base + show_ms(1350U), &shot_bank[WEAPON_SHOTGUN][2],
                   GWV89_EVENT_REPORT, 32767, 0, 1100U, 300,
                   show_pitch(-2), 32003U, 1U);
    show_add_event(base + show_ms(2220U), &mech_bank[MECH_SHOTGUN_PUMP],
                   GWV89_EVENT_MECHANISM, 18000, 600, 900U, 60,
                   show_pitch(4), 32004U, 1U);
    show_add_casing_event(base + show_ms(2370U), GT89_SHELL_SHOTGUN_PLASTIC,
                          GT89_SURFACE_CONCRETE, 192U, 230U, 5200,
                          1150U, 14500, 32005U, 32005U);
}

static void show_schedule_machine(show_u32 base)
{
    show_u16 i;
    show_u32 t;
    show_add_event(base + show_ms(170U), &mech_bank[MECH_MACHINE_FEED],
                   GWV89_EVENT_MECHANISM, 14500, -900, 900U, 38,
                   show_pitch(-4), 33001U, 1U);
    for (i = 0U; i < 10U; ++i) {
        t = base + show_ms(850U + (show_u32)i * 86U);
        show_add_event(t, &shot_bank[WEAPON_MACHINE][i & 3U],
                       GWV89_EVENT_REPORT, 26000, 0, 1250U, 235,
                       show_pitch((show_s32)(i % 5U) - 2), 33010U, 12U);
        show_add_event(t - show_ms(8U), &mech_bank[MECH_MACHINE_CYCLE],
                       GWV89_EVENT_MECHANISM, 6900, 900, 1400U, 12,
                       show_pitch((show_s32)(i & 3U) - 2), 33020U, 3U);
        show_add_casing_event(t + show_ms(24U), GT89_SHELL_RIFLE_BRASS,
                              GT89_SURFACE_METAL, 184U,
                              (show_u8)(205U + i), 4800,
                              1450U, 9600, 33100U + i, 33010U);
        show_add_casing_event(t + show_ms(15U), GT89_SHELL_STEEL_CASE,
                              GT89_SURFACE_METAL, 112U,
                              (show_u8)(150U + i), -3600,
                              1500U, 5200, 33200U + i, 33010U);
    }
}

static void show_schedule_sniper(show_u32 base)
{
    show_add_event(base + show_ms(210U), &mech_bank[MECH_SNIPER_BOLT],
                   GWV89_EVENT_MECHANISM, 15500, -800, 800U, 50,
                   show_pitch(-10), 34001U, 1U);
    show_add_event(base + show_ms(1250U), &shot_bank[WEAPON_SNIPER][0],
                   GWV89_EVENT_REPORT, 32767, 0, 1500U, 320,
                   show_pitch(0), 34002U, 1U);
    show_add_event(base + show_ms(1900U), &mech_bank[MECH_SNIPER_BOLT],
                   GWV89_EVENT_MECHANISM, 17000, 1000, 850U, 55,
                   show_pitch(6), 34003U, 1U);
    show_add_casing_event(base + show_ms(2150U), GT89_SHELL_RIFLE_BRASS,
                          GT89_SURFACE_WOOD, 170U, 215U, 5200,
                          1200U, 12800, 34004U, 34004U);
}

static void show_schedule_grenade(show_u32 base)
{
    show_add_event(base + show_ms(220U), &mech_bank[MECH_GRENADE_PIN],
                   GWV89_EVENT_MECHANISM, 12000, -1000, 650U, 30,
                   show_pitch(8), 35001U, 1U);
    show_add_event(base + show_ms(520U), &mech_bank[MECH_GRENADE_PIN],
                   GWV89_EVENT_MECHANISM, 10500, 800, 900U, 20,
                   show_pitch(-5), 35002U, 1U);
    show_add_event(base + show_ms(2950U), &grenade_blast_bank,
                   GWV89_EVENT_EXPLOSION, 32767, 0, 3600U, 340,
                   show_pitch(0), 35003U, 1U);
}

static void show_schedule_rocket(show_u32 base)
{
    show_add_event(base + show_ms(210U), &mech_bank[MECH_ROCKET_LAUNCHER],
                   GWV89_EVENT_MECHANISM, 12800, -1000, 700U, 35,
                   show_pitch(-4), 36001U, 1U);
    show_add_event(base + show_ms(900U), &shot_bank[WEAPON_ROCKET][0],
                   GWV89_EVENT_REPORT, 31500, -5000, 1200U, 300,
                   show_pitch(-4), 36002U, 1U);
    show_add_event(base + show_ms(1050U), &rocket_whistle_bank,
                   GWV89_EVENT_AMBIENCE, 12500, 400, 3900U, 90,
                   show_pitch(-2), 36004U, 1U);
    show_add_event(base + show_ms(2700U), &rocket_blast_bank,
                   GWV89_EVENT_EXPLOSION, 32767, 6800, 4700U, 370,
                   show_pitch(0), 36003U, 1U);
}

static void show_schedule_grenade_launcher(show_u32 base)
{
    show_add_event(base + show_ms(190U), &mech_bank[MECH_GRENADE_LAUNCHER],
                   GWV89_EVENT_MECHANISM, 14200, -900, 750U, 40,
                   show_pitch(-5), 37001U, 1U);
    show_add_event(base + show_ms(920U), &shot_bank[WEAPON_GRENADE_LAUNCHER][1],
                   GWV89_EVENT_REPORT, 31800, -2800, 1100U, 280,
                   show_pitch(-8), 37002U, 1U);
    show_add_event(base + show_ms(2050U), &grenade_blast_bank,
                   GWV89_EVENT_EXPLOSION, 32767, 5800, 3900U, 350,
                   show_pitch(-8), 37003U, 1U);
    show_add_event(base + show_ms(3400U), &mech_bank[MECH_GRENADE_LAUNCHER],
                   GWV89_EVENT_MECHANISM, 13500, 900, 850U, 34,
                   show_pitch(7), 37004U, 1U);
}

static int show_scene_complete_sequences(gv89_stats *stats)
{
    show_clear_events();
    show_schedule_pistol(show_ms(0U));
    show_schedule_magnum(show_ms(3800U));
    show_schedule_shotgun(show_ms(7600U));
    show_schedule_machine(show_ms(11900U));
    show_schedule_sniper(show_ms(15700U));
    show_schedule_grenade(show_ms(20200U));
    show_schedule_rocket(show_ms(26300U));
    show_schedule_grenade_launcher(show_ms(33100U));
    return show_render_scene("audio/07_complete_weapon_sequences_balanced.wav",
                             show_ms(39900U), 36000, stats);
}

static int show_scene_pistol_complete(gv89_stats *stats)
{
    show_clear_events(); show_schedule_pistol(0U);
    return show_render_scene("audio/08_pistol_complete.wav", show_ms(3400U), 36000, stats);
}
static int show_scene_magnum_complete(gv89_stats *stats)
{
    show_clear_events(); show_schedule_magnum(0U);
    return show_render_scene("audio/09_magnum_complete.wav", show_ms(3500U), 36000, stats);
}
static int show_scene_shotgun_complete(gv89_stats *stats)
{
    show_clear_events(); show_schedule_shotgun(0U);
    return show_render_scene("audio/10_shotgun_complete_balanced.wav", show_ms(4200U), 36000, stats);
}
static int show_scene_machine_complete(gv89_stats *stats)
{
    show_clear_events(); show_schedule_machine(0U);
    return show_render_scene("audio/11_machinegun_complete.wav", show_ms(3400U), 35000, stats);
}
static int show_scene_sniper_complete(gv89_stats *stats)
{
    show_clear_events(); show_schedule_sniper(0U);
    return show_render_scene("audio/12_sniper_complete.wav", show_ms(4400U), 36000, stats);
}
static int show_scene_grenade_complete(gv89_stats *stats)
{
    show_clear_events(); show_schedule_grenade(0U);
    return show_render_scene("audio/13_grenade_complete.wav", show_ms(8200U), 36000, stats);
}
static int show_scene_rocket_complete(gv89_stats *stats)
{
    show_clear_events(); show_schedule_rocket(0U);
    return show_render_scene("audio/14_rocket_launcher_complete.wav", show_ms(8500U), 36000, stats);
}
static int show_scene_grenade_launcher_complete(gv89_stats *stats)
{
    show_clear_events(); show_schedule_grenade_launcher(0U);
    return show_render_scene("audio/15_grenade_launcher_complete.wav", show_ms(7600U), 36000, stats);
}


static void show_schedule_pistol_capacity(show_u32 base)
{
    show_u16 i;
    show_u32 t;
    show_add_event(base + show_ms(500U), &shot_bank[WEAPON_PISTOL][0],
                   GWV89_EVENT_REPORT, 32767, 0, 850U, 300,
                   show_pitch(0), 40001U, 2U);
    show_add_event(base + show_ms(512U), &mech_bank[MECH_PISTOL_AUTO],
                   GWV89_EVENT_MECHANISM, 8600, 900, 900U, 34,
                   show_pitch(110), 40002U, 4U);
    show_add_casing_event(base + show_ms(525U), GT89_SHELL_PISTOL_BRASS,
                          GT89_SURFACE_CONCRETE, 180U, 220U, 5200,
                          1100U, 13000, 40003U, 40003U);

    show_add_event(base + show_ms(2500U), &mech_bank[MECH_PISTOL_MAG],
                   GWV89_EVENT_MECHANISM, 18200, -900, 700U, 52,
                   show_pitch(-20), 40100U, 1U);
    show_add_event(base + show_ms(3600U), &mech_bank[MECH_PISTOL_SLIDE],
                   GWV89_EVENT_MECHANISM, 16500, 700, 750U, 58,
                   show_pitch(-10), 40101U, 1U);
    for (i = 0U; i < 20U; ++i) {
        t = base + show_ms(4450U + (show_u32)i * 255U);
        show_add_event(t, &shot_bank[WEAPON_PISTOL][i & 3U],
                       GWV89_EVENT_REPORT,
                       (show_s16)(31800 - (i & 3U) * 260),
                       (show_s16)(-700 + (show_s16)(i & 3U) * 450),
                       950U, 270, show_pitch((show_s32)(i % 5U) - 2),
                       40200U, 4U);
        show_add_event(t + show_ms(7U), &mech_bank[MECH_PISTOL_AUTO],
                       GWV89_EVENT_MECHANISM, 7600, 1000, 1050U, 25,
                       show_pitch(130 + (show_s32)(i & 3U) * 12),
                       40210U, 5U);
        show_add_casing_event(t + show_ms(23U), GT89_SHELL_PISTOL_BRASS,
                              GT89_SURFACE_CONCRETE,
                              (show_u8)(170U + (i & 15U)),
                              (show_u8)(205U + (i & 31U)),
                              (show_s16)(5200 + (show_s16)(i & 3U) * 350),
                              1250U, 11800, 40300U + i, 40200U);
    }
    t = base + show_ms(4450U + 20U * 255U + 650U);
    show_add_event(t, &mech_bank[MECH_PISTOL_MAG],
                   GWV89_EVENT_MECHANISM, 18000, -700, 720U, 50,
                   show_pitch(15), 40400U, 1U);
    show_add_event(t + show_ms(1080U), &mech_bank[MECH_PISTOL_SLIDE],
                   GWV89_EVENT_MECHANISM, 16000, 800, 760U, 55,
                   show_pitch(20), 40401U, 1U);
}

static void show_schedule_magnum_capacity(show_u32 base)
{
    show_u16 i;
    show_u32 t;
    show_add_event(base + show_ms(500U), &shot_bank[WEAPON_MAGNUM][0],
                   GWV89_EVENT_REPORT, 32767, 0, 900U, 320,
                   show_pitch(-2), 41001U, 1U);
    show_add_event(base + show_ms(410U), &mech_bank[MECH_MAGNUM_ACTION],
                   GWV89_EVENT_MECHANISM, 9000, 600, 980U, 28,
                   show_pitch(80), 41002U, 2U);

    show_add_event(base + show_ms(2500U), &mech_bank[MECH_MAGNUM_RELOAD],
                   GWV89_EVENT_MECHANISM, 18500, -800, 760U, 55,
                   show_pitch(-10), 41100U, 1U);
    for (i = 0U; i < 6U; ++i) {
        t = base + show_ms(4300U + (show_u32)i * 540U);
        show_add_event(t, &shot_bank[WEAPON_MAGNUM][i & 3U],
                       GWV89_EVENT_REPORT, 32767,
                       (show_s16)(-500 + (show_s16)(i & 1U) * 1000),
                       980U, 315, show_pitch((show_s32)i - 3),
                       41200U, 2U);
        show_add_event(t - show_ms(85U), &mech_bank[MECH_MAGNUM_ACTION],
                       GWV89_EVENT_MECHANISM, 9000, 500, 1050U, 30,
                       show_pitch(65 + (show_s32)i * 4),
                       41210U, 2U);
    }
    t = base + show_ms(4300U + 6U * 540U + 650U);
    show_add_event(t, &mech_bank[MECH_MAGNUM_ACTION],
                   GWV89_EVENT_MECHANISM, 15000, -900, 780U, 50,
                   show_pitch(-20), 41300U, 1U);
    for (i = 0U; i < 6U; ++i) {
        show_add_casing_event(t + show_ms(260U + (show_u32)i * 42U),
                              GT89_SHELL_MAGNUM_BRASS, GT89_SURFACE_METAL,
                              (show_u8)(105U + i * 5U),
                              (show_u8)(150U + i * 9U),
                              (show_s16)(-4500 + (show_s16)i * 1700),
                              950U, 10800, 41310U + i, 41300U);
    }
    show_add_event(t + show_ms(900U), &mech_bank[MECH_MAGNUM_RELOAD],
                   GWV89_EVENT_MECHANISM, 18800, 700, 760U, 58,
                   show_pitch(10), 41320U, 1U);
}

static void show_schedule_shotgun_capacity(show_u32 base)
{
    show_u16 i;
    show_u32 t;
    show_add_event(base + show_ms(500U), &shot_bank[WEAPON_SHOTGUN][0],
                   GWV89_EVENT_REPORT, 32767, 0, 1000U, 340,
                   show_pitch(0), 42001U, 1U);
    show_add_event(base + show_ms(1180U), &mech_bank[MECH_SHOTGUN_PUMP],
                   GWV89_EVENT_MECHANISM, 17500, 500, 900U, 62,
                   show_pitch(-5), 42002U, 1U);
    show_add_casing_event(base + show_ms(1340U), GT89_SHELL_SHOTGUN_PLASTIC,
                          GT89_SURFACE_CONCRETE, 198U, 228U, 5200,
                          1150U, 14000, 42003U, 42003U);

    show_add_event(base + show_ms(3000U),
                   &mech_bank[MECH_SHOTGUN_MULTI_INSERT],
                   GWV89_EVENT_MECHANISM, 17500, -700, 700U, 48,
                   show_pitch(-15), 42100U, 1U);
    show_add_event(base + show_ms(5100U),
                   &mech_bank[MECH_SHOTGUN_MULTI_INSERT],
                   GWV89_EVENT_MECHANISM, 17600, 700, 720U, 48,
                   show_pitch(12), 42101U, 1U);
    show_add_event(base + show_ms(7200U), &mech_bank[MECH_SHOTGUN_PUMP],
                   GWV89_EVENT_MECHANISM, 18200, 0, 780U, 65,
                   show_pitch(-8), 42102U, 1U);

    for (i = 0U; i < 10U; ++i) {
        t = base + show_ms(8250U + (show_u32)i * 1180U);
        show_add_event(t, &shot_bank[WEAPON_SHOTGUN][i & 3U],
                       GWV89_EVENT_REPORT,
                       (show_s16)(32767 - (i & 3U) * 180),
                       (show_s16)(-600 + (show_s16)(i & 3U) * 400),
                       1120U, 340, show_pitch((show_s32)(i % 5U) - 2),
                       42200U, 2U);
        show_add_event(t + show_ms(650U), &mech_bank[MECH_SHOTGUN_PUMP],
                       GWV89_EVENT_MECHANISM, 17800, 600, 930U, 64,
                       show_pitch((show_s32)(i & 3U) * 6 - 7),
                       42210U, 1U);
        show_add_casing_event(t + show_ms(805U),
                              GT89_SHELL_SHOTGUN_PLASTIC,
                              GT89_SURFACE_CONCRETE,
                              (show_u8)(190U + (i & 7U) * 3U),
                              (show_u8)(220U + (i & 15U)),
                              (show_s16)(5200 + (show_s16)(i & 3U) * 400),
                              1250U, 13800, 42300U + i, 42200U);
    }
    t = base + show_ms(8250U + 10U * 1180U + 1050U);
    show_add_event(t, &mech_bank[MECH_SHOTGUN_MULTI_INSERT],
                   GWV89_EVENT_MECHANISM, 17400, -650, 720U, 48,
                   show_pitch(-12), 42400U, 1U);
    show_add_event(t + show_ms(2100U),
                   &mech_bank[MECH_SHOTGUN_MULTI_INSERT],
                   GWV89_EVENT_MECHANISM, 17400, 650, 720U, 48,
                   show_pitch(10), 42401U, 1U);
}

static void show_schedule_machine_capacity(show_u32 base)
{
    show_u16 i;
    show_u32 t;
    show_add_event(base + show_ms(500U), &shot_bank[WEAPON_MACHINE][0],
                   GWV89_EVENT_REPORT, 32767, 0, 1100U, 285,
                   show_pitch(0), 43001U, 4U);
    show_add_event(base + show_ms(492U), &mech_bank[MECH_MACHINE_CYCLE],
                   GWV89_EVENT_MECHANISM, 7000, 700, 1200U, 22,
                   show_pitch(140), 43002U, 4U);
    show_add_casing_event(base + show_ms(525U), GT89_SHELL_RIFLE_BRASS,
                          GT89_SURFACE_METAL, 190U, 220U, 5200,
                          1300U, 10500, 43003U, 43003U);

    show_add_event(base + show_ms(2350U), &mech_bank[MECH_MACHINE_BELT],
                   GWV89_EVENT_MECHANISM, 17800, -800, 760U, 55,
                   show_pitch(-10), 43100U, 1U);
    for (i = 0U; i < 40U; ++i) {
        t = base + show_ms(4300U + (show_u32)i * 78U);
        show_add_event(t, &shot_bank[WEAPON_MACHINE][i & 3U],
                       GWV89_EVENT_REPORT,
                       (show_s16)(29200 - (i & 3U) * 220),
                       (show_s16)(-1200 + (show_s16)(i % 7U) * 380),
                       1280U, 240,
                       show_pitch((show_s32)((i * 7U) % 17U) - 8),
                       43200U, 16U);
        show_add_event(t - show_ms(8U), &mech_bank[MECH_MACHINE_CYCLE],
                       GWV89_EVENT_MECHANISM, 6100, 900, 1380U, 18,
                       show_pitch(150 + (show_s32)(i & 3U) * 15),
                       43210U, 6U);
        show_add_casing_event(t + show_ms(22U), GT89_SHELL_RIFLE_BRASS,
                              GT89_SURFACE_METAL,
                              (show_u8)(180U + (i & 15U)),
                              (show_u8)(205U + (i & 31U)),
                              (show_s16)(5400 + (show_s16)(i % 5U) * 350),
                              1450U, 9000, 43300U + i, 43200U);
        show_add_casing_event(t + show_ms(14U), GT89_SHELL_STEEL_CASE,
                              GT89_SURFACE_METAL,
                              (show_u8)(105U + (i & 7U)),
                              (show_u8)(145U + (i & 15U)),
                              (show_s16)(-3600 - (show_s16)(i % 4U) * 180),
                              1500U, 4700, 43500U + i, 43200U);
    }
    t = base + show_ms(4300U + 40U * 78U + 720U);
    show_add_event(t, &mech_bank[MECH_MACHINE_BELT],
                   GWV89_EVENT_MECHANISM, 17600, 750, 780U, 54,
                   show_pitch(8), 43400U, 1U);
}

static void show_schedule_sniper_capacity(show_u32 base)
{
    show_u16 i;
    show_u32 t;
    show_add_event(base + show_ms(500U), &shot_bank[WEAPON_SNIPER][0],
                   GWV89_EVENT_REPORT, 32767, 0, 1300U, 360,
                   show_pitch(0), 44001U, 1U);
    show_add_event(base + show_ms(1080U), &mech_bank[MECH_SNIPER_BOLT],
                   GWV89_EVENT_MECHANISM, 16800, 700, 900U, 58,
                   show_pitch(4), 44002U, 1U);
    show_add_casing_event(base + show_ms(1320U), GT89_SHELL_RIFLE_BRASS,
                          GT89_SURFACE_WOOD, 172U, 214U, 4900,
                          1100U, 12500, 44003U, 44003U);

    show_add_event(base + show_ms(3000U), &mech_bank[MECH_SNIPER_MAG],
                   GWV89_EVENT_MECHANISM, 18000, -700, 760U, 58,
                   show_pitch(-10), 44100U, 1U);
    show_add_event(base + show_ms(4300U), &mech_bank[MECH_SNIPER_BOLT],
                   GWV89_EVENT_MECHANISM, 17000, 700, 820U, 60,
                   show_pitch(-4), 44101U, 1U);
    for (i = 0U; i < 4U; ++i) {
        t = base + show_ms(5350U + (show_u32)i * 2150U);
        show_add_event(t, &shot_bank[WEAPON_SNIPER][i & 3U],
                       GWV89_EVENT_REPORT, 32767,
                       (show_s16)(-400 + (show_s16)i * 280),
                       1450U, 370, show_pitch((show_s32)i - 2),
                       44200U, 1U);
        show_add_event(t + show_ms(650U), &mech_bank[MECH_SNIPER_BOLT],
                       GWV89_EVENT_MECHANISM, 17000, 800, 900U, 62,
                       show_pitch((show_s32)i * 4 - 5),
                       44210U, 1U);
        show_add_casing_event(t + show_ms(900U), GT89_SHELL_RIFLE_BRASS,
                              GT89_SURFACE_WOOD,
                              (show_u8)(168U + i * 4U),
                              (show_u8)(210U + i * 7U),
                              (show_s16)(4800 + (show_s16)i * 350),
                              1200U, 12600, 44300U + i, 44200U);
    }
    t = base + show_ms(5350U + 4U * 2150U + 900U);
    show_add_event(t, &mech_bank[MECH_SNIPER_MAG],
                   GWV89_EVENT_MECHANISM, 17800, -650, 760U, 58,
                   show_pitch(8), 44400U, 1U);
    show_add_event(t + show_ms(1250U), &mech_bank[MECH_SNIPER_BOLT],
                   GWV89_EVENT_MECHANISM, 16600, 650, 820U, 60,
                   show_pitch(3), 44401U, 1U);
}

static void show_schedule_rocket_capacity(show_u32 base)
{
    show_u16 i;
    show_u32 cycle;
    show_add_event(base + show_ms(500U), &shot_bank[WEAPON_ROCKET][0],
                   GWV89_EVENT_REPORT, 32767, -4200, 1100U, 350,
                   show_pitch(0), 45001U, 1U);
    show_add_event(base + show_ms(610U), &rocket_whistle_bank,
                   GWV89_EVENT_AMBIENCE, 13200, 300, 3600U, 95,
                   show_pitch(0), 45004U, 1U);
    show_add_event(base + show_ms(2200U), &rocket_blast_bank,
                   GWV89_EVENT_EXPLOSION, 32767, 5600, 4300U, 390,
                   show_pitch(0), 45002U, 1U);
    show_add_fire_event(base + show_ms(2230U), GSSE89_FIRE_ROLE_AMBIENCE,
                        GFIRE89_PRESET_DEBRIS, 1300U, 5600, 4400U,
                        11500, 20, 45003U, 45003U);

    for (i = 0U; i < 4U; ++i) {
        cycle = base + show_ms(4900U + (show_u32)i * 4400U);
        show_add_event(cycle, &mech_bank[MECH_ROCKET_RELOAD],
                       GWV89_EVENT_MECHANISM, 18000, -1200, 720U, 58,
                       show_pitch((show_s32)i * 4 - 6),
                       45100U + i, 1U);
        show_add_event(cycle + show_ms(1000U),
                       &shot_bank[WEAPON_ROCKET][i & 3U],
                       GWV89_EVENT_REPORT, 32767, -5000, 1200U, 355,
                       show_pitch((show_s32)i - 2),
                       45200U + i, 1U);
        show_add_event(cycle + show_ms(1110U), &rocket_whistle_bank,
                       GWV89_EVENT_AMBIENCE,
                       (show_s16)(12800 - (show_s16)i * 150),
                       (show_s16)(-300 + (show_s16)i * 250),
                       (show_u16)(2300U + i * 220U), 165,
                       show_pitch((show_s32)i * 2 - 3),
                       45250U + i, 1U);
        show_add_event(cycle + show_ms(2650U), &rocket_blast_bank,
                       GWV89_EVENT_EXPLOSION, 32767, 6500, 4700U, 400,
                       show_pitch((show_s32)(i & 1U) * 3 - 1),
                       45300U + i, 1U);
        show_add_fire_event(cycle + show_ms(2680U),
                            GSSE89_FIRE_ROLE_AMBIENCE,
                            GFIRE89_PRESET_DEBRIS, 1400U, 6500, 4800U,
                            11800, 22, 45400U + i, 45400U + i);
    }
    cycle = base + show_ms(4900U + 4U * 4400U);
    show_add_event(cycle, &mech_bank[MECH_ROCKET_RELOAD],
                   GWV89_EVENT_MECHANISM, 17800, -900, 720U, 58,
                   show_pitch(5), 45500U, 1U);
}

static int show_scene_pistol_capacity(gv89_stats *stats)
{
    show_clear_events();
    show_schedule_pistol_capacity(0U);
    return show_render_scene("audio/16_pistol_one_plus_20_full_reload.wav",
                             show_ms(13000U), 39000, stats);
}

static int show_scene_magnum_capacity(gv89_stats *stats)
{
    show_clear_events();
    show_schedule_magnum_capacity(0U);
    return show_render_scene("audio/17_magnum_one_plus_6_full_reload.wav",
                             show_ms(11500U), 39000, stats);
}

static int show_scene_shotgun_capacity(gv89_stats *stats)
{
    show_clear_events();
    show_schedule_shotgun_capacity(0U);
    return show_render_scene("audio/18_shotgun_one_plus_10_full_reload.wav",
                             show_ms(25500U), 39000, stats);
}

static int show_scene_machine_capacity(gv89_stats *stats)
{
    show_clear_events();
    show_schedule_machine_capacity(0U);
    return show_render_scene("audio/19_machinegun_one_plus_40_full_reload.wav",
                             show_ms(10000U), 38000, stats);
}

static int show_scene_sniper_capacity(gv89_stats *stats)
{
    show_clear_events();
    show_schedule_sniper_capacity(0U);
    return show_render_scene("audio/20_sniper_one_plus_4_full_reload.wav",
                             show_ms(16000U), 39000, stats);
}

static int show_scene_rocket_capacity(gv89_stats *stats)
{
    show_clear_events();
    show_schedule_rocket_capacity(0U);
    return show_render_scene("audio/21_rocket_one_plus_4_full_reload.wav",
                             show_ms(27000U), 40000, stats);
}

static int show_scene_all_capacity(gv89_stats *stats)
{
    show_clear_events();
    show_schedule_pistol_capacity(show_ms(0U));
    show_schedule_magnum_capacity(show_ms(13500U));
    show_schedule_shotgun_capacity(show_ms(25500U));
    show_schedule_machine_capacity(show_ms(51500U));
    show_schedule_sniper_capacity(show_ms(62000U));
    show_schedule_rocket_capacity(show_ms(78500U));
    return show_render_scene("audio/22_all_one_shot_then_full_capacity_sequences.wav",
                             show_ms(106000U), 38500, stats);
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
    gv89_stats stats[22];
    if (!show_build_banks()) {
        fprintf(stderr, "failed to build procedural banks\n");
        return 1;
    }
    if (!show_scene_shotgun(&stats[0])) return 2;
    if (!show_scene_machine(&stats[1])) return 3;
    if (!show_scene_pistols(&stats[2])) return 4;
    if (!show_scene_firefight(&stats[3])) return 5;
    if (!show_scene_rocket_pair(&stats[4])) return 6;
    if (!show_scene_extras(&stats[5])) return 7;
    if (!show_scene_complete_sequences(&stats[6])) return 8;
    if (!show_scene_pistol_complete(&stats[7])) return 9;
    if (!show_scene_magnum_complete(&stats[8])) return 10;
    if (!show_scene_shotgun_complete(&stats[9])) return 11;
    if (!show_scene_machine_complete(&stats[10])) return 12;
    if (!show_scene_sniper_complete(&stats[11])) return 13;
    if (!show_scene_grenade_complete(&stats[12])) return 14;
    if (!show_scene_rocket_complete(&stats[13])) return 15;
    if (!show_scene_grenade_launcher_complete(&stats[14])) return 16;
    if (!show_scene_pistol_capacity(&stats[15])) return 17;
    if (!show_scene_magnum_capacity(&stats[16])) return 18;
    if (!show_scene_shotgun_capacity(&stats[17])) return 19;
    if (!show_scene_machine_capacity(&stats[18])) return 20;
    if (!show_scene_sniper_capacity(&stats[19])) return 21;
    if (!show_scene_rocket_capacity(&stats[20])) return 22;
    if (!show_scene_all_capacity(&stats[21])) return 23;
    show_print_stats("shotgun legacy", &stats[0]);
    show_print_stats("machinegun legacy", &stats[1]);
    show_print_stats("pistols legacy", &stats[2]);
    show_print_stats("firefight+rocket", &stats[3]);
    show_print_stats("rocket launch+impact", &stats[4]);
    show_print_stats("gtinkle+gfire", &stats[5]);
    show_print_stats("all complete sequences", &stats[6]);
    show_print_stats("pistol complete", &stats[7]);
    show_print_stats("magnum complete", &stats[8]);
    show_print_stats("shotgun complete", &stats[9]);
    show_print_stats("machinegun complete", &stats[10]);
    show_print_stats("sniper complete", &stats[11]);
    show_print_stats("grenade complete", &stats[12]);
    show_print_stats("rocket complete", &stats[13]);
    show_print_stats("grenade launcher complete", &stats[14]);
    show_print_stats("pistol one+20+reload", &stats[15]);
    show_print_stats("magnum one+6+reload", &stats[16]);
    show_print_stats("shotgun one+10+reload", &stats[17]);
    show_print_stats("machine one+40+reload", &stats[18]);
    show_print_stats("sniper one+4+reload", &stats[19]);
    show_print_stats("rocket one+4+reload", &stats[20]);
    show_print_stats("all full-capacity sequences", &stats[21]);
    printf("banks: pistol=%lu machine=%lu shotgun=%lu rocket=%lu magnum=%lu sniper=%lu grenade_launcher=%lu grenade_blast=%lu rocket_blast=%lu pump=%lu\n",
           (unsigned long)shot_bank[WEAPON_PISTOL][0].frames,
           (unsigned long)shot_bank[WEAPON_MACHINE][0].frames,
           (unsigned long)shot_bank[WEAPON_SHOTGUN][0].frames,
           (unsigned long)shot_bank[WEAPON_ROCKET][0].frames,
           (unsigned long)shot_bank[WEAPON_MAGNUM][0].frames,
           (unsigned long)shot_bank[WEAPON_SNIPER][0].frames,
           (unsigned long)shot_bank[WEAPON_GRENADE_LAUNCHER][0].frames,
           (unsigned long)grenade_blast_bank.frames,
           (unsigned long)rocket_blast_bank.frames,
           (unsigned long)mech_bank[MECH_SHOTGUN_PUMP].frames);
    return 0;
}
