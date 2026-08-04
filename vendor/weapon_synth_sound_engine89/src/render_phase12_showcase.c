#include <stdio.h>
#include "weapon_synth_sound_engine89.h"

#define P12_RATE 44100U
#define P12_LOGICAL 128U
#define P12_SEGMENT_SECONDS 5U
#define P12_SEGMENT_FRAMES (P12_RATE * P12_SEGMENT_SECONDS)

static gv89_voice p12_logical[P12_LOGICAL];
static gssr89_voice p12_report_voices[6];
static gsse89_casing_voice p12_casing[12];
static gsse89_fire_voice p12_fire[2];
static gsso89_bullet_voice p12_bullet[8];
static gsso89_grenade_voice p12_grenade[2];
static gsso89_rocket_voice p12_rocket[2];
static gssw89_projectile_voice p12_projectile[8];
static gssw89_impact_voice p12_impact[8];
static gssw89_ricochet_voice p12_ricochet[8];
static wsound89_i16 p12_outdoor[P12_RATE];
static wsound89_i16 p12_portal[P12_RATE / 2U];
static wsound89_i16 p12_spatial_l[128];
static wsound89_i16 p12_spatial_r[128];
static wsound89_i16 p12_world[14000];
static wsound89_i16 p12_direct[P12_RATE + 4096U];

static void p12_u16(FILE *file, unsigned int value)
{
    unsigned char b[2];
    b[0] = (unsigned char)(value & 255U);
    b[1] = (unsigned char)((value >> 8) & 255U);
    (void)fwrite(b, 1U, 2U, file);
}

static void p12_u32(FILE *file, unsigned long value)
{
    unsigned char b[4];
    b[0] = (unsigned char)(value & 255UL);
    b[1] = (unsigned char)((value >> 8) & 255UL);
    b[2] = (unsigned char)((value >> 16) & 255UL);
    b[3] = (unsigned char)((value >> 24) & 255UL);
    (void)fwrite(b, 1U, 4U, file);
}

static int p12_header(FILE *file, unsigned long frames)
{
    unsigned long bytes;
    bytes = frames * 4UL;
    if (fwrite("RIFF", 1U, 4U, file) != 4U) return 0;
    p12_u32(file, 36UL + bytes);
    if (fwrite("WAVEfmt ", 1U, 8U, file) != 8U) return 0;
    p12_u32(file, 16UL);
    p12_u16(file, 1U);
    p12_u16(file, 2U);
    p12_u32(file, P12_RATE);
    p12_u32(file, P12_RATE * 4UL);
    p12_u16(file, 4U);
    p12_u16(file, 16U);
    if (fwrite("data", 1U, 4U, file) != 4U) return 0;
    p12_u32(file, bytes);
    return 1;
}

static void p12_storage(wsse89_storage *s)
{
    s->logical_voices = p12_logical;
    s->report_voices = p12_report_voices;
    s->report_capacity = 6U;
    s->casing_voices = p12_casing;
    s->casing_capacity = 12U;
    s->fire_voices = p12_fire;
    s->fire_capacity = 2U;
    s->bullet_voices = p12_bullet;
    s->bullet_capacity = 8U;
    s->grenade_voices = p12_grenade;
    s->grenade_capacity = 2U;
    s->rocket_voices = p12_rocket;
    s->rocket_capacity = 2U;
    s->projectile_voices = p12_projectile;
    s->projectile_capacity = 8U;
    s->impact_voices = p12_impact;
    s->impact_capacity = 8U;
    s->ricochet_voices = p12_ricochet;
    s->ricochet_capacity = 8U;
    s->expansion_memory.outdoor = p12_outdoor;
    s->expansion_memory.outdoor_frames = P12_RATE;
    s->expansion_memory.portal = p12_portal;
    s->expansion_memory.portal_frames = P12_RATE / 2U;
    s->expansion_memory.spatial_left = p12_spatial_l;
    s->expansion_memory.spatial_right = p12_spatial_r;
    s->expansion_memory.spatial_frames = 128U;
    s->world_memory.room_delay = p12_world;
    s->world_memory.room_delay_samples = 14000U;
    s->world_memory.prop_delay = p12_direct;
    s->world_memory.prop_delay_samples = P12_RATE + 4096U;
}

