#include "blank3d_audio.h"
#include "blank3d_weapon_modules.h"

#include <stdio.h>
#include <string.h>

static void b3d_audio_set_status(Blank3DAudio *audio, const char *text)
{
    if (!audio) return;
    if (!text) text = "";
    strncpy(audio->status, text, sizeof(audio->status) - 1U);
    audio->status[sizeof(audio->status) - 1U] = '\0';
}

static int b3d_audio_dispatch(Blank3DAudio *audio,
                              const wsse89_event *event,
                              gv89_handle *handle)
{
    int result;
    if (!audio || !event) return 0;
    result = wsse89_dispatch(&audio->synth, event, handle);
    if (result != WSSE89_OK) {
        char message[128];
        audio->dispatch_failures++;
        sprintf(message, "weapon synth dispatch failed: %s (%d)",
                wsse89_event_name(event->type), result);
        b3d_audio_set_status(audio, message);
        return 0;
    }
    return 1;
}

static void b3d_audio_fill_storage(Blank3DAudio *audio)
{
    wsse89_storage *storage;
    storage = &audio->storage;
    memset(storage, 0, sizeof(*storage));
    storage->logical_voices = audio->logical_voices;
    storage->report_voices = audio->report_voices;
    storage->report_capacity = B3D_AUDIO_REPORT_VOICES;
    storage->casing_voices = audio->casing_voices;
    storage->casing_capacity = B3D_AUDIO_CASING_VOICES;
    storage->fire_voices = audio->fire_voices;
    storage->fire_capacity = B3D_AUDIO_FIRE_VOICES;
    storage->bullet_voices = audio->bullet_voices;
    storage->bullet_capacity = B3D_AUDIO_BULLET_VOICES;
    storage->grenade_voices = audio->grenade_voices;
    storage->grenade_capacity = B3D_AUDIO_GRENADE_VOICES;
    storage->rocket_voices = audio->rocket_voices;
    storage->rocket_capacity = B3D_AUDIO_ROCKET_VOICES;
    storage->projectile_voices = audio->projectile_voices;
    storage->projectile_capacity = B3D_AUDIO_PROJECTILE_VOICES;
    storage->impact_voices = audio->impact_voices;
    storage->impact_capacity = B3D_AUDIO_IMPACT_VOICES;
    storage->ricochet_voices = audio->ricochet_voices;
    storage->ricochet_capacity = B3D_AUDIO_RICOCHET_VOICES;
    storage->expansion_memory.outdoor = audio->expansion_outdoor;
    storage->expansion_memory.outdoor_frames = B3D_AUDIO_RATE;
    storage->expansion_memory.portal = audio->expansion_portal;
    storage->expansion_memory.portal_frames = B3D_AUDIO_RATE / 2U;
    storage->expansion_memory.spatial_left = audio->expansion_spatial_l;
    storage->expansion_memory.spatial_right = audio->expansion_spatial_r;
    storage->expansion_memory.spatial_frames = 64U;
    storage->world_memory.room_delay = audio->world_room;
    storage->world_memory.room_delay_samples = 10000U;
    storage->world_memory.prop_delay = audio->world_prop;
    storage->world_memory.prop_delay_samples = B3D_AUDIO_RATE + 4U;
}

typedef struct B3DAudioWeaponRecipeTag {
    int enabled;
    wsounddna89_profile_id profile;
    wsoundaction89_type action;
    int magazine_preset;
    wsoundammo89_type ammo_type;
    wsoundmuzzledevice89_type muzzle_device;
    gt89_shell_type shell;
    int detachable_magazine;
    int emits_casing;
    int projectile_mode;
    int explosion_mode;
    int continuous_rocket;
    gv89_s16 fire_gain_q15;
    gv89_u16 pressure_energy_q15;
    gv89_u16 ammo_motion_q15;
    gv89_u16 magazine_velocity_q15;
    gv89_s16 magazine_gain_q15;
    gv89_u16 reload_remove_motion_q15;
    gv89_u16 reload_insert_motion_q15;
    gv89_s16 dry_receiver_impulse;
    gv89_u32 action_speed_q16;
    gv89_u16 casing_velocity;
    gv89_u16 casing_angular_velocity;
    gv89_s16 casing_gain_q15;
    gv89_s16 projectile_gain_q15;
    gv89_u16 projectile_proximity_q15;
    gv89_u16 projectile_instance_limit;
    gv89_s16 explosion_gain_q15;
    gv89_s16 rocket_whistle_gain_q15;
    gv89_s16 rocket_spin_gain_q15;
} B3DAudioWeaponRecipe;

static wsounddna89_profile_id b3d_audio_profile_map(int value)
{
    switch (value) {
        case B3D_AUDIO_PROFILE_SMG: return WSOUNDDNA89_PROFILE_SMG;
        case B3D_AUDIO_PROFILE_SHOTGUN: return WSOUNDDNA89_PROFILE_SHOTGUN;
        case B3D_AUDIO_PROFILE_MAGNUM: return WSOUNDDNA89_PROFILE_MAGNUM;
        case B3D_AUDIO_PROFILE_SNIPER: return WSOUNDDNA89_PROFILE_SNIPER;
        case B3D_AUDIO_PROFILE_LAUNCHER: return WSOUNDDNA89_PROFILE_LAUNCHER;
        case B3D_AUDIO_PROFILE_HEAVY: return WSOUNDDNA89_PROFILE_HEAVY;
        case B3D_AUDIO_PROFILE_PISTOL:
        default: return WSOUNDDNA89_PROFILE_SERVICE_PISTOL;
    }
}

