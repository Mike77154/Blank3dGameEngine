#ifndef MP3_ACM_CODEC89_H
#define MP3_ACM_CODEC89_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>
#include "mp3_frame89.h"

#define MP3_ACM_CODEC89_PCM_CAP 16384
#define MP3_ACM_CODEC89_ERROR_CAP 256

typedef struct mp3_acm_codec89 {
    HACMSTREAM stream;
    MPEGLAYER3WAVEFORMAT source_format;
    WAVEFORMATEX pcm_format;
    int opened;
    int started;
    char error[MP3_ACM_CODEC89_ERROR_CAP];
} mp3_acm_codec89;

void mp3_acm_codec89_init(mp3_acm_codec89 *c);
int mp3_acm_codec89_open(mp3_acm_codec89 *c, const mp3_frame89_info *info);
int mp3_acm_codec89_decode(mp3_acm_codec89 *c,
                           const unsigned char *mp3, int mp3_bytes,
                           unsigned char *pcm, int pcm_cap,
                           int *pcm_bytes, int end_stream);
void mp3_acm_codec89_restart(mp3_acm_codec89 *c);
void mp3_acm_codec89_close(mp3_acm_codec89 *c);
const char *mp3_acm_codec89_last_error(const mp3_acm_codec89 *c);
const WAVEFORMATEX *mp3_acm_codec89_pcm_format(const mp3_acm_codec89 *c);

#endif
