/*
 * game_engine_event_bridge.c
 * Minimal engine-side adapter for weapon_synth_sound_engine89 v1.8.1.
 * This file intentionally owns no audio backend: the host engine calls these
 * functions from gameplay and renders through wsse89_render_stereo() or
 * wsse89_process_stereo_sample() inside its audio callback.
 */
#include "weapon_synth_sound_engine89.h"

typedef struct game_audio_bus_s {
    wsse89_event_sink_fn sink;
    void *sink_user;
} game_audio_bus;

static void game_audio_zero_event(wsse89_event *event)
{
    unsigned char *bytes;
    unsigned long i;
    bytes = (unsigned char *)event;
    for (i = 0UL; i < (unsigned long)sizeof(*event); ++i) bytes[i] = 0U;
}

void game_audio_bus_bind(game_audio_bus *bus, wsse89_context *engine)
{
    if (bus == 0) return;
    bus->sink = wsse89_event_sink;
    bus->sink_user = engine;
}

int game_audio_weapon_report(game_audio_bus *bus,
                             gpaah89_preset_id preset,
                             gv89_s16 pan_q15,
                             gv89_u16 distance_q15,
                             gv89_u16 occlusion_q15,
                             gv89_handle *out_handle)
{
    wsse89_event event;
    gv89_handle handle;
    int result;
    if (bus == 0 || bus->sink == 0) return WSSE89_EINVAL;

    game_audio_zero_event(&event);
    event.type = WSSE89_EVENT_REPORT;
    gssr89_defaults(&event.data.report, preset);
    event.data.report.pan_q15 = pan_q15;
    result = bus->sink(bus->sink_user, &event, &handle);
    if (result != WSSE89_OK) return result;

    game_audio_zero_event(&event);
    event.type = WSSE89_EVENT_SET_HANDLE_SPATIAL;
    event.data.spatial.handle = handle;
    event.data.spatial.pan_q15 = pan_q15;
    event.data.spatial.distance_q15 = distance_q15;
    event.data.spatial.occlusion_q15 = occlusion_q15;
    event.data.spatial.focus_q15 = 32767U;
    result = bus->sink(bus->sink_user, &event, 0);
    if (out_handle != 0) *out_handle = handle;
    return result;
}

int game_audio_surface_impact(game_audio_bus *bus,
                              wsoundimpact89_material material,
                              gv89_s16 pan_q15,
                              gv89_u16 energy_q15)
{
    wsse89_event event;
    if (bus == 0 || bus->sink == 0) return WSSE89_EINVAL;
    game_audio_zero_event(&event);
    event.type = WSSE89_EVENT_IMPACT;
    gssw89_impact_defaults(&event.data.impact);
    event.data.impact.material = material;
    event.data.impact.energy_q15 = energy_q15;
    event.data.impact.common.pan_q15 = pan_q15;
    return bus->sink(bus->sink_user, &event, 0);
}

int game_audio_action(game_audio_bus *bus,
                      wsoundaction89_type action,
                      wsound89_u32 speed_q16)
{
    wsse89_event event;
    if (bus == 0 || bus->sink == 0) return WSSE89_EINVAL;
    game_audio_zero_event(&event);
    event.type = WSSE89_EVENT_ACTION_START;
    event.data.action.action = action;
    event.data.action.speed_q16 = speed_q16;
    return bus->sink(bus->sink_user, &event, 0);
}

