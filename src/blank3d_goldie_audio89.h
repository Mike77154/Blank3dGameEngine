#ifndef BLANK3D_GOLDIE_AUDIO89_H
#define BLANK3D_GOLDIE_AUDIO89_H

#include "goldie_audio89.h"

#ifdef __cplusplus
extern "C" {
#endif

#define B3D_GOLDIE_STREAM_FRAMES 512U

typedef enum Blank3DAudioBus89Tag {
    B3D_AUDIO_BUS_MASTER = 0,
    B3D_AUDIO_BUS_DIALOGUE = 1,
    B3D_AUDIO_BUS_SFX = 2,
    B3D_AUDIO_BUS_WEAPONS = 3,
    B3D_AUDIO_BUS_UI = 4,
    B3D_AUDIO_BUS_AMBIENCE = 5,
    B3D_AUDIO_BUS_AIR = 6,
    B3D_AUDIO_BUS_BIOFAUNA = 7,
    B3D_AUDIO_BUS_MUSIC = 8,
    B3D_AUDIO_BUS_RHYTHM = 9,
    B3D_AUDIO_BUS_HARMONY = 10,
    B3D_AUDIO_BUS_COUNT = 11
} Blank3DAudioBus89;

typedef int (*Blank3DGoldieAudioRenderFn89)(void *user,
                                             short *dst_interleaved,
                                             unsigned int frames);

typedef struct Blank3DGoldieAudio89Tag {
    goldie_audio89 runtime;
    goldie_audio89_console_handle buses[B3D_AUDIO_BUS_COUNT];
    rm_voice_handle weapon_stream_voice;
    void *weapon_stream_user;
    Blank3DGoldieAudioRenderFn89 weapon_stream_render;
    short weapon_stream_pcm[B3D_GOLDIE_STREAM_FRAMES * 2U];
    unsigned int weapon_stream_at;
    unsigned int weapon_stream_valid;
    int initialized;
    char status[160];
} Blank3DGoldieAudio89;

int blank3d_goldie_audio89_init(Blank3DGoldieAudio89 *host,
                                void *weapon_stream_user,
                                Blank3DGoldieAudioRenderFn89 weapon_stream_render,
                                unsigned long sample_rate);
void blank3d_goldie_audio89_shutdown(Blank3DGoldieAudio89 *host);
int blank3d_goldie_audio89_render(Blank3DGoldieAudio89 *host,
                                  short *dst_interleaved,
                                  unsigned int frames);
int blank3d_goldie_audio89_set_bus_gain_q15(Blank3DGoldieAudio89 *host,
                                             int bus,
                                             short gain_q15);
int blank3d_goldie_audio89_set_bus_mute(Blank3DGoldieAudio89 *host,
                                         int bus,
                                         int mute_on);
int blank3d_goldie_audio89_play_pcm(Blank3DGoldieAudio89 *host,
                                     int bus,
                                     const goldie_audio89_pcm_view *pcm,
                                     int loop,
                                     goldie_audio89_voice_handle *out_voice);
const char *blank3d_goldie_audio89_status(const Blank3DGoldieAudio89 *host);

#ifdef __cplusplus
}
#endif

#endif
