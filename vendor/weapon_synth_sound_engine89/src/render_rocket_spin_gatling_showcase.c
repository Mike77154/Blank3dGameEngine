#include <stdio.h>
#include <string.h>
#include "weapon_synth_sound_engine89.h"

#define RGS_RATE 44100U
#define RGS_SECONDS 13U
#define RGS_FRAMES (RGS_RATE * RGS_SECONDS)
#define RGS_LOGICAL 256U
#define RGS_GATLING_SHOTS 80U
#define RGS_GATLING_STEP 882U

static wsse89_context engine;
static gv89_voice logical[RGS_LOGICAL];
static gssr89_voice reports[96];
static gsse89_casing_voice casings[64];
static gsse89_fire_voice fires[6];
static gsso89_bullet_voice bullets[4];
static gsso89_grenade_voice grenades[2];
static gsso89_rocket_voice rockets[3];
static gssw89_projectile_voice projectiles[4];
static gssw89_impact_voice impacts[4];
static gssw89_ricochet_voice ricochets[4];
static wsound89_i16 outdoor[RGS_RATE];
static wsound89_i16 portal[RGS_RATE / 2U];
static wsound89_i16 spatial_l[64];
static wsound89_i16 spatial_r[64];
static wsound89_i16 room[16000];
static wsound89_i16 prop[RGS_RATE + 64U];
static gv89_s16 pcm[RGS_FRAMES * 2U];
static gv89_handle flight_fire_handle;
static int flight_fire_valid;

static void rgs_u16le(FILE *f, unsigned int v)
{
    fputc((int)(v & 255U), f);
    fputc((int)((v >> 8) & 255U), f);
}

static void rgs_u32le(FILE *f, unsigned long v)
{
    fputc((int)(v & 255UL), f);
    fputc((int)((v >> 8) & 255UL), f);
    fputc((int)((v >> 16) & 255UL), f);
    fputc((int)((v >> 24) & 255UL), f);
}

static int rgs_write_wav_range(const char *path, const gv89_s16 *data,
                               gv89_u32 start, gv89_u32 frames)
{
    FILE *f;
    gv89_u32 i;
    unsigned long bytes;
    f = fopen(path, "wb");
    if (f == 0) return 0;
    bytes = (unsigned long)frames * 4UL;
    fwrite("RIFF", 1U, 4U, f); rgs_u32le(f, 36UL + bytes);
    fwrite("WAVEfmt ", 1U, 8U, f); rgs_u32le(f, 16UL);
    rgs_u16le(f, 1U); rgs_u16le(f, 2U); rgs_u32le(f, RGS_RATE);
    rgs_u32le(f, RGS_RATE * 4UL); rgs_u16le(f, 4U); rgs_u16le(f, 16U);
    fwrite("data", 1U, 4U, f); rgs_u32le(f, bytes);
    for (i = 0U; i < frames * 2U; ++i)
        rgs_u16le(f, (unsigned int)(unsigned short)data[start * 2U + i]);
    fclose(f);
    return 1;
}

static void rgs_storage(wsse89_storage *s)
{
    memset(s, 0, sizeof(*s));
    s->logical_voices = logical;
    s->report_voices = reports; s->report_capacity = 96U;
    s->casing_voices = casings; s->casing_capacity = 64U;
    s->fire_voices = fires; s->fire_capacity = 6U;
    s->bullet_voices = bullets; s->bullet_capacity = 4U;
    s->grenade_voices = grenades; s->grenade_capacity = 2U;
    s->rocket_voices = rockets; s->rocket_capacity = 3U;
    s->projectile_voices = projectiles; s->projectile_capacity = 4U;
    s->impact_voices = impacts; s->impact_capacity = 4U;
    s->ricochet_voices = ricochets; s->ricochet_capacity = 4U;
    s->expansion_memory.outdoor = outdoor;
    s->expansion_memory.outdoor_frames = RGS_RATE;
    s->expansion_memory.portal = portal;
    s->expansion_memory.portal_frames = RGS_RATE / 2U;
    s->expansion_memory.spatial_left = spatial_l;
    s->expansion_memory.spatial_right = spatial_r;
    s->expansion_memory.spatial_frames = 64U;
    s->world_memory.room_delay = room;
    s->world_memory.room_delay_samples = 16000U;
    s->world_memory.prop_delay = prop;
    s->world_memory.prop_delay_samples = RGS_RATE + 64U;
}

