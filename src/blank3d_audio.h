#ifndef BLANK3D_AUDIO_H
#define BLANK3D_AUDIO_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

#include "weapon_synth_sound_engine89.h"
#include "blank3d_goldie_audio89.h"

#ifdef __cplusplus
extern "C" {
#endif

#define B3D_AUDIO_RATE 44100U
#define B3D_AUDIO_LOGICAL_VOICES 128U
#define B3D_AUDIO_BUFFER_COUNT 8
#define B3D_AUDIO_FRAMES_PER_BUFFER 512U

#define B3D_AUDIO_REPORT_VOICES 16
#define B3D_AUDIO_CASING_VOICES 32
#define B3D_AUDIO_FIRE_VOICES 16
#define B3D_AUDIO_BULLET_VOICES 32
#define B3D_AUDIO_GRENADE_VOICES 8
#define B3D_AUDIO_ROCKET_VOICES 8
#define B3D_AUDIO_PROJECTILE_VOICES 32
#define B3D_AUDIO_IMPACT_VOICES 32
#define B3D_AUDIO_RICOCHET_VOICES 16

typedef struct Blank3DAudioTag {
    int initialized;
    int enabled;
    HWAVEOUT wave_out;
    WAVEFORMATEX format;
    WAVEHDR headers[B3D_AUDIO_BUFFER_COUNT];
    gv89_s16 pcm[B3D_AUDIO_BUFFER_COUNT][B3D_AUDIO_FRAMES_PER_BUFFER * 2U];

    wsse89_context synth;
    Blank3DGoldieAudio89 goldie;
    wsse89_storage storage;
    gv89_voice logical_voices[B3D_AUDIO_LOGICAL_VOICES];
    gssr89_voice report_voices[B3D_AUDIO_REPORT_VOICES];
    gsse89_casing_voice casing_voices[B3D_AUDIO_CASING_VOICES];
    gsse89_fire_voice fire_voices[B3D_AUDIO_FIRE_VOICES];
    gsso89_bullet_voice bullet_voices[B3D_AUDIO_BULLET_VOICES];
    gsso89_grenade_voice grenade_voices[B3D_AUDIO_GRENADE_VOICES];
    gsso89_rocket_voice rocket_voices[B3D_AUDIO_ROCKET_VOICES];
    gssw89_projectile_voice projectile_voices[B3D_AUDIO_PROJECTILE_VOICES];
    gssw89_impact_voice impact_voices[B3D_AUDIO_IMPACT_VOICES];
    gssw89_ricochet_voice ricochet_voices[B3D_AUDIO_RICOCHET_VOICES];
    wsound89_i16 expansion_outdoor[B3D_AUDIO_RATE];
    wsound89_i16 expansion_portal[B3D_AUDIO_RATE / 2U];
    wsound89_i16 expansion_spatial_l[64];
    wsound89_i16 expansion_spatial_r[64];
    wsound89_i16 world_room[10000];
    wsound89_i16 world_prop[B3D_AUDIO_RATE + 4U];
    unsigned long fire_serial;
    gv89_u32 gatling_instance_key;
    int gatling_active;
    int gatling_firing;
    unsigned int dispatch_failures;
    char status[128];
} Blank3DAudio;

int blank3d_audio_init(Blank3DAudio *audio, int enabled);
void blank3d_audio_pump(Blank3DAudio *audio);
void blank3d_audio_shutdown(Blank3DAudio *audio);
void blank3d_audio_fire(Blank3DAudio *audio, int weapon_id);
void blank3d_audio_fire_sync(Blank3DAudio *audio, int weapon_id,
                             int clip_ammo, int clip_capacity);
void blank3d_audio_gatling_begin(Blank3DAudio *audio);
void blank3d_audio_gatling_fire_start(Blank3DAudio *audio);
void blank3d_audio_gatling_release(Blank3DAudio *audio);
void blank3d_audio_reload_begin(Blank3DAudio *audio, int weapon_id);
void blank3d_audio_reload_end(Blank3DAudio *audio, int weapon_id);
void blank3d_audio_reload_begin_sync(Blank3DAudio *audio, int weapon_id,
                                     int clip_ammo, int clip_capacity);
void blank3d_audio_reload_end_sync(Blank3DAudio *audio, int weapon_id,
                                   int clip_ammo, int clip_capacity);
void blank3d_audio_dry_fire(Blank3DAudio *audio, int weapon_id);
void blank3d_audio_casing(Blank3DAudio *audio, int weapon_id);
void blank3d_audio_projectile(Blank3DAudio *audio, int weapon_id, int speed);
void blank3d_audio_projectile_begin(Blank3DAudio *audio, int weapon_id,
                                    int speed, gv89_u32 *primary_key,
                                    gv89_u32 *secondary_key);
void blank3d_audio_projectile_motion(Blank3DAudio *audio, int weapon_id,
                                     int speed, gv89_u32 primary_key,
                                     gv89_u32 secondary_key);
void blank3d_audio_projectile_end(Blank3DAudio *audio, int weapon_id,
                                  gv89_u32 primary_key,
                                  gv89_u32 secondary_key);
void blank3d_audio_explosion(Blank3DAudio *audio, int weapon_id, int strength);
void blank3d_audio_impact(Blank3DAudio *audio, int material_id, int strength);

int blank3d_audio_set_bus_gain_q15(Blank3DAudio *audio, int bus, short gain_q15);
int blank3d_audio_set_bus_mute(Blank3DAudio *audio, int bus, int mute_on);
int blank3d_audio_play_pcm(Blank3DAudio *audio, int bus,
                           const goldie_audio89_pcm_view *pcm, int loop,
                           goldie_audio89_voice_handle *out_voice);
int blank3d_audio_decode_wav_file(const char *path,
                                  unsigned char *file_workspace,
                                  unsigned long file_workspace_bytes,
                                  short *pcm_dst,
                                  unsigned long pcm_sample_capacity,
                                  goldie_audio89_pcm_view *out_pcm);
int blank3d_audio_decode_mp3_file(const char *path,
                                  short *pcm_dst,
                                  unsigned long pcm_sample_capacity,
                                  goldie_audio89_pcm_view *out_pcm,
                                  char *error_text,
                                  unsigned int error_text_capacity);
const char *blank3d_audio_status(const Blank3DAudio *audio);

#ifdef __cplusplus
}
#endif

#endif
