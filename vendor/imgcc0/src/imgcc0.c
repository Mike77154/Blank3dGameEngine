#include <stdio.h>
#include <string.h>

#include "imgcc0.h"
#include "c89jpeg.h"
#include "gafnyf_tga.h"
#include "giff/giff.h"
#include "qoi89.h"
#include "webp/types.h"
#include "webp/decode.h"
#include "webp/anim_decode.h"
#include "tifx.h"
#include "png_decoder.h"
#include "png_mem89.h"
#include "giffany_dds.h"
#include "bmp/bmp.h"
#include "pcx/pcx.h"
#include "imgcc0_zragf_bridge.h"
#include "psd89/psd89.h"

#ifndef IMGCC0_DEFAULT_MAX_DIM
#define IMGCC0_DEFAULT_MAX_DIM 16384U
#endif

#define IMGCC0_MAX_MSG 127
#define IMGCC0_ALIGN 8U
#define IMGCC0_WEBP_EXTRA_SCRATCH 1048576U

typedef struct imgcc0_arena_s {
    imgcc0_u8 *base;
    imgcc0_u32 size;
    imgcc0_u32 pos;
    imgcc0_u32 peak;
    imgcc0_u32 failed_need;
} imgcc0_arena;

typedef struct imgcc0_decode_ctx_s {
    imgcc0_arena output;
    imgcc0_arena temp;
    imgcc0_image *img;
} imgcc0_decode_ctx;

static void imgcc0_set_error(imgcc0_image *img, int code, const char *msg)
{
    imgcc0_u32 i;
    if (img == 0) return;
    img->ok = 0;
    img->error_code = code;
    if (msg == 0) msg = "unknown";
    i = 0U;
    while (i < IMGCC0_MAX_MSG && msg[i] != '\0') {
        img->error_message[i] = msg[i];
        ++i;
    }
    img->error_message[i] = '\0';
}

static int imgcc0_u32_add(imgcc0_u32 a, imgcc0_u32 b, imgcc0_u32 *out)
{
    if (out == 0) return 0;
    if (a > 0xFFFFFFFFU - b) return 0;
    *out = a + b;
    return 1;
}

static int imgcc0_u32_mul(imgcc0_u32 a, imgcc0_u32 b, imgcc0_u32 *out)
{
    if (out == 0) return 0;
    if (a == 0U || b == 0U) {
        *out = 0U;
        return 1;
    }
    if (a > 0xFFFFFFFFU / b) return 0;
    *out = a * b;
    return 1;
}

static imgcc0_u32 imgcc0_align_up(imgcc0_u32 value)
{
    imgcc0_u32 add;
    add = IMGCC0_ALIGN - 1U;
    if (value > 0xFFFFFFFFU - add) return 0xFFFFFFFFU;
    return (value + add) & ~add;
}

static void imgcc0_arena_init(imgcc0_arena *a, void *buffer, imgcc0_u32 size)
{
    if (a == 0) return;
    a->base = (imgcc0_u8 *)buffer;
    a->size = size;
    a->pos = 0U;
    a->peak = 0U;
    a->failed_need = 0U;
}

static void imgcc0_arena_rewind(imgcc0_arena *a)
{
    if (a == 0) return;
    a->pos = 0U;
    a->failed_need = 0U;
}

static void *imgcc0_arena_take(imgcc0_arena *a, imgcc0_u32 bytes, int clear)
{
    imgcc0_u32 start;
    imgcc0_u32 end;
    void *p;
    if (a == 0 || a->base == 0) return 0;
    start = imgcc0_align_up(a->pos);
    if (start == 0xFFFFFFFFU && a->pos != 0xFFFFFFFFU) return 0;
    if (!imgcc0_u32_add(start, bytes, &end) || end > a->size) {
        if (imgcc0_u32_add(start, bytes, &end)) a->failed_need = end;
        else a->failed_need = 0xFFFFFFFFU;
        return 0;
    }
    p = (void *)(a->base + start);
    a->pos = end;
    if (a->pos > a->peak) a->peak = a->pos;
    if (clear && bytes != 0U) memset(p, 0, bytes);
    return p;
}

static void imgcc0_sync_usage(imgcc0_decode_ctx *ctx)
{
    if (ctx == 0 || ctx->img == 0) return;
    ctx->img->output_used = ctx->output.pos;
    ctx->img->temp_peak = ctx->temp.peak;
    if (ctx->output.failed_need != 0U) {
        ctx->img->output_required_at_least = ctx->output.failed_need;
    }
    if (ctx->temp.failed_need != 0U) {
        ctx->img->temp_required_at_least = ctx->temp.failed_need;
    }
}

static int imgcc0_check_dimensions(imgcc0_decode_ctx *ctx,
                                   const imgcc0_open_options *opt,
                                   imgcc0_u32 width,
                                   imgcc0_u32 height)
{
    imgcc0_u32 max_w;
    imgcc0_u32 max_h;
    if (width == 0U || height == 0U) {
        imgcc0_set_error(ctx->img, IMGCC0_ERR_CORRUPT, "zero image dimension");
        return 0;
    }
    max_w = IMGCC0_DEFAULT_MAX_DIM;
    max_h = IMGCC0_DEFAULT_MAX_DIM;
    if (opt != 0) {
        if (opt->max_width != 0U) max_w = opt->max_width;
        if (opt->max_height != 0U) max_h = opt->max_height;
    }
    if (width > max_w || height > max_h) {
        imgcc0_set_error(ctx->img, IMGCC0_ERR_LIMIT, "image dimension limit exceeded");
        return 0;
    }
    return 1;
}

static int imgcc0_frames_take(imgcc0_decode_ctx *ctx, imgcc0_u32 count)
{
    imgcc0_u32 bytes;
    if (count == 0U) return 0;
    if (!imgcc0_u32_mul((imgcc0_u32)sizeof(imgcc0_frame), count, &bytes)) {
        imgcc0_set_error(ctx->img, IMGCC0_ERR_LIMIT, "frame table overflow");
        return 0;
    }
    ctx->img->frames = (imgcc0_frame *)imgcc0_arena_take(&ctx->output, bytes, 1);
    if (ctx->img->frames == 0) {
        imgcc0_sync_usage(ctx);
        imgcc0_set_error(ctx->img, IMGCC0_ERR_STORAGE, "output buffer too small for frame table");
        return 0;
    }
    ctx->img->frame_count = count;
    return 1;
}

static int imgcc0_frame_pixels_take(imgcc0_decode_ctx *ctx,
                                    imgcc0_frame *frame,
                                    imgcc0_u32 width,
                                    imgcc0_u32 height,
                                    imgcc0_u32 delay_ms)
{
    imgcc0_u32 stride;
    imgcc0_u32 total;
    if (!imgcc0_u32_mul(width, 4U, &stride) ||
        !imgcc0_u32_mul(stride, height, &total)) {
        imgcc0_set_error(ctx->img, IMGCC0_ERR_LIMIT, "rgba output overflow");
        return 0;
    }
    frame->pixels = (imgcc0_u8 *)imgcc0_arena_take(&ctx->output, total, 1);
    if (frame->pixels == 0) {
        imgcc0_sync_usage(ctx);
        imgcc0_set_error(ctx->img, IMGCC0_ERR_STORAGE, "output buffer too small for pixels");
        return 0;
    }
    frame->width = width;
    frame->height = height;
    frame->stride = stride;
    frame->delay_ms = delay_ms;
    return 1;
}

static void *imgcc0_temp_take(imgcc0_decode_ctx *ctx, imgcc0_u32 bytes, const char *what)
{
    void *p;
    p = imgcc0_arena_take(&ctx->temp, bytes, 0);
    if (p == 0) {
        imgcc0_sync_usage(ctx);
        imgcc0_set_error(ctx->img, IMGCC0_ERR_STORAGE, what);
    }
    return p;
}

static int imgcc0_png_has_actl(const imgcc0_u8 *data, imgcc0_u32 size)
{
    imgcc0_u32 pos;
    if (data == 0 || size < 16U) return 0;
    if (memcmp(data, "\x89PNG\r\n\x1a\n", 8) != 0) return 0;
    pos = 8U;
    while (pos <= size && size - pos >= 12U) {
        imgcc0_u32 len;
        const imgcc0_u8 *typ;
        len = ((imgcc0_u32)data[pos] << 24)
            | ((imgcc0_u32)data[pos + 1U] << 16)
            | ((imgcc0_u32)data[pos + 2U] << 8)
            | (imgcc0_u32)data[pos + 3U];
        typ = data + pos + 4U;
        if (memcmp(typ, "acTL", 4) == 0) return 1;
        if (len > size - pos - 12U) break;
        if (memcmp(typ, "IEND", 4) == 0) break;
        pos += 12U + len;
    }
    return 0;
}

