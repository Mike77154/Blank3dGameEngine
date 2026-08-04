#include <stdio.h>
#include "weapon_synth_sound_engine89.h"

#define TMW_RATE 44100U
#define TMW_LOGICAL 32U

static wsse89_context engine;
static gv89_voice logical[TMW_LOGICAL];
static gssr89_voice reports[2];
static gsse89_casing_voice casings[2];
static gsse89_fire_voice fires[1];
static gsso89_bullet_voice bullets[2];
static gsso89_grenade_voice grenades[1];
static gsso89_rocket_voice rockets[1];
static gssw89_projectile_voice projectiles[2];
static gssw89_impact_voice impacts[2];
static gssw89_ricochet_voice ricochets[2];
static wsound89_i16 outdoor[TMW_RATE];
static wsound89_i16 portal[TMW_RATE / 2U];
static wsound89_i16 spatial_l[64];
static wsound89_i16 spatial_r[64];
static wsound89_i16 room[12000];
static wsound89_i16 prop[TMW_RATE + 64U];
static gv89_s16 block[512U * 2U];

static void tmw_storage(wsse89_storage *s)
{
    s->logical_voices = logical;
    s->report_voices = reports; s->report_capacity = 2U;
    s->casing_voices = casings; s->casing_capacity = 2U;
    s->fire_voices = fires; s->fire_capacity = 1U;
    s->bullet_voices = bullets; s->bullet_capacity = 2U;
    s->grenade_voices = grenades; s->grenade_capacity = 1U;
    s->rocket_voices = rockets; s->rocket_capacity = 1U;
    s->projectile_voices = projectiles; s->projectile_capacity = 2U;
    s->impact_voices = impacts; s->impact_capacity = 2U;
    s->ricochet_voices = ricochets; s->ricochet_capacity = 2U;
    s->expansion_memory.outdoor = outdoor;
    s->expansion_memory.outdoor_frames = TMW_RATE;
    s->expansion_memory.portal = portal;
    s->expansion_memory.portal_frames = TMW_RATE / 2U;
    s->expansion_memory.spatial_left = spatial_l;
    s->expansion_memory.spatial_right = spatial_r;
    s->expansion_memory.spatial_frames = 64U;
    s->world_memory.room_delay = room;
    s->world_memory.room_delay_samples = 12000U;
    s->world_memory.prop_delay = prop;
    s->world_memory.prop_delay_samples = TMW_RATE + 64U;
}

static gv89_u32 tmw_render_energy(wsse89_context *ctx, gv89_u32 frames)
{
    gv89_u32 total;
    gv89_u32 chunk;
    gv89_u32 i;
    gv89_u32 energy;
    total = 0U;
    energy = 0U;
    while (total < frames) {
        chunk = frames - total;
        if (chunk > 512U) chunk = 512U;
        (void)wsse89_render_stereo(ctx, block, chunk, 0);
        for (i = 0U; i < chunk * 2U; ++i) {
            if (block[i] < 0) energy += (gv89_u32)(-(gv89_s32)block[i]);
            else energy += (gv89_u32)block[i];
        }
        total += chunk;
    }
    return energy;
}

