#ifndef BMP_CHUNKS_H
#define BMP_CHUNKS_H

#include "../include/bmp/bmp_export.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "bmp_types.h"

typedef struct bmp_stream_s {
    const bmp_u8 *data;
    bmp_u32 size;
    bmp_u32 pos;
    int error;
} bmp_stream;

BMP_EXPORT void bmp_stream_init(bmp_stream *s, const bmp_u8 *data, bmp_u32 size);

BMP_EXPORT bmp_u8  bmp_read_u8 (bmp_stream *s);
BMP_EXPORT bmp_u16 bmp_read_u16(bmp_stream *s);
BMP_EXPORT bmp_u32 bmp_read_u32(bmp_stream *s);
BMP_EXPORT bmp_s32 bmp_read_s32(bmp_stream *s);

BMP_EXPORT void bmp_stream_skip(bmp_stream *s, bmp_u32 count);
BMP_EXPORT void bmp_stream_seek(bmp_stream *s, bmp_u32 pos);
BMP_EXPORT const bmp_u8 *bmp_stream_peek(bmp_stream *s, bmp_u32 count);
BMP_EXPORT bmp_u32 bmp_stream_remaining(const bmp_stream *s);
BMP_EXPORT int bmp_stream_failed(const bmp_stream *s);

#ifdef __cplusplus
}
#endif

#endif /* BMP_CHUNKS_H */
