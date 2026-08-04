#ifndef PNG89_H
#define PNG89_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#ifndef PNG89_U8_DEFINED
#define PNG89_U8_DEFINED
typedef unsigned char png89_u8;
#endif
#ifndef PNG89_U16_DEFINED
#define PNG89_U16_DEFINED
typedef unsigned short png89_u16;
#endif
#ifndef PNG89_U32_DEFINED
#define PNG89_U32_DEFINED
typedef unsigned long png89_u32;
#endif

enum {
    PNG89_OK = 0,
    PNG89_EINVAL = -1,
    PNG89_EFORMAT = -2,
    PNG89_EUNSUPPORTED = -3,
    PNG89_ERANGE = -4,
    PNG89_ENOSPC = -5,
    PNG89_ECRC = -6,
    PNG89_EINFLATE = -7
};

enum {
    PNG89_FLAG_IGNORE_CRC = 1u << 0,
    PNG89_FLAG_ALLOW_MISSING_PLTE = 1u << 1,
    PNG89_FLAG_FORCE_STRIP_16 = 1u << 2
};

typedef struct Png89Info {
    png89_u32 width;
    png89_u32 height;
    png89_u8 bit_depth;
    png89_u8 color_type;
    png89_u8 compression_method;
    png89_u8 filter_method;
    png89_u8 interlace_method;
    png89_u32 idat_size;
    png89_u32 raw_size;
    int has_plte;
    int has_trns;
    png89_u16 transparent_gray;
    png89_u16 transparent_red;
    png89_u16 transparent_green;
    png89_u16 transparent_blue;
    png89_u16 palette_entries;
    png89_u8 palette_rgba[256u * 4u];
} Png89Info;

typedef struct Png89Requirements {
    png89_u32 row_bytes;
    png89_u32 bytes_per_pixel;
    png89_u32 idat_size;
    png89_u32 inflate_out_size;
    png89_u32 prev_row_size;
    png89_u32 cur_row_size;
    png89_u32 pass_row_size;
    png89_u32 scratch_size;
} Png89Requirements;

typedef struct Png89InflateHooks {
    int (*inflate_zlib)(void *user,
                        const png89_u8 *src, png89_u32 src_size,
                        png89_u8 *dst, png89_u32 dst_size,
                        png89_u32 *out_written);
    void *user;
} Png89InflateHooks;

typedef struct Png89Scratch {
    png89_u8 *idat_data;
    png89_u32 idat_size;
    png89_u8 *inflate_out;
    png89_u32 inflate_out_size;
    png89_u8 *prev_row;
    png89_u32 prev_row_size;
    png89_u8 *cur_row;
    png89_u32 cur_row_size;
    png89_u8 *pass_row;
    png89_u32 pass_row_size;
} Png89Scratch;

int png89_read_info(const png89_u8 *data, png89_u32 size, png89_u32 flags, Png89Info *out_info);
int png89_get_requirements(const png89_u8 *data, png89_u32 size, png89_u32 flags,
                           Png89Info *out_info, Png89Requirements *out_req);
int png89_decode_indexed8(const png89_u8 *data, png89_u32 size,
                           png89_u32 flags,
                           const Png89InflateHooks *inflate,
                           const Png89Scratch *scratch,
                           png89_u8 *out_pixels, png89_u32 out_pixels_size,
                           png89_u8 out_pal_rgb[768], int *out_has_palette,
                           Png89Info *out_info);

int png89_decode_rgba8888(const png89_u8 *data, png89_u32 size,
                          png89_u32 flags,
                          const Png89InflateHooks *inflate,
                          const Png89Scratch *scratch,
                          png89_u8 *out_rgba, png89_u32 out_rgba_size,
                          Png89Info *out_info);

#ifdef __cplusplus
}
#endif

#endif