static int imgcc0_riff_is_webp(const imgcc0_u8 *data, imgcc0_u32 size)
{
    if (data == 0 || size < 12U) return 0;
    if (memcmp(data, "RIFF", 4) != 0) return 0;
    return memcmp(data + 8U, "WEBP", 4) == 0;
}

static int imgcc0_pcx_signature(const imgcc0_u8 *data, imgcc0_u32 size)
{
    imgcc0_u8 bpp;
    if (data == 0 || size < 128U) return 0;
    if (data[0] != 0x0AU || data[2] != 1U) return 0;
    if (data[1] > 5U) return 0;
    bpp = data[3];
    return bpp == 1U || bpp == 2U || bpp == 4U || bpp == 8U;
}

void imgcc0_open_options_init(imgcc0_open_options *opt)
{
    if (opt == 0) return;
    memset(opt, 0, sizeof(*opt));
    opt->max_width = IMGCC0_DEFAULT_MAX_DIM;
    opt->max_height = IMGCC0_DEFAULT_MAX_DIM;
    opt->strict = 1;
}

void imgcc0_image_init(imgcc0_image *img)
{
    if (img == 0) return;
    memset(img, 0, sizeof(*img));
}

void imgcc0_image_reset(imgcc0_image *img)
{
    imgcc0_image_init(img);
}

const char *imgcc0_format_name(imgcc0_format fmt)
{
    switch (fmt) {
        case IMGCC0_FMT_PNG: return "PNG";
        case IMGCC0_FMT_APNG: return "APNG";
        case IMGCC0_FMT_GIF: return "GIF";
        case IMGCC0_FMT_JPEG: return "JPEG";
        case IMGCC0_FMT_BMP: return "BMP";
        case IMGCC0_FMT_TGA: return "TGA";
        case IMGCC0_FMT_PCX: return "PCX";
        case IMGCC0_FMT_QOI: return "QOI";
        case IMGCC0_FMT_WEBP: return "WEBP";
        case IMGCC0_FMT_WEBP_ANIM: return "WEBP_ANIM";
        case IMGCC0_FMT_DDS: return "DDS";
        case IMGCC0_FMT_TIFF: return "TIFF";
        case IMGCC0_FMT_PSD: return "PSD";
        default: return "UNKNOWN";
    }
}

const char *imgcc0_error_string(int code)
{
    switch (code) {
        case IMGCC0_OK: return "ok";
        case IMGCC0_ERR_ARGUMENT: return "bad argument";
        case IMGCC0_ERR_FORMAT: return "bad or unknown format";
        case IMGCC0_ERR_UNSUPPORTED: return "unsupported";
        case IMGCC0_ERR_CORRUPT: return "corrupt data";
        case IMGCC0_ERR_STORAGE: return "caller storage too small";
        case IMGCC0_ERR_IO: return "io error";
        case IMGCC0_ERR_LIMIT: return "limit exceeded";
        case IMGCC0_ERR_CODEC: return "codec error";
        case IMGCC0_ERR_PROTOCOL: return "codec quarantined by memory protocol";
        default: return "unknown error";
    }
}

int imgcc0_format_is_protocol_ready(imgcc0_format fmt)
{
    switch (fmt) {
        case IMGCC0_FMT_PNG:
        case IMGCC0_FMT_APNG:
        case IMGCC0_FMT_GIF:
        case IMGCC0_FMT_JPEG:
        case IMGCC0_FMT_BMP:
        case IMGCC0_FMT_TGA:
        case IMGCC0_FMT_PCX:
        case IMGCC0_FMT_QOI:
        case IMGCC0_FMT_WEBP:
        case IMGCC0_FMT_WEBP_ANIM:
        case IMGCC0_FMT_DDS:
        case IMGCC0_FMT_TIFF:
        case IMGCC0_FMT_PSD:
            return 1;
        default:
            return 0;
    }
}

imgcc0_format imgcc0_detect_format_memory(const void *data_void, imgcc0_u32 size)
{
    const imgcc0_u8 *data;
    tga_inspect inspect;
    if (data_void == 0) return IMGCC0_FMT_UNKNOWN;
    data = (const imgcc0_u8 *)data_void;
    if (size >= 8U && memcmp(data, "\x89PNG\r\n\x1a\n", 8) == 0) {
        return imgcc0_png_has_actl(data, size) ? IMGCC0_FMT_APNG : IMGCC0_FMT_PNG;
    }
    if (size >= 6U && (memcmp(data, "GIF87a", 6) == 0 || memcmp(data, "GIF89a", 6) == 0)) {
        return IMGCC0_FMT_GIF;
    }
    if (size >= 3U && data[0] == 0xFFU && data[1] == 0xD8U && data[2] == 0xFFU) {
        return IMGCC0_FMT_JPEG;
    }
    if (size >= 2U && data[0] == (imgcc0_u8)'B' && data[1] == (imgcc0_u8)'M') {
        return IMGCC0_FMT_BMP;
    }
    if (size >= 4U && memcmp(data, "qoif", 4) == 0) return IMGCC0_FMT_QOI;
    if (size >= 4U && memcmp(data, "DDS ", 4) == 0) return IMGCC0_FMT_DDS;
    if (size >= 4U && memcmp(data, "8BPS", 4) == 0) return IMGCC0_FMT_PSD;
    if (size >= 4U &&
        ((data[0] == (imgcc0_u8)'I' && data[1] == (imgcc0_u8)'I' &&
          ((data[2] == 42U && data[3] == 0U) || (data[2] == 43U && data[3] == 0U))) ||
         (data[0] == (imgcc0_u8)'M' && data[1] == (imgcc0_u8)'M' &&
          ((data[2] == 0U && data[3] == 42U) || (data[2] == 0U && data[3] == 43U))))) {
        return IMGCC0_FMT_TIFF;
    }
    if (imgcc0_riff_is_webp(data, size)) {
        GWPBitstreamFeatures f;
        memset(&f, 0, sizeof(f));
        if (GWPGetFeatures(data, (GWPu32)size, &f) == GWP_STATUS_OK) {
            return f.has_animation ? IMGCC0_FMT_WEBP_ANIM : IMGCC0_FMT_WEBP;
        }
        return IMGCC0_FMT_WEBP;
    }
    if (imgcc0_pcx_signature(data, size)) return IMGCC0_FMT_PCX;
    memset(&inspect, 0, sizeof(inspect));
    if (tga_inspect_memory(data_void, size, &inspect) == TGA_OK) return IMGCC0_FMT_TGA;
    return IMGCC0_FMT_UNKNOWN;
}

int imgcc0_is_animated_memory(const void *data_void, imgcc0_u32 size, imgcc0_format fmt)
{
    const imgcc0_u8 *data;
    imgcc0_u32 i;
    imgcc0_u32 seen;
    if (fmt == IMGCC0_FMT_UNKNOWN) fmt = imgcc0_detect_format_memory(data_void, size);
    if (fmt == IMGCC0_FMT_APNG || fmt == IMGCC0_FMT_WEBP_ANIM) return 1;
    if (fmt != IMGCC0_FMT_GIF || data_void == 0) return 0;
    data = (const imgcc0_u8 *)data_void;
    seen = 0U;
    for (i = 0U; i < size; ++i) {
        if (data[i] == 0x2CU) {
            ++seen;
            if (seen > 1U) return 1;
        }
    }
    return 0;
}

imgcc0_u32 imgcc0_rgba_storage_bytes(imgcc0_u32 width,
                                     imgcc0_u32 height,
                                     imgcc0_u32 frame_count)
{
    imgcc0_u32 pixels;
    imgcc0_u32 all_pixels;
    imgcc0_u32 table;
    imgcc0_u32 total;
    if (frame_count == 0U) return 0U;
    if (!imgcc0_u32_mul(width, height, &pixels)) return 0U;
    if (!imgcc0_u32_mul(pixels, 4U, &pixels)) return 0U;
    if (!imgcc0_u32_mul(pixels, frame_count, &all_pixels)) return 0U;
    if (!imgcc0_u32_mul((imgcc0_u32)sizeof(imgcc0_frame), frame_count, &table)) return 0U;
    table = imgcc0_align_up(table);
    if (!imgcc0_u32_add(table, all_pixels, &total)) return 0U;
    return total;
}

