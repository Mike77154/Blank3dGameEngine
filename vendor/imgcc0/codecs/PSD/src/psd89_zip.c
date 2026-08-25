#include "psd89_internal.h"

#include <zlib.h>

#ifndef PSD89_ZLIB_ARENA_BYTES
#define PSD89_ZLIB_ARENA_BYTES 393216U
#endif

#ifndef PSD89_ZIP_IO_CHUNK
#define PSD89_ZIP_IO_CHUNK 4096U
#endif

typedef struct psd89_zlib_arena {
    psd89_u32 used;
    psd89_u8 mem[PSD89_ZLIB_ARENA_BYTES];
} psd89_zlib_arena;

static voidpf psd89_zalloc(voidpf opaque, uInt items, uInt size)
{
    psd89_zlib_arena *arena;
    psd89_u32 need;
    psd89_u32 off;
    psd89_u32 align;

    arena = (psd89_zlib_arena *)opaque;
    if (arena == 0) {
        return Z_NULL;
    }
    need = (psd89_u32)items * (psd89_u32)size;
    if (items != 0U && need / (psd89_u32)items != (psd89_u32)size) {
        return Z_NULL;
    }
    align = (psd89_u32)sizeof(psd89_u32) - 1U;
    off = arena->used;
    off = (off + align) & ~align;
    if (off > (psd89_u32)sizeof(arena->mem) || need > (psd89_u32)sizeof(arena->mem) - off) {
        return Z_NULL;
    }
    arena->used = off + need;
    return arena->mem + off;
}

static void psd89_zfree(voidpf opaque, voidpf address)
{
    (void)opaque;
    (void)address;
}

static int psd89_zip_fill_input(psd89_io *io,
                                psd89_u32 *remain,
                                z_stream *zs,
                                Bytef *inbuf)
{
    psd89_u32 chunk;

    if (zs->avail_in != 0U) {
        return 1;
    }
    if (*remain == 0U) {
        return 1;
    }
    chunk = *remain > (psd89_u32)PSD89_ZIP_IO_CHUNK ? (psd89_u32)PSD89_ZIP_IO_CHUNK : *remain;
    if (!psd89_io_read(io, inbuf, chunk)) {
        return 0;
    }
    zs->next_in = inbuf;
    zs->avail_in = (uInt)chunk;
    *remain -= chunk;
    return 1;
}

static void psd89_zip_unpredict_row(psd89_u8 *row, psd89_u32 cols)
{
    psd89_u32 x;
    if (row == 0 || cols == 0U) {
        return;
    }
    for (x = 1U; x < cols; ++x) {
        row[x] = (psd89_u8)(row[x] + row[x - 1U]);
    }
}

static void psd89_zip_predict_row(const psd89_u8 *src, psd89_u8 *dst, psd89_u32 cols)
{
    psd89_u32 x;
    if (dst == 0 || cols == 0U) {
        return;
    }
    if (src == 0) {
        dst[0] = 0U;
        for (x = 1U; x < cols; ++x) {
            dst[x] = 0U;
        }
        return;
    }
    dst[0] = src[0];
    for (x = 1U; x < cols; ++x) {
        dst[x] = (psd89_u8)(src[x] - src[x - 1U]);
    }
}

static int psd89_zip_decode_plane_mode(psd89_io *io,
                                       psd89_u32 data_offset,
                                       psd89_u32 encoded_size,
                                       psd89_u8 *dst,
                                       psd89_u32 stride,
                                       psd89_u32 rows,
                                       psd89_u32 cols,
                                       int prediction,
                                       int raw_deflate)
{
    z_stream zs;
    psd89_zlib_arena arena;
    Bytef inbuf[PSD89_ZIP_IO_CHUNK];
    int ret;
    psd89_u32 remain;
    psd89_u32 y;
    psd89_u8 *row;

    memset(&zs, 0, sizeof(zs));
    memset(&arena, 0, sizeof(arena));
    if (!psd89_io_seek(io, data_offset)) {
        return 0;
    }
    zs.zalloc = psd89_zalloc;
    zs.zfree = psd89_zfree;
    zs.opaque = &arena;
    ret = raw_deflate ? inflateInit2(&zs, -MAX_WBITS) : inflateInit(&zs);
    if (ret != Z_OK) {
        return 0;
    }
    remain = encoded_size;
    for (y = 0U; y < rows; ++y) {
        row = dst + (psd89_u32)y * stride;
        zs.next_out = row;
        zs.avail_out = (uInt)cols;
        while (zs.avail_out != 0U) {
            if (!psd89_zip_fill_input(io, &remain, &zs, inbuf)) {
                inflateEnd(&zs);
                return 0;
            }
            ret = inflate(&zs, Z_NO_FLUSH);
            if (ret != Z_OK && ret != Z_STREAM_END) {
                inflateEnd(&zs);
                return 0;
            }
            if (ret == Z_STREAM_END && zs.avail_out != 0U) {
                inflateEnd(&zs);
                return 0;
            }
        }
        if (prediction) {
            psd89_zip_unpredict_row(row, cols);
        }
    }
    inflateEnd(&zs);
    return 1;
}