int game_audio_set_acoustic_environment(game_audio_bus *bus,
                                        wsounda89_profile profile,
                                        wsounda89_space space,
                                        wsounda89_material material,
                                        wsound89_u16 thickness_q15,
                                        const wsounda89_path_params *path,
                                        const wsounda89_portal_params *portal)
{
    wsse89_event event;
    int result;
    if (bus == 0 || bus->sink == 0 || path == 0 || portal == 0)
        return WSSE89_EINVAL;

    game_audio_zero_event(&event);
    event.type = WSSE89_EVENT_ACOUSTIC_PROFILE;
    event.data.acoustic_profile = profile;
    result = bus->sink(bus->sink_user, &event, 0);
    if (result != WSSE89_OK) return result;

    game_audio_zero_event(&event);
    event.type = WSSE89_EVENT_ACOUSTIC_SPACE;
    event.data.acoustic_space = space;
    result = bus->sink(bus->sink_user, &event, 0);
    if (result != WSSE89_OK) return result;

    game_audio_zero_event(&event);
    event.type = WSSE89_EVENT_ACOUSTIC_MATERIAL;
    event.data.acoustic_material.material = material;
    event.data.acoustic_material.thickness_q15 = thickness_q15;
    result = bus->sink(bus->sink_user, &event, 0);
    if (result != WSSE89_OK) return result;

    game_audio_zero_event(&event);
    event.type = WSSE89_EVENT_ACOUSTIC_PORTAL;
    event.data.acoustic_portal = *portal;
    result = bus->sink(bus->sink_user, &event, 0);
    if (result != WSSE89_OK) return result;

    game_audio_zero_event(&event);
    event.type = WSSE89_EVENT_ACOUSTIC_PATH;
    event.data.acoustic_path = *path;
    return bus->sink(bus->sink_user, &event, 0);
}

int game_audio_set_weapon_dna(game_audio_bus *bus,
                              const wsounddna89_profile *profile,
                              wsounddna89_mode mode)
{
    wsse89_event event;
    int result;
    if (bus == 0 || bus->sink == 0 || profile == 0) return WSSE89_EINVAL;
    game_audio_zero_event(&event);
    event.type = WSSE89_EVENT_WEAPON_PROFILE;
    event.data.weapon_profile = *profile;
    result = bus->sink(bus->sink_user, &event, 0);
    if (result != WSSE89_OK) return result;
    game_audio_zero_event(&event);
    event.type = WSSE89_EVENT_WEAPON_MODE;
    event.data.weapon_mode = mode;
    return bus->sink(bus->sink_user, &event, 0);
}

int game_audio_fire_physical_weapon(game_audio_bus *bus,
                                    gv89_s16 pan_q15,
                                    gv89_u16 distance_q15,
                                    gv89_u32 instance_key,
                                    gv89_handle *out_handle)
{
    wsse89_event event;
    if (bus == 0 || bus->sink == 0) return WSSE89_EINVAL;
    game_audio_zero_event(&event);
    event.type = WSSE89_EVENT_WEAPON_FIRE;
    wsse89_weapon_fire_defaults(&event.data.weapon_fire);
    event.data.weapon_fire.pan_q15 = pan_q15;
    event.data.weapon_fire.distance_q15 = distance_q15;
    event.data.weapon_fire.instance_key = instance_key;
    return bus->sink(bus->sink_user, &event, out_handle);
}


int game_audio_magazine_action(game_audio_bus *bus,
                               int preset,
                               int action,
                               gv89_u16 velocity_q15,
                               gv89_s16 gain_q15,
                               gv89_s16 pan_q15,
                               gv89_u32 instance_key)
{
    wsse89_event event;
    if (bus == 0 || bus->sink == 0) return WSSE89_EINVAL;
    game_audio_zero_event(&event);
    event.type = WSSE89_EVENT_MAGAZINE_ACTION;
    wsse89_magazine_defaults(&event.data.magazine);
    event.data.magazine.preset = preset;
    event.data.magazine.action = action;
    event.data.magazine.velocity_q15 = velocity_q15;
    event.data.magazine.gain_q15 = gain_q15;
    event.data.magazine.pan_q15 = pan_q15;
    event.data.magazine.instance_key = instance_key;
    return bus->sink(bus->sink_user, &event, 0);
}

int game_audio_rocket_whistle_start(game_audio_bus *bus,
                                    int preset,
                                    gv89_s32 radial_velocity_mps,
                                    gv89_u16 distance_gain_q15,
                                    gv89_s16 pan_q15,
                                    gv89_u32 instance_key)
{
    wsse89_event event;
    if (bus == 0 || bus->sink == 0) return WSSE89_EINVAL;
    game_audio_zero_event(&event);
    event.type = WSSE89_EVENT_ROCKET_WHISTLE_START;
    wsse89_rocket_whistle_defaults(&event.data.rocket_whistle_start);
    event.data.rocket_whistle_start.preset = preset;
    event.data.rocket_whistle_start.radial_velocity_mps = radial_velocity_mps;
    event.data.rocket_whistle_start.distance_gain_q15 = distance_gain_q15;
    event.data.rocket_whistle_start.pan_q15 = pan_q15;
    event.data.rocket_whistle_start.instance_key = instance_key;
    return bus->sink(bus->sink_user, &event, 0);
}