static int imgcc0_copy_rgb_to_rgba(imgcc0_frame *dst,
                                   const imgcc0_u8 *src,
                                   imgcc0_u32 src_stride,
                                   int gray)
{
    imgcc0_u32 y;
    imgcc0_u32 x;
    imgcc0_u8 *out_row;
    const imgcc0_u8 *in_row;
    for (y = 0U; y < dst->height; ++y) {
        out_row = dst->pixels + y * dst->stride;
        in_row = src + y * src_stride;
        for (x = 0U; x < dst->width; ++x) {
            if (gray) {
                imgcc0_u8 g;
                g = in_row[x];
                out_row[x * 4U + 0U] = g;
                out_row[x * 4U + 1U] = g;
                out_row[x * 4U + 2U] = g;
                out_row[x * 4U + 3U] = 255U;
            } else {
                out_row[x * 4U + 0U] = in_row[x * 3U + 0U];
                out_row[x * 4U + 1U] = in_row[x * 3U + 1U];
                out_row[x * 4U + 2U] = in_row[x * 3U + 2U];
                out_row[x * 4U + 3U] = 255U;
            }
        }
    }
    return 1;
}

static int imgcc0_decode_jpeg(const void *data,
                              imgcc0_u32 size,
                              const imgcc0_open_options *opt,
                              imgcc0_decode_ctx *ctx)
{
    c89jpeg_decoder dec;
    c89jpeg_image_info info;
    c89jpeg_decode_params params;
    c89jpeg_status rc;
    c89jpeg_u32 row_stride;
    c89jpeg_u32 raw_size;
    imgcc0_u8 *raw;
    void *progressive_ws;
    c89jpeg_u32 progressive_ws_size;
    imgcc0_frame *frame;
    int gray;
    c89jpeg_decoder_init(&dec);
    memset(&info, 0, sizeof(info));
    rc = c89jpeg_probe(&dec, (const c89jpeg_u8 *)data, (c89jpeg_u32)size, &info);
    if (rc != C89JPEG_OK) {
        imgcc0_set_error(ctx->img, IMGCC0_ERR_CODEC, "jpeg probe failed");
        return 0;
    }
    if (!imgcc0_check_dimensions(ctx, opt, (imgcc0_u32)info.width, (imgcc0_u32)info.height)) return 0;
    gray = info.components == 1U;
    row_stride = c89jpeg_decoder_row_stride(info.width,
                                            gray ? C89JPEG_DECODE_GRAY8 : C89JPEG_DECODE_RGB24,
                                            info.components);
    raw_size = c89jpeg_decoder_output_size(info.width,
                                           info.height,
                                           gray ? C89JPEG_DECODE_GRAY8 : C89JPEG_DECODE_RGB24,
                                           info.components);
    imgcc0_arena_rewind(&ctx->temp);
    raw = (imgcc0_u8 *)imgcc0_temp_take(ctx, (imgcc0_u32)raw_size, "temp buffer too small for jpeg decode");
    if (raw == 0) return 0;
    progressive_ws = 0;
    progressive_ws_size = c89jpeg_decoder_progressive_workspace_size(&info);
    if (info.progressive) {
        if (progressive_ws_size == 0) {
            imgcc0_set_error(ctx->img, IMGCC0_ERR_LIMIT, "progressive jpeg workspace overflow");
            return 0;
        }
        progressive_ws = imgcc0_temp_take(ctx, (imgcc0_u32)progressive_ws_size,
                                          "temp buffer too small for progressive jpeg coefficients");
        if (progressive_ws == 0) return 0;
    }
    if (!imgcc0_frames_take(ctx, 1U)) return 0;
    frame = &ctx->img->frames[0];
    if (!imgcc0_frame_pixels_take(ctx, frame, info.width, info.height, 0U)) return 0;
    memset(&params, 0, sizeof(params));
    params.data = (const c89jpeg_u8 *)data;
    params.size = (c89jpeg_u32)size;
    params.out_pixels = raw;
    params.out_capacity = raw_size;
    params.out_stride = row_stride;
    params.out_format = gray ? C89JPEG_DECODE_GRAY8 : C89JPEG_DECODE_RGB24;
    params.upsampling = C89JPEG_UPSAMPLE_LINEAR;
    params.progressive_workspace = progressive_ws;
    params.progressive_workspace_size = progressive_ws_size;
    rc = c89jpeg_decode(&dec, &params, &info);
    if (rc != C89JPEG_OK) {
        imgcc0_set_error(ctx->img, IMGCC0_ERR_CODEC, "jpeg decode failed");
        return 0;
    }
    imgcc0_copy_rgb_to_rgba(frame, raw, row_stride, gray);
    ctx->img->ok = 1;
    ctx->img->format = IMGCC0_FMT_JPEG;
    ctx->img->width = frame->width;
    ctx->img->height = frame->height;
    return 1;
}

static int imgcc0_decode_tga(const void *data,
                             imgcc0_u32 size,
                             const imgcc0_open_options *opt,
                             imgcc0_decode_ctx *ctx)
{
    tga_inspect inspect;
    tga_decode_params params;
    imgcc0_frame *frame;
    int rc;
    memset(&inspect, 0, sizeof(inspect));
    rc = tga_inspect_memory(data, size, &inspect);
    if (rc != TGA_OK) {
        imgcc0_set_error(ctx->img, IMGCC0_ERR_CODEC, "tga inspect failed");
        return 0;
    }
    if (!imgcc0_check_dimensions(ctx, opt, inspect.info.width, inspect.info.height)) return 0;
    if (!imgcc0_frames_take(ctx, 1U)) return 0;
    frame = &ctx->img->frames[0];
    if (!imgcc0_frame_pixels_take(ctx, frame, inspect.info.width, inspect.info.height, 0U)) return 0;
    memset(&params, 0, sizeof(params));
    params.pixels = frame->pixels;
    params.stride_bytes = (unsigned)frame->stride;
    params.pixel_format = TGA_PIXFMT_RGBA32;
    params.output_top_origin = 1;
    rc = tga_decode_memory(data, size, &params, &inspect.info);
    if (rc != TGA_OK) {
        imgcc0_set_error(ctx->img, IMGCC0_ERR_CODEC, "tga decode failed");
        return 0;
    }
    ctx->img->ok = 1;
    ctx->img->format = IMGCC0_FMT_TGA;
    ctx->img->width = frame->width;
    ctx->img->height = frame->height;
    return 1;
}

static int imgcc0_decode_qoi(const void *data,
                             imgcc0_u32 size,
                             const imgcc0_open_options *opt,
                             imgcc0_decode_ctx *ctx)
{
    qoi89_desc desc;
    qoi89_status qs;
    imgcc0_frame *frame;
    size_t pixels_written;
    memset(&desc, 0, sizeof(desc));
    qs = qoi89_decode_header((const unsigned char *)data, size, &desc);
    if (qs != QOI89_OK) {
        imgcc0_set_error(ctx->img, IMGCC0_ERR_CODEC, qoi89_status_string(qs));
        return 0;
    }
    if (!imgcc0_check_dimensions(ctx, opt, desc.width, desc.height)) return 0;
    if (!imgcc0_frames_take(ctx, 1U)) return 0;
    frame = &ctx->img->frames[0];
    if (!imgcc0_frame_pixels_take(ctx, frame, desc.width, desc.height, 0U)) return 0;
    pixels_written = 0U;
    qs = qoi89_decode((const unsigned char *)data,
                      size,
                      4U,
                      frame->pixels,
                      frame->stride * frame->height,
                      &desc,
                      &pixels_written);
    if (qs != QOI89_OK) {
        imgcc0_set_error(ctx->img, IMGCC0_ERR_CODEC, qoi89_status_string(qs));
        return 0;
    }
    ctx->img->ok = 1;
    ctx->img->format = IMGCC0_FMT_QOI;
    ctx->img->width = frame->width;
    ctx->img->height = frame->height;
    return 1;
}