static wsoundaction89_type b3d_audio_action_map(int value)
{
    switch (value) {
        case B3D_AUDIO_ACTION_MACHINE: return WSOUNDACTION89_MACHINE;
        case B3D_AUDIO_ACTION_PUMP: return WSOUNDACTION89_PUMP_SHOTGUN;
        case B3D_AUDIO_ACTION_REVOLVER: return WSOUNDACTION89_REVOLVER;
        case B3D_AUDIO_ACTION_RIFLE: return WSOUNDACTION89_RIFLE;
        case B3D_AUDIO_ACTION_PISTOL:
        default: return WSOUNDACTION89_PISTOL;
    }
}

static int b3d_audio_magazine_map(int value)
{
    switch (value) {
        case B3D_AUDIO_MAG_PISTOL_METAL: return WMAG89_PRESET_PISTOL_METAL;
        case B3D_AUDIO_MAG_SMG_STEEL: return WMAG89_PRESET_SMG_STEEL;
        case B3D_AUDIO_MAG_RIFLE_POLYMER: return WMAG89_PRESET_RIFLE_POLYMER;
        case B3D_AUDIO_MAG_RIFLE_ALUMINUM: return WMAG89_PRESET_RIFLE_ALUMINUM;
        case B3D_AUDIO_MAG_SNIPER_BOX: return WMAG89_PRESET_SNIPER_BOX;
        case B3D_AUDIO_MAG_DRUM_HEAVY: return WMAG89_PRESET_DRUM_HEAVY;
        case B3D_AUDIO_MAG_PISTOL_POLYMER:
        default: return WMAG89_PRESET_PISTOL_POLYMER;
    }
}

static wsoundammo89_type b3d_audio_ammo_map(int value)
{
    switch (value) {
        case B3D_AUDIO_AMMO_TUBE: return WSOUNDAMMO89_TUBE;
        case B3D_AUDIO_AMMO_LOOSE_SHELLS: return WSOUNDAMMO89_LOOSE_SHELLS;
        case B3D_AUDIO_AMMO_BELT_BOX: return WSOUNDAMMO89_BELT_BOX;
        case B3D_AUDIO_AMMO_BOX_MAG:
        default: return WSOUNDAMMO89_BOX_MAG;
    }
}

static wsoundmuzzledevice89_type b3d_audio_muzzle_map(int value)
{
    switch (value) {
        case B3D_AUDIO_MUZZLE_BRAKE: return WSOUNDMUZZLEDEVICE89_BRAKE;
        case B3D_AUDIO_MUZZLE_COMPENSATOR: return WSOUNDMUZZLEDEVICE89_COMPENSATOR;
        case B3D_AUDIO_MUZZLE_PORTED: return WSOUNDMUZZLEDEVICE89_PORTED;
        case B3D_AUDIO_MUZZLE_BARE:
        default: return WSOUNDMUZZLEDEVICE89_BARE;
    }
}

static gt89_shell_type b3d_audio_shell_map(int value)
{
    switch (value) {
        case B3D_AUDIO_SHELL_STEEL_CASE: return GT89_SHELL_STEEL_CASE;
        case B3D_AUDIO_SHELL_SHOTGUN_PLASTIC: return GT89_SHELL_SHOTGUN_PLASTIC;
        case B3D_AUDIO_SHELL_MAGNUM_BRASS: return GT89_SHELL_MAGNUM_BRASS;
        case B3D_AUDIO_SHELL_RIFLE_BRASS: return GT89_SHELL_RIFLE_BRASS;
        case B3D_AUDIO_SHELL_SHOTGUN_BRASS: return GT89_SHELL_SHOTGUN_BRASS;
        case B3D_AUDIO_SHELL_RIMFIRE_BRASS: return GT89_SHELL_RIMFIRE_BRASS;
        case B3D_AUDIO_SHELL_PISTOL_BRASS:
        default: return GT89_SHELL_PISTOL_BRASS;
    }
}

static const B3DAudioWeaponRecipe *b3d_audio_recipe_for_weapon(int weapon_id)
{
    static B3DAudioWeaponRecipe recipes[B3D_WEAPON_MODULE_CAPACITY];
    B3DAudioWeaponRecipe *recipe;
    const Blank3DWeaponModules *modules;
    int slot;
    slot = weapon_id;
    if (slot < 0 || slot >= B3D_WEAPON_MODULE_CAPACITY) slot = 0;
    recipe = &recipes[slot];
    memset(recipe, 0, sizeof(*recipe));
    modules = blank3d_weapon_modules_get(weapon_id);
    if (!modules) return recipe;
    recipe->enabled = modules->audio_enabled;
    recipe->profile = b3d_audio_profile_map(modules->audio_profile);
    recipe->action = b3d_audio_action_map(modules->audio_action);
    recipe->magazine_preset = b3d_audio_magazine_map(modules->audio_magazine);
    recipe->ammo_type = b3d_audio_ammo_map(modules->audio_ammo);
    recipe->muzzle_device = b3d_audio_muzzle_map(modules->audio_muzzle);
    recipe->shell = b3d_audio_shell_map(modules->audio_shell);
    recipe->detachable_magazine = modules->audio_detachable_magazine;
    recipe->emits_casing = modules->audio_emits_casing;
    recipe->projectile_mode = modules->audio_projectile;
    recipe->explosion_mode = modules->audio_explosion;
    recipe->continuous_rocket = modules->audio_continuous_rocket;
    recipe->fire_gain_q15 = (gv89_s16)modules->audio_fire_gain_q15;
    recipe->pressure_energy_q15 =
        (gv89_u16)modules->audio_pressure_energy_q15;
    recipe->ammo_motion_q15 = (gv89_u16)modules->audio_ammo_motion_q15;
    recipe->magazine_velocity_q15 =
        (gv89_u16)modules->audio_magazine_velocity_q15;
    recipe->magazine_gain_q15 =
        (gv89_s16)modules->audio_magazine_gain_q15;
    recipe->reload_remove_motion_q15 =
        (gv89_u16)modules->audio_reload_remove_motion_q15;
    recipe->reload_insert_motion_q15 =
        (gv89_u16)modules->audio_reload_insert_motion_q15;
    recipe->dry_receiver_impulse =
        (gv89_s16)modules->audio_dry_receiver_impulse;
    recipe->action_speed_q16 = (gv89_u32)modules->audio_action_speed_q16;
    recipe->casing_velocity = (gv89_u16)modules->audio_casing_velocity;
    recipe->casing_angular_velocity =
        (gv89_u16)modules->audio_casing_angular_velocity;
    recipe->casing_gain_q15 = (gv89_s16)modules->audio_casing_gain_q15;
    recipe->projectile_gain_q15 =
        (gv89_s16)modules->audio_projectile_gain_q15;
    recipe->projectile_proximity_q15 =
        (gv89_u16)modules->audio_projectile_proximity_q15;
    recipe->projectile_instance_limit =
        (gv89_u16)modules->audio_projectile_instance_limit;
    recipe->explosion_gain_q15 =
        (gv89_s16)modules->audio_explosion_gain_q15;
    recipe->rocket_whistle_gain_q15 =
        (gv89_s16)modules->audio_rocket_whistle_gain_q15;
    recipe->rocket_spin_gain_q15 =
        (gv89_s16)modules->audio_rocket_spin_gain_q15;
    return recipe;
}

