#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <mmsystem.h>
#include <string.h>
#include "mwinmaudio89.h"

static void mwinmaudio89_clear_header(mwinmaudio89_state *state)
{
    memset(&state->header, 0, sizeof(state->header));
    state->prepared = 0;
    state->playing = 0;
}

static int mwinmaudio89_release_buffer(mwinmaudio89_state *state)
{
    if (state->device == (HWAVEOUT)0) {
        mwinmaudio89_clear_header(state);
        return MAUDIO89_OK;
    }
    (void)waveOutReset(state->device);
    if (state->prepared) {
        (void)waveOutUnprepareHeader(
            state->device,
            &state->header,
            (UINT)sizeof(state->header)
        );
    }
    mwinmaudio89_clear_header(state);
    return MAUDIO89_OK;
}

static int mwinmaudio89_open(void *context, const maudio89_format *format)
{
    mwinmaudio89_state *state;
    MMRESULT result;

    state = (mwinmaudio89_state *)context;
    if (state == (mwinmaudio89_state *)0 ||
        format == (const maudio89_format *)0) {
        return MAUDIO89_ERR_ARGUMENT;
    }

    if (state->device != (HWAVEOUT)0) {
        (void)mwinmaudio89_release_buffer(state);
        (void)waveOutClose(state->device);
        state->device = (HWAVEOUT)0;
        state->opened = 0;
    }

    memset(&state->format, 0, sizeof(state->format));
    state->format.wFormatTag = WAVE_FORMAT_PCM;
    state->format.nChannels = (WORD)format->channels;
    state->format.nSamplesPerSec = (DWORD)format->sample_rate;
    state->format.nAvgBytesPerSec = (DWORD)format->byte_rate;
    state->format.nBlockAlign = (WORD)format->block_align;
    state->format.wBitsPerSample = (WORD)format->bits_per_sample;
    state->format.cbSize = 0;

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
        return MAUDIO89_ERR_BACKEND;
    }
    state->opened = 1;
    return MAUDIO89_OK;
}

static int mwinmaudio89_play(
    void *context,
    const maudio89_u8 *bytes,
    maudio89_u32 byte_count)
{
    mwinmaudio89_state *state;
    MMRESULT result;

    state = (mwinmaudio89_state *)context;
    if (state == (mwinmaudio89_state *)0 ||
        bytes == (const maudio89_u8 *)0 || byte_count == 0U) {
        return MAUDIO89_ERR_ARGUMENT;
    }
    if (!state->opened || state->device == (HWAVEOUT)0) {
        return MAUDIO89_ERR_NOT_OPEN;
    }

    (void)mwinmaudio89_release_buffer(state);
    state->header.lpData = (LPSTR)bytes;
    state->header.dwBufferLength = (DWORD)byte_count;
    if (state->looping) {
        state->header.dwFlags = WHDR_BEGINLOOP | WHDR_ENDLOOP;
        state->header.dwLoops = 0xFFFFFFFFUL;
    }

    result = waveOutPrepareHeader(
        state->device,
        &state->header,
        (UINT)sizeof(state->header)
    );
    if (result != MMSYSERR_NOERROR) {
        mwinmaudio89_clear_header(state);
        return MAUDIO89_ERR_BACKEND;
    }
    state->prepared = 1;

    result = waveOutWrite(
        state->device,
        &state->header,
        (UINT)sizeof(state->header)
    );
    if (result != MMSYSERR_NOERROR) {
        (void)mwinmaudio89_release_buffer(state);
        return MAUDIO89_ERR_BACKEND;
    }
    state->playing = 1;
    return MAUDIO89_OK;
}

static int mwinmaudio89_stop(void *context)
{
    mwinmaudio89_state *state;
    state = (mwinmaudio89_state *)context;
    if (state == (mwinmaudio89_state *)0) {
        return MAUDIO89_ERR_ARGUMENT;
    }
    return mwinmaudio89_release_buffer(state);
}

static int mwinmaudio89_is_playing(void *context)
{
    mwinmaudio89_state *state;
    state = (mwinmaudio89_state *)context;
    if (state == (mwinmaudio89_state *)0 || !state->playing) {
        return 0;
    }
#ifdef WHDR_DONE
    if ((state->header.dwFlags & WHDR_DONE) != 0U) {
        state->playing = 0;
        return 0;
    }
#endif
    return 1;
}

static void mwinmaudio89_close(void *context)
{
    mwinmaudio89_state *state;
    state = (mwinmaudio89_state *)context;
    if (state == (mwinmaudio89_state *)0) {
        return;
    }
    (void)mwinmaudio89_release_buffer(state);
    if (state->device != (HWAVEOUT)0) {
        (void)waveOutClose(state->device);
    }
    state->device = (HWAVEOUT)0;
    state->opened = 0;
}

void mwinmaudio89_init(mwinmaudio89_state *state)
{
    if (state == (mwinmaudio89_state *)0) {
        return;
    }
    memset(state, 0, sizeof(*state));
}

void mwinmaudio89_set_loop(mwinmaudio89_state *state, int loop)
{
    if (state == (mwinmaudio89_state *)0) {
        return;
    }
    state->looping = loop ? 1 : 0;
}

maudio89_provider mwinmaudio89_make_provider(mwinmaudio89_state *state)
{
    maudio89_provider provider;
    provider.context = state;
    provider.open = mwinmaudio89_open;
    provider.play = mwinmaudio89_play;
    provider.stop = mwinmaudio89_stop;
    provider.is_playing = mwinmaudio89_is_playing;
    provider.close = mwinmaudio89_close;
    return provider;
}