static int imgcc0_gif_pass(const imgcc0_u8 *data,
                           imgcc0_u32 size,
                           const imgcc0_open_options *opt,
                           imgcc0_decode_ctx *ctx,
                           int second_pass,
                           imgcc0_u32 *out_width,
                           imgcc0_u32 *out_height,
                           imgcc0_u32 *out_frames,
                           imgcc0_s32 *out_loop)
{
    giff_decoder dec;
    giff_decoder_config cfg;
    giff_event ev;
    giff_result gr;
    giff_u32 ws_size;
    void *ws;
    imgcc0_u32 frames_seen;
    imgcc0_u32 cur_index;
    imgcc0_u32 gif_w;
    imgcc0_u32 gif_h;
    imgcc0_u32 limit_w;
    imgcc0_u32 limit_h;
    memset(&cfg, 0, sizeof(cfg));
    if (size < 10U) {
        imgcc0_set_error(ctx->img, IMGCC0_ERR_CORRUPT, "gif header truncated");
        return 0;
    }
    gif_w = (imgcc0_u32)data[6] | ((imgcc0_u32)data[7] << 8);
    gif_h = (imgcc0_u32)data[8] | ((imgcc0_u32)data[9] << 8);
    limit_w = (opt != 0 && opt->max_width != 0U) ? opt->max_width : IMGCC0_DEFAULT_MAX_DIM;
    limit_h = (opt != 0 && opt->max_height != 0U) ? opt->max_height : IMGCC0_DEFAULT_MAX_DIM;
    if (gif_w == 0U || gif_h == 0U || gif_w > limit_w || gif_h > limit_h) {
        imgcc0_set_error(ctx->img, IMGCC0_ERR_LIMIT, "gif dimension limit exceeded");
        return 0;
    }
    cfg.max_width = (giff_u16)gif_w;
    cfg.max_height = (giff_u16)gif_h;
    cfg.strict_mode = (giff_u8)((opt == 0 || opt->strict) ? 1U : 0U);
    cfg.output_mode = (giff_u8)GIFF_OUTPUT_RGBA8888;
    cfg.restore_previous_mode = (giff_u8)GIFF_RESTORE_PREVIOUS_BOUNDS;
    cfg.capture_comments = 0U;
    ws_size = giff_decoder_workspace_size(&cfg);
    imgcc0_arena_rewind(&ctx->temp);
    ws = imgcc0_temp_take(ctx, (imgcc0_u32)ws_size, "temp buffer too small for gif workspace");
    if (ws == 0) return 0;
    gr = giff_decoder_init(&dec, &cfg, ws, ws_size);
    if (gr != GIFF_OK) {
        imgcc0_set_error(ctx->img, IMGCC0_ERR_CODEC, giff_result_string(gr));
        return 0;
    }
    gr = giff_decoder_begin_memory(&dec, (const giff_u8 *)data, (giff_u32)size);
    if (gr != GIFF_OK) {
        imgcc0_set_error(ctx->img, IMGCC0_ERR_CODEC, giff_result_string(gr));
        return 0;
    }
    frames_seen = 0U;
    cur_index = 0U;
    while ((gr = giff_decoder_next_event(&dec, &ev)) == GIFF_OK) {
        if (ev.type == GIFF_EVENT_INFO) {
            if (out_width != 0) *out_width = ev.info.width;
            if (out_height != 0) *out_height = ev.info.height;
            if (out_loop != 0) *out_loop = (imgcc0_s32)ev.info.loop_count;
        } else if (ev.type == GIFF_EVENT_FRAME_BEGIN) {
            if (!second_pass) {
                ++frames_seen;
            } else {
                if (cur_index >= ctx->img->frame_count) {
                    imgcc0_set_error(ctx->img, IMGCC0_ERR_CORRUPT, "gif frame count changed between passes");
                    return 0;
                }
                if (!imgcc0_frame_pixels_take(ctx,
                                              &ctx->img->frames[cur_index],
                                              *out_width,
                                              *out_height,
                                              (imgcc0_u32)ev.frame.delay_cs * 10U)) return 0;
            }
        } else if (second_pass && ev.type == GIFF_EVENT_FRAME_ROW) {
            if (ev.rgba_row != 0 && cur_index < ctx->img->frame_count) {
                memcpy(ctx->img->frames[cur_index].pixels +
                       ctx->img->frames[cur_index].stride * (imgcc0_u32)ev.row_index,
                       ev.rgba_row,
                       ctx->img->frames[cur_index].width * 4U);
            }
        } else if (second_pass && ev.type == GIFF_EVENT_FRAME_END) {
            ++cur_index;
        }
    }
    if (gr != GIFF_E_DONE && gr != GIFF_E_TRUNCATED) {
        imgcc0_set_error(ctx->img, IMGCC0_ERR_CODEC, giff_result_string(gr));
        return 0;
    }
    if (!second_pass && out_frames != 0) *out_frames = frames_seen;
    return 1;
}

static int imgcc0_decode_gif(const void *data,
                             imgcc0_u32 size,
                             const imgcc0_open_options *opt,
                             imgcc0_decode_ctx *ctx)
{
    imgcc0_u32 w;
    imgcc0_u32 h;
    imgcc0_u32 frames;
    imgcc0_s32 loop_count;
    w = 0U;
    h = 0U;
    frames = 0U;
    loop_count = 0;
    if (!imgcc0_gif_pass((const imgcc0_u8 *)data, size, opt, ctx, 0, &w, &h, &frames, &loop_count)) return 0;
    if (frames == 0U) {
        imgcc0_set_error(ctx->img, IMGCC0_ERR_CORRUPT, "gif has no frames");
        return 0;
    }
    if (!imgcc0_check_dimensions(ctx, opt, w, h)) return 0;
    if (!imgcc0_frames_take(ctx, frames)) return 0;
    if (!imgcc0_gif_pass((const imgcc0_u8 *)data, size, opt, ctx, 1, &w, &h, &frames, &loop_count)) return 0;
    ctx->img->ok = 1;
    ctx->img->format = IMGCC0_FMT_GIF;
    ctx->img->is_animated = frames > 1U;
    ctx->img->width = w;
    ctx->img->height = h;
    ctx->img->loop_count = loop_count;
    return 1;
}

static int imgcc0_webp_scratch_bytes(imgcc0_u32 width,
                                     imgcc0_u32 height,
                                     imgcc0_u32 *out)
{
    imgcc0_u32 pixels;
    imgcc0_u32 bytes;
    if (!imgcc0_u32_mul(width, height, &pixels)) return 0;
    if (!imgcc0_u32_mul(pixels, 8U, &bytes)) return 0;
    return imgcc0_u32_add(bytes, IMGCC0_WEBP_EXTRA_SCRATCH, out);
}