static int b3d_audio_magazine_for_weapon(int weapon_id)
{
    return b3d_audio_recipe_for_weapon(weapon_id)->magazine_preset;
}

static wsound89_u16 b3d_audio_clip_fill_q15(int clip_ammo,
                                            int clip_capacity)
{
    long fill;
    if (clip_capacity < 1) clip_capacity = 1;
    if (clip_ammo < 0) clip_ammo = 0;
    if (clip_ammo > clip_capacity) clip_ammo = clip_capacity;
    fill = ((long)clip_ammo * 32767L) / (long)clip_capacity;
    if (fill < 0L) fill = 0L;
    if (fill > 32767L) fill = 32767L;
    return (wsound89_u16)fill;
}

static void b3d_audio_emit_action(Blank3DAudio *audio, int weapon_id,
                                  gv89_u32 speed_q16)
{
    const B3DAudioWeaponRecipe *recipe;
    wsse89_event event;
    recipe = b3d_audio_recipe_for_weapon(weapon_id);
    if (!audio || !recipe->enabled) return;
    memset(&event, 0, sizeof(event));
    event.type = WSSE89_EVENT_ACTION_START;
    event.data.action.action = recipe->action;
    event.data.action.speed_q16 = speed_q16 != 0U
        ? speed_q16 : recipe->action_speed_q16;
    (void)b3d_audio_dispatch(audio, &event, 0);
}

static void b3d_audio_emit_ammo(Blank3DAudio *audio, int weapon_id,
                                int clip_ammo, int clip_capacity,
                                wsound89_u16 motion_q15)
{
    const B3DAudioWeaponRecipe *recipe;
    wsse89_event event;
    recipe = b3d_audio_recipe_for_weapon(weapon_id);
    if (!audio || !recipe->enabled) return;
    memset(&event, 0, sizeof(event));
    event.type = WSSE89_EVENT_AMMO;
    event.data.ammo.type = recipe->ammo_type;
    event.data.ammo.fill_q15 = b3d_audio_clip_fill_q15(clip_ammo,
                                                       clip_capacity);
    event.data.ammo.motion_q15 = motion_q15;
    (void)b3d_audio_dispatch(audio, &event, 0);
}

static gv89_u32 b3d_audio_next_key(Blank3DAudio *audio)
{
    if (!audio) return 1U;
    audio->fire_serial++;
    if (audio->fire_serial == 0UL) audio->fire_serial = 1UL;
    return (gv89_u32)audio->fire_serial;
}

static void b3d_audio_render_and_queue(Blank3DAudio *audio, int index)
{
    WAVEHDR *header;
    if (!audio || !audio->initialized || index < 0 || index >= B3D_AUDIO_BUFFER_COUNT)
        return;
    header = &audio->headers[index];
    if (!blank3d_goldie_audio89_render(&audio->goldie, audio->pcm[index],
                                        B3D_AUDIO_FRAMES_PER_BUFFER)) {
        memset(audio->pcm[index], 0, sizeof(audio->pcm[index]));
        b3d_audio_set_status(audio, blank3d_goldie_audio89_status(&audio->goldie));
    }
    header->lpData = (LPSTR)audio->pcm[index];
    header->dwBufferLength = (DWORD)(B3D_AUDIO_FRAMES_PER_BUFFER * 2U * sizeof(gv89_s16));
    header->dwBytesRecorded = 0U;
    header->dwUser = (DWORD_PTR)index;
    header->dwLoops = 0U;
    if (waveOutWrite(audio->wave_out, header, sizeof(*header)) != MMSYSERR_NOERROR)
        b3d_audio_set_status(audio, "waveOutWrite failed");
}

static int b3d_audio_goldie_weapon_render(void *user,
                                           short *dst_interleaved,
                                           unsigned int frames)
{
    wsse89_context *synth;
    if (!user || !dst_interleaved || frames == 0U) return 0;
    synth = (wsse89_context *)user;
    (void)wsse89_render_stereo(synth, dst_interleaved,
                               (gv89_u32)frames, 0);
    return 1;
}

