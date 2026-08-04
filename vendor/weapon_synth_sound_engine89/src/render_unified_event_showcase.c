#include <stdio.h>
#include "weapon_synth_sound_engine89.h"

#define PREVIEW_RATE 44100U
#define PREVIEW_SECONDS 12U
#define PREVIEW_FRAMES (PREVIEW_RATE * PREVIEW_SECONDS)
#define PREVIEW_LOGICAL 128U

static gv89_voice logical_voices[PREVIEW_LOGICAL];
static gssr89_voice report_voices[6];
static gsse89_casing_voice casing_voices[24];
static gsse89_fire_voice fire_voices[4];
static gsso89_bullet_voice bullet_voices[16];
static gsso89_grenade_voice grenade_voices[4];
static gsso89_rocket_voice rocket_voices[4];
static gssw89_projectile_voice projectile_voices[16];
static gssw89_impact_voice impact_voices[16];
static gssw89_ricochet_voice ricochet_voices[16];
static wsound89_i16 expansion_outdoor[PREVIEW_RATE];
static wsound89_i16 expansion_portal[PREVIEW_RATE / 2U];
static wsound89_i16 expansion_spatial_l[128];
static wsound89_i16 expansion_spatial_r[128];
static wsound89_i16 world_room[12000];
static wsound89_i16 world_prop[PREVIEW_RATE + 4U];

static void put_u16(FILE *file, unsigned int value)
{
    unsigned char bytes[2];
    bytes[0] = (unsigned char)(value & 255U);
    bytes[1] = (unsigned char)((value >> 8) & 255U);
    (void)fwrite(bytes, 1U, 2U, file);
}

static void put_u32(FILE *file, unsigned long value)
{
    unsigned char bytes[4];
    bytes[0] = (unsigned char)(value & 255UL);
    bytes[1] = (unsigned char)((value >> 8) & 255UL);
    bytes[2] = (unsigned char)((value >> 16) & 255UL);
    bytes[3] = (unsigned char)((value >> 24) & 255UL);
    (void)fwrite(bytes, 1U, 4U, file);
}

static int write_header(FILE *file, unsigned long frames)
{
    unsigned long data_bytes;
    data_bytes = frames * 4UL;
    if (fwrite("RIFF", 1U, 4U, file) != 4U) return 0;
    put_u32(file, 36UL + data_bytes);
    if (fwrite("WAVEfmt ", 1U, 8U, file) != 8U) return 0;
    put_u32(file, 16UL);
    put_u16(file, 1U);
    put_u16(file, 2U);
    put_u32(file, PREVIEW_RATE);
    put_u32(file, PREVIEW_RATE * 4UL);
    put_u16(file, 4U);
    put_u16(file, 16U);
    if (fwrite("data", 1U, 4U, file) != 4U) return 0;
    put_u32(file, data_bytes);
    return 1;
}

static void setup_storage(wsse89_storage *storage)
{
    storage->logical_voices = logical_voices;
    storage->report_voices = report_voices;
    storage->report_capacity = 6U;
    storage->casing_voices = casing_voices;
    storage->casing_capacity = 24U;
    storage->fire_voices = fire_voices;
    storage->fire_capacity = 4U;
    storage->bullet_voices = bullet_voices;
    storage->bullet_capacity = 16U;
    storage->grenade_voices = grenade_voices;
    storage->grenade_capacity = 4U;
    storage->rocket_voices = rocket_voices;
    storage->rocket_capacity = 4U;
    storage->projectile_voices = projectile_voices;
    storage->projectile_capacity = 16U;
    storage->impact_voices = impact_voices;
    storage->impact_capacity = 16U;
    storage->ricochet_voices = ricochet_voices;
    storage->ricochet_capacity = 16U;
    storage->expansion_memory.outdoor = expansion_outdoor;
    storage->expansion_memory.outdoor_frames = PREVIEW_RATE;
    storage->expansion_memory.portal = expansion_portal;
    storage->expansion_memory.portal_frames = PREVIEW_RATE / 2U;
    storage->expansion_memory.spatial_left = expansion_spatial_l;
    storage->expansion_memory.spatial_right = expansion_spatial_r;
    storage->expansion_memory.spatial_frames = 128U;
    storage->world_memory.room_delay = world_room;
    storage->world_memory.room_delay_samples = 12000U;
    storage->world_memory.prop_delay = world_prop;
    storage->world_memory.prop_delay_samples = PREVIEW_RATE + 4U;
}

