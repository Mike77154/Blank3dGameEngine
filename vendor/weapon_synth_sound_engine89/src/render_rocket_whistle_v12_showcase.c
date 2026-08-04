#include <stdio.h>
#include <string.h>
#include "weapon_synth_sound_engine89.h"

#define RW12_RATE 44100U
#define RW12_SECONDS 7U
#define RW12_FRAMES (RW12_RATE * RW12_SECONDS)
#define RW12_LOGICAL 48U

static wsse89_context rw12_engine;
static gv89_voice rw12_logical[RW12_LOGICAL];
static gssr89_voice rw12_reports[4];
static gsse89_casing_voice rw12_casings[2];
static gsse89_fire_voice rw12_fires[2];
static gsso89_bullet_voice rw12_bullets[2];
static gsso89_grenade_voice rw12_grenades[1];
static gsso89_rocket_voice rw12_rockets[2];
static gssw89_projectile_voice rw12_projectiles[2];
static gssw89_impact_voice rw12_impacts[2];
static gssw89_ricochet_voice rw12_ricochets[2];
static wsound89_i16 rw12_outdoor[RW12_RATE];
static wsound89_i16 rw12_portal[RW12_RATE / 2U];
static wsound89_i16 rw12_spatial_l[64];
static wsound89_i16 rw12_spatial_r[64];
static wsound89_i16 rw12_room[14000];
static wsound89_i16 rw12_prop[RW12_RATE + 64U];
static gv89_s16 rw12_pcm[RW12_FRAMES * 2U];

static void rw12_u16le(FILE *file, unsigned int value)
{
    fputc((int)(value & 255U), file);
    fputc((int)((value >> 8) & 255U), file);
}

static void rw12_u32le(FILE *file, unsigned long value)
{
    fputc((int)(value & 255UL), file);
    fputc((int)((value >> 8) & 255UL), file);
    fputc((int)((value >> 16) & 255UL), file);
    fputc((int)((value >> 24) & 255UL), file);
}

static int rw12_write_wav(const char *path, const gv89_s16 *pcm,
                          gv89_u32 frames)
{
    FILE *file;
    gv89_u32 i;
    unsigned long data_bytes;
    file = fopen(path, "wb");
    if (file == 0) return 0;
    data_bytes = (unsigned long)frames * 4UL;
    fwrite("RIFF", 1U, 4U, file);
    rw12_u32le(file, 36UL + data_bytes);
    fwrite("WAVEfmt ", 1U, 8U, file);
    rw12_u32le(file, 16UL);
    rw12_u16le(file, 1U);
    rw12_u16le(file, 2U);
    rw12_u32le(file, RW12_RATE);
    rw12_u32le(file, RW12_RATE * 4UL);
    rw12_u16le(file, 4U);
    rw12_u16le(file, 16U);
    fwrite("data", 1U, 4U, file);
    rw12_u32le(file, data_bytes);
    for (i = 0U; i < frames * 2U; ++i)
        rw12_u16le(file, (unsigned int)(unsigned short)pcm[i]);
    fclose(file);
    return 1;
}

static void rw12_storage(wsse89_storage *storage)
{
    memset(storage, 0, sizeof(*storage));
    storage->logical_voices = rw12_logical;
    storage->report_voices = rw12_reports;
    storage->report_capacity = 4U;
    storage->casing_voices = rw12_casings;
    storage->casing_capacity = 2U;
    storage->fire_voices = rw12_fires;
    storage->fire_capacity = 2U;
    storage->bullet_voices = rw12_bullets;
    storage->bullet_capacity = 2U;
    storage->grenade_voices = rw12_grenades;
    storage->grenade_capacity = 1U;
    storage->rocket_voices = rw12_rockets;
    storage->rocket_capacity = 2U;
    storage->projectile_voices = rw12_projectiles;
    storage->projectile_capacity = 2U;
    storage->impact_voices = rw12_impacts;
    storage->impact_capacity = 2U;
    storage->ricochet_voices = rw12_ricochets;
    storage->ricochet_capacity = 2U;
    storage->expansion_memory.outdoor = rw12_outdoor;
    storage->expansion_memory.outdoor_frames = RW12_RATE;
    storage->expansion_memory.portal = rw12_portal;
    storage->expansion_memory.portal_frames = RW12_RATE / 2U;
    storage->expansion_memory.spatial_left = rw12_spatial_l;
    storage->expansion_memory.spatial_right = rw12_spatial_r;
    storage->expansion_memory.spatial_frames = 64U;
    storage->world_memory.room_delay = rw12_room;
    storage->world_memory.room_delay_samples = 14000U;
    storage->world_memory.prop_delay = rw12_prop;
    storage->world_memory.prop_delay_samples = RW12_RATE + 64U;
}

