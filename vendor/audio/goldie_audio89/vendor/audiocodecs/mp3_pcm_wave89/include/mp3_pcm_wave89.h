#ifndef MP3_PCM_WAVE89_H
#define MP3_PCM_WAVE89_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include "audio89.h"
#include "mp3_frame89.h"
#include "mp3_acm_codec89.h"

#define MP3_PCM_WAVE89_SLOTS 4
#define MP3_PCM_WAVE89_PCM_BYTES 16384
#define MP3_PCM_WAVE89_ERROR_CAP 256

typedef struct mp3_pcm_wave89_state {
    FILE *file;
    HWAVEOUT out;
    mp3_acm_codec89 codec;
    mp3_frame89_info first_info;
    unsigned long first_frame_offset;
    unsigned char mp3_frame[MP3_FRAME89_MAX_FRAME_BYTES];
    unsigned char decode_pcm[MP3_PCM_WAVE89_PCM_BYTES];
    WAVEHDR headers[MP3_PCM_WAVE89_SLOTS];
    unsigned char buffers[MP3_PCM_WAVE89_SLOTS][MP3_PCM_WAVE89_PCM_BYTES];
    int prepared[MP3_PCM_WAVE89_SLOTS];
    int opened;
    int playing;
    int paused;
    int loop;
    int eof;
    char error[MP3_PCM_WAVE89_ERROR_CAP];
} mp3_pcm_wave89_state;

void mp3_pcm_wave89_init(mp3_pcm_wave89_state *st);
const audio89_provider *mp3_pcm_wave89_provider(void);

#endif