static int rgs_report(int preset, gv89_s16 gain, gv89_s16 pan,
                      gv89_u32 key, gv89_u32 seed)
{
    wsse89_event e;
    gv89_handle h;
    memset(&e, 0, sizeof(e));
    e.type = WSSE89_EVENT_REPORT;
    e.seed = seed;
    gssr89_defaults(&e.data.report, preset);
    e.data.report.gain_q15 = gain;
    e.data.report.output_gain_q15 = 32767;
    e.data.report.pan_q15 = pan;
    e.data.report.distance_q15 = 1000U;
    e.data.report.priority_bias = 420;
    e.data.report.instance_key = key;
    e.data.report.instance_limit = 32U;
    return wsse89_dispatch(&engine, &e, &h) == WSSE89_OK;
}

static int rgs_fire_start(gsse89_fire_role role, int preset, gv89_u32 duration,
                          gv89_s16 gain, gv89_s16 pan, gv89_u32 key,
                          gv89_handle *handle)
{
    wsse89_event e;
    memset(&e, 0, sizeof(e));
    e.type = WSSE89_EVENT_FIRE_START;
    e.seed = key ^ 0xF1AEU;
    gsse89_fire_defaults(&e.data.fire, role);
    e.data.fire.preset_id = preset;
    e.data.fire.duration_ms = duration;
    e.data.fire.release_ms = 100U;
    e.data.fire.gain_q15 = gain;
    e.data.fire.pan_q15 = pan;
    e.data.fire.distance_q15 = 1700U;
    e.data.fire.intensity_q15 = role == GSSE89_FIRE_ROLE_FLAMETHROWER ? 31000 : 16000;
    e.data.fire.airflow_q15 = role == GSSE89_FIRE_ROLE_FLAMETHROWER ? 28000 : 23500;
    e.data.fire.crackle_q15 = role == GSSE89_FIRE_ROLE_FLAMETHROWER ? 15000 : 5500;
    e.data.fire.size_q15 = role == GSSE89_FIRE_ROLE_FLAMETHROWER ? 24000 : 9000;
    e.data.fire.pressure_q15 = role == GSSE89_FIRE_ROLE_FLAMETHROWER ? 30000 : 11000;
    e.data.fire.drive_q15 = 17000;
    e.data.fire.brightness_q15 = 13500;
    e.data.fire.output_gain_q15 = 23000;
    e.data.fire.instance_key = key;
    e.data.fire.instance_limit = 4U;
    return wsse89_dispatch(&engine, &e, handle) == WSSE89_OK;
}

static int rgs_rocket_spin_start(void)
{
    wsse89_event e;
    memset(&e, 0, sizeof(e));
    e.type = WSSE89_EVENT_ROCKET_SPIN_START;
    e.seed = 0x190401U;
    wsse89_rocket_spin_defaults(&e.data.rocket_spin_start);
    e.data.rocket_spin_start.preset = GRS89_PRESET_RPG7_SUSTAINER;
    e.data.rocket_spin_start.gain_q15 = 5600;
    e.data.rocket_spin_start.pan_q15 = -4200;
    e.data.rocket_spin_start.instance_key = 190401U;
    return wsse89_dispatch(&engine, &e, 0) == WSSE89_OK;
}