static int rw12_dispatch_report(void)
{
    wsse89_event event;
    gv89_handle handle;
    memset(&event, 0, sizeof(event));
    event.type = WSSE89_EVENT_REPORT;
    event.seed = 0x520001U;
    gssr89_defaults(&event.data.report, GPAAH89_PRESET_ROCKET_LAUNCHER);
    event.data.report.pan_q15 = -7000;
    event.data.report.distance_q15 = 1200U;
    event.data.report.gain_q15 = 31000;
    event.data.report.output_gain_q15 = 30000;
    event.data.report.instance_key = 52001U;
    return wsse89_dispatch(&rw12_engine, &event, &handle) == WSSE89_OK;
}

static int rw12_dispatch_whistle_start(void)
{
    wsse89_event event;
    memset(&event, 0, sizeof(event));
    event.type = WSSE89_EVENT_ROCKET_WHISTLE_START;
    event.seed = 0x520002U;
    wsse89_rocket_whistle_defaults(&event.data.rocket_whistle_start);
    event.data.rocket_whistle_start.preset = GWH89_PRESET_RPG7_SUSTAINED;
    event.data.rocket_whistle_start.radial_velocity_mps = -95;
    event.data.rocket_whistle_start.distance_gain_q15 = 18000U;
    event.data.rocket_whistle_start.auto_hold_ms = 0U;
    event.data.rocket_whistle_start.gain_q15 = 14500;
    event.data.rocket_whistle_start.pan_q15 = -4500;
    event.data.rocket_whistle_start.instance_key = 52002U;
    if (wsse89_dispatch(&rw12_engine, &event, 0) != WSSE89_OK) return 0;

    memset(&event, 0, sizeof(event));
    event.type = WSSE89_EVENT_ROCKET_WHISTLE_FX;
    wsse89_rocket_whistle_fx_defaults(&event.data.rocket_whistle_fx);
    event.data.rocket_whistle_fx.instance_key = 52002U;
    event.data.rocket_whistle_fx.output_eq_mode = 2U;
    event.data.rocket_whistle_fx.output_eq_gain_q15[GWH89_OUTPUT_EQ_RUMBLE_120] = 1200U;
    event.data.rocket_whistle_fx.output_eq_gain_q15[GWH89_OUTPUT_EQ_LOW_120_300] = 9000U;
    event.data.rocket_whistle_fx.output_eq_gain_q15[GWH89_OUTPUT_EQ_CORE_300_900] = 25000U;
    event.data.rocket_whistle_fx.output_eq_gain_q15[GWH89_OUTPUT_EQ_WHISTLE_900_2400] = 33000U;
    event.data.rocket_whistle_fx.output_eq_gain_q15[GWH89_OUTPUT_EQ_EDGE_2400_6000] = 37000U;
    event.data.rocket_whistle_fx.output_eq_gain_q15[GWH89_OUTPUT_EQ_AIR_ABOVE_6000] = 34500U;
    event.data.rocket_whistle_fx.reverb_mode = 2U;
    event.data.rocket_whistle_fx.reverb_wet_q15 = 2100U;
    event.data.rocket_whistle_fx.reverb_feedback_q15 = 17000U;
    event.data.rocket_whistle_fx.reverb_damping_q15 = 8000U;
    event.data.rocket_whistle_fx.reverb_tail_ms = 210U;
    return wsse89_dispatch(&rw12_engine, &event, 0) == WSSE89_OK;
}