static int psd89_zip_decode_composite_mode(psd89_io *io,
                                           psd89_u32 data_offset,
                                           psd89_u32 encoded_size,
                                           psd89_u8 **planes,
                                           psd89_u32 stride,
                                           psd89_u16 channels,
                                           psd89_u32 rows,
                                           psd89_u32 cols,
                                           int prediction,
                                           int raw_deflate)
{
    z_stream zs;
    psd89_zlib_arena arena;
    Bytef inbuf[PSD89_ZIP_IO_CHUNK];
    int ret;
    psd89_u32 remain;
    psd89_u32 c;
    psd89_u32 y;
    psd89_u8 *row;

    memset(&zs, 0, sizeof(zs));
    memset(&arena, 0, sizeof(arena));
    if (!psd89_io_seek(io, data_offset)) {
        return 0;
    }
    zs.zalloc = psd89_zalloc;
    zs.zfree = psd89_zfree;
    zs.opaque = &arena;
    ret = raw_deflate ? inflateInit2(&zs, -MAX_WBITS) : inflateInit(&zs);
    if (ret != Z_OK) {
        return 0;
    }
    remain = encoded_size;
    for (c = 0U; c < (psd89_u32)channels; ++c) {
        if (planes[c] == 0) {
            inflateEnd(&zs);
            return 0;
        }
        for (y = 0U; y < rows; ++y) {
            row = planes[c] + (psd89_u32)y * stride;
            zs.next_out = row;
            zs.avail_out = (uInt)cols;
            while (zs.avail_out != 0U) {
                if (!psd89_zip_fill_input(io, &remain, &zs, inbuf)) {
                    inflateEnd(&zs);
                    return 0;
                }
                ret = inflate(&zs, Z_NO_FLUSH);
                if (ret != Z_OK && ret != Z_STREAM_END) {
                    inflateEnd(&zs);
                    return 0;
                }
                if (ret == Z_STREAM_END && zs.avail_out != 0U) {
                    inflateEnd(&zs);
                    return 0;
                }
            }
            if (prediction) {
                psd89_zip_unpredict_row(row, cols);
            }
        }
    }
    inflateEnd(&zs);
    return 1;
}

int psd89_zip_decode_plane(psd89_io *io,
                           psd89_u32 data_offset,
                           psd89_u32 encoded_size,
                           psd89_u8 *dst,
                           psd89_u32 stride,
                           psd89_u32 rows,
                           psd89_u32 cols,
                           int prediction)
{
    if (psd89_zip_decode_plane_mode(io, data_offset, encoded_size, dst, stride, rows, cols, prediction, 0)) {
        return 1;
    }
    return psd89_zip_decode_plane_mode(io, data_offset, encoded_size, dst, stride, rows, cols, prediction, 1);
}

int psd89_zip_decode_composite(psd89_io *io,
                               psd89_u32 data_offset,
                               psd89_u32 encoded_size,
                               psd89_u8 **planes,
                               psd89_u32 stride,
                               psd89_u16 channels,
                               psd89_u32 rows,
                               psd89_u32 cols,
                               int prediction)
{
    if (psd89_zip_decode_composite_mode(io, data_offset, encoded_size, planes, stride, channels, rows, cols, prediction, 0)) {
        return 1;
    }
    return psd89_zip_decode_composite_mode(io, data_offset, encoded_size, planes, stride, channels, rows, cols, prediction, 1);
}

static int psd89_zip_flush_output(psd89_io *io, z_stream *zs, Bytef *outbuf)
{
    psd89_u32 used;

    used = (psd89_u32)PSD89_ZIP_IO_CHUNK - (psd89_u32)zs->avail_out;
    if (used != 0U) {
        if (!psd89_io_write(io, outbuf, used)) {
            return 0;
        }
        zs->next_out = outbuf;
        zs->avail_out = PSD89_ZIP_IO_CHUNK;
    }
    return 1;
}