int blank3d_audio_init(Blank3DAudio *audio, int enabled)
{
    wsse89_config config;
    MMRESULT result;
    int i;
    if (!audio) return 0;
    memset(audio, 0, sizeof(*audio));
    audio->enabled = enabled ? 1 : 0;
    if (!audio->enabled) {
        b3d_audio_set_status(audio, "audio disabled by config");
        return 1;
    }

    b3d_audio_fill_storage(audio);
    wsse89_config_defaults(&config);
    config.sample_rate = B3D_AUDIO_RATE;
    config.logical_voice_capacity = B3D_AUDIO_LOGICAL_VOICES;
    config.physical_voice_limit = 96U;
    config.expansion_mask = GSSEXP89_ALL;
    if (!wsse89_init(&audio->synth, &config, &audio->storage)) {
        b3d_audio_set_status(audio, "weapon synth init failed");
        return 0;
    }
    if (!blank3d_goldie_audio89_init(&audio->goldie, &audio->synth,
                                      b3d_audio_goldie_weapon_render,
                                      B3D_AUDIO_RATE)) {
        b3d_audio_set_status(audio, blank3d_goldie_audio89_status(&audio->goldie));
        return 0;
    }

    memset(&audio->format, 0, sizeof(audio->format));
    audio->format.wFormatTag = WAVE_FORMAT_PCM;
    audio->format.nChannels = 2U;
    audio->format.nSamplesPerSec = B3D_AUDIO_RATE;
    audio->format.wBitsPerSample = 16U;
    audio->format.nBlockAlign = (WORD)(audio->format.nChannels * audio->format.wBitsPerSample / 8U);
    audio->format.nAvgBytesPerSec = audio->format.nSamplesPerSec * audio->format.nBlockAlign;
    audio->format.cbSize = 0U;

    result = waveOutOpen(&audio->wave_out, WAVE_MAPPER, &audio->format,
                         0U, 0U, CALLBACK_NULL);
    if (result != MMSYSERR_NOERROR) {
        b3d_audio_set_status(audio, "waveOutOpen failed");
        blank3d_goldie_audio89_shutdown(&audio->goldie);
        return 0;
    }
    audio->initialized = 1;
    for (i = 0; i < B3D_AUDIO_BUFFER_COUNT; ++i) {
        memset(&audio->headers[i], 0, sizeof(audio->headers[i]));
        audio->headers[i].lpData = (LPSTR)audio->pcm[i];
        audio->headers[i].dwBufferLength = (DWORD)(B3D_AUDIO_FRAMES_PER_BUFFER * 2U * sizeof(gv89_s16));
        if (waveOutPrepareHeader(audio->wave_out, &audio->headers[i],
                                 sizeof(audio->headers[i])) != MMSYSERR_NOERROR) {
            b3d_audio_set_status(audio, "waveOutPrepareHeader failed");
            blank3d_audio_shutdown(audio);
            return 0;
        }
        b3d_audio_render_and_queue(audio, i);
    }
    b3d_audio_set_status(audio, "Goldie Matryoshka audio online: weapon synth -> SFX/Weapons -> MASTER -> WinMM");
    return 1;
}

void blank3d_audio_pump(Blank3DAudio *audio)
{
    int i;
    if (!audio || !audio->initialized || !audio->enabled) return;
    for (i = 0; i < B3D_AUDIO_BUFFER_COUNT; ++i) {
        if (audio->headers[i].dwFlags & WHDR_DONE)
            b3d_audio_render_and_queue(audio, i);
    }
}

void blank3d_audio_shutdown(Blank3DAudio *audio)
{
    int i;
    if (!audio) return;
    if (audio->gatling_active)
        blank3d_audio_gatling_release(audio);
    if (audio->wave_out) {
        waveOutReset(audio->wave_out);
        for (i = 0; i < B3D_AUDIO_BUFFER_COUNT; ++i) {
            if (audio->headers[i].dwFlags & WHDR_PREPARED)
                waveOutUnprepareHeader(audio->wave_out, &audio->headers[i],
                                       sizeof(audio->headers[i]));
        }
        waveOutClose(audio->wave_out);
    }
    audio->wave_out = 0;
    blank3d_goldie_audio89_shutdown(&audio->goldie);
    audio->initialized = 0;
}

int blank3d_audio_set_bus_gain_q15(Blank3DAudio *audio, int bus, short gain_q15)
{
    if (!audio || !audio->initialized) return 0;
    return blank3d_goldie_audio89_set_bus_gain_q15(&audio->goldie, bus, gain_q15);
}

int blank3d_audio_set_bus_mute(Blank3DAudio *audio, int bus, int mute_on)
{
    if (!audio || !audio->initialized) return 0;
    return blank3d_goldie_audio89_set_bus_mute(&audio->goldie, bus, mute_on);
}

int blank3d_audio_play_pcm(Blank3DAudio *audio, int bus,
                           const goldie_audio89_pcm_view *pcm, int loop,
                           goldie_audio89_voice_handle *out_voice)
{
    if (!audio || !audio->initialized) return 0;
    return blank3d_goldie_audio89_play_pcm(&audio->goldie, bus, pcm, loop, out_voice);
}

int blank3d_audio_decode_wav_file(const char *path,
                                  unsigned char *file_workspace,
                                  unsigned long file_workspace_bytes,
                                  short *pcm_dst,
                                  unsigned long pcm_sample_capacity,
                                  goldie_audio89_pcm_view *out_pcm)
{
    return goldie_audio89_decode_wav_file(path, file_workspace,
                                          file_workspace_bytes, pcm_dst,
                                          pcm_sample_capacity, out_pcm)
           == GOLDIE_AUDIO89_OK ? 1 : 0;
}

