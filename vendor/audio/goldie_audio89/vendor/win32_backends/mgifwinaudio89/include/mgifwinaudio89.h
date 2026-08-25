#ifndef MGIFWINAUDIO89_H
#define MGIFWINAUDIO89_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <mmsystem.h>
#include "mgeneralsynth89.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MGIFWINAUDIO89_BUFFER_COUNT 8
#define MGIFWINAUDIO89_BUFFER_FRAMES 1024U

typedef struct mgifwinaudio89_state {
    mgeneralsynth89_state synth;
    HWAVEOUT device;
    WAVEFORMATEX format;
    WAVEHDR header[MGIFWINAUDIO89_BUFFER_COUNT];
    gif89_s16 audio[MGIFWINAUDIO89_BUFFER_COUNT]
        [MGIFWINAUDIO89_BUFFER_FRAMES * 2U];
    int prepared[MGIFWINAUDIO89_BUFFER_COUNT];
    unsigned long underrun_recovery_count;
    int opened;
} mgifwinaudio89_state;

void mgifwinaudio89_init(mgifwinaudio89_state *state);
int mgifwinaudio89_play(
    mgifwinaudio89_state *state,
    const char *database_path,
    const char *preset_id,
    int gain_q15,
    int pan_q15,
    int loop
);
int mgifwinaudio89_pump(mgifwinaudio89_state *state);
int mgifwinaudio89_stop(mgifwinaudio89_state *state);
void mgifwinaudio89_stop_loop(mgifwinaudio89_state *state);

#ifdef __cplusplus
}
#endif

#endif
