#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "mp3_pcm_wave89.h"
#include <string.h>

static void mp3_pcm_wave89_copy(char *dst, int cap, const char *src)
{
    int i;
    i = 0;
    while (src[i] != '\0' && i < cap - 1) {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
}

static int mp3_pcm_wave89_same_format(const mp3_frame89_info *a, const mp3_frame89_info *b)
{
    return a->sample_rate == b->sample_rate && a->channels == b->channels && a->mpeg_version == b->mpeg_version;
}

static int mp3_pcm_wave89_decode_next(mp3_pcm_wave89_state *st, unsigned char *dst, int cap, int *bytes)
{
    mp3_frame89_info info;
    int frame_bytes;
    int pcm_bytes;
    int tries;
    *bytes = 0;
    for (tries = 0; tries < 8; ++tries) {
        frame_bytes = mp3_frame89_read_next(st->file, st->mp3_frame, (int)sizeof(st->mp3_frame), &info);
        if (frame_bytes <= 0) {
            st->eof = 1;
            return 1;
        }
        if (!mp3_pcm_wave89_same_format(&st->first_info, &info)) {
            mp3_pcm_wave89_copy(st->error, MP3_PCM_WAVE89_ERROR_CAP, "MP3 changes sample rate/channels mid-stream");
            return 0;
        }
        pcm_bytes = 0;
        if (!mp3_acm_codec89_decode(&st->codec, st->mp3_frame, frame_bytes,
                                    st->decode_pcm, (int)sizeof(st->decode_pcm),
                                    &pcm_bytes, 0)) {
            mp3_pcm_wave89_copy(st->error, MP3_PCM_WAVE89_ERROR_CAP, mp3_acm_codec89_last_error(&st->codec));
            return 0;
        }
        if (pcm_bytes > 0) {
            if (pcm_bytes > cap) {
                mp3_pcm_wave89_copy(st->error, MP3_PCM_WAVE89_ERROR_CAP, "decoded PCM frame exceeds static buffer");
                return 0;
            }
            memcpy(dst, st->decode_pcm, (size_t)pcm_bytes);
            *bytes = pcm_bytes;
            return 1;
        }
    }
    mp3_pcm_wave89_copy(st->error, MP3_PCM_WAVE89_ERROR_CAP, "MP3 codec returned no PCM for several frames");
    return 0;
}

static int mp3_pcm_wave89_queue(mp3_pcm_wave89_state *st, int slot)
{
    int bytes;
    MMRESULT mm;
    WAVEHDR *h;
    if (st->eof) return 0;
    bytes = 0;
    if (!mp3_pcm_wave89_decode_next(st, st->buffers[slot], (int)sizeof(st->buffers[slot]), &bytes)) return -1;
    if (st->eof || bytes <= 0) return 0;
    h = &st->headers[slot];
    if (st->prepared[slot]) {
        mm = waveOutUnprepareHeader(st->out, h, sizeof(*h));
        if (mm != MMSYSERR_NOERROR) {
            mp3_pcm_wave89_copy(st->error, MP3_PCM_WAVE89_ERROR_CAP, "waveOutUnprepareHeader failed while recycling MP3 PCM slot");
            return -1;
        }
        st->prepared[slot] = 0;
    }
    memset(h, 0, sizeof(*h));
    h->lpData = (LPSTR)st->buffers[slot];
    h->dwBufferLength = (DWORD)bytes;
    mm = waveOutPrepareHeader(st->out, h, sizeof(*h));
    if (mm != MMSYSERR_NOERROR) {
        mp3_pcm_wave89_copy(st->error, MP3_PCM_WAVE89_ERROR_CAP, "waveOutPrepareHeader failed for MP3 PCM");
        return -1;
    }
    st->prepared[slot] = 1;
    mm = waveOutWrite(st->out, h, sizeof(*h));
    if (mm != MMSYSERR_NOERROR) {
        mp3_pcm_wave89_copy(st->error, MP3_PCM_WAVE89_ERROR_CAP, "waveOutWrite failed for MP3 PCM");
        return -1;
    }
    return 1;
}

static int mp3_pcm_wave89_all_done(mp3_pcm_wave89_state *st)
{
    int i;
    for (i = 0; i < MP3_PCM_WAVE89_SLOTS; ++i) {
        if (st->prepared[i] && (st->headers[i].dwFlags & WHDR_DONE) == 0) return 0;
    }
    return 1;
}

static int mp3_pcm_wave89_rewind(mp3_pcm_wave89_state *st)
{
    if (fseek(st->file, (long)st->first_frame_offset, SEEK_SET) != 0) return 0;
    mp3_acm_codec89_restart(&st->codec);
    st->eof = 0;
    return 1;
}

static int mp3_pcm_wave89_open_impl(void *state, const char *path)
{
    mp3_pcm_wave89_state *st;
    const WAVEFORMATEX *fmt;
    MMRESULT mm;
    st = (mp3_pcm_wave89_state *)state;
    st->error[0] = '\0';
    st->file = fopen(path, "rb");
    if (st->file == 0) {
        mp3_pcm_wave89_copy(st->error, MP3_PCM_WAVE89_ERROR_CAP, "cannot open MP3 file");
        return 0;
    }
    if (!mp3_frame89_find_first(st->file, &st->first_frame_offset, &st->first_info)) {
        mp3_pcm_wave89_copy(st->error, MP3_PCM_WAVE89_ERROR_CAP, "no valid MPEG Layer III frame found");
        fclose(st->file);
        st->file = 0;
        return 0;
    }
    if (!mp3_acm_codec89_open(&st->codec, &st->first_info)) {
        mp3_pcm_wave89_copy(st->error, MP3_PCM_WAVE89_ERROR_CAP, mp3_acm_codec89_last_error(&st->codec));
        fclose(st->file);
        st->file = 0;
        return 0;
    }
    fmt = mp3_acm_codec89_pcm_format(&st->codec);
    mm = waveOutOpen(&st->out, WAVE_MAPPER, fmt, 0, 0, CALLBACK_NULL);
    if (mm != MMSYSERR_NOERROR) {
        mp3_pcm_wave89_copy(st->error, MP3_PCM_WAVE89_ERROR_CAP, "waveOutOpen failed for decoded MP3 PCM");
        mp3_acm_codec89_close(&st->codec);
        fclose(st->file);
        st->file = 0;
        return 0;
    }
    if (fseek(st->file, (long)st->first_frame_offset, SEEK_SET) != 0) {
        mp3_pcm_wave89_copy(st->error, MP3_PCM_WAVE89_ERROR_CAP, "cannot seek to first MP3 frame");
        waveOutClose(st->out);
        st->out = 0;
        mp3_acm_codec89_close(&st->codec);
        fclose(st->file);
        st->file = 0;
        return 0;
    }
    st->opened = 1;
    st->eof = 0;
    return 1;
}

static int mp3_pcm_wave89_play_impl(void *state, int loop)
{
    mp3_pcm_wave89_state *st;
    int i;
    int q;
    st = (mp3_pcm_wave89_state *)state;
    if (!st->opened) return 0;
    waveOutReset(st->out);
    if (!mp3_pcm_wave89_rewind(st)) return 0;
    st->loop = loop;
    st->playing = 1;
    st->paused = 0;
    for (i = 0; i < MP3_PCM_WAVE89_SLOTS; ++i) {
        q = mp3_pcm_wave89_queue(st, i);
        if (q < 0) {
            st->playing = 0;
            return 0;
        }
        if (q == 0) break;
    }
    return 1;
}

static void mp3_pcm_wave89_update_impl(void *state)
{
    mp3_pcm_wave89_state *st;
    int i;
    int q;
    st = (mp3_pcm_wave89_state *)state;
    if (!st->playing || st->paused) return;
    for (i = 0; i < MP3_PCM_WAVE89_SLOTS; ++i) {
        if (st->prepared[i] && (st->headers[i].dwFlags & WHDR_DONE) != 0 && !st->eof) {
            q = mp3_pcm_wave89_queue(st, i);
            if (q < 0) {
                st->playing = 0;
                return;
            }
        }
    }
    if (st->eof && mp3_pcm_wave89_all_done(st)) {
        if (st->loop && mp3_pcm_wave89_rewind(st)) {
            for (i = 0; i < MP3_PCM_WAVE89_SLOTS; ++i) {
                q = mp3_pcm_wave89_queue(st, i);
                if (q <= 0) break;
            }
        } else {
            st->playing = 0;
        }
    }
}

static void mp3_pcm_wave89_pause_impl(void *state)
{
    mp3_pcm_wave89_state *st;
    st = (mp3_pcm_wave89_state *)state;
    if (st->opened && st->playing && !st->paused) {
        if (waveOutPause(st->out) == MMSYSERR_NOERROR) st->paused = 1;
    }
}

static void mp3_pcm_wave89_resume_impl(void *state)
{
    mp3_pcm_wave89_state *st;
    st = (mp3_pcm_wave89_state *)state;
    if (st->opened && st->playing && st->paused) {
        if (waveOutRestart(st->out) == MMSYSERR_NOERROR) st->paused = 0;
    }
}

static void mp3_pcm_wave89_stop_impl(void *state)
{
    mp3_pcm_wave89_state *st;
    st = (mp3_pcm_wave89_state *)state;
    if (st->opened) waveOutReset(st->out);
    st->playing = 0;
    st->paused = 0;
}

static void mp3_pcm_wave89_close_impl(void *state)
{
    mp3_pcm_wave89_state *st;
    int i;
    st = (mp3_pcm_wave89_state *)state;
    if (st->out != 0) {
        waveOutReset(st->out);
        for (i = 0; i < MP3_PCM_WAVE89_SLOTS; ++i) {
            if (st->prepared[i]) {
                waveOutUnprepareHeader(st->out, &st->headers[i], sizeof(st->headers[i]));
                st->prepared[i] = 0;
            }
        }
        waveOutClose(st->out);
    }
    st->out = 0;
    mp3_acm_codec89_close(&st->codec);
    if (st->file != 0) fclose(st->file);
    st->file = 0;
    st->opened = 0;
    st->playing = 0;
    st->paused = 0;
    st->eof = 0;
}

static const char *mp3_pcm_wave89_error_impl(void *state)
{
    mp3_pcm_wave89_state *st;
    st = (mp3_pcm_wave89_state *)state;
    return st->error[0] != '\0' ? st->error : "no MP3 PCM error";
}

static const audio89_provider g_mp3_pcm_wave89_provider = {
    mp3_pcm_wave89_open_impl,
    mp3_pcm_wave89_play_impl,
    mp3_pcm_wave89_update_impl,
    mp3_pcm_wave89_pause_impl,
    mp3_pcm_wave89_resume_impl,
    mp3_pcm_wave89_stop_impl,
    mp3_pcm_wave89_close_impl,
    mp3_pcm_wave89_error_impl
};

void mp3_pcm_wave89_init(mp3_pcm_wave89_state *st)
{
    memset(st, 0, sizeof(*st));
    mp3_acm_codec89_init(&st->codec);
}

const audio89_provider *mp3_pcm_wave89_provider(void)
{
    return &g_mp3_pcm_wave89_provider;
}
