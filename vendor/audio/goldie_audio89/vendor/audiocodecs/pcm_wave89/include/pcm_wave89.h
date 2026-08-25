#ifndef PCM_WAVE89_H
#define PCM_WAVE89_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include "audio89.h"

#define PCM_WAVE89_BUFFER_BYTES 32768
#define PCM_WAVE89_ERROR_CAP 256

typedef struct pcm_wave89_state {
    FILE *file;
    HWAVEOUT out;
    WAVEFORMATEX format;
    WAVEHDR headers[2];
    unsigned char buffers[2][PCM_WAVE89_BUFFER_BYTES];
    unsigned long data_offset;
    unsigned long data_size;
    unsigned long remaining;
    int opened;
    int playing;
    int loop;
    int prepared[2];
    int raw_sample_rate;
    int raw_channels;
    int raw_bits;
    char error[PCM_WAVE89_ERROR_CAP];
} pcm_wave89_state;

void pcm_wave89_init(pcm_wave89_state *st);
void pcm_wave89_set_raw_format(pcm_wave89_state *st, int sample_rate, int channels, int bits);
const audio89_provider *pcm_wave89_provider(void);

#endif
