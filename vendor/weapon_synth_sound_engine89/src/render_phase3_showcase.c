#include <stdio.h>
#include "weapon_synth_sound_engine89.h"

#define P3_RATE 44100U
#define P3_LOGICAL 64U
#define P3_SEGMENT_SECONDS 4U
#define P3_SEGMENT_FRAMES (P3_RATE * P3_SEGMENT_SECONDS)

static gv89_voice logical[P3_LOGICAL];
static gssr89_voice reports[8];
static gsse89_casing_voice casings[4];
static gsse89_fire_voice fires[1];
static gsso89_bullet_voice bullets[2];
static gsso89_grenade_voice grenades[1];
static gsso89_rocket_voice rockets[1];
static gssw89_projectile_voice projectiles[2];
static gssw89_impact_voice impacts[2];
static gssw89_ricochet_voice ricochets[2];
static wsound89_i16 outdoor[P3_RATE];
static wsound89_i16 portal[P3_RATE / 2U];
static wsound89_i16 spatial_l[64];
static wsound89_i16 spatial_r[64];
static wsound89_i16 room[16000];
static wsound89_i16 prop[P3_RATE + 4096U];

static void p3_u16(FILE *f, unsigned int v)
{
    unsigned char b[2];
    b[0] = (unsigned char)(v & 255U); b[1] = (unsigned char)((v >> 8) & 255U);
    (void)fwrite(b, 1U, 2U, f);
}

static void p3_u32(FILE *f, unsigned long v)
{
    unsigned char b[4];
    b[0] = (unsigned char)(v & 255UL); b[1] = (unsigned char)((v >> 8) & 255UL);
    b[2] = (unsigned char)((v >> 16) & 255UL); b[3] = (unsigned char)((v >> 24) & 255UL);
    (void)fwrite(b, 1U, 4U, f);
}

static int p3_header(FILE *f, unsigned long frames)
{
    unsigned long bytes;
    bytes = frames * 4UL;
    if (fwrite("RIFF", 1U, 4U, f) != 4U) return 0;
    p3_u32(f, 36UL + bytes);
    if (fwrite("WAVEfmt ", 1U, 8U, f) != 8U) return 0;
    p3_u32(f, 16UL); p3_u16(f, 1U); p3_u16(f, 2U);
    p3_u32(f, P3_RATE); p3_u32(f, P3_RATE * 4UL);
    p3_u16(f, 4U); p3_u16(f, 16U);
    if (fwrite("data", 1U, 4U, f) != 4U) return 0;
    p3_u32(f, bytes);
    return 1;
}

static void p3_storage(wsse89_storage *s)
{
    s->logical_voices = logical;
    s->report_voices = reports; s->report_capacity = 8U;
    s->casing_voices = casings; s->casing_capacity = 4U;
    s->fire_voices = fires; s->fire_capacity = 1U;
    s->bullet_voices = bullets; s->bullet_capacity = 2U;
    s->grenade_voices = grenades; s->grenade_capacity = 1U;
    s->rocket_voices = rockets; s->rocket_capacity = 1U;
    s->projectile_voices = projectiles; s->projectile_capacity = 2U;
    s->impact_voices = impacts; s->impact_capacity = 2U;
    s->ricochet_voices = ricochets; s->ricochet_capacity = 2U;
    s->expansion_memory.outdoor = outdoor; s->expansion_memory.outdoor_frames = P3_RATE;
    s->expansion_memory.portal = portal; s->expansion_memory.portal_frames = P3_RATE / 2U;
    s->expansion_memory.spatial_left = spatial_l; s->expansion_memory.spatial_right = spatial_r;
    s->expansion_memory.spatial_frames = 64U;
    s->world_memory.room_delay = room; s->world_memory.room_delay_samples = 16000U;
    s->world_memory.prop_delay = prop; s->world_memory.prop_delay_samples = P3_RATE + 4096U;
}

static int p3_init(wsse89_context *ctx)
{
    wsse89_config c;
    wsse89_storage s;
    wsse89_config_defaults(&c);
    c.sample_rate = P3_RATE; c.logical_voice_capacity = P3_LOGICAL;
    c.physical_voice_limit = 32U; c.expansion_mask = GSSEXP89_ALL;
    p3_storage(&s);
    return wsse89_init(ctx, &c, &s);
}

