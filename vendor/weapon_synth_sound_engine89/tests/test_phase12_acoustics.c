#include "weapon_synth_sound_engine89.h"

#define T12_RATE 44100U
#define T12_LOGICAL 32U

static gv89_voice logical[T12_LOGICAL];
static gssr89_voice reports[2];
static gsse89_casing_voice casings[2];
static gsse89_fire_voice fires[1];
static gsso89_bullet_voice bullets[2];
static gsso89_grenade_voice grenades[1];
static gsso89_rocket_voice rockets[1];
static gssw89_projectile_voice projectiles[2];
static gssw89_impact_voice impacts[2];
static gssw89_ricochet_voice ricochets[2];
static wsound89_i16 outdoor[T12_RATE];
static wsound89_i16 portal[T12_RATE / 2U];
static wsound89_i16 spatial_l[64];
static wsound89_i16 spatial_r[64];
static wsound89_i16 room[14000];
static wsound89_i16 prop[T12_RATE + 4096U];

int main(void)
{
    wsse89_context ctx;
    wsse89_config config;
    wsse89_storage storage;
    wsse89_event event;
    gv89_handle handle;
    gv89_s16 l;
    gv89_s16 r;
    gv89_u32 i;
    gv89_u32 nonzero;
    wsse89_config_defaults(&config);
    config.sample_rate = T12_RATE;
    config.logical_voice_capacity = T12_LOGICAL;
    config.physical_voice_limit = 16U;
    storage.logical_voices = logical;
    storage.report_voices = reports;
    storage.report_capacity = 2U;
    storage.casing_voices = casings;
    storage.casing_capacity = 2U;
    storage.fire_voices = fires;
    storage.fire_capacity = 1U;
    storage.bullet_voices = bullets;
    storage.bullet_capacity = 2U;
    storage.grenade_voices = grenades;
    storage.grenade_capacity = 1U;
    storage.rocket_voices = rockets;
    storage.rocket_capacity = 1U;
    storage.projectile_voices = projectiles;
    storage.projectile_capacity = 2U;
    storage.impact_voices = impacts;
    storage.impact_capacity = 2U;
    storage.ricochet_voices = ricochets;
    storage.ricochet_capacity = 2U;
    storage.expansion_memory.outdoor = outdoor;
    storage.expansion_memory.outdoor_frames = T12_RATE;
    storage.expansion_memory.portal = portal;
    storage.expansion_memory.portal_frames = T12_RATE / 2U;
    storage.expansion_memory.spatial_left = spatial_l;
    storage.expansion_memory.spatial_right = spatial_r;
    storage.expansion_memory.spatial_frames = 64U;
    storage.world_memory.room_delay = room;
    storage.world_memory.room_delay_samples = 14000U;
    storage.world_memory.prop_delay = prop;
    storage.world_memory.prop_delay_samples = T12_RATE + 4096U;
    if (!wsse89_init(&ctx, &config, &storage)) return 1;

    event.type = WSSE89_EVENT_ACOUSTIC_PROFILE;
    event.seed = 0U;
    event.data.acoustic_profile = WSOUNDA89_HYBRID;
    if (wsse89_dispatch(&ctx, &event, 0) != WSSE89_OK) return 2;
    event.type = WSSE89_EVENT_ACOUSTIC_SPACE;
    event.data.acoustic_space = WSOUNDA89_SPACE_URBAN;
    if (wsse89_dispatch(&ctx, &event, 0) != WSSE89_OK) return 3;
    event.type = WSSE89_EVENT_ACOUSTIC_MATERIAL;
    event.data.acoustic_material.material = WSOUNDA89_MATERIAL_BRICK;
    event.data.acoustic_material.thickness_q15 = 22000U;
    if (wsse89_dispatch(&ctx, &event, 0) != WSSE89_OK) return 4;
    event.type = WSSE89_EVENT_ACOUSTIC_PATH;
    wsounda89_path_defaults(&event.data.acoustic_path);
    event.data.acoustic_path.distance_cm = 300U;
    event.data.acoustic_path.source = WSOUNDA89_SOURCE_REPORT;
    if (wsse89_dispatch(&ctx, &event, 0) != WSSE89_OK) return 5;
    event.type = WSSE89_EVENT_REPORT;
    event.seed = 44U;
    gssr89_defaults(&event.data.report, GPAAH89_PRESET_SNIPER_RIFLE);
    if (wsse89_dispatch(&ctx, &event, &handle) != WSSE89_OK) return 6;
    nonzero = 0U;
    for (i = 0U; i < T12_RATE; ++i) {
        wsse89_process_stereo_sample(&ctx, &l, &r);
        if (l != 0 || r != 0) nonzero++;
    }
    if (nonzero < 100U) return 7;
    return 0;
}
