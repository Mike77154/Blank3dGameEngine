#include "png_decoder.h"

/* Optional zlib wrappers.
   Define PNG_DEC_USE_ZLIB and link with -lz to enable.
 */

#ifdef PNG_DEC_USE_ZLIB

#include <zlib.h>
#include <string.h>
#include "png_mem89.h"

static voidpf png_zlib_alloc89(voidpf opaque, uInt items, uInt size)
{
    (void)opaque;
    return (voidpf)png_mem89_alloc_zero((unsigned int)items, (unsigned int)size);
}

static void png_zlib_release89(voidpf opaque, voidpf address)
{
    (void)opaque;
    png_mem89_release((void*)address);
}

int png_zlib_decompress(png_u8* dest, png_u32* dest_len,
                        const png_u8* src, png_u32 src_len)
{
    z_stream zs;
    png_u32 out_cap;
    int zr;
    if (!dest || !dest_len || (!src && src_len != 0u))
        return -1;
    out_cap = *dest_len;
    memset(&zs, 0, sizeof(zs));
    zs.zalloc = png_zlib_alloc89;
    zs.zfree = png_zlib_release89;
    zs.next_in = (Bytef*)src;
    zs.avail_in = (uInt)src_len;
    zs.next_out = (Bytef*)dest;
    zs.avail_out = (uInt)out_cap;
    zr = inflateInit(&zs);
    if (zr != Z_OK)
        return -1;
    zr = inflate(&zs, Z_FINISH);
    *dest_len = out_cap - (png_u32)zs.avail_out;
    inflateEnd(&zs);
    return (zr == Z_STREAM_END) ? 0 : -1;
}

int png_zlib_compress(png_u8* dest, png_u32* dest_len,
                      const png_u8* src, png_u32 src_len,
                      int level)
{
    z_stream zs;
    png_u32 out_cap;
    int zr;
    if (!dest || !dest_len || (!src && src_len != 0u))
        return -1;
    out_cap = *dest_len;
    memset(&zs, 0, sizeof(zs));
    zs.zalloc = png_zlib_alloc89;
    zs.zfree = png_zlib_release89;
    zs.next_in = (Bytef*)src;
    zs.avail_in = (uInt)src_len;
    zs.next_out = (Bytef*)dest;
    zs.avail_out = (uInt)out_cap;
    zr = deflateInit(&zs, level);
    if (zr != Z_OK)
        return -1;
    zr = deflate(&zs, Z_FINISH);
    *dest_len = out_cap - (png_u32)zs.avail_out;
    deflateEnd(&zs);
    return (zr == Z_STREAM_END) ? 0 : -1;
}

int png_decode_memory_zlib(const png_u8* data, png_u32 size, png_image* out_image)
{
    return png_decode_memory(data, size, png_zlib_decompress, out_image);
}

int png_decode_memory_ex_zlib(const png_u8* data, png_u32 size,
                              const png_decode_options* options,
                              png_image* out_image)
{
    return png_decode_memory_ex(data, size, png_zlib_decompress, options, out_image);
}

int png_load_file_zlib(const char* filename, png_image* out_image)
{
    return png_load_file(filename, png_zlib_decompress, out_image);
}

int png_load_file_ex_zlib(const char* filename,
                          const png_decode_options* options,
                          png_image* out_image)
{
    return png_load_file_ex(filename, png_zlib_decompress, options, out_image);
}

int png_encode_rgba8_memory_zlib(const png_u8* rgba,
                                png_u32 width,
                                png_u32 height,
                                int zlevel,
                                png_u8** out_png,
                                png_u32* out_png_size)
{
    return png_encode_rgba8_memory(rgba, width, height, png_zlib_compress, zlevel, out_png, out_png_size);
}

int png_encode_memory_ex_zlib(const png_u8* pixels,
                              png_u32 width,
                              png_u32 height,
                              const png_encode_options* options,
                              png_u8** out_png,
                              png_u32* out_png_size)
{
    return png_encode_memory_ex(pixels, width, height, png_zlib_compress, options, out_png, out_png_size);
}

int png_encode_image_ex_zlib(const png_image* image,
                             const png_encode_options* options,
                             png_u8** out_png,
                             png_u32* out_png_size)
{
    return png_encode_image_ex(image, png_zlib_compress, options, out_png, out_png_size);
}

int png_decode_apng_memory_zlib(const png_u8* data,
                                png_u32 size,
                                png_apng* out_apng)
{
    return png_decode_apng_memory(data, size, png_zlib_decompress, out_apng);
}

int png_decode_apng_memory_ex_zlib(const png_u8* data,
                                   png_u32 size,
                                   const png_decode_options* options,
                                   png_apng* out_apng)
{
    return png_decode_apng_memory_ex(data, size, png_zlib_decompress, options, out_apng);
}

int png_load_apng_file_zlib(const char* filename, png_apng* out_apng)
{
    return png_load_apng_file(filename, png_zlib_decompress, out_apng);
}

int png_load_apng_file_ex_zlib(const char* filename,
                               const png_decode_options* options,
                               png_apng* out_apng)
{
    return png_load_apng_file_ex(filename, png_zlib_decompress, options, out_apng);
}

int png_encode_apng_memory_ex_zlib(const png_apng_encode_frame* frames,
                                   png_u32 frame_count,
                                   png_u32 canvas_width,
                                   png_u32 canvas_height,
                                   png_u32 num_plays,
                                   const png_encode_options* options,
                                   png_u8** out_png,
                                   png_u32* out_png_size)
{
    return png_encode_apng_memory_ex(frames, frame_count, canvas_width, canvas_height, num_plays, png_zlib_compress, options, out_png, out_png_size);
}