int blank3d_audio_decode_mp3_file(const char *path,
                                  short *pcm_dst,
                                  unsigned long pcm_sample_capacity,
                                  goldie_audio89_pcm_view *out_pcm,
                                  char *error_text,
                                  unsigned int error_text_capacity)
{
    return goldie_audio89_decode_mp3_file(path, pcm_dst,
                                          pcm_sample_capacity, out_pcm,
                                          error_text, error_text_capacity)
           == GOLDIE_AUDIO89_OK ? 1 : 0;
}

void blank3d_audio_fire_sync(Blank3DAudio *audio, int weapon_id,
                             int clip_ammo, int clip_capacity)
{
    const B3DAudioWeaponRecipe *recipe;
    wsse89_event event;
    gv89_handle handle;
    if (!audio || !audio->initialized || !audio->enabled) return;
    recipe = b3d_audio_recipe_for_weapon(weapon_id);
    if (!recipe->enabled) return;

    /* Select the exact firearm family before every accepted shot.  This keeps
       report DNA, receiver/body response and muzzle gas tied to the projectile
       family even when the player cycles weapons during a dense audio tail. */
    memset(&event, 0, sizeof(event));
    event.type = WSSE89_EVENT_WEAPON_PROFILE;
    wsounddna89_profile_defaults(recipe->profile,
                                 &event.data.weapon_profile);
    if (recipe->muzzle_device == WSOUNDMUZZLEDEVICE89_BRAKE)
        event.data.weapon_profile.muzzle_brake_q15 = 21000U;
    else if (recipe->muzzle_device == WSOUNDMUZZLEDEVICE89_COMPENSATOR)
        event.data.weapon_profile.muzzle_brake_q15 = 12500U;
    else if (recipe->muzzle_device == WSOUNDMUZZLEDEVICE89_PORTED)
        event.data.weapon_profile.muzzle_brake_q15 = 8500U;
    (void)b3d_audio_dispatch(audio, &event, 0);

    memset(&event, 0, sizeof(event));
    event.type = WSSE89_EVENT_WEAPON_MODE;
    event.data.weapon_mode = WSOUNDDNA89_MODE_HYBRID;
    (void)b3d_audio_dispatch(audio, &event, 0);

    /* Mechanism and ammunition movement are event-synchronous with the shot,
       while casing contact remains synchronized to CASING_REQUEST. */
    b3d_audio_emit_action(audio, weapon_id, recipe->action_speed_q16);
    b3d_audio_emit_ammo(audio, weapon_id, clip_ammo, clip_capacity,
                        recipe->ammo_motion_q15);

    memset(&event, 0, sizeof(event));
    event.type = WSSE89_EVENT_WEAPON_FIRE;
    wsse89_weapon_fire_defaults(&event.data.weapon_fire);
    event.data.weapon_fire.instance_key = b3d_audio_next_key(audio);
    event.data.weapon_fire.instance_limit =
        recipe->projectile_instance_limit > 0U
        ? recipe->projectile_instance_limit : 24U;
    event.data.weapon_fire.gain_q15 = recipe->fire_gain_q15;
    event.data.weapon_fire.pressure_energy_q15 =
        recipe->pressure_energy_q15;
    memset(&handle, 0, sizeof(handle));
    (void)b3d_audio_dispatch(audio, &event, &handle);
}

void blank3d_audio_fire(Blank3DAudio *audio, int weapon_id)
{
    blank3d_audio_fire_sync(audio, weapon_id, 1, 1);
}


void blank3d_audio_gatling_begin(Blank3DAudio *audio)
{
    wsse89_event event;
    if (!audio || !audio->initialized || !audio->enabled) return;
    if (audio->gatling_active) return;
    memset(&event, 0, sizeof(event));
    event.type = WSSE89_EVENT_GATLING_START;
    wsse89_gatling_defaults(&event.data.gatling_start);
    audio->gatling_instance_key = b3d_audio_next_key(audio);
    event.data.gatling_start.instance_key = audio->gatling_instance_key;
    event.data.gatling_start.motor_preset = GGM89_PRESET_M134D;
    event.data.gatling_start.rotator_preset = GGTR89_PRESET_MEDIUM;
    event.data.gatling_start.gain_q15 = 27800;
    event.data.gatling_start.motor_gain_q15 = 9000;
    event.data.gatling_start.rotator_gain_q15 = 9800;
    event.data.gatling_start.whistle_gain_q15 = 6200;
    if (b3d_audio_dispatch(audio, &event, 0)) {
        audio->gatling_active = 1;
        audio->gatling_firing = 0;
    } else {
        audio->gatling_instance_key = 0U;
    }
}

void blank3d_audio_gatling_fire_start(Blank3DAudio *audio)
{
    wsse89_event event;
    if (!audio || !audio->initialized || !audio->enabled) return;
    if (!audio->gatling_active)
        blank3d_audio_gatling_begin(audio);
    if (!audio->gatling_active || audio->gatling_firing) return;

    memset(&event, 0, sizeof(event));
    event.type = WSSE89_EVENT_GATLING_FIRE_START;
    event.data.gatling_fire.instance_key = audio->gatling_instance_key;
    event.data.gatling_fire.firing_load_q15 = 29200;
    if (b3d_audio_dispatch(audio, &event, 0)) {
        memset(&event, 0, sizeof(event));
        event.type = WSSE89_EVENT_BELT_START;
        event.data.belt.rpm = 1800U;
        event.data.belt.tension_q15 = 27600U;
        (void)b3d_audio_dispatch(audio, &event, 0);
        audio->gatling_firing = 1;
    }
}