static void dispatch_report(wsse89_context *ctx, gpaah89_preset_id preset,
                            gv89_s16 pan, gv89_u32 seed)
{
    wsse89_event event;
    gv89_handle handle;
    event.type = WSSE89_EVENT_REPORT;
    event.seed = seed;
    gssr89_defaults(&event.data.report, preset);
    event.data.report.pan_q15 = pan;
    (void)wsse89_dispatch(ctx, &event, &handle);
    event.type = WSSE89_EVENT_EXPANSION_SHOT;
    event.seed = seed + 99U;
    event.data.expansion_shot.device = preset == GPAAH89_PRESET_SNIPER_RIFLE ?
        WSOUNDMUZZLEDEVICE89_BRAKE : WSOUNDMUZZLEDEVICE89_BARE;
    event.data.expansion_shot.energy_q15 = 29000U;
    event.data.expansion_shot.distance_q15 = 1800U;
    (void)wsse89_dispatch(ctx, &event, 0);
}

static void schedule_event(wsse89_context *ctx, gv89_u32 frame)
{
    wsse89_event event;
    gv89_handle handle;
    gv89_u32 base;
    base = PREVIEW_RATE;
    if (frame == 0U) dispatch_report(ctx, GPAAH89_PRESET_PISTOL, -10000, 10U);
    if (frame == base / 2U) {
        event.type = WSSE89_EVENT_CASING;
        event.seed = 20U;
        gsse89_casing_defaults(&event.data.casing);
        event.data.casing.pan_q15 = -14000;
        (void)wsse89_dispatch(ctx, &event, &handle);
    }
    if (frame == base + base / 2U) dispatch_report(ctx, GPAAH89_PRESET_SHOTGUN, 7000, 30U);
    if (frame == base * 2U) {
        event.type = WSSE89_EVENT_PROJECTILE;
        event.seed = 40U;
        gssw89_projectile_defaults(&event.data.projectile);
        event.data.projectile.common.pan_q15 = 18000;
        (void)wsse89_dispatch(ctx, &event, &handle);
    }
    if (frame == base * 2U + base * 2U / 5U) {
        event.type = WSSE89_EVENT_IMPACT;
        event.seed = 50U;
        gssw89_impact_defaults(&event.data.impact);
        event.data.impact.material = WSOUNDIMPACT89_METAL;
        event.data.impact.common.pan_q15 = 20000;
        (void)wsse89_dispatch(ctx, &event, &handle);
    }
    if (frame == base * 2U + base * 4U / 5U) {
        event.type = WSSE89_EVENT_RICOCHET;
        event.seed = 60U;
        gssw89_ricochet_defaults(&event.data.ricochet);
        event.data.ricochet.common.pan_q15 = -18000;
        (void)wsse89_dispatch(ctx, &event, &handle);
    }
    if (frame >= base * 3U + base / 3U && frame < base * 4U + base / 3U &&
        ((frame - (base * 3U + base / 3U)) % (base / 8U)) == 0U)
        dispatch_report(ctx, GPAAH89_PRESET_METRALLA,
                        (frame & 1U) ? -12000 : 12000, 100U + frame);
    if (frame == base * 5U) {
        event.type = WSSE89_EVENT_GRENADE_BLAST;
        event.seed = 70U;
        gsso89_grenade_defaults(&event.data.grenade);
        event.data.grenade.common.pan_q15 = -5000;
        (void)wsse89_dispatch(ctx, &event, &handle);
    }
    if (frame == base * 6U) {
        dispatch_report(ctx, GPAAH89_PRESET_ROCKET_LAUNCHER, -7000, 79U);
    }
    if (frame == base * 7U + base / 4U) {
        event.type = WSSE89_EVENT_ROCKET_BLAST;
        event.seed = 80U;
        gsso89_rocket_defaults(&event.data.rocket);
        event.data.rocket.common.pan_q15 = 9000;
        event.data.rocket.common.distance_q15 = 6200U;
        (void)wsse89_dispatch(ctx, &event, &handle);
    }
    if (frame == base * 8U) {
        event.type = WSSE89_EVENT_FIRE_START;
        event.seed = 90U;
        gsse89_fire_defaults(&event.data.fire, GSSE89_FIRE_ROLE_FLAMETHROWER);
        event.data.fire.duration_ms = 1500U;
        event.data.fire.pan_q15 = -4000;
        (void)wsse89_dispatch(ctx, &event, &handle);
        event.type = WSSE89_EVENT_AERO_START;
        event.seed = 91U;
        event.data.aero.mode = WSOUNDAERO89_NEAR_CAMERA;
        event.data.aero.speed_q15 = 26000U;
        event.data.aero.size_q15 = 18000U;
        event.data.aero.duration_frames = PREVIEW_RATE;
        (void)wsse89_dispatch(ctx, &event, 0);
    }
    if (frame == base * 9U + base / 2U) {
        event.type = WSSE89_EVENT_PARTICLES;
        event.seed = 0U;
        event.data.particles.material = WSOUNDPARTICLES89_GRAVEL;
        event.data.particles.density_q15 = 22000U;
        event.data.particles.energy_q15 = 25000U;
        (void)wsse89_dispatch(ctx, &event, 0);
        event.type = WSSE89_EVENT_AMMO;
        event.data.ammo.type = WSOUNDAMMO89_LOOSE_SHELLS;
        event.data.ammo.fill_q15 = 18000U;
        event.data.ammo.motion_q15 = 25000U;
        (void)wsse89_dispatch(ctx, &event, 0);
    }
    if (frame == base * 10U + base / 2U)
        dispatch_report(ctx, GPAAH89_PRESET_SNIPER_RIFLE, 0, 120U);
}

