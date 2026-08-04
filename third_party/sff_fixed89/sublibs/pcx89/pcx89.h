#ifndef PCX89_H
#define PCX89_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#ifndef PCX89_U8_DEFINED
#define PCX89_U8_DEFINED
typedef unsigned char pcx89_u8;
#endif
#ifndef PCX89_U16_DEFINED
#define PCX89_U16_DEFINED
typedef unsigned short pcx89_u16;
#endif
#ifndef PCX89_U32_DEFINED
#define PCX89_U32_DEFINED
typedef unsigned long pcx89_u32;
#endif

enum {
    PCX89_OK = 0,
    PCX89_EINVAL = -1,
    PCX89_EFORMAT = -2,
    PCX89_EUNSUPPORTED = -3,
    PCX89_ERANGE = -4,
    PCX89_ENOSPC = -5,
    PCX89_EIO = -6
};

typedef struct Pcx89Header {
    pcx89_u8 manufacturer;
    pcx89_u8 version;
    pcx89_u8 encoding;
    pcx89_u8 bits_per_pixel;
    pcx89_u16 xmin;
    pcx89_u16 ymin;
    pcx89_u16 xmax;
    pcx89_u16 ymax;
    pcx89_u16 hdpi;
    pcx89_u16 vdpi;
    pcx89_u8 ega_palette[48];
    pcx89_u8 reserved0;
    pcx89_u8 num_planes;
    pcx89_u16 bytes_per_line;
    pcx89_u16 palette_info;
    pcx89_u16 hscreen_size;
    pcx89_u16 vscreen_size;
} Pcx89Header;

typedef struct Pcx89Info {
    Pcx89Header hdr;
    pcx89_u32 width;
    pcx89_u32 height;
    pcx89_u32 decoded_scanline_bytes;
    int has_vga_palette;
    int has_alpha_palette;
    int is_supported;
} Pcx89Info;

typedef struct Pcx89Scratch {
    pcx89_u8 *scanline;
    pcx89_u32 scanline_size;
} Pcx89Scratch;

int pcx89_read_info(const pcx89_u8 *data, pcx89_u32 size, Pcx89Info *out_info);
int pcx89_extract_vga_palette_rgb(const pcx89_u8 *data, pcx89_u32 size, pcx89_u8 out_rgb[768]);
int pcx89_decode_indexed8(const pcx89_u8 *data, pcx89_u32 size,
                          pcx89_u8 *out_pixels, pcx89_u32 out_pixels_size,
                          pcx89_u8 out_pal_rgb[768], int *out_has_palette,
                          const Pcx89Scratch *scratch,
                          Pcx89Info *out_info);

int pcx89_decode_rgba8888(const pcx89_u8 *data, pcx89_u32 size,
                          pcx89_u8 *out_rgba, pcx89_u32 out_rgba_size,
                          const Pcx89Scratch *scratch,
                          Pcx89Info *out_info);

#ifdef __cplusplus
}
#endif

#endif