int png_encode_apng_auto_memory_ex_zlib(const png_apng_encode_frame* full_canvas_frames,
                                        png_u32 frame_count,
                                        png_u32 canvas_width,
                                        png_u32 canvas_height,
                                        png_u32 num_plays,
                                        const png_encode_options* options,
                                        png_u8** out_png,
                                        png_u32* out_png_size)
{
    return png_encode_apng_auto_memory_ex(full_canvas_frames, frame_count, canvas_width, canvas_height, num_plays, png_zlib_compress, options, out_png, out_png_size);
}

#else

int png_zlib_decompress(png_u8* dest, png_u32* dest_len,
                        const png_u8* src, png_u32 src_len)
{
    (void)dest; (void)dest_len; (void)src; (void)src_len;
    return -1;
}

int png_zlib_compress(png_u8* dest, png_u32* dest_len,
                      const png_u8* src, png_u32 src_len,
                      int level)
{
    (void)dest; (void)dest_len; (void)src; (void)src_len; (void)level;
    return -1;
}

int png_decode_memory_zlib(const png_u8* data, png_u32 size, png_image* out_image)
{
    (void)data; (void)size; (void)out_image;
    return PNG_DEC_ERR_UNSUPPORTED;
}

int png_decode_memory_ex_zlib(const png_u8* data, png_u32 size,
                              const png_decode_options* options,
                              png_image* out_image)
{
    (void)data; (void)size; (void)options; (void)out_image;
    return PNG_DEC_ERR_UNSUPPORTED;
}

int png_load_file_zlib(const char* filename, png_image* out_image)
{
    (void)filename; (void)out_image;
    return PNG_DEC_ERR_UNSUPPORTED;
}

int png_load_file_ex_zlib(const char* filename,
                          const png_decode_options* options,
                          png_image* out_image)
{
    (void)filename; (void)options; (void)out_image;
    return PNG_DEC_ERR_UNSUPPORTED;
}

int png_encode_rgba8_memory_zlib(const png_u8* rgba,
                                png_u32 width,
                                png_u32 height,
                                int zlevel,
                                png_u8** out_png,
                                png_u32* out_png_size)
{
    (void)rgba; (void)width; (void)height; (void)zlevel; (void)out_png; (void)out_png_size;
    return PNG_DEC_ERR_UNSUPPORTED;
}

int png_encode_memory_ex_zlib(const png_u8* pixels,
                              png_u32 width,
                              png_u32 height,
                              const png_encode_options* options,
                              png_u8** out_png,
                              png_u32* out_png_size)
{
    (void)pixels; (void)width; (void)height; (void)options; (void)out_png; (void)out_png_size;
    return PNG_DEC_ERR_UNSUPPORTED;
}

int png_encode_image_ex_zlib(const png_image* image,
                             const png_encode_options* options,
                             png_u8** out_png,
                             png_u32* out_png_size)
{
    (void)image; (void)options; (void)out_png; (void)out_png_size;
    return PNG_DEC_ERR_UNSUPPORTED;
}

int png_decode_apng_memory_zlib(const png_u8* data,
                                png_u32 size,
                                png_apng* out_apng)
{
    (void)data; (void)size; (void)out_apng;
    return PNG_DEC_ERR_UNSUPPORTED;
}

int png_decode_apng_memory_ex_zlib(const png_u8* data,
                                   png_u32 size,
                                   const png_decode_options* options,
                                   png_apng* out_apng)
{
    (void)data; (void)size; (void)options; (void)out_apng;
    return PNG_DEC_ERR_UNSUPPORTED;
}

int png_load_apng_file_zlib(const char* filename, png_apng* out_apng)
{
    (void)filename; (void)out_apng;
    return PNG_DEC_ERR_UNSUPPORTED;
}

int png_load_apng_file_ex_zlib(const char* filename,
                               const png_decode_options* options,
                               png_apng* out_apng)
{
    (void)filename; (void)options; (void)out_apng;
    return PNG_DEC_ERR_UNSUPPORTED;
}

int png_encode_apng_memory_ex_zlib(const png_apng_encode_frame* frames,
                                   png_u32 frame_count,
                                   png_u32 canvas_width,
                                   png_u32 canvas_height,
                                   png_u32 num_plays,
                                   const png_encode_options* options,
                                   png_u8** out_png,
                                   png_u32* out_png_size)
{
    (void)frames; (void)frame_count; (void)canvas_width; (void)canvas_height;
    (void)num_plays; (void)options; (void)out_png; (void)out_png_size;
    return PNG_DEC_ERR_UNSUPPORTED;
}

int png_encode_apng_auto_memory_ex_zlib(const png_apng_encode_frame* full_canvas_frames,
                                        png_u32 frame_count,
                                        png_u32 canvas_width,
                                        png_u32 canvas_height,
                                        png_u32 num_plays,
                                        const png_encode_options* options,
                                        png_u8** out_png,
                                        png_u32* out_png_size)
{
    (void)full_canvas_frames; (void)frame_count; (void)canvas_width; (void)canvas_height;
    (void)num_plays; (void)options; (void)out_png; (void)out_png_size;
    return PNG_DEC_ERR_UNSUPPORTED;
}

#endif
