#ifndef MWINMAUDIO89_H
#define MWINMAUDIO89_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <mmsystem.h>
#include "maudio89.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mwinmaudio89_state {
    HWAVEOUT device;
    WAVEHDR header;
    WAVEFORMATEX format;
    int opened;
    int prepared;
    int playing;
    int looping;
} mwinmaudio89_state;

void mwinmaudio89_init(mwinmaudio89_state *state);
void mwinmaudio89_set_loop(mwinmaudio89_state *state, int loop);
maudio89_provider mwinmaudio89_make_provider(mwinmaudio89_state *state);

#ifdef __cplusplus
}
#endif

#endif
