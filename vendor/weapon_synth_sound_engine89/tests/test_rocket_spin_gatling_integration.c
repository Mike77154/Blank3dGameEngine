#include <stdio.h>
#include <string.h>
#include "weapon_synth_sound_engine89.h"

#define TRG_RATE 44100U
#define TRG_LOGICAL 64U

static wsse89_context engine;
static gv89_voice logical[TRG_LOGICAL];
static gssr89_voice reports[8];
static gsse89_casing_voice casings[8];
static gsse89_fire_voice fires[4];
static gsso89_bullet_voice bullets[2];
static gsso89_grenade_voice grenades[1];
static gsso89_rocket_voice rockets[2];
static gssw89_projectile_voice projectiles[2];
static gssw89_impact_voice impacts[2];
static gssw89_ricochet_voice ricochets[2];
static wsound89_i16 outdoor[TRG_RATE];
static wsound89_i16 portal[TRG_RATE / 2U];
static wsound89_i16 spatial_l[64];
static wsound89_i16 spatial_r[64];
static wsound89_i16 room[14000];
static wsound89_i16 prop[TRG_RATE + 64U];
static gv89_s16 block[512U * 2U];

static void trg_storage(wsse89_storage *s)
{
    memset(s, 0, sizeof(*s));
    s->logical_voices = logical;
    s->report_voices = reports; s->report_capacity = 8U;
    s->casing_voices = casings; s->casing_capacity = 8U;
    s->fire_voices = fires; s->fire_capacity = 4U;
    s->bullet_voices = bullets; s->bullet_capacity = 2U;
    s->grenade_voices = grenades; s->grenade_capacity = 1U;
    s->rocket_voices = rockets; s->rocket_capacity = 2U;
    s->projectile_voices = projectiles; s->projectile_capacity = 2U;
    s->impact_voices = impacts; s->impact_capacity = 2U;
    s->ricochet_voices = ricochets; s->ricochet_capacity = 2U;
    s->expansion_memory.outdoor = outdoor;
    s->expansion_memory.outdoor_frames = TRG_RATE;
    s->expansion_memory.portal = portal;
    s->expansion_memory.portal_frames = TRG_RATE / 2U;
    s->expansion_memory.spatial_left = spatial_l;
    s->expansion_memory.spatial_right = spatial_r;
    s->expansion_memory.spatial_frames = 64U;
    s->world_memory.room_delay = room;
    s->world_memory.room_delay_samples = 14000U;
    s->world_memory.prop_delay = prop;
    s->world_memory.prop_delay_samples = TRG_RATE + 64U;
}

static gv89_u32 trg_energy(gv89_u32 frames)
{
    gv89_u32 done;
    gv89_u32 chunk;
    gv89_u32 i;
    gv89_u32 energy;
    done = 0U;
    energy = 0U;
    while (done < frames) {
        chunk = frames - done;
        if (chunk > 512U) chunk = 512U;
        wsse89_render_stereo(&engine, block, chunk, 0);
        for (i = 0U; i < chunk * 2U; ++i) {
            if (block[i] < 0) energy += (gv89_u32)(-(gv89_s32)block[i]);
            else energy += (gv89_u32)block[i];
        }
        done += chunk;
    }
    return energy;
}

int main(void)
{
    wsse89_config config;
    wsse89_storage storage;
    wsse89_event event;
    gv89_u32 spin_energy;
    gv89_u32 spool_energy;
    gv89_u32 firing_energy;
    gv89_u32 tail_energy;

    wsse89_config_defaults(&config);
    config.sample_rate = TRG_RATE;
    config.logical_voice_capacity = TRG_LOGICAL;
    config.physical_voice_limit = 32U;
    config.seed = 0x190000U;
    trg_storage(&storage);
    if (!wsse89_init(&engine, &config, &storage)) return 1;

    memset(&event, 0, sizeof(event));
    event.type = WSSE89_EVENT_ROCKET_SPIN_START;
    event.seed = 0x190101U;
    wsse89_rocket_spin_defaults(&event.data.rocket_spin_start);
    event.data.rocket_spin_start.instance_key = 101U;
    if (wsse89_dispatch(&engine, &event, 0) != WSSE89_OK) return 2;
    spin_energy = trg_energy(TRG_RATE / 2U);
    if (spin_energy == 0U) return 3;

    memset(&event, 0, sizeof(event));
    event.type = WSSE89_EVENT_ROCKET_SPIN_MOTION;
    event.data.rocket_spin_motion.instance_key = 101U;
    event.data.rocket_spin_motion.gain_q15 = 7000;
    event.data.rocket_spin_motion.pan_q15 = 5000;
    if (wsse89_dispatch(&engine, &event, 0) != WSSE89_OK) return 4;

    memset(&event, 0, sizeof(event));
    event.type = WSSE89_EVENT_ROCKET_SPIN_STOP;
    event.data.rocket_spin_control.instance_key = 101U;
    if (wsse89_dispatch(&engine, &event, 0) != WSSE89_OK) return 5;
    if (trg_energy(TRG_RATE) == 0U) return 6;

    memset(&event, 0, sizeof(event));
    event.type = WSSE89_EVENT_GATLING_START;
    event.seed = 0x190202U;
    wsse89_gatling_defaults(&event.data.gatling_start);
    event.data.gatling_start.instance_key = 202U;
    if (wsse89_dispatch(&engine, &event, 0) != WSSE89_OK) return 7;
    spool_energy = trg_energy(TRG_RATE / 2U);
    if (spool_energy == 0U) return 8;

    memset(&event, 0, sizeof(event));
    event.type = WSSE89_EVENT_GATLING_FIRE_START;
    event.data.gatling_fire.instance_key = 202U;
    event.data.gatling_fire.firing_load_q15 = 22000;
    if (wsse89_dispatch(&engine, &event, 0) != WSSE89_OK) return 9;
    firing_energy = trg_energy(TRG_RATE / 2U);
    if (firing_energy <= spool_energy / 4U) return 10;

    memset(&event, 0, sizeof(event));
    event.type = WSSE89_EVENT_GATLING_FIRE_STOP;
    event.data.gatling_control.instance_key = 202U;
    if (wsse89_dispatch(&engine, &event, 0) != WSSE89_OK) return 11;
    tail_energy = trg_energy(TRG_RATE * 2U);
    if (tail_energy == 0U) return 12;

    if (wsse89_event_name(WSSE89_EVENT_ROCKET_SPIN_START)[7] != 's') return 13;
    if (wsse89_event_name(WSSE89_EVENT_GATLING_FIRE_START)[0] != 'g') return 14;

    printf("rocket_spin_gatling_integration PASS context=%lu spin=%lu spool=%lu firing=%lu tail=%lu\n",
           (unsigned long)wsse89_context_bytes(),
           (unsigned long)spin_energy,
           (unsigned long)spool_energy,
           (unsigned long)firing_energy,
           (unsigned long)tail_energy);
    return 0;
}