static int imgcc0_decode_webp(const void *data,
                              imgcc0_u32 size,
                              const imgcc0_open_options *opt,
                              imgcc0_decode_ctx *ctx,
                              int animated)
{
    GWPBitstreamFeatures feat;
    GWPStatusCode st;
    imgcc0_u32 scratch_bytes;
    void *scratch;
    memset(&feat, 0, sizeof(feat));
    st = GWPGetFeatures((const GWPu8 *)data, (GWPu32)size, &feat);
    if (st != GWP_STATUS_OK) {
        imgcc0_set_error(ctx->img, IMGCC0_ERR_CODEC, GWPStatusToString(st));
        return 0;
    }
    if (!imgcc0_check_dimensions(ctx, opt, feat.width, feat.height)) return 0;
    if (!imgcc0_webp_scratch_bytes(feat.width, feat.height, &scratch_bytes)) {
        imgcc0_set_error(ctx->img, IMGCC0_ERR_LIMIT, "webp scratch size overflow");
        return 0;
    }
    imgcc0_arena_rewind(&ctx->temp);
    scratch = imgcc0_temp_take(ctx, scratch_bytes, "temp buffer too small for webp scratch");
    if (scratch == 0) return 0;
    if (animated || feat.has_animation) {
        GWPAnimDecoder dec;
        GWPAnimDecoderOptions aopt;
        GWPAnimInfo info;
        imgcc0_u32 i;
        GWPu32 last_ts;
        GWPu32 ts;
        GWPAnimDecoderOptionsInit(&aopt);
        aopt.pixel_format = GWP_PIXFMT_RGBA;
        aopt.strict = GWP_TRUE;
        aopt.max_width = (GWPu32)((opt != 0 && opt->max_width != 0U) ? opt->max_width : IMGCC0_DEFAULT_MAX_DIM);
        aopt.max_height = (GWPu32)((opt != 0 && opt->max_height != 0U) ? opt->max_height : IMGCC0_DEFAULT_MAX_DIM);
        aopt.scratch = scratch;
        aopt.scratch_size = (GWPu32)scratch_bytes;
        st = GWPAnimDecoderInit(&dec, (const GWPu8 *)data, (GWPu32)size, &aopt);
        if (st != GWP_STATUS_OK) {
            imgcc0_set_error(ctx->img, IMGCC0_ERR_CODEC, GWPStatusToString(st));
            return 0;
        }
        st = GWPAnimDecoderGetInfo(&dec, &info);
        if (st != GWP_STATUS_OK) {
            imgcc0_set_error(ctx->img, IMGCC0_ERR_CODEC, GWPStatusToString(st));
            return 0;
        }
        if (info.frame_count == 0U) {
            imgcc0_set_error(ctx->img, IMGCC0_ERR_CORRUPT, "webp animation has no frames");
            return 0;
        }
        if (!imgcc0_frames_take(ctx, info.frame_count)) return 0;
        last_ts = 0U;
        for (i = 0U; i < ctx->img->frame_count; ++i) {
            if (!imgcc0_frame_pixels_take(ctx, &ctx->img->frames[i], info.canvas_width, info.canvas_height, 0U)) return 0;
            ts = 0U;
            st = GWPAnimDecoderGetNext(&dec,
                                       ctx->img->frames[i].pixels,
                                       (GWPu32)(ctx->img->frames[i].stride * ctx->img->frames[i].height),
                                       (GWPu32)ctx->img->frames[i].stride,
                                       &ts);
            if (st != GWP_STATUS_OK) {
                imgcc0_set_error(ctx->img, IMGCC0_ERR_CODEC, GWPStatusToString(st));
                return 0;
            }
            if (i > 0U) ctx->img->frames[i - 1U].delay_ms = (imgcc0_u32)(ts - last_ts);
            last_ts = ts;
        }
        if (ctx->img->frame_count > 0U) {
            if (ctx->img->frame_count > 1U) {
                ctx->img->frames[ctx->img->frame_count - 1U].delay_ms = ctx->img->frames[ctx->img->frame_count - 2U].delay_ms;
            } else {
                ctx->img->frames[0].delay_ms = 100U;
            }
        }
        ctx->img->ok = 1;
        ctx->img->format = IMGCC0_FMT_WEBP_ANIM;
        ctx->img->is_animated = ctx->img->frame_count > 1U;
        ctx->img->width = info.canvas_width;
        ctx->img->height = info.canvas_height;
        ctx->img->loop_count = (imgcc0_s32)info.loop_count;
        return 1;
    } else {
        GWPDecoderOptions wopt;
        imgcc0_frame *frame;
        if (!imgcc0_frames_take(ctx, 1U)) return 0;
        frame = &ctx->img->frames[0];
        if (!imgcc0_frame_pixels_take(ctx, frame, feat.width, feat.height, 0U)) return 0;
        memset(&wopt, 0, sizeof(wopt));
        wopt.pixel_format = GWP_PIXFMT_RGBA;
        wopt.strict = GWP_TRUE;
        wopt.max_width = (GWPu32)((opt != 0 && opt->max_width != 0U) ? opt->max_width : IMGCC0_DEFAULT_MAX_DIM);
        wopt.max_height = (GWPu32)((opt != 0 && opt->max_height != 0U) ? opt->max_height : IMGCC0_DEFAULT_MAX_DIM);
        wopt.output_buffer = frame->pixels;
        wopt.output_buffer_size = (GWPu32)(frame->stride * frame->height);
        wopt.output_stride = (GWPu32)frame->stride;
        wopt.scratch = scratch;
        wopt.scratch_size = (GWPu32)scratch_bytes;
        st = GWPDecode((const GWPu8 *)data, (GWPu32)size, &wopt, &feat);
        if (st != GWP_STATUS_OK) {
            imgcc0_set_error(ctx->img, IMGCC0_ERR_CODEC, GWPStatusToString(st));
            return 0;
        }
        ctx->img->ok = 1;
        ctx->img->format = IMGCC0_FMT_WEBP;
        ctx->img->width = frame->width;
        ctx->img->height = frame->height;
        return 1;
    }
}


static imgcc0_u32 imgcc0_png_delay_ms(png_u16 num, png_u16 den)
{
    imgcc0_u32 d;
    imgcc0_u32 n;
    d = (den == 0U) ? 100U : (imgcc0_u32)den;
    n = (imgcc0_u32)num * 1000U;
    return (n + (d / 2U)) / d;
}

static int imgcc0_decode_png(const void *data,
                             imgcc0_u32 size,
                             const imgcc0_open_options *opt,
                             imgcc0_decode_ctx *ctx,
                             int animated)
{
    png_decode_options popt;
    int pr;
    png_decode_options_init(&popt);
    popt.output_format = PNG_OUTPUT_RGBA8;
    popt.keep_text = 0;
    popt.keep_unknown_chunks = 0;
    popt.strict_trailing_data = (opt != 0 && opt->strict) ? 1 : 0;
    popt.max_width = (opt != 0 && opt->max_width != 0U) ? opt->max_width : IMGCC0_DEFAULT_MAX_DIM;
    popt.max_height = (opt != 0 && opt->max_height != 0U) ? opt->max_height : IMGCC0_DEFAULT_MAX_DIM;
    popt.max_file_bytes = size;

    png_mem89_reset();
    if (!animated) {
        png_image pi;
        imgcc0_frame *frame;
        memset(&pi, 0, sizeof(pi));
        pr = png_decode_memory_ex((const png_u8 *)data, (png_u32)size, imgcc0_zragf_png_decompress, &popt, &pi);
        if (pr != PNG_DEC_OK) {
            png_mem89_reset();
            imgcc0_set_error(ctx->img, IMGCC0_ERR_CODEC, png_strerror(pr));
            return 0;
        }
        if (!imgcc0_check_dimensions(ctx, opt, (imgcc0_u32)pi.width, (imgcc0_u32)pi.height)) {
            png_free_image(&pi);
            png_mem89_reset();
            return 0;
        }
        if (!imgcc0_frames_take(ctx, 1U)) {
            png_free_image(&pi);
            png_mem89_reset();
            return 0;
        }
        frame = &ctx->img->frames[0];
        if (!imgcc0_frame_pixels_take(ctx, frame, (imgcc0_u32)pi.width, (imgcc0_u32)pi.height, 0U)) {
            png_free_image(&pi);
            png_mem89_reset();
            return 0;
        }
        if (pi.pixels == 0 || pi.output_format != PNG_OUTPUT_RGBA8 ||
            pi.pixel_rowbytes < frame->stride) {
            png_free_image(&pi);
            png_mem89_reset();
            imgcc0_set_error(ctx->img, IMGCC0_ERR_CODEC, "png decoder did not return RGBA8");
            return 0;
        }
        {
            imgcc0_u32 y;
            for (y = 0U; y < frame->height; ++y) {
                memcpy(frame->pixels + y * frame->stride,
                       pi.pixels + y * pi.pixel_rowbytes,
                       frame->stride);
            }
        }
        png_free_image(&pi);
        png_mem89_reset();
        ctx->img->ok = 1;
        ctx->img->format = IMGCC0_FMT_PNG;
        ctx->img->width = frame->width;
        ctx->img->height = frame->height;
        return 1;
    } else {
        png_apng pa;
        imgcc0_u32 i;
        memset(&pa, 0, sizeof(pa));
        pr = png_decode_apng_memory_ex((const png_u8 *)data, (png_u32)size, imgcc0_zragf_png_decompress, &popt, &pa);
        if (pr != PNG_DEC_OK) {
            png_mem89_reset();
            imgcc0_set_error(ctx->img, IMGCC0_ERR_CODEC, png_strerror(pr));
            return 0;
        }
        if (pa.frame_count == 0U || pa.frames == 0) {
            png_free_apng(&pa);
            png_mem89_reset();
            imgcc0_set_error(ctx->img, IMGCC0_ERR_CODEC, "apng contains no frames");
            return 0;
        }
        if (!imgcc0_check_dimensions(ctx, opt, (imgcc0_u32)pa.width, (imgcc0_u32)pa.height)) {
            png_free_apng(&pa);
            png_mem89_reset();
            return 0;
        }
        if (!imgcc0_frames_take(ctx, (imgcc0_u32)pa.frame_count)) {
            png_free_apng(&pa);
            png_mem89_reset();
            return 0;
        }
        for (i = 0U; i < (imgcc0_u32)pa.frame_count; ++i) {
            imgcc0_frame *frame;
            png_apng_frame *pf;
            imgcc0_u32 y;
            pf = &pa.frames[i];
            frame = &ctx->img->frames[i];
            if (!imgcc0_frame_pixels_take(ctx, frame,
                                          (imgcc0_u32)pa.width,
                                          (imgcc0_u32)pa.height,
                                          imgcc0_png_delay_ms(pf->control.delay_num, pf->control.delay_den))) {
                png_free_apng(&pa);
                png_mem89_reset();
                return 0;
            }
            if (pf->pixels == 0 || pf->output_format != PNG_OUTPUT_RGBA8 ||
                pf->pixel_rowbytes < frame->stride) {
                png_free_apng(&pa);
                png_mem89_reset();
                imgcc0_set_error(ctx->img, IMGCC0_ERR_CODEC, "apng frame is not RGBA8 canvas");
                return 0;
            }
            for (y = 0U; y < frame->height; ++y) {
                memcpy(frame->pixels + y * frame->stride,
                       pf->pixels + y * pf->pixel_rowbytes,
                       frame->stride);
            }
        }
        ctx->img->ok = 1;
        ctx->img->format = IMGCC0_FMT_APNG;
        ctx->img->is_animated = pa.frame_count > 1U;
        ctx->img->width = (imgcc0_u32)pa.width;
        ctx->img->height = (imgcc0_u32)pa.height;
        ctx->img->loop_count = (imgcc0_s32)pa.num_plays;
        png_free_apng(&pa);
        png_mem89_reset();
        return 1;
    }
}