static int p12_init(wsse89_context *ctx)
{
    wsse89_config config;
    wsse89_storage storage;
    wsse89_config_defaults(&config);
    config.sample_rate = P12_RATE;
    config.logical_voice_capacity = P12_LOGICAL;
    config.physical_voice_limit = 64U;
    config.expansion_mask = GSSEXP89_ALL;
    p12_storage(&storage);
    return wsse89_init(ctx, &config, &storage);
}

static void p12_set_path(wsse89_context *ctx, wsounda89_source source,
                         wsound89_u32 distance_cm, wsound89_i16 dot)
{
    wsse89_event event;
    event.type = WSSE89_EVENT_ACOUSTIC_PATH;
    event.seed = 0U;
    wsounda89_path_defaults(&event.data.acoustic_path);
    event.data.acoustic_path.distance_cm = distance_cm;
    event.data.acoustic_path.source_dot_q15 = dot;
    event.data.acoustic_path.source = source;
    event.data.acoustic_path.air_absorb_q15[0] = 90U;
    event.data.acoustic_path.air_absorb_q15[1] = 430U;
    event.data.acoustic_path.air_absorb_q15[2] = 1800U;
    (void)wsse89_dispatch(ctx, &event, 0);
}

static void p12_configure(wsse89_context *ctx, int acoustic,
                          wsounda89_profile profile, wsounda89_space space,
                          wsounda89_material material, int portal)
{
    wsse89_event event;
    event.type = WSSE89_EVENT_ACOUSTIC_ENABLE;
    event.seed = 0U;
    event.data.acoustic_enabled = acoustic ? 1U : 0U;
    (void)wsse89_dispatch(ctx, &event, 0);
    if (!acoustic) {
        gssw89_enable_post(&ctx->world, 0, 1, 0);
        gssw89_set_world_wet(&ctx->world, 9000);
        return;
    }
    event.type = WSSE89_EVENT_ACOUSTIC_PROFILE;
    event.data.acoustic_profile = profile;
    (void)wsse89_dispatch(ctx, &event, 0);
    event.type = WSSE89_EVENT_ACOUSTIC_MATERIAL;
    event.data.acoustic_material.material = material;
    event.data.acoustic_material.thickness_q15 = 24000U;
    (void)wsse89_dispatch(ctx, &event, 0);
    event.type = WSSE89_EVENT_ACOUSTIC_SPACE;
    event.data.acoustic_space = space;
    (void)wsse89_dispatch(ctx, &event, 0);
    event.type = WSSE89_EVENT_ACOUSTIC_PORTAL;
    wsounda89_portal_defaults(&event.data.acoustic_portal);
    if (portal) {
        event.data.acoustic_portal.delay_samples = P12_RATE / 80U;
        event.data.acoustic_portal.opening_q15 = 15500U;
        event.data.acoustic_portal.transmit_q15[0] = 26000U;
        event.data.acoustic_portal.transmit_q15[1] = 18500U;
        event.data.acoustic_portal.transmit_q15[2] = 9200U;
        event.data.acoustic_portal.pan_q15 = 19000;
    }
    (void)wsse89_dispatch(ctx, &event, 0);
}

static void p12_report(wsse89_context *ctx, gv89_u32 seed)
{
    wsse89_event event;
    gv89_handle handle;
    event.type = WSSE89_EVENT_REPORT;
    event.seed = seed;
    gssr89_defaults(&event.data.report, GPAAH89_PRESET_ROCKET_LAUNCHER);
    event.data.report.pan_q15 = -8500;
    event.data.report.gain_q15 = 24500;
    (void)wsse89_dispatch(ctx, &event, &handle);
}

