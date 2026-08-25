#ifndef MP3_MCI89_H
#define MP3_MCI89_H

#include "audio89.h"

#define MP3_MCI89_ERROR_CAP 256

typedef struct mp3_mci89_state {
    int opened;
    int loop;
    int paused;
    char error[MP3_MCI89_ERROR_CAP];
} mp3_mci89_state;

void mp3_mci89_init(mp3_mci89_state *st);
const audio89_provider *mp3_mci89_provider(void);

#endif