int main(void)
{
    wsse89_context ctx;
    wsse89_config config;
    wsse89_storage storage;
    gv89_u32 frame;
    gv89_s16 left;
    gv89_s16 right;
    FILE *file;
    gv89_stats stats;

    wsse89_config_defaults(&config);
    config.sample_rate = PREVIEW_RATE;
    config.logical_voice_capacity = PREVIEW_LOGICAL;
    config.physical_voice_limit = 64U;
    config.expansion_mask = GSSEXP89_ALL;
    setup_storage(&storage);
    if (!wsse89_init(&ctx, &config, &storage)) return 1;
    gssw89_enable_post(&ctx.world, 0, 1, 0);
    gssw89_set_world_wet(&ctx.world, 8500);

    file = fopen("audio/00_v1_6_0_unified_event_showcase.wav", "wb");
    if (file == 0) return 2;
    if (!write_header(file, PREVIEW_FRAMES)) { fclose(file); return 3; }
    for (frame = 0U; frame < PREVIEW_FRAMES; ++frame) {
        schedule_event(&ctx, frame);
        wsse89_process_stereo_sample(&ctx, &left, &right);
        put_u16(file, (unsigned short)left);
        put_u16(file, (unsigned short)right);
    }
    fclose(file);
    wsse89_get_stats(&ctx, &stats);
    printf("unified preview: starts=%u steals=%u rejects=%u peak_logical=%u peak_physical=%u limiter=%u\n",
           stats.starts, stats.steals, stats.rejects, stats.peak_logical,
           stats.peak_physical, stats.limiter_hits);
    return 0;
}
