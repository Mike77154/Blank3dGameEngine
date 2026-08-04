#include "weapon_synth_sound_engine89.h"

#define T3_RATE 44100U
#define T3_LOGICAL 32U

static gv89_voice logical[T3_LOGICAL];
static gssr89_voice reports[4];
static gsse89_casing_voice casings[2];
static gsse89_fire_voice fires[1];
static gsso89_bullet_voice bullets[2];
static gsso89_grenade_voice grenades[1];
static gsso89_rocket_voice rockets[1];
static gssw89_projectile_voice projectiles[2];
static gssw89_impact_voice impacts[2];
static gssw89_ricochet_voice ricochets[2];
static wsound89_i16 outdoor[T3_RATE];
static wsound89_i16 portal[T3_RATE / 2U];
static wsound89_i16 spatial_l[64];
static wsound89_i16 spatial_r[64];
static wsound89_i16 room[16000];
static wsound89_i16 prop[T3_RATE + 4096U];

static void t3_storage(wsse89_storage *s)
{
    s->logical_voices = logical;
    s->report_voices = reports; s->report_capacity = 4U;
    s->casing_voices = casings; s->casing_capacity = 2U;
    s->fire_voices = fires; s->fire_capacity = 1U;
    s->bullet_voices = bullets; s->bullet_capacity = 2U;
    s->grenade_voices = grenades; s->grenade_capacity = 1U;
    s->rocket_voices = rockets; s->rocket_capacity = 1U;
    s->projectile_voices = projectiles; s->projectile_capacity = 2U;
    s->impact_voices = impacts; s->impact_capacity = 2U;
    s->ricochet_voices = ricochets; s->ricochet_capacity = 2U;
    s->expansion_memory.outdoor = outdoor; s->expansion_memory.outdoor_frames = T3_RATE;
    s->expansion_memory.portal = portal; s->expansion_memory.portal_frames = T3_RATE / 2U;
    s->expansion_memory.spatial_left = spatial_l;
    s->expansion_memory.spatial_right = spatial_r;
    s->expansion_memory.spatial_frames = 64U;
    s->world_memory.room_delay = room; s->world_memory.room_delay_samples = 16000U;
    s->world_memory.prop_delay = prop; s->world_memory.prop_delay_samples = T3_RATE + 4096U;
}

int main(void)
{
    wsounddna89_context a;
    wsounddna89_context b;
    wsounddna89_profile profile;
    wsounddna89_shot sa;
    wsounddna89_shot sb;
    wsound89_i32 correlation;
    wsound89_u16 i;
    wsse89_context engine;
    wsse89_config config;
    wsse89_storage storage;
    wsse89_event event;
    gv89_handle handle;
    gv89_s16 l;
    gv89_s16 r;
    wsoundmetrics89_result metrics;
    wsoundmetrics89_target target;
    wsound89_u16 score;

    if (wsounddna89_init(&a, 77U) != WSOUND89_OK) return 1;
    if (wsounddna89_init(&b, 77U) != WSOUND89_OK) return 2;
    wsounddna89_profile_defaults(WSOUNDDNA89_PROFILE_RIFLE, &profile);
    if (wsounddna89_set_profile(&a, &profile) != WSOUND89_OK) return 3;
    if (wsounddna89_set_profile(&b, &profile) != WSOUND89_OK) return 4;
    if (wsounddna89_set_mode(&a, WSOUNDDNA89_MODE_HYBRID) != WSOUND89_OK) return 5;
    if (wsounddna89_set_mode(&b, WSOUNDDNA89_MODE_HYBRID) != WSOUND89_OK) return 6;
    correlation = 0;
    for (i = 0U; i < 16U; ++i) {
        if (wsounddna89_next_profiled(&a, &sa) != WSOUND89_OK) return 7;
        if (wsounddna89_next_profiled(&b, &sb) != WSOUND89_OK) return 8;
        if (sa.seed != sb.seed || sa.energy_q15 != sb.energy_q15 ||
            sa.pressure_q15 != sb.pressure_q15 || sa.pitch_q16 != sb.pitch_q16)
            return 9;
        correlation += ((wsound89_i32)sa.energy_variation_q15 *
                        (wsound89_i32)sa.powder_variation_q15) / 256;
    }
    if (correlation <= 0) return 10;

    wsse89_config_defaults(&config);
    config.sample_rate = T3_RATE;
    config.logical_voice_capacity = T3_LOGICAL;
    config.physical_voice_limit = 16U;
    t3_storage(&storage);
    if (!wsse89_init(&engine, &config, &storage)) return 11;
    event.type = WSSE89_EVENT_WEAPON_PROFILE; event.seed = 0U;
    event.data.weapon_profile = profile;
    if (wsse89_dispatch(&engine, &event, 0) != WSSE89_OK) return 12;
    event.type = WSSE89_EVENT_WEAPON_MODE;
    event.data.weapon_mode = WSOUNDDNA89_MODE_HYBRID;
    if (wsse89_dispatch(&engine, &event, 0) != WSSE89_OK) return 13;
    event.type = WSSE89_EVENT_WEAPON_FIRE; event.seed = 999U;
    event.data.weapon_fire.pan_q15 = 0;
    event.data.weapon_fire.distance_q15 = 1000U;
    event.data.weapon_fire.occlusion_q15 = 32767U;
    event.data.weapon_fire.focus_q15 = 32767U;
    event.data.weapon_fire.gain_q15 = 26000;
    event.data.weapon_fire.priority_bias = 180;
    event.data.weapon_fire.instance_key = 44U;
    event.data.weapon_fire.instance_limit = 8U;
    event.data.weapon_fire.pressure_energy_q15 = 0U;
    if (wsse89_dispatch(&engine, &event, &handle) != WSSE89_OK) return 14;
    for (i = 0U; i < 2U; ++i) {
        wsound89_u32 frame;
        for (frame = 0U; frame < T3_RATE; ++frame)
            wsse89_process_stereo_sample(&engine, &l, &r);
    }
    if (!wsse89_get_last_dna(&engine, &sa)) return 15;
    if (wsse89_get_metrics(&engine, &metrics) != WSOUND89_OK) return 16;
    if (metrics.peak_abs == 0U || metrics.rms_q15 == 0U) return 17;
    wsoundmetrics89_target_defaults(&profile, WSOUNDDNA89_MODE_HYBRID, &target);
    score = wsoundmetrics89_score_q15(&metrics, &target);
    if (score < 12000U) return 18;
    return 0;
}
