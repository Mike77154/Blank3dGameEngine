#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <mmsystem.h>
#include "mwinplaysound89.h"

static int mwinplaysound89_play(void *user, const char *path, int loop)
{
    mwinplaysound89_state *state;
    DWORD flags;
    state = (mwinplaysound89_state *)user;
    if (state == (mwinplaysound89_state *)0 ||
        path == (const char *)0 || path[0] == '\0') {
        return MSOUNDPROVIDER89_ERROR;
    }
    flags = SND_FILENAME | SND_ASYNC | SND_NODEFAULT;
    if (loop) {
        flags |= SND_LOOP;
    }
    if (!PlaySoundA(path, (HMODULE)0, flags)) {
        state->playing = 0;
        state->looping = 0;
        return MSOUNDPROVIDER89_ERROR;
    }
    state->playing = 1;
    state->looping = loop ? 1 : 0;
    return MSOUNDPROVIDER89_OK;
}

static int mwinplaysound89_stop(void *user)
{
    mwinplaysound89_state *state;
    state = (mwinplaysound89_state *)user;
    PlaySoundA((LPCSTR)0, (HMODULE)0, 0U);
    if (state != (mwinplaysound89_state *)0) {
        state->playing = 0;
        state->looping = 0;
    }
    return MSOUNDPROVIDER89_OK;
}

static void mwinplaysound89_shutdown(void *user)
{
    (void)mwinplaysound89_stop(user);
}

void mwinplaysound89_init(mwinplaysound89_state *state)
{
    if (state == (mwinplaysound89_state *)0) {
        return;
    }
    state->playing = 0;
    state->looping = 0;
}

msoundprovider89 mwinplaysound89_make_provider(mwinplaysound89_state *state)
{
    msoundprovider89 provider;
    msoundprovider89_init(&provider);
    provider.user = state;
    provider.play = mwinplaysound89_play;
    provider.stop = mwinplaysound89_stop;
    provider.shutdown = mwinplaysound89_shutdown;
    return provider;
}