static int rgs_whistle_start(void)
{
    wsse89_event e;
    memset(&e, 0, sizeof(e));
    e.type = WSSE89_EVENT_ROCKET_WHISTLE_START;
    e.seed = 0x190402U;
    wsse89_rocket_whistle_defaults(&e.data.rocket_whistle_start);
    e.data.rocket_whistle_start.preset = GWH89_PRESET_RPG7_SUSTAINED;
    e.data.rocket_whistle_start.radial_velocity_mps = -90;
    e.data.rocket_whistle_start.distance_gain_q15 = 18500U;
    e.data.rocket_whistle_start.auto_hold_ms = 0U;
    e.data.rocket_whistle_start.gain_q15 = 11800;
    e.data.rocket_whistle_start.pan_q15 = -4200;
    e.data.rocket_whistle_start.instance_key = 190402U;
    return wsse89_dispatch(&engine, &e, 0) == WSSE89_OK;
}

static int rgs_rocket_motion(gv89_u32 frame)
{
    wsse89_event e;
    gv89_u32 start;
    gv89_u32 end;
    gv89_u32 elapsed;
    gv89_s32 pan;
    gv89_s32 radial;
    gv89_u32 distance;
    gv89_s16 fire_pan;
    start = (RGS_RATE * 58U) / 100U;
    end = (RGS_RATE * 220U) / 100U;
    if (frame < start || frame >= end) return 1;
    elapsed = frame - start;
    pan = -4200 + (gv89_s32)((11200U * elapsed) / (end - start));
    radial = -90 + (gv89_s32)((205U * elapsed) / (end - start));
    if (elapsed < (end - start) / 2U)
        distance = 18500U + (11000U * elapsed) / ((end - start) / 2U);
    else
        distance = 29500U - (12500U * (elapsed - (end - start) / 2U)) /
                   ((end - start) - (end - start) / 2U);

    memset(&e, 0, sizeof(e));
    e.type = WSSE89_EVENT_ROCKET_WHISTLE_MOTION;
    e.data.rocket_whistle_motion.instance_key = 190402U;
    e.data.rocket_whistle_motion.radial_velocity_mps = radial;
    e.data.rocket_whistle_motion.distance_gain_q15 = (gv89_u16)distance;
    e.data.rocket_whistle_motion.gain_q15 = 11800;
    e.data.rocket_whistle_motion.pan_q15 = (gv89_s16)pan;
    if (wsse89_dispatch(&engine, &e, 0) != WSSE89_OK) { fprintf(stderr, "whistle motion fail frame=%lu\n", (unsigned long)frame); return 0; }

    memset(&e, 0, sizeof(e));
    e.type = WSSE89_EVENT_ROCKET_SPIN_MOTION;
    e.data.rocket_spin_motion.instance_key = 190401U;
    e.data.rocket_spin_motion.gain_q15 = 5600;
    e.data.rocket_spin_motion.pan_q15 = (gv89_s16)pan;
    if (wsse89_dispatch(&engine, &e, 0) != WSSE89_OK) { fprintf(stderr, "spin motion fail frame=%lu\n", (unsigned long)frame); return 0; }

    if (flight_fire_valid) {
        fire_pan = (gv89_s16)pan;
        memset(&e, 0, sizeof(e));
        e.type = WSSE89_EVENT_SET_HANDLE_SPATIAL;
        e.data.spatial.handle = flight_fire_handle;
        e.data.spatial.pan_q15 = fire_pan;
        e.data.spatial.distance_q15 = (gv89_u16)(32767U - distance / 3U);
        e.data.spatial.occlusion_q15 = 32767U;
        e.data.spatial.focus_q15 = 32767U;
        if (wsse89_dispatch(&engine, &e, 0) != WSSE89_OK) { fprintf(stderr, "fire spatial fail frame=%lu index=%u gen=%u\n", (unsigned long)frame, (unsigned)flight_fire_handle.index, (unsigned)flight_fire_handle.generation); return 0; }
    }
    return 1;
}

static int rgs_rocket_release(void)
{
    wsse89_event e;
    memset(&e, 0, sizeof(e));
    e.type = WSSE89_EVENT_ROCKET_WHISTLE_RELEASE;
    e.data.rocket_whistle_control.instance_key = 190402U;
    if (wsse89_dispatch(&engine, &e, 0) != WSSE89_OK) return 0;
    memset(&e, 0, sizeof(e));
    e.type = WSSE89_EVENT_ROCKET_SPIN_STOP;
    e.data.rocket_spin_control.instance_key = 190401U;
    return wsse89_dispatch(&engine, &e, 0) == WSSE89_OK;
}

