#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <mmsystem.h>
#include <string.h>
#include "mgifwinaudio89.h"

static int mgifwinaudio89_open(mgifwinaudio89_state *state)
{
    MMRESULT result;
    if (state->opened) {
        return 1;
    }
    memset(&state->format, 0, sizeof(state->format));
    state->format.wFormatTag = WAVE_FORMAT_PCM;
    state->format.nChannels = 2U;
    state->format.nSamplesPerSec = state->synth.output_sample_rate;
    state->format.wBitsPerSample = 16U;
    state->format.nBlockAlign = 4U;
    state->format.nAvgBytesPerSec =
        state->format.nSamplesPerSec * (DWORD)state->format.nBlockAlign;
    result = waveOutOpen(
        &state->device,
        WAVE_MAPPER,
        &state->format,
        (DWORD_PTR)0,
        (DWORD_PTR)0,
        CALLBACK_NULL
    );
    if (result != MMSYSERR_NOERROR) {
        state->device = (HWAVEOUT)0;
        return 0;
    }
    state->opened = 1;
    return 1;
}

static int mgifwinaudio89_queue(
    mgifwinaudio89_state *state,
    int index)
{
    MMRESULT result;
    gif89_u32 rendered;
    if (!mgeneralsynth89_is_active(&state->synth)) {
        return 0;
    }
    rendered = mgeneralsynth89_process_interleaved(
        &state->synth,
        state->audio[index],
        MGIFWINAUDIO89_BUFFER_FRAMES
    );
    if (rendered != MGIFWINAUDIO89_BUFFER_FRAMES) {
        return -1;
    }
    memset(&state->header[index], 0, sizeof(state->header[index]));
    state->header[index].lpData = (LPSTR)state->audio[index];
    state->header[index].dwBufferLength = (DWORD)(
        MGIFWINAUDIO89_BUFFER_FRAMES * 2U * sizeof(gif89_s16)
    );
    result = waveOutPrepareHeader(
        state->device,
        &state->header[index],
        (UINT)sizeof(WAVEHDR)
    );
    if (result != MMSYSERR_NOERROR) {
        return -1;
    }
    state->prepared[index] = 1;
    result = waveOutWrite(
        state->device,
        &state->header[index],
        (UINT)sizeof(WAVEHDR)
    );
    if (result != MMSYSERR_NOERROR) {
        (void)waveOutUnprepareHeader(
            state->device,
            &state->header[index],
            (UINT)sizeof(WAVEHDR)
        );
        state->prepared[index] = 0;
        return -1;
    }
    return 1;
}

void mgifwinaudio89_init(mgifwinaudio89_state *state)
{
    if (state == (mgifwinaudio89_state *)0) {
        return;
    }
    memset(state, 0, sizeof(*state));
    mgeneralsynth89_init(&state->synth, 44100U);
}

int mgifwinaudio89_play(
    mgifwinaudio89_state *state,
    const char *database_path,
    const char *preset_id,
    int gain_q15,
    int pan_q15,
    int loop)
{
    int i;
    int result;
    if (state == (mgifwinaudio89_state *)0) {
        return 0;
    }
    if (!mgeneralsynth89_trigger(
            &state->synth,
            database_path,
            preset_id,
            gain_q15,
            pan_q15,
            loop)) {
        return 0;
    }
    if (!mgifwinaudio89_open(state)) {
        return 0;
    }
    result = 1;
    i = 0;
    while (i < MGIFWINAUDIO89_BUFFER_COUNT) {
        if (!state->prepared[i]) {
            if (mgifwinaudio89_queue(state, i) < 0) {
                result = 0;
            }
        }
        ++i;
    }
    return result;
}

int mgifwinaudio89_pump(mgifwinaudio89_state *state)
{
    int i;
    int result;
    int queued_before;
    if (state == (mgifwinaudio89_state *)0 || !state->opened) {
        return 1;
    }
    result = 1;
    queued_before = 0;
    i = 0;
    while (i < MGIFWINAUDIO89_BUFFER_COUNT) {
        if (state->prepared[i] &&
            (state->header[i].dwFlags & WHDR_DONE) == 0U) {
            ++queued_before;
        }
        if (state->prepared[i] &&
            (state->header[i].dwFlags & WHDR_DONE) != 0U) {
            if (waveOutUnprepareHeader(
                    state->device,
                    &state->header[i],
                    (UINT)sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
                result = 0;
            }
            state->prepared[i] = 0;
            memset(&state->header[i], 0, sizeof(state->header[i]));
        }
        ++i;
    }
    if (queued_before == 0 &&
        mgeneralsynth89_is_active(&state->synth)) {
        ++state->underrun_recovery_count;
    }
    i = 0;
    while (i < MGIFWINAUDIO89_BUFFER_COUNT) {
        if (!state->prepared[i] &&
            mgeneralsynth89_is_active(&state->synth)) {
            if (mgifwinaudio89_queue(state, i) < 0) {
                result = 0;
            }
        }
        ++i;
    }
    return result;
}

int mgifwinaudio89_stop(mgifwinaudio89_state *state)
{
    int i;
    if (state == (mgifwinaudio89_state *)0) {
        return 0;
    }
    mgeneralsynth89_stop(&state->synth);
    if (state->opened) {
        (void)waveOutReset(state->device);
        i = 0;
        while (i < MGIFWINAUDIO89_BUFFER_COUNT) {
            if (state->prepared[i]) {
                (void)waveOutUnprepareHeader(
                    state->device,
                    &state->header[i],
                    (UINT)sizeof(WAVEHDR)
                );
            }
            state->prepared[i] = 0;
            memset(&state->header[i], 0, sizeof(state->header[i]));
            ++i;
        }
        (void)waveOutClose(state->device);
    }
    state->device = (HWAVEOUT)0;
    state->opened = 0;
    memset(&state->format, 0, sizeof(state->format));
    return 1;
}

void mgifwinaudio89_stop_loop(mgifwinaudio89_state *state)
{
    if (state != (mgifwinaudio89_state *)0) {
        mgeneralsynth89_stop_loop(&state->synth);
    }
}