void blank3d_audio_gatling_release(Blank3DAudio *audio)
{
    wsse89_event event;
    if (!audio || !audio->initialized || !audio->enabled) return;
    if (!audio->gatling_active) return;

    memset(&event, 0, sizeof(event));
    event.type = WSSE89_EVENT_BELT_STOP;
    (void)b3d_audio_dispatch(audio, &event, 0);

    memset(&event, 0, sizeof(event));
    event.type = WSSE89_EVENT_GATLING_FIRE_STOP;
    event.data.gatling_control.instance_key = audio->gatling_instance_key;
    (void)b3d_audio_dispatch(audio, &event, 0);
    audio->gatling_firing = 0;
    audio->gatling_active = 0;
    audio->gatling_instance_key = 0U;
}

static void b3d_audio_magazine(Blank3DAudio *audio, int weapon_id, int action)
{
    const B3DAudioWeaponRecipe *recipe;
    wsse89_event event;
    gv89_handle handle;
    if (!audio || !audio->initialized || !audio->enabled) return;
    recipe = b3d_audio_recipe_for_weapon(weapon_id);
    if (!recipe->enabled || !recipe->detachable_magazine) return;
    memset(&event, 0, sizeof(event));
    event.type = WSSE89_EVENT_MAGAZINE_ACTION;
    wsse89_magazine_defaults(&event.data.magazine);
    event.data.magazine.preset = b3d_audio_magazine_for_weapon(weapon_id);
    event.data.magazine.action = action;
    event.data.magazine.velocity_q15 = recipe->magazine_velocity_q15;
    event.data.magazine.gain_q15 = recipe->magazine_gain_q15;
    event.data.magazine.instance_key = b3d_audio_next_key(audio);
    memset(&handle, 0, sizeof(handle));
    (void)b3d_audio_dispatch(audio, &event, &handle);
}

static void b3d_audio_receiver_click(Blank3DAudio *audio, int weapon_id)
{
    const B3DAudioWeaponRecipe *recipe;
    wsse89_event event;
    recipe = b3d_audio_recipe_for_weapon(weapon_id);
    if (!audio || !recipe->enabled) return;
    memset(&event, 0, sizeof(event));
    event.type = WSSE89_EVENT_RECEIVER_EXCITE;
    event.data.receiver_impulse = recipe->dry_receiver_impulse;
    (void)b3d_audio_dispatch(audio, &event, 0);
}

void blank3d_audio_reload_begin_sync(Blank3DAudio *audio, int weapon_id,
                                     int clip_ammo, int clip_capacity)
{
    const B3DAudioWeaponRecipe *recipe;
    recipe = b3d_audio_recipe_for_weapon(weapon_id);
    if (!audio || !recipe->enabled) return;
    if (recipe->detachable_magazine)
        b3d_audio_magazine(audio, weapon_id, WMAG89_ACTION_REMOVE);
    else
        b3d_audio_emit_action(audio, weapon_id,
                              (recipe->action_speed_q16 * 3U) / 4U);
    b3d_audio_emit_ammo(audio, weapon_id, clip_ammo, clip_capacity,
                        recipe->reload_remove_motion_q15);
}

void blank3d_audio_reload_end_sync(Blank3DAudio *audio, int weapon_id,
                                   int clip_ammo, int clip_capacity)
{
    const B3DAudioWeaponRecipe *recipe;
    recipe = b3d_audio_recipe_for_weapon(weapon_id);
    if (!audio || !recipe->enabled) return;
    if (recipe->detachable_magazine) {
        b3d_audio_magazine(audio, weapon_id, WMAG89_ACTION_INSERT);
        b3d_audio_magazine(audio, weapon_id, WMAG89_ACTION_SEAT_TAP);
    }
    /* Chamber, close the cylinder/breech, pump, cycle the bolt or settle the
       feed mechanism according to the selected family. */
    b3d_audio_emit_action(audio, weapon_id, recipe->action_speed_q16);
    b3d_audio_emit_ammo(audio, weapon_id, clip_ammo, clip_capacity,
                        recipe->reload_insert_motion_q15);
}

void blank3d_audio_reload_begin(Blank3DAudio *audio, int weapon_id)
{
    blank3d_audio_reload_begin_sync(audio, weapon_id, 0, 1);
}

void blank3d_audio_reload_end(Blank3DAudio *audio, int weapon_id)
{
    blank3d_audio_reload_end_sync(audio, weapon_id, 1, 1);
}

void blank3d_audio_dry_fire(Blank3DAudio *audio, int weapon_id)
{
    const B3DAudioWeaponRecipe *recipe;
    if (!audio || !audio->initialized || !audio->enabled) return;
    recipe = b3d_audio_recipe_for_weapon(weapon_id);
    if (!recipe->enabled) return;
    b3d_audio_emit_action(audio, weapon_id,
                          (recipe->action_speed_q16 * 2U) / 3U);
    b3d_audio_receiver_click(audio, weapon_id);
}


void blank3d_audio_casing(Blank3DAudio *audio, int weapon_id)
{
    const B3DAudioWeaponRecipe *recipe;
    wsse89_event event;
    gv89_handle handle;
    if (!audio || !audio->initialized || !audio->enabled) return;
    recipe = b3d_audio_recipe_for_weapon(weapon_id);
    if (!recipe->enabled || !recipe->emits_casing) return;
    memset(&event, 0, sizeof(event));
    event.type = WSSE89_EVENT_CASING;
    gsse89_casing_defaults(&event.data.casing);
    event.data.casing.shell = recipe->shell;
    event.data.casing.surface = GT89_SURFACE_CONCRETE;
    event.data.casing.velocity = recipe->casing_velocity;
    event.data.casing.angular_velocity = recipe->casing_angular_velocity;
    event.data.casing.variation = (gt89_u8)(b3d_audio_next_key(audio) & 255U);
    event.data.casing.instance_key = b3d_audio_next_key(audio);
    event.data.casing.instance_limit =
        recipe->projectile_instance_limit > 0U
        ? recipe->projectile_instance_limit : 24U;
    event.data.casing.gain_q15 = recipe->casing_gain_q15;
    memset(&handle, 0, sizeof(handle));
    (void)b3d_audio_dispatch(audio, &event, &handle);
}