static int imgcc0_decode_bmp(const void *data,
                             imgcc0_u32 size,
                             const imgcc0_open_options *opt,
                             imgcc0_decode_ctx *ctx)
{
    bmp_image bi;
    bmp_limits limits;
    imgcc0_frame *frame;
    int br;
    memset(&bi, 0, sizeof(bi));
    bmp_limits_default(&limits);
    limits.max_input_bytes = size;
    limits.max_width = (opt != 0 && opt->max_width != 0U) ? opt->max_width : IMGCC0_DEFAULT_MAX_DIM;
    limits.max_height = (opt != 0 && opt->max_height != 0U) ? opt->max_height : IMGCC0_DEFAULT_MAX_DIM;
    br = bmp_parse_memory_with_limits((const bmp_u8 *)data, (bmp_u32)size, &limits, &bi);
    if (br != BMP_OK) {
        imgcc0_set_error(ctx->img, IMGCC0_ERR_CODEC, bmp_error_string(br));
        return 0;
    }
    if (!imgcc0_check_dimensions(ctx, opt, (imgcc0_u32)bi.meta.width, (imgcc0_u32)bi.meta.height)) return 0;
    if (!imgcc0_frames_take(ctx, 1U)) return 0;
    frame = &ctx->img->frames[0];
    if (!imgcc0_frame_pixels_take(ctx, frame,
                                  (imgcc0_u32)bi.meta.width,
                                  (imgcc0_u32)bi.meta.height,
                                  0U)) return 0;
    br = bmp_decode_to_rgba32_with_limits(&bi, &limits, frame->pixels, frame->stride);
    if (br != BMP_OK) {
        imgcc0_set_error(ctx->img, IMGCC0_ERR_CODEC, bmp_error_string(br));
        return 0;
    }
    ctx->img->ok = 1;
    ctx->img->format = IMGCC0_FMT_BMP;
    ctx->img->width = frame->width;
    ctx->img->height = frame->height;
    return 1;
}

static int imgcc0_decode_pcx(const void *data,
                             imgcc0_u32 size,
                             const imgcc0_open_options *opt,
                             imgcc0_decode_ctx *ctx)
{
    PCXImage pi;
    PCXFileInfo info;
    PCXDecodeLimits limits;
    imgcc0_frame *frame;
    unsigned flags;
    int pr;
    pcx_image_init(&pi);
    memset(&info, 0, sizeof(info));
    pcx_decode_limits_default(&limits);
    limits.maxInputBytes = (pcx_size)size;
    limits.maxWidth = (int)((opt != 0 && opt->max_width != 0U) ? opt->max_width : IMGCC0_DEFAULT_MAX_DIM);
    limits.maxHeight = (int)((opt != 0 && opt->max_height != 0U) ? opt->max_height : IMGCC0_DEFAULT_MAX_DIM);
    flags = (opt != 0 && opt->strict) ? PCX_PARSE_FLAG_STRICT : PCX_PARSE_FLAG_NONE;
    pr = pcx_load_memory_ex(data, (pcx_size)size, &pi, &info, flags, &limits);
    if (pr != PCX_OK) {
        imgcc0_set_error(ctx->img, IMGCC0_ERR_CODEC, pcx_result_to_string((PCXResult)pr));
        return 0;
    }
    if (pi.width <= 0 || pi.height <= 0 || pi.channels != 3 || pi.pixels == 0) {
        pcx_image_release(&pi);
        imgcc0_set_error(ctx->img, IMGCC0_ERR_CODEC, "pcx decoder returned invalid RGB image");
        return 0;
    }
    if (!imgcc0_check_dimensions(ctx, opt, (imgcc0_u32)pi.width, (imgcc0_u32)pi.height)) {
        pcx_image_release(&pi);
        return 0;
    }
    if (!imgcc0_frames_take(ctx, 1U)) {
        pcx_image_release(&pi);
        return 0;
    }
    frame = &ctx->img->frames[0];
    if (!imgcc0_frame_pixels_take(ctx, frame, (imgcc0_u32)pi.width, (imgcc0_u32)pi.height, 0U)) {
        pcx_image_release(&pi);
        return 0;
    }
    imgcc0_copy_rgb_to_rgba(frame, (const imgcc0_u8 *)pi.pixels, (imgcc0_u32)pi.width * 3U, 0);
    pcx_image_release(&pi);
    ctx->img->ok = 1;
    ctx->img->format = IMGCC0_FMT_PCX;
    ctx->img->width = frame->width;
    ctx->img->height = frame->height;
    return 1;
}

static int imgcc0_decode_dds(const void *data,
                             imgcc0_u32 size,
                             const imgcc0_open_options *opt,
                             imgcc0_decode_ctx *ctx)
{
    gdds_info info;
    gdds_image gi;
    gdds_parse_options gopt;
    gdds_result gr;
    imgcc0_frame *frame;
    memset(&info, 0, sizeof(info));
    memset(&gi, 0, sizeof(gi));
    gopt = gdds_parse_options_default();
    gopt.mode = (opt != 0 && opt->strict) ? GDDS_PARSE_MODE_STRICT : GDDS_PARSE_MODE_PERMISSIVE;
    gr = gdds_inspect_memory_ex(data, (gdds_size)size, &gopt, &info);
    if (gr != GDDS_RESULT_OK) {
        imgcc0_set_error(ctx->img, IMGCC0_ERR_CODEC, gdds_result_string(gr));
        return 0;
    }
    if (!imgcc0_check_dimensions(ctx, opt, (imgcc0_u32)info.width, (imgcc0_u32)info.height)) return 0;
    if (!imgcc0_frames_take(ctx, 1U)) return 0;
    frame = &ctx->img->frames[0];
    if (!imgcc0_frame_pixels_take(ctx, frame, (imgcc0_u32)info.width, (imgcc0_u32)info.height, 0U)) return 0;
    gr = gdds_decode_memory_ex(data, (gdds_size)size, &gopt, &gi);
    if (gr != GDDS_RESULT_OK) {
        imgcc0_set_error(ctx->img, IMGCC0_ERR_CODEC, gdds_result_string(gr));
        return 0;
    }
    if (gi.pixels == 0 || gi.pixel_format != GDDS_FORMAT_RGBA8 || gi.size < frame->stride * frame->height) {
        gdds_image_release(&gi);
        imgcc0_set_error(ctx->img, IMGCC0_ERR_CODEC, "dds decoder did not return complete RGBA8");
        return 0;
    }
    memcpy(frame->pixels, gi.pixels, frame->stride * frame->height);
    gdds_image_release(&gi);
    ctx->img->ok = 1;
    ctx->img->format = IMGCC0_FMT_DDS;
    ctx->img->width = frame->width;
    ctx->img->height = frame->height;
    return 1;
}

