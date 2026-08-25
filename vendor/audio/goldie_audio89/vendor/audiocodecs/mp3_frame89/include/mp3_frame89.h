#ifndef MP3_FRAME89_H
#define MP3_FRAME89_H

#include <stdio.h>

#define MP3_FRAME89_MAX_FRAME_BYTES 2048

typedef struct mp3_frame89_info {
    int mpeg_version;
    int bitrate_kbps;
    int sample_rate;
    int channels;
    int padding;
    int frame_bytes;
    int samples_per_frame;
} mp3_frame89_info;

int mp3_frame89_parse_header(const unsigned char h[4], mp3_frame89_info *out);
int mp3_frame89_find_first(FILE *fp, unsigned long *offset, mp3_frame89_info *out);
int mp3_frame89_read_next(FILE *fp, unsigned char *dst, int cap, mp3_frame89_info *out);

#endif