static int rgs_rocket_blast(void)
{
    wsse89_event e;
    gv89_handle h;
    memset(&e, 0, sizeof(e));
    e.type = WSSE89_EVENT_ROCKET_BLAST;
    e.seed = 0x190403U;
    gsso89_rocket_defaults(&e.data.rocket);
    e.data.rocket.preset_id = WSRB89_PRESET_HEAVY_IMPACT;
    e.data.rocket.velocity_q15 = 32767U;
    e.data.rocket.common.pan_q15 = 7600;
    e.data.rocket.common.distance_q15 = 3900U;
    e.data.rocket.common.gain_q15 = 32767;
    e.data.rocket.common.priority_bias = 480;
    e.data.rocket.common.instance_key = 190403U;
    return wsse89_dispatch(&engine, &e, &h) == WSSE89_OK;
}

static int rgs_gatling_start(void)
{
    wsse89_event e;
    memset(&e, 0, sizeof(e));
    e.type = WSSE89_EVENT_AMMO;
    e.data.ammo.type = WSOUNDAMMO89_BELT_BOX;
    e.data.ammo.fill_q15 = 29000U;
    e.data.ammo.motion_q15 = 24500U;
    if (wsse89_dispatch(&engine, &e, 0) != WSSE89_OK) return 0;
    memset(&e, 0, sizeof(e));
    e.type = WSSE89_EVENT_GATLING_START;
    e.seed = 0x190501U;
    wsse89_gatling_defaults(&e.data.gatling_start);
    e.data.gatling_start.gain_q15 = 25500;
    e.data.gatling_start.motor_gain_q15 = 5600;
    e.data.gatling_start.rotator_gain_q15 = 6200;
    e.data.gatling_start.whistle_gain_q15 = 3400;
    e.data.gatling_start.pan_q15 = -800;
    e.data.gatling_start.instance_key = 190501U;
    return wsse89_dispatch(&engine, &e, 0) == WSSE89_OK;
}

static int rgs_gatling_fire_start(void)
{
    wsse89_event e;
    memset(&e, 0, sizeof(e));
    e.type = WSSE89_EVENT_BELT_START;
    e.data.belt.rpm = 3000U;
    e.data.belt.tension_q15 = 27000U;
    if (wsse89_dispatch(&engine, &e, 0) != WSSE89_OK) return 0;
    memset(&e, 0, sizeof(e));
    e.type = WSSE89_EVENT_GATLING_FIRE_START;
    e.data.gatling_fire.instance_key = 190501U;
    e.data.gatling_fire.firing_load_q15 = 23000;
    return wsse89_dispatch(&engine, &e, 0) == WSSE89_OK;
}

static int rgs_gatling_shot(gv89_u32 shot)
{
    wsse89_event e;
    gv89_handle h;
    gv89_s16 pan;
    pan = (gv89_s16)(-1700 + (gv89_s16)(shot % 9U) * 360);
    if (!rgs_report(GPAAH89_PRESET_GATLING,
                    (gv89_s16)(28700 - (gv89_s16)(shot & 3U) * 220),
                    pan, 190600U, 0x190600U + shot)) {
        fprintf(stderr, "gatling report fail shot=%lu\n", (unsigned long)shot);
        return 0;
    }
    memset(&e, 0, sizeof(e));
    e.type = WSSE89_EVENT_CASING;
    e.seed = 0x190800U + shot;
    gsse89_casing_defaults(&e.data.casing);
    e.data.casing.shell = GT89_SHELL_RIFLE_BRASS;
    e.data.casing.surface = GT89_SURFACE_METAL;
    e.data.casing.velocity = (gt89_u8)(185U + (shot & 15U));
    e.data.casing.angular_velocity = (gt89_u8)(205U + (shot & 31U));
    e.data.casing.pan_q15 = (gv89_s16)(5200 + (gv89_s16)(shot % 4U) * 350);
    e.data.casing.distance_q15 = 1500U;
    e.data.casing.gain_q15 = 7700;
    e.data.casing.instance_key = 190800U + shot;
    e.data.casing.instance_limit = 32U;
    if (wsse89_dispatch(&engine, &e, &h) != WSSE89_OK) {
        fprintf(stderr, "gatling casing fail shot=%lu\n", (unsigned long)shot);
        return 0;
    }
    memset(&e, 0, sizeof(e));
    e.type = WSSE89_EVENT_RECEIVER_EXCITE;
    e.data.receiver_impulse = 7200;
    if (wsse89_dispatch(&engine, &e, 0) != WSSE89_OK) {
        fprintf(stderr, "gatling receiver fail shot=%lu\n", (unsigned long)shot);
        return 0;
    }
    return 1;
}