static int p3_render_segment(FILE *f, wsounddna89_profile_id id,
                             wsounddna89_mode mode, wsound89_u32 seed,
                             wsoundmetrics89_result *metrics,
                             wsound89_u16 *score)
{
    wsse89_context ctx;
    wsse89_event e;
    wsounddna89_profile profile;
    wsoundmetrics89_target target;
    gv89_handle h;
    gv89_s16 l;
    gv89_s16 r;
    wsound89_u32 frame;
    if (!p3_init(&ctx)) return 0;
    wsounddna89_profile_defaults(id, &profile);
    e.type = WSSE89_EVENT_WEAPON_PROFILE; e.seed = 0U; e.data.weapon_profile = profile;
    if (wsse89_dispatch(&ctx, &e, 0) != WSSE89_OK) return 0;
    e.type = WSSE89_EVENT_WEAPON_MODE; e.data.weapon_mode = mode;
    if (wsse89_dispatch(&ctx, &e, 0) != WSSE89_OK) return 0;
    e.type = WSSE89_EVENT_ACOUSTIC_SPACE; e.data.acoustic_space = WSOUNDA89_SPACE_WAREHOUSE;
    (void)wsse89_dispatch(&ctx, &e, 0);
    e.type = WSSE89_EVENT_WEAPON_FIRE; e.seed = seed;
    e.data.weapon_fire.pan_q15 = 0; e.data.weapon_fire.distance_q15 = 1200U;
    e.data.weapon_fire.occlusion_q15 = 32767U; e.data.weapon_fire.focus_q15 = 32767U;
    e.data.weapon_fire.gain_q15 = 25000; e.data.weapon_fire.priority_bias = 180;
    e.data.weapon_fire.instance_key = seed; e.data.weapon_fire.instance_limit = 8U;
    e.data.weapon_fire.pressure_energy_q15 = 0U;
    for (frame = 0U; frame < P3_SEGMENT_FRAMES; ++frame) {
        if (frame == P3_RATE / 3U)
            (void)wsse89_dispatch(&ctx, &e, &h);
        wsse89_process_stereo_sample(&ctx, &l, &r);
        p3_u16(f, (unsigned short)l); p3_u16(f, (unsigned short)r);
    }
    if (wsse89_get_metrics(&ctx, metrics) != WSOUND89_OK) return 0;
    wsoundmetrics89_target_defaults(&profile, mode, &target);
    *score = wsoundmetrics89_score_q15(metrics, &target);
    return 1;
}

int main(void)
{
    FILE *wav;
    FILE *json;
    wsoundmetrics89_result m[3];
    wsound89_u16 score[3];
    wav = fopen("audio/16_phase3_realistic_hybrid_cinematic.wav", "wb");
    if (wav == 0) return 1;
    if (!p3_header(wav, (unsigned long)P3_SEGMENT_FRAMES * 3UL)) return 2;
    if (!p3_render_segment(wav, WSOUNDDNA89_PROFILE_RIFLE, WSOUNDDNA89_MODE_REALISTIC, 6100U, &m[0], &score[0])) return 3;
    if (!p3_render_segment(wav, WSOUNDDNA89_PROFILE_RIFLE, WSOUNDDNA89_MODE_HYBRID, 6100U, &m[1], &score[1])) return 4;
    if (!p3_render_segment(wav, WSOUNDDNA89_PROFILE_RIFLE, WSOUNDDNA89_MODE_CINEMATIC, 6100U, &m[2], &score[2])) return 5;
    fclose(wav);
    json = fopen("PHASE3_METRICS_V1_6.json", "w");
    if (json == 0) return 6;
    fprintf(json, "{\n  \"profiles\": [\n");
    fprintf(json, "    {\"mode\":\"realistic\",\"peak\":%u,\"rms\":%u,\"crest_q8\":%u,\"low\":%u,\"mid\":%u,\"high\":%u,\"attack_ms_x10\":%u,\"decay_ms\":%u,\"clipped\":%lu,\"score_q15\":%u},\n", m[0].peak_abs,m[0].rms_q15,m[0].crest_q8,m[0].band_ratio_q15[0],m[0].band_ratio_q15[1],m[0].band_ratio_q15[2],m[0].attack_ms_x10,m[0].decay_ms,(unsigned long)m[0].clipped_samples,score[0]);
    fprintf(json, "    {\"mode\":\"hybrid\",\"peak\":%u,\"rms\":%u,\"crest_q8\":%u,\"low\":%u,\"mid\":%u,\"high\":%u,\"attack_ms_x10\":%u,\"decay_ms\":%u,\"clipped\":%lu,\"score_q15\":%u},\n", m[1].peak_abs,m[1].rms_q15,m[1].crest_q8,m[1].band_ratio_q15[0],m[1].band_ratio_q15[1],m[1].band_ratio_q15[2],m[1].attack_ms_x10,m[1].decay_ms,(unsigned long)m[1].clipped_samples,score[1]);
    fprintf(json, "    {\"mode\":\"cinematic\",\"peak\":%u,\"rms\":%u,\"crest_q8\":%u,\"low\":%u,\"mid\":%u,\"high\":%u,\"attack_ms_x10\":%u,\"decay_ms\":%u,\"clipped\":%lu,\"score_q15\":%u}\n", m[2].peak_abs,m[2].rms_q15,m[2].crest_q8,m[2].band_ratio_q15[0],m[2].band_ratio_q15[1],m[2].band_ratio_q15[2],m[2].attack_ms_x10,m[2].decay_ms,(unsigned long)m[2].clipped_samples,score[2]);
    fprintf(json, "  ]\n}\n");
    fclose(json);
    printf("phase3 preview rendered\n");
    return 0;
}