static int imgcc0_decode_tiff(const void *data,
                              imgcc0_u32 size,
                              const imgcc0_open_options *opt,
                              imgcc0_decode_ctx *ctx)
{
    tifx_image_info info;
    imgcc0_u32 src_stride;
    imgcc0_u32 src_size;
    imgcc0_u32 ws_size;
    unsigned long tifx_src_stride;
    unsigned long tifx_src_size;
    unsigned long tifx_ws_size;
    void *src_buf;
    void *workspace;
    imgcc0_frame *frame;
    int tr;
    imgcc0_u32 y;
    tifx_image_info_init(&info);
    tr = tifx_parse_memory(&info, data, size);
    if (tr != TIFX_OK) {
        imgcc0_set_error(ctx->img, IMGCC0_ERR_CODEC, tifx_strerror(tr));
        return 0;
    }
    if (!imgcc0_check_dimensions(ctx, opt, (imgcc0_u32)info.width, (imgcc0_u32)info.height)) return 0;
    tifx_src_stride = 0UL;
    tifx_src_size = tifx_decode_buffer_size(&info, &tifx_src_stride);
    tifx_ws_size = tifx_decode_workspace_size(&info);
    if (tifx_src_stride > 0xFFFFFFFFUL || tifx_src_size > 0xFFFFFFFFUL || tifx_ws_size > 0xFFFFFFFFUL) {
        imgcc0_set_error(ctx->img, IMGCC0_ERR_LIMIT, "tiff workspace exceeds 32-bit protocol");
        return 0;
    }
    src_stride = (imgcc0_u32)tifx_src_stride;
    src_size = (imgcc0_u32)tifx_src_size;
    ws_size = (imgcc0_u32)tifx_ws_size;
    imgcc0_arena_rewind(&ctx->temp);
    src_buf = imgcc0_temp_take(ctx, src_size, "temp buffer too small for tiff pixels");
    if (src_buf == 0) return 0;
    workspace = imgcc0_temp_take(ctx, ws_size, "temp buffer too small for tiff workspace");
    if (workspace == 0) return 0;
    tr = tifx_decode_memory(&info, data, size, src_buf, src_size, src_stride, workspace, ws_size);
    if (tr != TIFX_OK) {
        imgcc0_set_error(ctx->img, IMGCC0_ERR_CODEC, tifx_strerror(tr));
        return 0;
    }
    if (!imgcc0_frames_take(ctx, 1U)) return 0;
    frame = &ctx->img->frames[0];
    if (!imgcc0_frame_pixels_take(ctx, frame, (imgcc0_u32)info.width, (imgcc0_u32)info.height, 0U)) return 0;
    if (info.pixel_format == TIFX_PIXEL_RGBA32) {
        memcpy(frame->pixels, src_buf, frame->stride * frame->height);
    } else if (info.pixel_format == TIFX_PIXEL_RGB24) {
        imgcc0_copy_rgb_to_rgba(frame, (const imgcc0_u8 *)src_buf, src_stride, 0);
    } else if (info.pixel_format == TIFX_PIXEL_GRAY8) {
        imgcc0_copy_rgb_to_rgba(frame, (const imgcc0_u8 *)src_buf, src_stride, 1);
    } else {
        const imgcc0_u8 *in;
        in = (const imgcc0_u8 *)src_buf;
        for (y = 0U; y < frame->height; ++y) {
            imgcc0_u32 x;
            imgcc0_u8 *out_row;
            const imgcc0_u8 *row;
            out_row = frame->pixels + y * frame->stride;
            row = in + y * src_stride;
            for (x = 0U; x < frame->width; ++x) {
                imgcc0_u8 bit;
                imgcc0_u8 v;
                bit = (imgcc0_u8)((row[x >> 3] >> (7U - (x & 7U))) & 1U);
                v = bit ? 255U : 0U;
                out_row[x * 4U + 0U] = v;
                out_row[x * 4U + 1U] = v;
                out_row[x * 4U + 2U] = v;
                out_row[x * 4U + 3U] = 255U;
            }
        }
    }
    ctx->img->ok = 1;
    ctx->img->format = IMGCC0_FMT_TIFF;
    ctx->img->width = frame->width;
    ctx->img->height = frame->height;
    return 1;
}

static int imgcc0_decode_psd(const void *data,
                             imgcc0_u32 size,
                             const imgcc0_open_options *opt,
                             imgcc0_decode_ctx *ctx)
{
    psd89_doc *doc;
    psd89_memio mem;
    psd89_io io;
    psd89_u8 *planes[56];
    psd89_u8 *base0;
    psd89_u8 *base1;
    psd89_u8 *base2;
    psd89_u8 *alpha;
    psd89_u8 *discard;
    imgcc0_u32 plane_bytes;
    imgcc0_u32 base_channels;
    imgcc0_u32 alpha_index;
    imgcc0_u32 c;
    imgcc0_u32 x;
    imgcc0_u32 y;
    int rc;
    imgcc0_frame *frame;

    imgcc0_arena_rewind(&ctx->temp);
    doc = (psd89_doc *)imgcc0_temp_take(ctx, (imgcc0_u32)sizeof(psd89_doc),
                                        "temp buffer too small for PSD document");
    if (doc == 0) return 0;

    psd89_memio_init_read(&mem, data, (psd89_u32)size);
    psd89_memio_make_io(&mem, &io);
    rc = psd89_read(doc, &io);
    if (rc != PSD89_OK) {
        imgcc0_set_error(ctx->img, IMGCC0_ERR_CODEC, psd89_error_string(rc));
        return 0;
    }
    if (!imgcc0_check_dimensions(ctx, opt, (imgcc0_u32)doc->width, (imgcc0_u32)doc->height)) return 0;
    if (!imgcc0_u32_mul((imgcc0_u32)doc->width, (imgcc0_u32)doc->height, &plane_bytes)) {
        imgcc0_set_error(ctx->img, IMGCC0_ERR_LIMIT, "PSD plane size overflow");
        return 0;
    }

    base_channels = doc->color_mode == PSD89_MODE_GRAYSCALE ? 1U : 3U;
    if ((imgcc0_u32)doc->channels < base_channels || (imgcc0_u32)doc->channels > 56U) {
        imgcc0_set_error(ctx->img, IMGCC0_ERR_CODEC, "PSD channel count unsupported");
        return 0;
    }

    base0 = (psd89_u8 *)imgcc0_temp_take(ctx, plane_bytes, "temp buffer too small for PSD channel 0");
    if (base0 == 0) return 0;
    base1 = 0;
    base2 = 0;
    if (base_channels >= 2U) {
        base1 = (psd89_u8 *)imgcc0_temp_take(ctx, plane_bytes, "temp buffer too small for PSD channel 1");
        if (base1 == 0) return 0;
    }
    if (base_channels >= 3U) {
        base2 = (psd89_u8 *)imgcc0_temp_take(ctx, plane_bytes, "temp buffer too small for PSD channel 2");
        if (base2 == 0) return 0;
    }

    alpha = 0;
    alpha_index = 0xFFFFFFFFU;
    if ((imgcc0_u32)doc->channels > base_channels &&
        (doc->merged_alpha_in_first_channel || (opt != 0 && !opt->strict))) {
        alpha_index = base_channels;
        alpha = (psd89_u8 *)imgcc0_temp_take(ctx, plane_bytes, "temp buffer too small for PSD alpha");
        if (alpha == 0) return 0;
    }

    discard = 0;
    for (c = 0U; c < 56U; ++c) planes[c] = 0;
    planes[0] = base0;
    if (base_channels >= 2U) planes[1] = base1;
    if (base_channels >= 3U) planes[2] = base2;
    if (alpha != 0) planes[alpha_index] = alpha;
    for (c = base_channels; c < (imgcc0_u32)doc->channels; ++c) {
        if (c == alpha_index) continue;
        if (discard == 0) {
            discard = (psd89_u8 *)imgcc0_temp_take(ctx, plane_bytes, "temp buffer too small for PSD extra channel");
            if (discard == 0) return 0;
        }
        planes[c] = discard;
    }

    psd89_memio_init_read(&mem, data, (psd89_u32)size);
    psd89_memio_make_io(&mem, &io);
    rc = psd89_decode_composite_u8(doc, &io, planes, (psd89_u32)doc->width);
    if (rc != PSD89_OK) {
        imgcc0_set_error(ctx->img, IMGCC0_ERR_CODEC, psd89_error_string(rc));
        return 0;
    }

    if (!imgcc0_frames_take(ctx, 1U)) return 0;
    frame = &ctx->img->frames[0];
    if (!imgcc0_frame_pixels_take(ctx, frame, (imgcc0_u32)doc->width, (imgcc0_u32)doc->height, 0U)) return 0;

    for (y = 0U; y < frame->height; ++y) {
        imgcc0_u8 *dst;
        imgcc0_u32 row;
        row = y * (imgcc0_u32)doc->width;
        dst = frame->pixels + y * frame->stride;
        for (x = 0U; x < frame->width; ++x) {
            imgcc0_u8 a;
            a = alpha != 0 ? alpha[row + x] : 255U;
            if (base_channels == 1U) {
                imgcc0_u8 v;
                v = base0[row + x];
                dst[x * 4U + 0U] = v;
                dst[x * 4U + 1U] = v;
                dst[x * 4U + 2U] = v;
                dst[x * 4U + 3U] = a;
            } else {
                dst[x * 4U + 0U] = base0[row + x];
                dst[x * 4U + 1U] = base1[row + x];
                dst[x * 4U + 2U] = base2[row + x];
                dst[x * 4U + 3U] = a;
            }
        }
    }

    ctx->img->ok = 1;
    ctx->img->format = IMGCC0_FMT_PSD;
    ctx->img->width = frame->width;
    ctx->img->height = frame->height;
    return 1;
}

