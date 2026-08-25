#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <mmsystem.h>
#include "pcm_wave89.h"
#include "path89.h"
#include <string.h>

static void pcm_wave89_copy(char *dst, int cap, const char *src)
{
    int i;
    i = 0;
    while (src[i] != '\0' && i < cap - 1) {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
}

static unsigned short pcm_wave89_u16(const unsigned char *b)
{
    return (unsigned short)((unsigned short)b[0] | ((unsigned short)b[1] << 8));
}

static unsigned long pcm_wave89_u32(const unsigned char *b)
{
    return (unsigned long)b[0] |
           ((unsigned long)b[1] << 8) |
           ((unsigned long)b[2] << 16) |
           ((unsigned long)b[3] << 24);
}

static int pcm_wave89_read_exact(FILE *fp, unsigned char *buf, unsigned long n)
{
    return fread(buf, 1, (size_t)n, fp) == (size_t)n;
}

static int pcm_wave89_parse_wav(pcm_wave89_state *st)
{
    unsigned char h[16];
    unsigned char c[8];
    unsigned long size;
    unsigned long pos;
    int have_fmt;
    int have_data;
    unsigned short tag;
    unsigned short ch;
    unsigned long rate;
    unsigned short bits;
    have_fmt = 0;
    have_data = 0;
    if (!pcm_wave89_read_exact(st->file, h, 12)) return 0;
    if (memcmp(h, "RIFF", 4) != 0 || memcmp(h + 8, "WAVE", 4) != 0) return 0;
    while (pcm_wave89_read_exact(st->file, c, 8)) {
        size = pcm_wave89_u32(c + 4);
        if (memcmp(c, "fmt ", 4) == 0) {
            if (size < 16UL || !pcm_wave89_read_exact(st->file, h, 16)) return 0;
            tag = pcm_wave89_u16(h);
            ch = pcm_wave89_u16(h + 2);
            rate = pcm_wave89_u32(h + 4);
            bits = pcm_wave89_u16(h + 14);
            if (tag != 1 || (ch != 1 && ch != 2) || (bits != 8 && bits != 16) || rate == 0UL) return 0;
            st->format.wFormatTag = WAVE_FORMAT_PCM;
            st->format.nChannels = ch;
            st->format.nSamplesPerSec = rate;
            st->format.wBitsPerSample = bits;
            st->format.nBlockAlign = (WORD)((ch * bits) / 8);
            st->format.nAvgBytesPerSec = rate * (DWORD)st->format.nBlockAlign;
            st->format.cbSize = 0;
            if (size > 16UL) fseek(st->file, (long)(size - 16UL), SEEK_CUR);
            have_fmt = 1;
        } else if (memcmp(c, "data", 4) == 0) {
            pos = (unsigned long)ftell(st->file);
            st->data_offset = pos;
            st->data_size = size;
            have_data = 1;
            fseek(st->file, (long)size, SEEK_CUR);
        } else {
            fseek(st->file, (long)size, SEEK_CUR);
        }
        if (size & 1UL) fseek(st->file, 1L, SEEK_CUR);
        if (have_fmt && have_data) break;
    }
    if (!have_fmt || !have_data) return 0;
    return fseek(st->file, (long)st->data_offset, SEEK_SET) == 0;
}

static int pcm_wave89_parse_raw(pcm_wave89_state *st)
{
    long end;
    if (st->raw_sample_rate <= 0) return 0;
    if (st->raw_channels != 1 && st->raw_channels != 2) return 0;
    if (st->raw_bits != 8 && st->raw_bits != 16) return 0;
    if (fseek(st->file, 0L, SEEK_END) != 0) return 0;
    end = ftell(st->file);
    if (end < 0L) return 0;
    if (fseek(st->file, 0L, SEEK_SET) != 0) return 0;
    st->data_offset = 0UL;
    st->data_size = (unsigned long)end;
    st->format.wFormatTag = WAVE_FORMAT_PCM;
    st->format.nChannels = (WORD)st->raw_channels;
    st->format.nSamplesPerSec = (DWORD)st->raw_sample_rate;
    st->format.wBitsPerSample = (WORD)st->raw_bits;
    st->format.nBlockAlign = (WORD)((st->raw_channels * st->raw_bits) / 8);
    st->format.nAvgBytesPerSec = st->format.nSamplesPerSec * (DWORD)st->format.nBlockAlign;
    st->format.cbSize = 0;
    return 1;
}

static int pcm_wave89_fill(pcm_wave89_state *st, int slot)
{
    unsigned long want;
    size_t got;
    MMRESULT mm;
    WAVEHDR *h;
    if (st->remaining == 0UL) return 0;
    want = st->remaining > PCM_WAVE89_BUFFER_BYTES ? PCM_WAVE89_BUFFER_BYTES : st->remaining;
    got = fread(st->buffers[slot], 1, (size_t)want, st->file);
    if (got == 0) return 0;
    st->remaining -= (unsigned long)got;
    h = &st->headers[slot];
    if (!st->prepared[slot]) {
        memset(h, 0, sizeof(*h));
        h->lpData = (LPSTR)st->buffers[slot];
        h->dwBufferLength = (DWORD)got;
        mm = waveOutPrepareHeader(st->out, h, sizeof(*h));
        if (mm != MMSYSERR_NOERROR) return 0;
        st->prepared[slot] = 1;
    } else {
        h->dwBufferLength = (DWORD)got;
    }
    mm = waveOutWrite(st->out, h, sizeof(*h));
    return mm == MMSYSERR_NOERROR;
}

static int pcm_wave89_all_done(pcm_wave89_state *st)
{
    int i;
    for (i = 0; i < 2; ++i) {
        if (st->prepared[i] && (st->headers[i].dwFlags & WHDR_DONE) == 0) return 0;
    }
    return 1;
}

static int pcm_wave89_open_impl(void *state, const char *path)
{
    pcm_wave89_state *st;
    MMRESULT mm;
    st = (pcm_wave89_state *)state;
    st->error[0] = '\0';
    st->file = fopen(path, "rb");
    if (st->file == NULL) {
        pcm_wave89_copy(st->error, PCM_WAVE89_ERROR_CAP, "cannot open PCM file");
        return 0;
    }
    memset(&st->format, 0, sizeof(st->format));
    if (path89_iext(path, ".pcm")) {
        if (!pcm_wave89_parse_raw(st)) {
            pcm_wave89_copy(st->error, PCM_WAVE89_ERROR_CAP, "invalid raw PCM format settings");
            fclose(st->file);
            st->file = NULL;
            return 0;
        }
    } else {
        if (!pcm_wave89_parse_wav(st)) {
            pcm_wave89_copy(st->error, PCM_WAVE89_ERROR_CAP, "unsupported WAV/PCM file");
            fclose(st->file);
            st->file = NULL;
            return 0;
        }
    }
    mm = waveOutOpen(&st->out, WAVE_MAPPER, &st->format, 0, 0, CALLBACK_NULL);
    if (mm != MMSYSERR_NOERROR) {
        pcm_wave89_copy(st->error, PCM_WAVE89_ERROR_CAP, "waveOutOpen failed");
        fclose(st->file);
        st->file = NULL;
        return 0;
    }
    st->remaining = st->data_size;
    st->opened = 1;
    return 1;
}

static int pcm_wave89_play_impl(void *state, int loop)
{
    pcm_wave89_state *st;
    st = (pcm_wave89_state *)state;
    if (!st->opened) return 0;
    waveOutReset(st->out);
    if (fseek(st->file, (long)st->data_offset, SEEK_SET) != 0) return 0;
    st->remaining = st->data_size;
    st->loop = loop;
    st->playing = 1;
    pcm_wave89_fill(st, 0);
    pcm_wave89_fill(st, 1);
    return 1;
}

static void pcm_wave89_update_impl(void *state)
{
    pcm_wave89_state *st;
    int i;
    st = (pcm_wave89_state *)state;
    if (!st->playing) return;
    for (i = 0; i < 2; ++i) {
        if (st->prepared[i] && (st->headers[i].dwFlags & WHDR_DONE) != 0 && st->remaining > 0UL) pcm_wave89_fill(st, i);
    }
    if (st->remaining == 0UL && pcm_wave89_all_done(st)) {
        if (st->loop) {
            if (fseek(st->file, (long)st->data_offset, SEEK_SET) == 0) {
                st->remaining = st->data_size;
                pcm_wave89_fill(st, 0);
                pcm_wave89_fill(st, 1);
            }
        } else {
            st->playing = 0;
        }
    }
}

static void pcm_wave89_pause_impl(void *state)
{
    pcm_wave89_state *st;
    st = (pcm_wave89_state *)state;
    if (st->opened && st->playing) waveOutPause(st->out);
}

static void pcm_wave89_resume_impl(void *state)
{
    pcm_wave89_state *st;
    st = (pcm_wave89_state *)state;
    if (st->opened && st->playing) waveOutRestart(st->out);
}

static void pcm_wave89_stop_impl(void *state)
{
    pcm_wave89_state *st;
    st = (pcm_wave89_state *)state;
    if (st->opened) waveOutReset(st->out);
    st->playing = 0;
}

static void pcm_wave89_close_impl(void *state)
{
    pcm_wave89_state *st;
    int i;
    st = (pcm_wave89_state *)state;
    if (st->opened) {
        waveOutReset(st->out);
        for (i = 0; i < 2; ++i) {
            if (st->prepared[i]) {
                waveOutUnprepareHeader(st->out, &st->headers[i], sizeof(st->headers[i]));
                st->prepared[i] = 0;
            }
        }
        waveOutClose(st->out);
    }
    if (st->file != NULL) fclose(st->file);
    st->file = NULL;
    st->out = 0;
    st->opened = 0;
    st->playing = 0;
}

static const char *pcm_wave89_error_impl(void *state)
{
    pcm_wave89_state *st;
    st = (pcm_wave89_state *)state;
    return st->error[0] != '\0' ? st->error : "no PCM error";
}

static const audio89_provider g_pcm_wave89_provider = {
    pcm_wave89_open_impl,
    pcm_wave89_play_impl,
    pcm_wave89_update_impl,
    pcm_wave89_pause_impl,
    pcm_wave89_resume_impl,
    pcm_wave89_stop_impl,
    pcm_wave89_close_impl,
    pcm_wave89_error_impl
};

void pcm_wave89_init(pcm_wave89_state *st)
{
    memset(st, 0, sizeof(*st));
    st->raw_sample_rate = 22050;
    st->raw_channels = 1;
    st->raw_bits = 8;
}

void pcm_wave89_set_raw_format(pcm_wave89_state *st, int sample_rate, int channels, int bits)
{
    st->raw_sample_rate = sample_rate;
    st->raw_channels = channels;
    st->raw_bits = bits;
}

const audio89_provider *pcm_wave89_provider(void)
{
    return &g_pcm_wave89_provider;
}