static int psd89_zip_write_plane_internal(psd89_io *io,
                                          const psd89_u8 *plane,
                                          psd89_u32 stride,
                                          psd89_u32 rows,
                                          psd89_u32 cols,
                                          int prediction)
{
    z_stream zs;
    psd89_zlib_arena arena;
    Bytef outbuf[PSD89_ZIP_IO_CHUNK];
    psd89_u8 rowbuf[PSD89_MAX_COMPOSE_ROW_BYTES];
    int ret;
    psd89_u32 y;
    const psd89_u8 *src_row;

    if (cols > PSD89_MAX_COMPOSE_ROW_BYTES) {
        return 0;
    }
    memset(&zs, 0, sizeof(zs));
    memset(&arena, 0, sizeof(arena));
    zs.zalloc = psd89_zalloc;
    zs.zfree = psd89_zfree;
    zs.opaque = &arena;
    ret = deflateInit(&zs, Z_DEFAULT_COMPRESSION);
    if (ret != Z_OK) {
        return 0;
    }
    zs.next_out = outbuf;
    zs.avail_out = PSD89_ZIP_IO_CHUNK;
    for (y = 0U; y < rows; ++y) {
        src_row = plane != 0 ? plane + (psd89_u32)y * stride : 0;
        if (prediction || src_row == 0) {
            psd89_zip_predict_row(src_row, rowbuf, cols);
            zs.next_in = rowbuf;
        } else {
            zs.next_in = (Bytef *)src_row;
        }
        zs.avail_in = (uInt)cols;
        while (zs.avail_in != 0U) {
            ret = deflate(&zs, Z_NO_FLUSH);
            if (ret != Z_OK) {
                deflateEnd(&zs);
                return 0;
            }
            if (zs.avail_out == 0U) {
                if (!psd89_zip_flush_output(io, &zs, outbuf)) {
                    deflateEnd(&zs);
                    return 0;
                }
            }
        }
    }
    do {
        ret = deflate(&zs, Z_FINISH);
        if (ret != Z_OK && ret != Z_STREAM_END) {
            deflateEnd(&zs);
            return 0;
        }
        if (!psd89_zip_flush_output(io, &zs, outbuf)) {
            deflateEnd(&zs);
            return 0;
        }
    } while (ret != Z_STREAM_END);
    deflateEnd(&zs);
    return 1;
}

int psd89_zip_write_plane(psd89_io *io,
                          const psd89_u8 *plane,
                          psd89_u32 stride,
                          psd89_u32 rows,
                          psd89_u32 cols,
                          int prediction)
{
    return psd89_zip_write_plane_internal(io, plane, stride ? stride : cols, rows, cols, prediction);
}

int psd89_zip_write_composite(psd89_io *io,
                              const psd89_u8 **planes,
                              psd89_u32 stride,
                              psd89_u16 channels,
                              psd89_u32 rows,
                              psd89_u32 cols,
                              int prediction)
{
    z_stream zs;
    psd89_zlib_arena arena;
    Bytef outbuf[PSD89_ZIP_IO_CHUNK];
    psd89_u8 rowbuf[PSD89_MAX_COMPOSE_ROW_BYTES];
    int ret;
    psd89_u32 c;
    psd89_u32 y;
    const psd89_u8 *src_row;
    const psd89_u8 *plane;

    if (planes == 0 || cols > PSD89_MAX_COMPOSE_ROW_BYTES) {
        return 0;
    }
    memset(&zs, 0, sizeof(zs));
    memset(&arena, 0, sizeof(arena));
    zs.zalloc = psd89_zalloc;
    zs.zfree = psd89_zfree;
    zs.opaque = &arena;
    ret = deflateInit(&zs, Z_DEFAULT_COMPRESSION);
    if (ret != Z_OK) {
        return 0;
    }
    zs.next_out = outbuf;
    zs.avail_out = PSD89_ZIP_IO_CHUNK;
    for (c = 0U; c < (psd89_u32)channels; ++c) {
        plane = planes[c];
        for (y = 0U; y < rows; ++y) {
            src_row = plane != 0 ? plane + (psd89_u32)y * (stride ? stride : cols) : 0;
            if (prediction || src_row == 0) {
                psd89_zip_predict_row(src_row, rowbuf, cols);
                zs.next_in = rowbuf;
            } else {
                zs.next_in = (Bytef *)src_row;
            }
            zs.avail_in = (uInt)cols;
            while (zs.avail_in != 0U) {
                ret = deflate(&zs, Z_NO_FLUSH);
                if (ret != Z_OK) {
                    deflateEnd(&zs);
                    return 0;
                }
                if (zs.avail_out == 0U) {
                    if (!psd89_zip_flush_output(io, &zs, outbuf)) {
                        deflateEnd(&zs);
                        return 0;
                    }
                }
            }
        }
    }
    do {
        ret = deflate(&zs, Z_FINISH);
        if (ret != Z_OK && ret != Z_STREAM_END) {
            deflateEnd(&zs);
            return 0;
        }
        if (!psd89_zip_flush_output(io, &zs, outbuf)) {
            deflateEnd(&zs);
            return 0;
        }
    } while (ret != Z_STREAM_END);
    deflateEnd(&zs);
    return 1;
}