static int imgcc0_quarantined(imgcc0_image *img, imgcc0_format fmt)
{
    img->format = fmt;
    imgcc0_set_error(img, IMGCC0_ERR_PROTOCOL, "codec detected but adapter is quarantined until codec ownership is sanitized");
    return 0;
}

int imgcc0_open_memory(const void *data,
                       imgcc0_u32 size,
                       const imgcc0_open_options *opt,
                       imgcc0_image *out_img)
{
    imgcc0_open_options local_opt;
    imgcc0_decode_ctx ctx;
    imgcc0_format fmt;
    int ok;
    if (out_img == 0 || data == 0 || size == 0U) return IMGCC0_ERR_ARGUMENT;
    imgcc0_image_init(out_img);
    if (opt == 0) {
        imgcc0_open_options_init(&local_opt);
        opt = &local_opt;
    }
    if (opt->output_buffer == 0 || opt->output_buffer_size == 0U) {
        imgcc0_set_error(out_img, IMGCC0_ERR_STORAGE, "output buffer is required");
        return out_img->error_code;
    }
    imgcc0_arena_init(&ctx.output, opt->output_buffer, opt->output_buffer_size);
    imgcc0_arena_init(&ctx.temp, opt->temp_buffer, opt->temp_buffer_size);
    ctx.img = out_img;
    fmt = imgcc0_detect_format_memory(data, size);
    ok = 0;
    switch (fmt) {
        case IMGCC0_FMT_JPEG:
            ok = imgcc0_decode_jpeg(data, size, opt, &ctx);
            break;
        case IMGCC0_FMT_TGA:
            ok = imgcc0_decode_tga(data, size, opt, &ctx);
            break;
        case IMGCC0_FMT_QOI:
            ok = imgcc0_decode_qoi(data, size, opt, &ctx);
            break;
        case IMGCC0_FMT_GIF:
            ok = imgcc0_decode_gif(data, size, opt, &ctx);
            break;
        case IMGCC0_FMT_WEBP:
            ok = imgcc0_decode_webp(data, size, opt, &ctx, 0);
            break;
        case IMGCC0_FMT_WEBP_ANIM:
            ok = imgcc0_decode_webp(data, size, opt, &ctx, 1);
            break;
        case IMGCC0_FMT_PNG:
            ok = imgcc0_decode_png(data, size, opt, &ctx, 0);
            break;
        case IMGCC0_FMT_APNG:
            ok = imgcc0_decode_png(data, size, opt, &ctx, 1);
            break;
        case IMGCC0_FMT_BMP:
            ok = imgcc0_decode_bmp(data, size, opt, &ctx);
            break;
        case IMGCC0_FMT_PCX:
            ok = imgcc0_decode_pcx(data, size, opt, &ctx);
            break;
        case IMGCC0_FMT_DDS:
            ok = imgcc0_decode_dds(data, size, opt, &ctx);
            break;
        case IMGCC0_FMT_TIFF:
            ok = imgcc0_decode_tiff(data, size, opt, &ctx);
            break;
        case IMGCC0_FMT_PSD:
            ok = imgcc0_decode_psd(data, size, opt, &ctx);
            break;
        default:
            imgcc0_set_error(out_img, IMGCC0_ERR_FORMAT, "unknown format");
            break;
    }
    imgcc0_sync_usage(&ctx);
    if (ok) return IMGCC0_OK;
    if (out_img->error_code == 0) imgcc0_set_error(out_img, IMGCC0_ERR_CODEC, "decode failed");
    return out_img->error_code;
}

int imgcc0_open_file(const char *filename,
                     const imgcc0_open_options *opt,
                     imgcc0_image *out_img)
{
    FILE *fp;
    imgcc0_open_options local_opt;
    imgcc0_u8 *data;
    imgcc0_u32 cap;
    imgcc0_u32 used;
    int ch;
    int rc;
    if (filename == 0 || out_img == 0) return IMGCC0_ERR_ARGUMENT;
    if (opt == 0) {
        imgcc0_open_options_init(&local_opt);
        opt = &local_opt;
    }
    imgcc0_image_init(out_img);
    if (opt->file_buffer == 0 || opt->file_buffer_size == 0U) {
        imgcc0_set_error(out_img, IMGCC0_ERR_STORAGE, "file buffer is required for imgcc0_open_file");
        return out_img->error_code;
    }
    fp = fopen(filename, "rb");
    if (fp == 0) {
        imgcc0_set_error(out_img, IMGCC0_ERR_IO, "cannot open file");
        return out_img->error_code;
    }
    data = (imgcc0_u8 *)opt->file_buffer;
    cap = opt->file_buffer_size;
    used = (imgcc0_u32)fread(data, 1U, cap, fp);
    if (ferror(fp)) {
        fclose(fp);
        imgcc0_set_error(out_img, IMGCC0_ERR_IO, "file read failed");
        return out_img->error_code;
    }
    if (used == cap) {
        ch = fgetc(fp);
        if (ch != EOF) {
            fclose(fp);
            imgcc0_set_error(out_img, IMGCC0_ERR_STORAGE, "file buffer too small");
            return out_img->error_code;
        }
    }
    fclose(fp);
    if (used == 0U) {
        imgcc0_set_error(out_img, IMGCC0_ERR_FORMAT, "empty file");
        return out_img->error_code;
    }
    rc = imgcc0_open_memory(data, used, opt, out_img);
    return rc;
}

imgcc0_rgba8 imgcc0_frame_get_pixel(const imgcc0_frame *frame,
                                    imgcc0_u32 x,
                                    imgcc0_u32 y)
{
    imgcc0_rgba8 px;
    const imgcc0_u8 *p;
    px.r = 0U;
    px.g = 0U;
    px.b = 0U;
    px.a = 255U;
    if (frame == 0 || frame->pixels == 0 || x >= frame->width || y >= frame->height) return px;
    p = frame->pixels + y * frame->stride + x * 4U;
    px.r = p[0];
    px.g = p[1];
    px.b = p[2];
    px.a = p[3];
    return px;
}

int imgcc0_surface_blit(const imgcc0_frame *src,
                        imgcc0_surface *dst,
                        imgcc0_s32 dst_x,
                        imgcc0_s32 dst_y)
{
    imgcc0_u32 y;
    if (src == 0 || dst == 0 || src->pixels == 0 || dst->pixels == 0) return IMGCC0_ERR_ARGUMENT;
    for (y = 0U; y < src->height; ++y) {
        imgcc0_s32 ty;
        ty = dst_y + (imgcc0_s32)y;
        if (ty < 0 || (imgcc0_u32)ty >= dst->height) continue;
        {
            imgcc0_u32 x;
            for (x = 0U; x < src->width; ++x) {
                imgcc0_s32 tx;
                const imgcc0_u8 *sp;
                imgcc0_u8 *dp;
                imgcc0_u32 sa;
                imgcc0_u32 ia;
                tx = dst_x + (imgcc0_s32)x;
                if (tx < 0 || (imgcc0_u32)tx >= dst->width) continue;
                sp = src->pixels + y * src->stride + x * 4U;
                dp = dst->pixels + (imgcc0_u32)ty * dst->stride + (imgcc0_u32)tx * 4U;
                sa = sp[3];
                ia = 255U - sa;
                dp[0] = (imgcc0_u8)(((imgcc0_u32)sp[0] * sa + (imgcc0_u32)dp[0] * ia) / 255U);
                dp[1] = (imgcc0_u8)(((imgcc0_u32)sp[1] * sa + (imgcc0_u32)dp[1] * ia) / 255U);
                dp[2] = (imgcc0_u8)(((imgcc0_u32)sp[2] * sa + (imgcc0_u32)dp[2] * ia) / 255U);
                dp[3] = (imgcc0_u8)(sa + ((imgcc0_u32)dp[3] * ia) / 255U);
            }
        }
    }
    return IMGCC0_OK;
}
