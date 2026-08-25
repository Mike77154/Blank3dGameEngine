#ifndef MWINAUDIO89_H
#define MWINAUDIO89_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <mmsystem.h>
#include "msoundprovider89.h"
#include "mwinplaysound89.h"
#include "mwinmaudio89.h"
#include "mwavplayer89.h"
#include "mgifwinaudio89.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MWINAUDIO89_MAX_DECODED_WAV_BYTES 8388608U

typedef struct mwinaudio89_state {
    mwinplaysound89_state file_backend;
    msoundprovider89 file_provider;

    mwinmaudio89_state decoded_backend;
    maudio89_provider decoded_provider;
    mwavplayer89_state decoded_player;
    unsigned char decoded_wav_bytes[MWINAUDIO89_MAX_DECODED_WAV_BYTES];
    int decoded_ready;

    mgifwinaudio89_state synth_backend;
} mwinaudio89_state;

void mwinaudio89_init(mwinaudio89_state *state);
msoundprovider89 mwinaudio89_make_provider(mwinaudio89_state *state);

#ifdef __cplusplus
}
#endif

#endif