int game_audio_rocket_whistle_motion(game_audio_bus *bus,
                                     gv89_u32 instance_key,
                                     gv89_s32 radial_velocity_mps,
                                     gv89_u16 distance_gain_q15,
                                     gv89_s16 gain_q15,
                                     gv89_s16 pan_q15)
{
    wsse89_event event;
    if (bus == 0 || bus->sink == 0) return WSSE89_EINVAL;
    game_audio_zero_event(&event);
    event.type = WSSE89_EVENT_ROCKET_WHISTLE_MOTION;
    event.data.rocket_whistle_motion.instance_key = instance_key;
    event.data.rocket_whistle_motion.radial_velocity_mps = radial_velocity_mps;
    event.data.rocket_whistle_motion.distance_gain_q15 = distance_gain_q15;
    event.data.rocket_whistle_motion.gain_q15 = gain_q15;
    event.data.rocket_whistle_motion.pan_q15 = pan_q15;
    return bus->sink(bus->sink_user, &event, 0);
}

int game_audio_rocket_whistle_air_finish(game_audio_bus *bus,
                                          gv89_u32 instance_key)
{
    wsse89_event event;
    if (bus == 0 || bus->sink == 0) return WSSE89_EINVAL;
    game_audio_zero_event(&event);
    event.type = WSSE89_EVENT_ROCKET_WHISTLE_FX;
    wsse89_rocket_whistle_fx_defaults(&event.data.rocket_whistle_fx);
    event.data.rocket_whistle_fx.instance_key = instance_key;
    event.data.rocket_whistle_fx.output_eq_mode = 2U;
    event.data.rocket_whistle_fx.output_eq_gain_q15[
        GWH89_OUTPUT_EQ_RUMBLE_120] = 1200U;
    event.data.rocket_whistle_fx.output_eq_gain_q15[
        GWH89_OUTPUT_EQ_LOW_120_300] = 9000U;
    event.data.rocket_whistle_fx.output_eq_gain_q15[
        GWH89_OUTPUT_EQ_CORE_300_900] = 25000U;
    event.data.rocket_whistle_fx.output_eq_gain_q15[
        GWH89_OUTPUT_EQ_WHISTLE_900_2400] = 33000U;
    event.data.rocket_whistle_fx.output_eq_gain_q15[
        GWH89_OUTPUT_EQ_EDGE_2400_6000] = 37000U;
    event.data.rocket_whistle_fx.output_eq_gain_q15[
        GWH89_OUTPUT_EQ_AIR_ABOVE_6000] = 34500U;
    event.data.rocket_whistle_fx.reverb_mode = 2U;
    event.data.rocket_whistle_fx.reverb_wet_q15 = 2100U;
    event.data.rocket_whistle_fx.reverb_feedback_q15 = 17000U;
    event.data.rocket_whistle_fx.reverb_damping_q15 = 8000U;
    event.data.rocket_whistle_fx.reverb_tail_ms = 210U;
    return bus->sink(bus->sink_user, &event, 0);
}

static int game_audio_rocket_whistle_control(game_audio_bus *bus,
                                             wsse89_event_type type,
                                             gv89_u32 instance_key)
{
    wsse89_event event;
    if (bus == 0 || bus->sink == 0) return WSSE89_EINVAL;
    game_audio_zero_event(&event);
    event.type = type;
    event.data.rocket_whistle_control.instance_key = instance_key;
    return bus->sink(bus->sink_user, &event, 0);
}

int game_audio_rocket_whistle_release(game_audio_bus *bus,
                                      gv89_u32 instance_key)
{
    return game_audio_rocket_whistle_control(
        bus, WSSE89_EVENT_ROCKET_WHISTLE_RELEASE, instance_key);
}

int game_audio_rocket_whistle_stop(game_audio_bus *bus,
                                   gv89_u32 instance_key)
{
    return game_audio_rocket_whistle_control(
        bus, WSSE89_EVENT_ROCKET_WHISTLE_STOP, instance_key);
}