static int rgs_gatling_stop(void)
{
    wsse89_event e;
    memset(&e, 0, sizeof(e));
    e.type = WSSE89_EVENT_BELT_STOP;
    if (wsse89_dispatch(&engine, &e, 0) != WSSE89_OK) return 0;
    memset(&e, 0, sizeof(e));
    e.type = WSSE89_EVENT_GATLING_FIRE_STOP;
    e.data.gatling_control.instance_key = 190501U;
    return wsse89_dispatch(&engine, &e, 0) == WSSE89_OK;
}

static void rgs_scale_no_clip(void)
{
    gv89_u32 i;
    gv89_s32 peak;
    gv89_s32 a;
    gv89_s32 scale;
    peak = 0;
    for (i = 0U; i < RGS_FRAMES * 2U; ++i) {
        a = pcm[i] < 0 ? -(gv89_s32)pcm[i] : (gv89_s32)pcm[i];
        if (a > peak) peak = a;
    }
    if (peak <= 30000 || peak == 0) return;
    scale = (30000L << 15) / peak;
    for (i = 0U; i < RGS_FRAMES * 2U; ++i)
        pcm[i] = (gv89_s16)(((gv89_s32)pcm[i] * scale) >> 15);
}

int main(void)
{
    wsse89_config config;
    wsse89_storage storage;
    gv89_u32 frame;
    gv89_u32 rocket_launch;
    gv89_u32 flight_start;
    gv89_u32 flight_end;
    gv89_u32 impact;
    gv89_u32 gatling_start;
    gv89_u32 gatling_fire;
    gv89_u32 gatling_end;
    gv89_u32 shot;
    gv89_s16 left;
    gv89_s16 right;
    gv89_handle launch_fire;
    wsse89_event pressure;
    wsse89_event expansion;

    rocket_launch = (RGS_RATE * 35U) / 100U;
    flight_start = (RGS_RATE * 58U) / 100U;
    flight_end = (RGS_RATE * 220U) / 100U;
    impact = (RGS_RATE * 235U) / 100U;
    gatling_start = RGS_RATE * 5U;
    gatling_fire = gatling_start + (RGS_RATE * 75U) / 100U;
    gatling_end = gatling_fire + RGS_GATLING_SHOTS * RGS_GATLING_STEP;
    flight_fire_valid = 0;

    wsse89_config_defaults(&config);
    config.sample_rate = RGS_RATE;
    config.logical_voice_capacity = RGS_LOGICAL;
    config.physical_voice_limit = 128U;
    config.seed = 0x190000U;
    rgs_storage(&storage);
    if (!wsse89_init(&engine, &config, &storage)) return 1;

    for (frame = 0U; frame < RGS_FRAMES; ++frame) {
        if (frame == rocket_launch) {
            if (!rgs_report(GPAAH89_PRESET_ROCKET_LAUNCHER, 32767, -5200,
                            190301U, 0x190301U)) return 2;
            if (!rgs_fire_start(GSSE89_FIRE_ROLE_FLAMETHROWER,
                                GFIRE89_PRESET_FLAMETHROWER, 360U, 16500,
                                -5200, 190302U, &launch_fire)) return 3;
            memset(&pressure, 0, sizeof(pressure));
            pressure.type = WSSE89_EVENT_PRESSURE_TRIGGER;
            pressure.seed = 0x190305U;
            pressure.data.pressure.kind = WSOUNDA89_PRESSURE_ROCKET;
            pressure.data.pressure.energy_q15 = 32767U;
            pressure.data.pressure.pan_q15 = -5200;
            if (wsse89_dispatch(&engine, &pressure, 0) != WSSE89_OK) return 19;
            memset(&expansion, 0, sizeof(expansion));
            expansion.type = WSSE89_EVENT_EXPANSION_SHOT;
            expansion.seed = 0x190306U;
            expansion.data.expansion_shot.device = WSOUNDMUZZLEDEVICE89_BARE;
            expansion.data.expansion_shot.energy_q15 = 32767U;
            expansion.data.expansion_shot.distance_q15 = 900U;
            if (wsse89_dispatch(&engine, &expansion, 0) != WSSE89_OK) return 20;
        }
        if (frame == flight_start) {
            if (!rgs_whistle_start()) return 4;
            if (!rgs_rocket_spin_start()) return 5;
            if (!rgs_fire_start(GSSE89_FIRE_ROLE_AMBIENCE,
                                GFIRE89_PRESET_TORCH, 1700U, 2900,
                                -4200, 190303U, &flight_fire_handle)) return 6;
            flight_fire_valid = 1;
        }
        if (frame >= flight_start && frame < flight_end &&
            (frame & 127U) == 0U)
            if (!rgs_rocket_motion(frame)) return 7;
        if (frame == flight_end) {
            if (!rgs_rocket_release()) return 8;
        }
        if (frame == impact) {
            if (!rgs_rocket_blast()) return 9;
            if (!rgs_fire_start(GSSE89_FIRE_ROLE_EXPLOSIVE_DEBRIS,
                                GFIRE89_PRESET_DEBRIS, 1450U, 7200,
                                7600, 190304U, &launch_fire)) return 10;
        }

        if (frame == gatling_start) {
            if (!rgs_gatling_start()) return 11;
        }
        if (frame == gatling_fire) {
            if (!rgs_gatling_fire_start()) return 12;
        }
        if (frame >= gatling_fire && frame < gatling_end &&
            ((frame - gatling_fire) % RGS_GATLING_STEP) == 0U) {
            shot = (frame - gatling_fire) / RGS_GATLING_STEP;
            if (shot < RGS_GATLING_SHOTS && !rgs_gatling_shot(shot)) return 13;
        }
        if (frame == gatling_end) {
            if (!rgs_gatling_stop()) return 14;
        }
        if (frame == gatling_end + RGS_RATE * 2U) {
            wsse89_event e;
            memset(&e, 0, sizeof(e));
            e.type = WSSE89_EVENT_AMMO;
            e.data.ammo.type = WSOUNDAMMO89_BELT_BOX;
            e.data.ammo.fill_q15 = 9000U;
            e.data.ammo.motion_q15 = 21000U;
            if (wsse89_dispatch(&engine, &e, 0) != WSSE89_OK) return 15;
        }

        left = 0;
        right = 0;
        wsse89_process_stereo_sample(&engine, &left, &right);
        pcm[frame * 2U] = left;
        pcm[frame * 2U + 1U] = right;
    }

    rgs_scale_no_clip();
    if (!rgs_write_wav_range("audio/24_rocket_propulsion_spin_fire.wav",
                             pcm, 0U, RGS_RATE * 4U)) return 16;
    if (!rgs_write_wav_range("audio/25_gatling_complete_motor_bed.wav",
                             pcm, RGS_RATE * 4U, RGS_RATE * 7U)) return 17;
    if (!rgs_write_wav_range("audio/26_rocket_and_gatling_showcase.wav",
                             pcm, 0U, RGS_FRAMES)) return 18;
    printf("rocket_spin_gatling_showcase PASS context=%lu shots=%u\n",
           (unsigned long)wsse89_context_bytes(),
           (unsigned int)RGS_GATLING_SHOTS);
    return 0;
}