void blank3d_audio_projectile_begin(Blank3DAudio *audio, int weapon_id,
                                    int speed, gv89_u32 *primary_key,
                                    gv89_u32 *secondary_key)
{
    const B3DAudioWeaponRecipe *recipe;
    wsse89_event event;
    gv89_handle handle;
    gv89_u32 whistle_key;
    gv89_u32 spin_key;
    if (primary_key) *primary_key = 0U;
    if (secondary_key) *secondary_key = 0U;
    if (!audio || !audio->initialized || !audio->enabled) return;
    recipe = b3d_audio_recipe_for_weapon(weapon_id);
    if (!recipe->enabled) return;

    memset(&event, 0, sizeof(event));
    event.type = WSSE89_EVENT_PROJECTILE;
    gssw89_projectile_defaults(&event.data.projectile);
    if (recipe->projectile_mode == B3D_AUDIO_PROJECTILE_NONE)
        return;
    if (recipe->projectile_mode == B3D_AUDIO_PROJECTILE_NEAR_MISS)
        event.data.projectile.mode = WSOUNDPROJECTILE89_NEAR_MISS_SNAP;
    else if (recipe->projectile_mode == B3D_AUDIO_PROJECTILE_SUPERSONIC)
        event.data.projectile.mode = WSOUNDPROJECTILE89_SUPERSONIC_NWAVE;
    else if (recipe->projectile_mode == B3D_AUDIO_PROJECTILE_PELLET_SWARM)
        event.data.projectile.mode = WSOUNDPROJECTILE89_PELLET_SWARM;
    else if (recipe->projectile_mode == B3D_AUDIO_PROJECTILE_TRACER)
        event.data.projectile.mode = WSOUNDPROJECTILE89_TRACER_FLYBY;
    else if (speed < 28)
        event.data.projectile.mode = WSOUNDPROJECTILE89_NEAR_MISS_SNAP;
    else
        event.data.projectile.mode = WSOUNDPROJECTILE89_SUPERSONIC_NWAVE;
    if (speed < 1) speed = 1;
    if (speed > 100) speed = 100;
    event.data.projectile.amplitude_q15 = (wsound89_u16)(11000 + speed * 180);
    event.data.projectile.proximity_q15 = recipe->projectile_proximity_q15;
    event.data.projectile.common.instance_key = b3d_audio_next_key(audio);
    event.data.projectile.common.instance_limit =
        recipe->projectile_instance_limit > 0U
        ? recipe->projectile_instance_limit : 24U;
    event.data.projectile.common.gain_q15 = recipe->projectile_gain_q15;
    memset(&handle, 0, sizeof(handle));
    (void)b3d_audio_dispatch(audio, &event, &handle);

    /* RPG flight is continuous and follows the lifetime of the actual engine
       projectile.  Grenades and bullets use finite flyby voices only. */
    if (!recipe->continuous_rocket) return;
    whistle_key = b3d_audio_next_key(audio);
    spin_key = b3d_audio_next_key(audio);

    memset(&event, 0, sizeof(event));
    event.type = WSSE89_EVENT_ROCKET_WHISTLE_START;
    wsse89_rocket_whistle_defaults(&event.data.rocket_whistle_start);
    event.data.rocket_whistle_start.preset = GWH89_PRESET_RPG7_SUSTAINED;
    event.data.rocket_whistle_start.radial_velocity_mps = -speed;
    event.data.rocket_whistle_start.distance_gain_q15 = 21000U;
    event.data.rocket_whistle_start.pitch_scale_q15 = 32767U;
    event.data.rocket_whistle_start.auto_hold_ms = 0U;
    event.data.rocket_whistle_start.gain_q15 = recipe->rocket_whistle_gain_q15;
    event.data.rocket_whistle_start.instance_key = whistle_key;
    if (!b3d_audio_dispatch(audio, &event, 0)) whistle_key = 0U;

    memset(&event, 0, sizeof(event));
    event.type = WSSE89_EVENT_ROCKET_SPIN_START;
    wsse89_rocket_spin_defaults(&event.data.rocket_spin_start);
    event.data.rocket_spin_start.preset = GRS89_PRESET_RPG7_SUSTAINER;
    event.data.rocket_spin_start.sustain_ms = 7500U;
    event.data.rocket_spin_start.release_ms = 180U;
    event.data.rocket_spin_start.gain_q15 = recipe->rocket_spin_gain_q15;
    event.data.rocket_spin_start.instance_key = spin_key;
    if (!b3d_audio_dispatch(audio, &event, 0)) spin_key = 0U;

    if (primary_key) *primary_key = whistle_key;
    if (secondary_key) *secondary_key = spin_key;
}