int main(void)
{
    wsse89_config config;
    wsse89_storage storage;
    wsse89_event event;
    gv89_u32 mag_energy;
    gv89_u32 whistle_energy;
    gv89_u32 airy_energy;

    wsse89_config_defaults(&config);
    config.sample_rate = TMW_RATE;
    config.logical_voice_capacity = TMW_LOGICAL;
    config.physical_voice_limit = 16U;
    tmw_storage(&storage);
    if (!wsse89_init(&engine, &config, &storage)) return 1;

    event.type = WSSE89_EVENT_MAGAZINE_ACTION;
    event.seed = 0x1111U;
    wsse89_magazine_defaults(&event.data.magazine);
    event.data.magazine.preset = WMAG89_PRESET_PISTOL_POLYMER;
    event.data.magazine.action = WMAG89_ACTION_INSERT;
    event.data.magazine.instance_key = 77U;
    if (wsse89_dispatch(&engine, &event, 0) != WSSE89_OK) return 2;
    mag_energy = tmw_render_energy(&engine, TMW_RATE / 2U);
    if (mag_energy == 0U) return 3;

    event.type = WSSE89_EVENT_ROCKET_WHISTLE_START;
    event.seed = 0x2222U;
    wsse89_rocket_whistle_defaults(&event.data.rocket_whistle_start);
    event.data.rocket_whistle_start.preset = GWH89_PRESET_CHIFLADORA_AIR_REF;
    event.data.rocket_whistle_start.instance_key = 9001U;
    event.data.rocket_whistle_start.auto_hold_ms = 0U;
    event.data.rocket_whistle_start.pan_q15 = -4000;
    if (wsse89_dispatch(&engine, &event, 0) != WSSE89_OK) return 4;
    whistle_energy = tmw_render_energy(&engine, TMW_RATE / 3U);
    if (whistle_energy == 0U) return 5;

    event.type = WSSE89_EVENT_ROCKET_WHISTLE_FX;
    wsse89_rocket_whistle_fx_defaults(&event.data.rocket_whistle_fx);
    event.data.rocket_whistle_fx.instance_key = 9001U;
    event.data.rocket_whistle_fx.output_eq_mode = 2U;
    event.data.rocket_whistle_fx.output_eq_gain_q15[GWH89_OUTPUT_EQ_RUMBLE_120] = 1200U;
    event.data.rocket_whistle_fx.output_eq_gain_q15[GWH89_OUTPUT_EQ_EDGE_2400_6000] = 39000U;
    event.data.rocket_whistle_fx.output_eq_gain_q15[GWH89_OUTPUT_EQ_AIR_ABOVE_6000] = 37000U;
    if (wsse89_dispatch(&engine, &event, 0) != WSSE89_OK) return 8;
    airy_energy = tmw_render_energy(&engine, TMW_RATE / 5U);
    if (airy_energy == 0U) return 9;

    event.type = WSSE89_EVENT_ROCKET_WHISTLE_MOTION;
    event.seed = 0U;
    event.data.rocket_whistle_motion.instance_key = 9001U;
    event.data.rocket_whistle_motion.radial_velocity_mps = 110;
    event.data.rocket_whistle_motion.distance_gain_q15 = 21000U;
    event.data.rocket_whistle_motion.gain_q15 = 17000;
    event.data.rocket_whistle_motion.pan_q15 = 5000;
    if (wsse89_dispatch(&engine, &event, 0) != WSSE89_OK) return 8;
    whistle_energy = tmw_render_energy(&engine, TMW_RATE / 3U);
    if (whistle_energy == 0U) return 9;

    event.type = WSSE89_EVENT_ROCKET_WHISTLE_RELEASE;
    event.data.rocket_whistle_control.instance_key = 9001U;
    if (wsse89_dispatch(&engine, &event, 0) != WSSE89_OK) return 10;
    whistle_energy = tmw_render_energy(&engine, TMW_RATE);
    if (whistle_energy == 0U) return 11;

    event.type = WSSE89_EVENT_ROCKET_WHISTLE_MOTION;
    event.data.rocket_whistle_motion.instance_key = 123456U;
    if (wsse89_dispatch(&engine, &event, 0) != WSSE89_EINVAL) return 12;

    if (wsse89_event_name(WSSE89_EVENT_MAGAZINE_ACTION)[0] != 'm') return 13;
    if (wsse89_event_name(WSSE89_EVENT_ROCKET_WHISTLE_START)[0] != 'r') return 14;
    if (wsse89_event_name(WSSE89_EVENT_ROCKET_WHISTLE_FX)[15] != 'f') return 15;
    if (gwh89_get_output_eq_band_name(GWH89_OUTPUT_EQ_EDGE_2400_6000) == 0) return 16;

    printf("magazine_whistle_integration PASS context=%lu magazine=%lu whistle=%lu airy=%lu\n",
           (unsigned long)wsse89_context_bytes(),
           (unsigned long)mag_energy,
           (unsigned long)whistle_energy,
           (unsigned long)airy_energy);
    return 0;
}
