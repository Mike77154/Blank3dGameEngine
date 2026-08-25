#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <mmsystem.h>
#include "mp3_mci89.h"
#include <string.h>

static void mp3_mci89_copy(char *dst, int cap, const char *src)
{
    int i;
    i = 0;
    while (src[i] != '\0' && i < cap - 1) {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
}

static int mp3_mci89_append(char *dst, int cap, const char *src)
{
    int n;
    int i;
    n = (int)strlen(dst);
    i = 0;
    while (src[i] != '\0' && n < cap - 1) dst[n++] = src[i++];
    dst[n] = '\0';
    return src[i] == '\0';
}

static void mp3_mci89_set_error(mp3_mci89_state *st, MCIERROR err)
{
    if (err == 0) {
        st->error[0] = '\0';
        return;
    }
    if (!mciGetErrorStringA(err, st->error, MP3_MCI89_ERROR_CAP)) mp3_mci89_copy(st->error, MP3_MCI89_ERROR_CAP, "MCI operation failed");
}

static int mp3_mci89_open_impl(void *state, const char *path)
{
    mp3_mci89_state *st;
    char cmd[720];
    MCIERROR err;
    st = (mp3_mci89_state *)state;
    st->error[0] = '\0';
    mciSendStringA("close staffroll_mp3", 0, 0, 0);
    cmd[0] = '\0';
    if (!mp3_mci89_append(cmd, (int)sizeof(cmd), "open \"") ||
        !mp3_mci89_append(cmd, (int)sizeof(cmd), path) ||
        !mp3_mci89_append(cmd, (int)sizeof(cmd), "\" alias staffroll_mp3")) {
        mp3_mci89_copy(st->error, MP3_MCI89_ERROR_CAP, "MP3 path is too long");
        return 0;
    }
    err = mciSendStringA(cmd, 0, 0, 0);
    if (err != 0) {
        mp3_mci89_set_error(st, err);
        return 0;
    }
    st->opened = 1;
    return 1;
}

static int mp3_mci89_ieq(const char *a, const char *b)
{
    unsigned char ca;
    unsigned char cb;
    while (*a != '\0' && *b != '\0') {
        ca = (unsigned char)*a;
        cb = (unsigned char)*b;
        if (ca >= 'A' && ca <= 'Z') ca = (unsigned char)(ca - 'A' + 'a');
        if (cb >= 'A' && cb <= 'Z') cb = (unsigned char)(cb - 'A' + 'a');
        if (ca != cb) return 0;
        ++a;
        ++b;
    }
    return *a == '\0' && *b == '\0';
}

static int mp3_mci89_play_impl(void *state, int loop)
{
    mp3_mci89_state *st;
    MCIERROR err;
    st = (mp3_mci89_state *)state;
    if (!st->opened) return 0;
    st->loop = loop;
    st->paused = 0;
    err = mciSendStringA("play staffroll_mp3 from 0", 0, 0, 0);
    if (err != 0) {
        mp3_mci89_set_error(st, err);
        return 0;
    }
    return 1;
}

static void mp3_mci89_update_impl(void *state)
{
    mp3_mci89_state *st;
    char mode[64];
    MCIERROR err;
    st = (mp3_mci89_state *)state;
    if (!st->opened || !st->loop || st->paused) return;
    mode[0] = '\0';
    err = mciSendStringA("status staffroll_mp3 mode", mode, (UINT)sizeof(mode), 0);
    if (err == 0 && mp3_mci89_ieq(mode, "stopped")) mciSendStringA("play staffroll_mp3 from 0", 0, 0, 0);
}

static void mp3_mci89_pause_impl(void *state)
{
    mp3_mci89_state *st;
    st = (mp3_mci89_state *)state;
    if (st->opened && !st->paused) {
        if (mciSendStringA("pause staffroll_mp3", 0, 0, 0) == 0) st->paused = 1;
    }
}

static void mp3_mci89_resume_impl(void *state)
{
    mp3_mci89_state *st;
    st = (mp3_mci89_state *)state;
    if (st->opened && st->paused) {
        if (mciSendStringA("resume staffroll_mp3", 0, 0, 0) == 0) st->paused = 0;
    }
}

static void mp3_mci89_stop_impl(void *state)
{
    mp3_mci89_state *st;
    st = (mp3_mci89_state *)state;
    if (st->opened) mciSendStringA("stop staffroll_mp3", 0, 0, 0);
}

static void mp3_mci89_close_impl(void *state)
{
    mp3_mci89_state *st;
    st = (mp3_mci89_state *)state;
    if (st->opened) mciSendStringA("close staffroll_mp3", 0, 0, 0);
    st->opened = 0;
    st->loop = 0;
    st->paused = 0;
}

static const char *mp3_mci89_error_impl(void *state)
{
    mp3_mci89_state *st;
    st = (mp3_mci89_state *)state;
    return st->error[0] != '\0' ? st->error : "no MP3 error";
}

static const audio89_provider g_mp3_mci89_provider = {
    mp3_mci89_open_impl,
    mp3_mci89_play_impl,
    mp3_mci89_update_impl,
    mp3_mci89_pause_impl,
    mp3_mci89_resume_impl,
    mp3_mci89_stop_impl,
    mp3_mci89_close_impl,
    mp3_mci89_error_impl
};

void mp3_mci89_init(mp3_mci89_state *st)
{
    st->opened = 0;
    st->loop = 0;
    st->paused = 0;
    st->error[0] = '\0';
}

const audio89_provider *mp3_mci89_provider(void)
{
    return &g_mp3_mci89_provider;
}