static void p12_blast(wsse89_context *ctx, gv89_u32 seed)
{
    wsse89_event event;
    gv89_handle handle;
    event.type = WSSE89_EVENT_ROCKET_BLAST;
    event.seed = seed;
    gsso89_rocket_defaults(&event.data.rocket);
    event.data.rocket.common.pan_q15 = 10500;
    event.data.rocket.common.gain_q15 = 23500;
    (void)wsse89_dispatch(ctx, &event, &handle);
}

static void p12_write_silence(FILE *file, wsound89_u32 frames)
{
    wsound89_u32 i;
    for (i = 0U; i < frames; ++i) {
        p12_u16(file, 0U);
        p12_u16(file, 0U);
    }
}

static int p12_render_segment(FILE *file, int acoustic,
                              wsounda89_profile profile, wsounda89_space space,
                              wsounda89_material material, int portal,
                              wsound89_u32 distance_cm, gv89_u32 seed)
{
    wsse89_context ctx;
    wsound89_u32 frame;
    gv89_s16 left;
    gv89_s16 right;
    if (!p12_init(&ctx)) return 0;
    p12_configure(&ctx, acoustic, profile, space, material, portal);
    p12_set_path(&ctx, WSOUNDA89_SOURCE_REPORT, distance_cm, 25000);
    for (frame = 0U; frame < P12_SEGMENT_FRAMES; ++frame) {
        if (frame == P12_RATE / 3U) p12_report(&ctx, seed);
        if (frame == P12_RATE + P12_RATE * 2U / 5U) {
            p12_set_path(&ctx, WSOUNDA89_SOURCE_EXPLOSION,
                         distance_cm + 250U, 11000);
            p12_blast(&ctx, seed + 1U);
        }
        wsse89_process_stereo_sample(&ctx, &left, &right);
        p12_u16(file, (unsigned short)left);
        p12_u16(file, (unsigned short)right);
    }
    return 1;
}

int main(void)
{
    FILE *file;
    unsigned long ab_frames;
    unsigned long env_frames;
    ab_frames = (unsigned long)P12_SEGMENT_FRAMES * 2UL + (unsigned long)P12_RATE;
    env_frames = (unsigned long)P12_SEGMENT_FRAMES * 3UL;

    file = fopen("audio/14_phase12_ab_legacy_vs_hybrid.wav", "wb");
    if (file == 0) return 1;
    if (!p12_header(file, ab_frames)) { fclose(file); return 2; }
    if (!p12_render_segment(file, 0, WSOUNDA89_HYBRID, WSOUNDA89_SPACE_WAREHOUSE,
                            WSOUNDA89_MATERIAL_CONCRETE, 0, 0U, 700U)) {
        fclose(file); return 3;
    }
    p12_write_silence(file, P12_RATE);
    if (!p12_render_segment(file, 1, WSOUNDA89_HYBRID, WSOUNDA89_SPACE_WAREHOUSE,
                            WSOUNDA89_MATERIAL_CONCRETE, 0, 0U, 700U)) {
        fclose(file); return 4;
    }
    fclose(file);

    file = fopen("audio/15_phase12_environment_triptych.wav", "wb");
    if (file == 0) return 5;
    if (!p12_header(file, env_frames)) { fclose(file); return 6; }
    if (!p12_render_segment(file, 1, WSOUNDA89_HYBRID, WSOUNDA89_SPACE_WAREHOUSE,
                            WSOUNDA89_MATERIAL_CONCRETE, 0, 35U, 800U)) {
        fclose(file); return 7;
    }
    if (!p12_render_segment(file, 1, WSOUNDA89_HYBRID, WSOUNDA89_SPACE_URBAN,
                            WSOUNDA89_MATERIAL_BRICK, 1, 80U, 900U)) {
        fclose(file); return 8;
    }
    if (!p12_render_segment(file, 1, WSOUNDA89_REALISTIC, WSOUNDA89_SPACE_FIELD,
                            WSOUNDA89_MATERIAL_SOIL, 0, 150U, 1000U)) {
        fclose(file); return 9;
    }
    fclose(file);
    printf("phase12 previews rendered\n");
    return 0;
}
