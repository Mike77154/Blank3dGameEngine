#ifndef IMGCC0_H
#define IMGCC0_H

#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IMGCC0_VERSION_MAJOR 0
#define IMGCC0_VERSION_MINOR 6
#define IMGCC0_VERSION_PATCH 1

#if UINT_MAX != 0xFFFFFFFFU
#error imgcc0 requires a 32-bit unsigned int target
#endif

typedef unsigned char imgcc0_u8;
typedef unsigned short imgcc0_u16;
typedef unsigned int imgcc0_u32;
typedef signed int imgcc0_s32;

typedef enum imgcc0_format_e {
    IMGCC0_FMT_UNKNOWN = 0,
    IMGCC0_FMT_PNG,
    IMGCC0_FMT_APNG,
    IMGCC0_FMT_GIF,
    IMGCC0_FMT_JPEG,
    IMGCC0_FMT_BMP,
    IMGCC0_FMT_TGA,
    IMGCC0_FMT_PCX,
    IMGCC0_FMT_QOI,
    IMGCC0_FMT_WEBP,
    IMGCC0_FMT_WEBP_ANIM,
    IMGCC0_FMT_DDS,
    IMGCC0_FMT_TIFF,
    IMGCC0_FMT_PSD
} imgcc0_format;

typedef enum imgcc0_error_e {
    IMGCC0_OK = 0,
    IMGCC0_ERR_ARGUMENT = -1,
    IMGCC0_ERR_FORMAT = -2,
    IMGCC0_ERR_UNSUPPORTED = -3,
    IMGCC0_ERR_CORRUPT = -4,
    IMGCC0_ERR_STORAGE = -5,
    IMGCC0_ERR_IO = -6,
    IMGCC0_ERR_LIMIT = -7,
    IMGCC0_ERR_CODEC = -8,
    IMGCC0_ERR_PROTOCOL = -9
} imgcc0_error;

typedef struct imgcc0_rgba8_s {
    imgcc0_u8 r;
    imgcc0_u8 g;
    imgcc0_u8 b;
    imgcc0_u8 a;
} imgcc0_rgba8;

typedef struct imgcc0_frame_s {
    imgcc0_u32 width;
    imgcc0_u32 height;
    imgcc0_u32 stride;
    imgcc0_u32 delay_ms;
    imgcc0_u8 *pixels;
} imgcc0_frame;

typedef struct imgcc0_image_s {
    int ok;
    int error_code;
    char error_message[128];
    imgcc0_format format;
    int is_animated;
    imgcc0_u32 width;
    imgcc0_u32 height;
    imgcc0_u32 frame_count;
    imgcc0_s32 loop_count;
    imgcc0_frame *frames;
    imgcc0_u32 output_used;
    imgcc0_u32 output_required_at_least;
    imgcc0_u32 temp_peak;
    imgcc0_u32 temp_required_at_least;
} imgcc0_image;

typedef struct imgcc0_open_options_s {
    void *output_buffer;
    imgcc0_u32 output_buffer_size;
    void *temp_buffer;
    imgcc0_u32 temp_buffer_size;
    void *file_buffer;
    imgcc0_u32 file_buffer_size;
    imgcc0_u32 max_width;
    imgcc0_u32 max_height;
    int strict;
} imgcc0_open_options;

typedef struct imgcc0_surface_s {
    imgcc0_u8 *pixels;
    imgcc0_u32 width;
    imgcc0_u32 height;
    imgcc0_u32 stride;
} imgcc0_surface;

#define IMGCC0_DECLARE_BUFFER(name, count) \
    union name##_imgcc0_storage_u { void *align_ptr; imgcc0_u32 align_u32; imgcc0_u8 bytes[(count)]; } name
#define IMGCC0_BUFFER_DATA(name) ((void *)((name).bytes))
#define IMGCC0_BUFFER_SIZE(name) ((imgcc0_u32)sizeof((name).bytes))

void imgcc0_open_options_init(imgcc0_open_options *opt);
void imgcc0_image_init(imgcc0_image *img);
void imgcc0_image_reset(imgcc0_image *img);

const char *imgcc0_format_name(imgcc0_format fmt);
const char *imgcc0_error_string(int code);
int imgcc0_format_is_protocol_ready(imgcc0_format fmt);

imgcc0_format imgcc0_detect_format_memory(const void *data, imgcc0_u32 size);
int imgcc0_is_animated_memory(const void *data, imgcc0_u32 size, imgcc0_format fmt);

imgcc0_u32 imgcc0_rgba_storage_bytes(imgcc0_u32 width,
                                     imgcc0_u32 height,
                                     imgcc0_u32 frame_count);

int imgcc0_open_memory(const void *data,
                       imgcc0_u32 size,
                       const imgcc0_open_options *opt,
                       imgcc0_image *out_img);

int imgcc0_open_file(const char *filename,
                     const imgcc0_open_options *opt,
                     imgcc0_image *out_img);

imgcc0_rgba8 imgcc0_frame_get_pixel(const imgcc0_frame *frame,
                                    imgcc0_u32 x,
                                    imgcc0_u32 y);

int imgcc0_surface_blit(const imgcc0_frame *src,
                        imgcc0_surface *dst,
                        imgcc0_s32 dst_x,
                        imgcc0_s32 dst_y);

#ifdef __cplusplus
}
#endif

#endif