void blank3d_audio_projectile_motion(Blank3DAudio *audio, int weapon_id,
                                     int speed, gv89_u32 primary_key,
                                     gv89_u32 secondary_key)
{
    const B3DAudioWeaponRecipe *recipe;
    wsse89_event event;
    if (!audio || !audio->initialized || !audio->enabled) return;
    recipe = b3d_audio_recipe_for_weapon(weapon_id);
    if (!recipe->enabled || !recipe->continuous_rocket) return;
    if (speed < 1) speed = 1;
    if (speed > 180) speed = 180;
    if (primary_key != 0U) {
        memset(&event, 0, sizeof(event));
        event.type = WSSE89_EVENT_ROCKET_WHISTLE_MOTION;
        event.data.rocket_whistle_motion.instance_key = primary_key;
        event.data.rocket_whistle_motion.radial_velocity_mps = -speed;
        event.data.rocket_whistle_motion.distance_gain_q15 = 22000U;
        event.data.rocket_whistle_motion.gain_q15 = recipe->rocket_whistle_gain_q15;
        event.data.rocket_whistle_motion.pan_q15 = 0;
        (void)b3d_audio_dispatch(audio, &event, 0);
    }
    if (secondary_key != 0U) {
        memset(&event, 0, sizeof(event));
        event.type = WSSE89_EVENT_ROCKET_SPIN_MOTION;
        event.data.rocket_spin_motion.instance_key = secondary_key;
        event.data.rocket_spin_motion.gain_q15 = recipe->rocket_spin_gain_q15;
        event.data.rocket_spin_motion.pan_q15 = 0;
        (void)b3d_audio_dispatch(audio, &event, 0);
    }
}

void blank3d_audio_projectile_end(Blank3DAudio *audio, int weapon_id,
                                  gv89_u32 primary_key,
                                  gv89_u32 secondary_key)
{
    const B3DAudioWeaponRecipe *recipe;
    wsse89_event event;
    if (!audio || !audio->initialized || !audio->enabled) return;
    recipe = b3d_audio_recipe_for_weapon(weapon_id);
    if (!recipe->enabled || !recipe->continuous_rocket) return;
    if (primary_key != 0U) {
        memset(&event, 0, sizeof(event));
        event.type = WSSE89_EVENT_ROCKET_WHISTLE_RELEASE;
        event.data.rocket_whistle_control.instance_key = primary_key;
        (void)b3d_audio_dispatch(audio, &event, 0);
    }
    if (secondary_key != 0U) {
        memset(&event, 0, sizeof(event));
        event.type = WSSE89_EVENT_ROCKET_SPIN_STOP;
        event.data.rocket_spin_control.instance_key = secondary_key;
        (void)b3d_audio_dispatch(audio, &event, 0);
    }
}

void blank3d_audio_projectile(Blank3DAudio *audio, int weapon_id, int speed)
{
    gv89_u32 primary_key;
    gv89_u32 secondary_key;
    blank3d_audio_projectile_begin(audio, weapon_id, speed,
                                   &primary_key, &secondary_key);
    /* Compatibility wrapper: one-shot callers do not own projectile lifetime,
       so continuous rocket voices are released immediately. */
    blank3d_audio_projectile_end(audio, weapon_id,
                                 primary_key, secondary_key);
}


void blank3d_audio_explosion(Blank3DAudio *audio, int weapon_id, int strength)
{
    const B3DAudioWeaponRecipe *recipe;
    wsse89_event event;
    gv89_handle handle;
    if (!audio || !audio->initialized || !audio->enabled) return;
    recipe = b3d_audio_recipe_for_weapon(weapon_id);
    if (!recipe->enabled || recipe->explosion_mode == B3D_AUDIO_EXPLOSION_NONE)
        return;
    if (strength < 1) strength = 1;
    if (strength > 100) strength = 100;
    memset(&event, 0, sizeof(event));
    if (recipe->explosion_mode == B3D_AUDIO_EXPLOSION_ROCKET) {
        event.type = WSSE89_EVENT_ROCKET_BLAST;
        gsso89_rocket_defaults(&event.data.rocket);
        event.data.rocket.preset_id = WSRB89_PRESET_COMPACT_RPG;
        event.data.rocket.velocity_q15 = (gv89_u16)(18000 + strength * 140);
        event.data.rocket.common.instance_key = b3d_audio_next_key(audio);
        event.data.rocket.common.instance_limit = 8U;
        event.data.rocket.common.gain_q15 = recipe->explosion_gain_q15;
    } else {
        event.type = WSSE89_EVENT_GRENADE_BLAST;
        gsso89_grenade_defaults(&event.data.grenade);
        event.data.grenade.preset_id = strength > 70 ? 6 : 2;
        event.data.grenade.intensity_q15 = (gv89_s16)(12000 + strength * 190);
        event.data.grenade.common.instance_key = b3d_audio_next_key(audio);
        event.data.grenade.common.instance_limit = 8U;
        event.data.grenade.common.gain_q15 = recipe->explosion_gain_q15;
    }
    memset(&handle, 0, sizeof(handle));
    (void)b3d_audio_dispatch(audio, &event, &handle);
}

void blank3d_audio_impact(Blank3DAudio *audio, int material_id, int strength)
{
    wsse89_event event;
    gv89_handle handle;
    if (!audio || !audio->initialized || !audio->enabled) return;
    memset(&event, 0, sizeof(event));
    event.type = WSSE89_EVENT_IMPACT;
    gssw89_impact_defaults(&event.data.impact);
    if (material_id == 1) event.data.impact.material = WSOUNDIMPACT89_METAL;
    else if (material_id == 2) event.data.impact.material = WSOUNDIMPACT89_WOOD;
    else event.data.impact.material = WSOUNDIMPACT89_CONCRETE;
    if (strength < 1) strength = 1;
    if (strength > 100) strength = 100;
    event.data.impact.energy_q15 = (wsound89_u16)(6000 + strength * 250);
    memset(&handle, 0, sizeof(handle));
    (void)b3d_audio_dispatch(audio, &event, &handle);
}

const char *blank3d_audio_status(const Blank3DAudio *audio)
{
    return audio ? audio->status : "audio unavailable";
}
