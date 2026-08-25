#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "mp3_acm_codec89.h"
#include <string.h>

static void mp3_acm_codec89_copy(char *dst, int cap, const char *src)
{
    int i;
    i = 0;
    while (src[i] != '\0' && i < cap - 1) {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
}

static void mp3_acm_codec89_set_mm_error(mp3_acm_codec89 *c, MMRESULT mm, const char *fallback)
{
    char text[MP3_ACM_CODEC89_ERROR_CAP];
    text[0] = '\0';
    if (waveOutGetErrorTextA(mm, text, (UINT)sizeof(text)) == MMSYSERR_NOERROR && text[0] != '\0') {
        mp3_acm_codec89_copy(c->error, MP3_ACM_CODEC89_ERROR_CAP, text);
    } else {
        mp3_acm_codec89_copy(c->error, MP3_ACM_CODEC89_ERROR_CAP, fallback);
    }
}

void mp3_acm_codec89_init(mp3_acm_codec89 *c)
{
    memset(c, 0, sizeof(*c));
}

int mp3_acm_codec89_open(mp3_acm_codec89 *c, const mp3_frame89_info *info)
{
    MMRESULT mm;
    if (c == 0 || info == 0) return 0;
    mp3_acm_codec89_close(c);
    memset(&c->source_format, 0, sizeof(c->source_format));
    memset(&c->pcm_format, 0, sizeof(c->pcm_format));
    c->source_format.wfx.wFormatTag = WAVE_FORMAT_MPEGLAYER3;
    c->source_format.wfx.nChannels = (WORD)info->channels;
    c->source_format.wfx.nSamplesPerSec = (DWORD)info->sample_rate;
    c->source_format.wfx.nAvgBytesPerSec = (DWORD)((info->bitrate_kbps * 1000) / 8);
    c->source_format.wfx.nBlockAlign = 1;
    c->source_format.wfx.wBitsPerSample = 0;
    c->source_format.wfx.cbSize = MPEGLAYER3_WFX_EXTRA_BYTES;
    c->source_format.wID = MPEGLAYER3_ID_MPEG;
    c->source_format.fdwFlags = MPEGLAYER3_FLAG_PADDING_ISO;
    c->source_format.nBlockSize = (WORD)info->frame_bytes;
    c->source_format.nFramesPerBlock = 1;
    c->source_format.nCodecDelay = 0;

    c->pcm_format.wFormatTag = WAVE_FORMAT_PCM;
    c->pcm_format.nChannels = (WORD)info->channels;
    c->pcm_format.nSamplesPerSec = (DWORD)info->sample_rate;
    c->pcm_format.wBitsPerSample = 16;
    c->pcm_format.nBlockAlign = (WORD)(info->channels * 2);
    c->pcm_format.nAvgBytesPerSec = c->pcm_format.nSamplesPerSec * (DWORD)c->pcm_format.nBlockAlign;
    c->pcm_format.cbSize = 0;

    mm = acmStreamOpen(&c->stream, 0,
                       (LPWAVEFORMATEX)&c->source_format,
                       &c->pcm_format, 0, 0, 0, ACM_STREAMOPENF_NONREALTIME);
    if (mm != MMSYSERR_NOERROR) {
        c->stream = 0;
        if (mm == ACMERR_NOTPOSSIBLE) {
            mp3_acm_codec89_copy(c->error, MP3_ACM_CODEC89_ERROR_CAP,
                                 "no installed ACM codec accepts this MP3 format");
        } else {
            mp3_acm_codec89_set_mm_error(c, mm, "acmStreamOpen failed");
        }
        return 0;
    }
    c->opened = 1;
    c->started = 0;
    c->error[0] = '\0';
    return 1;
}

int mp3_acm_codec89_decode(mp3_acm_codec89 *c,
                           const unsigned char *mp3, int mp3_bytes,
                           unsigned char *pcm, int pcm_cap,
                           int *pcm_bytes, int end_stream)
{
    ACMSTREAMHEADER h;
    MMRESULT mm;
    DWORD flags;
    DWORD recommended;
    if (pcm_bytes != 0) *pcm_bytes = 0;
    if (c == 0 || !c->opened || mp3 == 0 || mp3_bytes <= 0 || pcm == 0 || pcm_cap <= 0) return 0;
    recommended = 0;
    mm = acmStreamSize(c->stream, (DWORD)mp3_bytes, &recommended, ACM_STREAMSIZEF_SOURCE);
    if (mm != MMSYSERR_NOERROR) {
        mp3_acm_codec89_set_mm_error(c, mm, "acmStreamSize failed");
        return 0;
    }
    /* acmStreamSize() returns a recommendation, not a hard minimum.
       Microsoft documents that the value is commonly an overestimate.
       Keep the query because it validates the stream and is useful for
       diagnostics, but let acmStreamConvert() operate on our fixed buffer. */
    (void)recommended;
    memset(&h, 0, sizeof(h));
    h.cbStruct = sizeof(h);
    h.pbSrc = (LPBYTE)mp3;
    h.cbSrcLength = (DWORD)mp3_bytes;
    h.pbDst = (LPBYTE)pcm;
    h.cbDstLength = (DWORD)pcm_cap;
    mm = acmStreamPrepareHeader(c->stream, &h, 0);
    if (mm != MMSYSERR_NOERROR) {
        mp3_acm_codec89_set_mm_error(c, mm, "acmStreamPrepareHeader failed");
        return 0;
    }
    flags = ACM_STREAMCONVERTF_BLOCKALIGN;
    if (!c->started) flags |= ACM_STREAMCONVERTF_START;
    if (end_stream) flags |= ACM_STREAMCONVERTF_END;
    mm = acmStreamConvert(c->stream, &h, flags);
    if (mm != MMSYSERR_NOERROR) {
        acmStreamUnprepareHeader(c->stream, &h, 0);
        mp3_acm_codec89_set_mm_error(c, mm, "acmStreamConvert failed");
        return 0;
    }
    if (h.cbDstLengthUsed > (DWORD)pcm_cap) {
        acmStreamUnprepareHeader(c->stream, &h, 0);
        mp3_acm_codec89_copy(c->error, MP3_ACM_CODEC89_ERROR_CAP,
                             "ACM reported PCM output beyond the fixed destination buffer");
        return 0;
    }
    if (h.cbSrcLengthUsed != (DWORD)mp3_bytes) {
        acmStreamUnprepareHeader(c->stream, &h, 0);
        mp3_acm_codec89_copy(c->error, MP3_ACM_CODEC89_ERROR_CAP,
                             "ACM did not consume the complete MP3 frame");
        return 0;
    }
    c->started = 1;
    if (pcm_bytes != 0) *pcm_bytes = (int)h.cbDstLengthUsed;
    mm = acmStreamUnprepareHeader(c->stream, &h, 0);
    if (mm != MMSYSERR_NOERROR) {
        mp3_acm_codec89_set_mm_error(c, mm, "acmStreamUnprepareHeader failed");
        return 0;
    }
    return 1;
}

void mp3_acm_codec89_restart(mp3_acm_codec89 *c)
{
    if (c != 0) c->started = 0;
}

void mp3_acm_codec89_close(mp3_acm_codec89 *c)
{
    if (c == 0) return;
    if (c->stream != 0) acmStreamClose(c->stream, 0);
    c->stream = 0;
    c->opened = 0;
    c->started = 0;
}

const char *mp3_acm_codec89_last_error(const mp3_acm_codec89 *c)
{
    if (c == 0) return "MP3 ACM codec state is null";
    return c->error[0] != '\0' ? c->error : "no MP3 ACM codec error";
}

const WAVEFORMATEX *mp3_acm_codec89_pcm_format(const mp3_acm_codec89 *c)
{
    return c != 0 ? &c->pcm_format : 0;
}
