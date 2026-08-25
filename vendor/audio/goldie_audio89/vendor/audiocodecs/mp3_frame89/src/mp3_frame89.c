#include "mp3_frame89.h"
#include <string.h>

static const int g_bitrate_mpeg1_l3[16] = {
    0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0
};

static const int g_bitrate_mpeg2_l3[16] = {
    0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0
};

static int mp3_frame89_rate(int version_bits, int rate_index)
{
    static const int base[3] = { 44100, 48000, 32000 };
    int r;
    if (rate_index < 0 || rate_index > 2) return 0;
    r = base[rate_index];
    if (version_bits == 3) return r;
    if (version_bits == 2) return r / 2;
    if (version_bits == 0) return r / 4;
    return 0;
}

int mp3_frame89_parse_header(const unsigned char h[4], mp3_frame89_info *out)
{
    int version_bits;
    int layer_bits;
    int bitrate_index;
    int rate_index;
    int mode;
    int bitrate;
    int rate;
    int padding;
    int bytes;
    int factor;
    if (h == 0 || out == 0) return 0;
    if (h[0] != 0xff || (h[1] & 0xe0) != 0xe0) return 0;
    version_bits = (h[1] >> 3) & 3;
    layer_bits = (h[1] >> 1) & 3;
    if (version_bits == 1 || layer_bits != 1) return 0;
    bitrate_index = (h[2] >> 4) & 15;
    rate_index = (h[2] >> 2) & 3;
    if (bitrate_index == 0 || bitrate_index == 15 || rate_index == 3) return 0;
    bitrate = version_bits == 3 ? g_bitrate_mpeg1_l3[bitrate_index] : g_bitrate_mpeg2_l3[bitrate_index];
    rate = mp3_frame89_rate(version_bits, rate_index);
    if (bitrate <= 0 || rate <= 0) return 0;
    padding = (h[2] >> 1) & 1;
    factor = version_bits == 3 ? 144 : 72;
    bytes = (factor * bitrate * 1000) / rate + padding;
    if (bytes < 4 || bytes > MP3_FRAME89_MAX_FRAME_BYTES) return 0;
    mode = (h[3] >> 6) & 3;
    out->mpeg_version = version_bits == 3 ? 1 : (version_bits == 2 ? 2 : 25);
    out->bitrate_kbps = bitrate;
    out->sample_rate = rate;
    out->channels = mode == 3 ? 1 : 2;
    out->padding = padding;
    out->frame_bytes = bytes;
    out->samples_per_frame = version_bits == 3 ? 1152 : 576;
    return 1;
}

static int mp3_frame89_skip_id3(FILE *fp)
{
    unsigned char h[10];
    unsigned long size;
    unsigned long skip;
    int flags;
    long start;
    start = ftell(fp);
    if (start < 0L) return 0;
    if (fread(h, 1, 10, fp) != 10) {
        fseek(fp, start, SEEK_SET);
        return 1;
    }
    if (memcmp(h, "ID3", 3) != 0) {
        fseek(fp, start, SEEK_SET);
        return 1;
    }
    if ((h[6] | h[7] | h[8] | h[9]) & 0x80) return 0;
    size = ((unsigned long)h[6] << 21) |
           ((unsigned long)h[7] << 14) |
           ((unsigned long)h[8] << 7) |
           (unsigned long)h[9];
    flags = h[5];
    skip = size;
    if (flags & 0x10) skip += 10UL;
    if (skip > 0x7fffffffUL) return 0;
    return fseek(fp, (long)skip, SEEK_CUR) == 0;
}

int mp3_frame89_find_first(FILE *fp, unsigned long *offset, mp3_frame89_info *out)
{
    unsigned char h[4];
    int c;
    int filled;
    long pos;
    if (fp == 0 || out == 0) return 0;
    if (fseek(fp, 0L, SEEK_SET) != 0) return 0;
    if (!mp3_frame89_skip_id3(fp)) return 0;
    filled = 0;
    while ((c = fgetc(fp)) != EOF) {
        if (filled < 4) {
            h[filled++] = (unsigned char)c;
        } else {
            h[0] = h[1];
            h[1] = h[2];
            h[2] = h[3];
            h[3] = (unsigned char)c;
        }
        if (filled == 4 && mp3_frame89_parse_header(h, out)) {
            pos = ftell(fp);
            if (pos < 4L) return 0;
            pos -= 4L;
            if (offset != 0) *offset = (unsigned long)pos;
            return fseek(fp, pos, SEEK_SET) == 0;
        }
    }
    return 0;
}

int mp3_frame89_read_next(FILE *fp, unsigned char *dst, int cap, mp3_frame89_info *out)
{
    unsigned char h[4];
    int c;
    int filled;
    int remain;
    if (fp == 0 || dst == 0 || out == 0 || cap < 4) return 0;
    filled = 0;
    while ((c = fgetc(fp)) != EOF) {
        if (filled < 4) {
            h[filled++] = (unsigned char)c;
        } else {
            h[0] = h[1];
            h[1] = h[2];
            h[2] = h[3];
            h[3] = (unsigned char)c;
        }
        if (filled == 4 && mp3_frame89_parse_header(h, out)) {
            if (out->frame_bytes > cap) return 0;
            dst[0] = h[0];
            dst[1] = h[1];
            dst[2] = h[2];
            dst[3] = h[3];
            remain = out->frame_bytes - 4;
            if (remain > 0 && fread(dst + 4, 1, (size_t)remain, fp) != (size_t)remain) return 0;
            return out->frame_bytes;
        }
    }
    return 0;
}
