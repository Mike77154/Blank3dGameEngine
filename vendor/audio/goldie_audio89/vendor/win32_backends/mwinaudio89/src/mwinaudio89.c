#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <mmsystem.h>
#include <string.h>
#include "mwinaudio89.h"

static int mwinaudio89_play_file(void *user, const char *path, int loop)
{
    mwinaudio89_state *state;
    state = (mwinaudio89_state *)user;
    if (state == (mwinaudio89_state *)0) {
        return MSOUNDPROVIDER89_ERROR;
    }
    return msoundprovider89_play(&state->file_provider, path, loop);
}

static int mwinaudio89_play_decoded_wav(
    void *user,
    const char *path,
    int loop)
{
    mwinaudio89_state *state;
    int rc;
    state = (mwinaudio89_state *)user;
    if (state == (mwinaudio89_state *)0 || !state->decoded_ready) {
        return MSOUNDPROVIDER89_ERROR;
    }
    mwinmaudio89_set_loop(&state->decoded_backend, loop);
    rc = mwavplayer89_play_file(&state->decoded_player, path);
    return rc == MWAVPLAYER89_OK ?
        MSOUNDPROVIDER89_OK : MSOUNDPROVIDER89_ERROR;
}

static int mwinaudio89_play_synth(
    void *user,
    const char *database_path,
    const char *preset_id,
    int gain_q15,
    int pan_q15,
    int loop)
{
    mwinaudio89_state *state;
    state = (mwinaudio89_state *)user;
    if (state == (mwinaudio89_state *)0) {
        return MSOUNDPROVIDER89_ERROR;
    }
    return mgifwinaudio89_play(
        &state->synth_backend,
        database_path,
        preset_id,
        gain_q15,
        pan_q15,
        loop
    ) ? MSOUNDPROVIDER89_OK : MSOUNDPROVIDER89_ERROR;
}

static int mwinaudio89_stop_synth(void *user)
{
    mwinaudio89_state *state;
    state = (mwinaudio89_state *)user;
    if (state == (mwinaudio89_state *)0) {
        return MSOUNDPROVIDER89_ERROR;
    }
    return mgifwinaudio89_stop(&state->synth_backend) ?
        MSOUNDPROVIDER89_OK : MSOUNDPROVIDER89_ERROR;
}

static int mwinaudio89_pump(void *user)
{
    mwinaudio89_state *state;
    state = (mwinaudio89_state *)user;
    if (state == (mwinaudio89_state *)0) {
        return MSOUNDPROVIDER89_ERROR;
    }
    return mgifwinaudio89_pump(&state->synth_backend) ?
        MSOUNDPROVIDER89_OK : MSOUNDPROVIDER89_ERROR;
}

static int mwinaudio89_stop(void *user)
{
    mwinaudio89_state *state;
    int file_rc;
    int decoded_rc;
    int synth_rc;
    state = (mwinaudio89_state *)user;
    if (state == (mwinaudio89_state *)0) {
        return MSOUNDPROVIDER89_ERROR;
    }
    file_rc = msoundprovider89_stop(&state->file_provider);
    decoded_rc = state->decoded_ready ?
        mwavplayer89_stop(&state->decoded_player) : MWAVPLAYER89_OK;
    synth_rc = mgifwinaudio89_stop(&state->synth_backend) ?
        MSOUNDPROVIDER89_OK : MSOUNDPROVIDER89_ERROR;
    if (file_rc == MSOUNDPROVIDER89_ERROR ||
        decoded_rc != MWAVPLAYER89_OK ||
        synth_rc == MSOUNDPROVIDER89_ERROR) {
        return MSOUNDPROVIDER89_ERROR;
    }
    return MSOUNDPROVIDER89_OK;
}

static void mwinaudio89_shutdown(void *user)
{
    mwinaudio89_state *state;
    state = (mwinaudio89_state *)user;
    if (state == (mwinaudio89_state *)0) {
        return;
    }
    (void)mwinaudio89_stop(state);
    if (state->decoded_ready) {
        mwavplayer89_close(&state->decoded_player);
        state->decoded_ready = 0;
    }
}

void mwinaudio89_init(mwinaudio89_state *state)
{
    int rc;
    if (state == (mwinaudio89_state *)0) {
        return;
    }
    memset(state, 0, sizeof(*state));
    mwinplaysound89_init(&state->file_backend);
    state->file_provider = mwinplaysound89_make_provider(&state->file_backend);

    mwinmaudio89_init(&state->decoded_backend);
    state->decoded_provider = mwinmaudio89_make_provider(&state->decoded_backend);
    rc = mwavplayer89_init(
        &state->decoded_player,
        &state->decoded_provider,
        state->decoded_wav_bytes,
        MWINAUDIO89_MAX_DECODED_WAV_BYTES
    );
    state->decoded_ready = rc == MWAVPLAYER89_OK ? 1 : 0;

    mgifwinaudio89_init(&state->synth_backend);
}

msoundprovider89 mwinaudio89_make_provider(mwinaudio89_state *state)
{
    msoundprovider89 provider;
    msoundprovider89_init(&provider);
    provider.user = state;
    provider.play = mwinaudio89_play_file;
    provider.play_wav = mwinaudio89_play_decoded_wav;
    provider.play_synth = mwinaudio89_play_synth;
    provider.stop_synth = mwinaudio89_stop_synth;
    provider.pump = mwinaudio89_pump;
    provider.stop = mwinaudio89_stop;
    provider.shutdown = mwinaudio89_shutdown;
    return provider;
}
