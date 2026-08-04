#include <stdio.h>
#include "weapon_synth_sound_engine89.h"

#define TEST_RATE 44100U
#define TEST_LOGICAL 64U

static gv89_voice logical_voices[TEST_LOGICAL];
static gssr89_voice report_voices[2];
static gsse89_casing_voice casing_voices[8];
static gsse89_fire_voice fire_voices[2];
static gsso89_bullet_voice bullet_voices[8];
static gsso89_grenade_voice grenade_voices[2];
static gsso89_rocket_voice rocket_voices[2];
static gssw89_projectile_voice projectile_voices[8];
static gssw89_impact_voice impact_voices[8];
static gssw89_ricochet_voice ricochet_voices[8];
static wsound89_i16 expansion_outdoor[TEST_RATE];
static wsound89_i16 expansion_portal[TEST_RATE / 2U];
static wsound89_i16 expansion_spatial_l[64];
static wsound89_i16 expansion_spatial_r[64];
static wsound89_i16 world_room[10000];
static wsound89_i16 world_prop[TEST_RATE + 4U];
static gv89_s16 render_buffer[2048];

static void fill_storage(wsse89_storage *storage)
{
    storage->logical_voices = logical_voices;
    storage->report_voices = report_voices;
    storage->report_capacity = 2U;
    storage->casing_voices = casing_voices;
    storage->casing_capacity = 8U;
    storage->fire_voices = fire_voices;
    storage->fire_capacity = 2U;
    storage->bullet_voices = bullet_voices;
    storage->bullet_capacity = 8U;
    storage->grenade_voices = grenade_voices;
    storage->grenade_capacity = 2U;
    storage->rocket_voices = rocket_voices;
    storage->rocket_capacity = 2U;
    storage->projectile_voices = projectile_voices;
    storage->projectile_capacity = 8U;
    storage->impact_voices = impact_voices;
    storage->impact_capacity = 8U;
    storage->ricochet_voices = ricochet_voices;
    storage->ricochet_capacity = 8U;
    storage->expansion_memory.outdoor = expansion_outdoor;
    storage->expansion_memory.outdoor_frames = TEST_RATE;
    storage->expansion_memory.portal = expansion_portal;
    storage->expansion_memory.portal_frames = TEST_RATE / 2U;
    storage->expansion_memory.spatial_left = expansion_spatial_l;
    storage->expansion_memory.spatial_right = expansion_spatial_r;
    storage->expansion_memory.spatial_frames = 64U;
    storage->world_memory.room_delay = world_room;
    storage->world_memory.room_delay_samples = 10000U;
    storage->world_memory.prop_delay = world_prop;
    storage->world_memory.prop_delay_samples = TEST_RATE + 4U;
}

static int render_has_signal(wsse89_context *ctx)
{
    gv89_u32 i;
    gv89_u32 block;
    gv89_s32 magnitude;
    magnitude = 0;
    for (block = 0U; block < 90U; ++block) {
        (void)wsse89_render_stereo(ctx, render_buffer, 1024U, 0);
        for (i = 0U; i < 2048U; ++i) {
            if (render_buffer[i] < 0) magnitude -= render_buffer[i];
            else magnitude += render_buffer[i];
        }
    }
    return magnitude != 0;
}

int main(void)
{
    wsse89_context ctx;
    wsse89_config config;
    wsse89_storage storage;
    wsse89_event event;
    gv89_handle h1;
    gv89_handle h2;
    gv89_handle h3;
    gv89_stats stats;
    wsoundaction89_event action_events[WSOUNDACTION89_MAX_EVENTS];
    wsound89_u16 action_count;
    int r;

    wsse89_config_defaults(&config);
    config.sample_rate = TEST_RATE;
    config.logical_voice_capacity = TEST_LOGICAL;
    config.physical_voice_limit = 32U;
    config.expansion_mask = GSSEXP89_ALL;
    fill_storage(&storage);
    if (!wsse89_init(&ctx, &config, &storage)) return 1;

    event.type = WSSE89_EVENT_REPORT;
    event.seed = 1U;
    gssr89_defaults(&event.data.report, GPAAH89_PRESET_PISTOL);
    if (wsse89_dispatch(&ctx, &event, &h1) != WSSE89_OK) return 2;
    event.seed = 2U;
    if (wsse89_dispatch(&ctx, &event, &h2) != WSSE89_OK) return 3;
    event.seed = 3U;
    r = wsse89_dispatch(&ctx, &event, &h3);
    if (r != WSSE89_EVOICE) return 4;

    event.type = WSSE89_EVENT_EXPANSION_SHOT;
    event.seed = 9U;
    event.data.expansion_shot.device = WSOUNDMUZZLEDEVICE89_BRAKE;
    event.data.expansion_shot.energy_q15 = 30000U;
    event.data.expansion_shot.distance_q15 = 1200U;
    if (wsse89_dispatch(&ctx, &event, 0) != WSSE89_OK) return 5;

    event.type = WSSE89_EVENT_IMPACT;
    event.seed = 10U;
    gssw89_impact_defaults(&event.data.impact);
    event.data.impact.material = WSOUNDIMPACT89_METAL;
    if (wsse89_dispatch(&ctx, &event, &h3) != WSSE89_OK) return 6;

    event.type = WSSE89_EVENT_RICOCHET;
    event.seed = 11U;
    gssw89_ricochet_defaults(&event.data.ricochet);
    if (wsse89_dispatch(&ctx, &event, &h3) != WSSE89_OK) return 7;

    event.type = WSSE89_EVENT_PROJECTILE;
    event.seed = 12U;
    gssw89_projectile_defaults(&event.data.projectile);
    if (wsse89_dispatch(&ctx, &event, &h3) != WSSE89_OK) return 8;

    if (!render_has_signal(&ctx)) return 9;

    event.type = WSSE89_EVENT_ACTION_START;
    event.seed = 0U;
    event.data.action.action = WSOUNDACTION89_PUMP_SHOTGUN;
    event.data.action.speed_q16 = 65536U;
    if (wsse89_dispatch(&ctx, &event, 0) != WSSE89_OK) return 10;
    action_count = 0U;
    if (wsse89_advance_action(&ctx, TEST_RATE, action_events,
                              WSOUNDACTION89_MAX_EVENTS,
                              &action_count) != WSOUND89_OK) return 11;
    if (action_count == 0U) return 12;

    event.type = WSSE89_EVENT_STOP_HANDLE;
    event.data.stop.handle = h1;
    event.data.stop.release_ms = 0U;
    (void)wsse89_dispatch(&ctx, &event, 0);

    if (wsse89_event_name(WSSE89_EVENT_GRENADE_BLAST)[0] == '\0') return 13;
    wsse89_get_stats(&ctx, &stats);
    printf("weapon_synth_sound_engine89 PASS starts=%u rejects=%u action_events=%u context=%u report_voice=%u\n",
           stats.starts, stats.rejects, action_count,
           wsse89_context_bytes(), gssr89_voice_bytes());
    return 0;
}