static int rw12_dispatch_motion(gv89_u32 frame)
{
    wsse89_event event;
    gv89_u32 start;
    gv89_u32 end;
    gv89_u32 elapsed;
    gv89_s32 radial;
    gv89_s32 pan;
    gv89_u32 distance;
    start = (RW12_RATE * 52U) / 100U;
    end = (RW12_RATE * 205U) / 100U;
    if (frame < start || frame >= end) return 1;
    elapsed = frame - start;
    radial = -95 + (gv89_s32)((220U * elapsed) / (end - start));
    pan = -4500 + (gv89_s32)((11500U * elapsed) / (end - start));
    if (elapsed < (end - start) / 2U)
        distance = 18000U + (11000U * elapsed) / ((end - start) / 2U);
    else
        distance = 29000U - (13000U * (elapsed - (end - start) / 2U)) /
                   ((end - start) - (end - start) / 2U);
    memset(&event, 0, sizeof(event));
    event.type = WSSE89_EVENT_ROCKET_WHISTLE_MOTION;
    event.data.rocket_whistle_motion.instance_key = 52002U;
    event.data.rocket_whistle_motion.radial_velocity_mps = radial;
    event.data.rocket_whistle_motion.distance_gain_q15 = (gv89_u16)distance;
    event.data.rocket_whistle_motion.gain_q15 = 14500;
    event.data.rocket_whistle_motion.pan_q15 = (gv89_s16)pan;
    return wsse89_dispatch(&rw12_engine, &event, 0) == WSSE89_OK;
}

static int rw12_dispatch_release(void)
{
    wsse89_event event;
    memset(&event, 0, sizeof(event));
    event.type = WSSE89_EVENT_ROCKET_WHISTLE_RELEASE;
    event.data.rocket_whistle_control.instance_key = 52002U;
    return wsse89_dispatch(&rw12_engine, &event, 0) == WSSE89_OK;
}

static int rw12_dispatch_blast(void)
{
    wsse89_event event;
    gv89_handle handle;
    memset(&event, 0, sizeof(event));
    event.type = WSSE89_EVENT_ROCKET_BLAST;
    event.seed = 0x520003U;
    gsso89_rocket_defaults(&event.data.rocket);
    event.data.rocket.preset_id = WSRB89_PRESET_COMPACT_RPG;
    event.data.rocket.velocity_q15 = 32767U;
    event.data.rocket.common.pan_q15 = 8000;
    event.data.rocket.common.distance_q15 = 4300U;
    event.data.rocket.common.gain_q15 = 32767;
    event.data.rocket.common.priority_bias = 420;
    event.data.rocket.common.instance_key = 52003U;
    return wsse89_dispatch(&rw12_engine, &event, &handle) == WSSE89_OK;
}

int main(void)
{
    wsse89_config config;
    wsse89_storage storage;
    gv89_u32 frame;
    gv89_s16 left;
    gv89_s16 right;
    wsse89_config_defaults(&config);
    config.sample_rate = RW12_RATE;
    config.logical_voice_capacity = RW12_LOGICAL;
    config.physical_voice_limit = 24U;
    config.seed = 0x520000U;
    rw12_storage(&storage);
    if (!wsse89_init(&rw12_engine, &config, &storage)) return 1;
    for (frame = 0U; frame < RW12_FRAMES; ++frame) {
        if (frame == (RW12_RATE * 40U) / 100U && !rw12_dispatch_report()) return 2;
        if (frame == (RW12_RATE * 52U) / 100U && !rw12_dispatch_whistle_start()) return 3;
        if (frame >= (RW12_RATE * 52U) / 100U &&
            frame < (RW12_RATE * 205U) / 100U && (frame & 127U) == 0U)
            if (!rw12_dispatch_motion(frame)) return 4;
        if (frame == (RW12_RATE * 205U) / 100U && !rw12_dispatch_release()) return 5;
        if (frame == (RW12_RATE * 220U) / 100U && !rw12_dispatch_blast()) return 6;
        left = 0;
        right = 0;
        wsse89_process_stereo_sample(&rw12_engine, &left, &right);
        rw12_pcm[frame * 2U] = left;
        rw12_pcm[frame * 2U + 1U] = right;
    }
    if (!rw12_write_wav("audio/23_rocket_whistle_v12_event_showcase.wav",
                        rw12_pcm, RW12_FRAMES)) return 7;
    printf("rocket_whistle_v12_event_showcase PASS context=%lu\n",
           (unsigned long)wsse89_context_bytes());
    return 0;
}
