#include "png89_zlib89.h"
#include <string.h>

typedef struct Png89Zlib89Ctx {
    Zlib89Scratch *scratch;
} Png89Zlib89Ctx;

static int png89_zlib89_inflate_cb(void *user,
                                   const png89_u8 *src, png89_u32 src_size,
                                   png89_u8 *dst, png89_u32 dst_size,
                                   png89_u32 *out_written)
{
    Png89Zlib89Ctx *ctx;
    int rc;
    ctx = (Png89Zlib89Ctx*)user;
    if (!ctx || !ctx->scratch) return -1;
    rc = zlib89_inflate_zlib(src, src_size,
                             dst, dst_size,
                             0u,
                             ctx->scratch,
                             out_written,
                             0);
    return (rc == ZLIB89_OK) ? 0 : rc;
}

static int png89_zlib89_make_scratch(const Png89Requirements *req,
                                     png89_u8 *work, png89_u32 work_size,
                                     Png89Scratch *out)
{
    png89_u8 *p;
    if (!req || !work || !out) return PNG89_EINVAL;
    if (work_size < req->scratch_size) return PNG89_ENOSPC;
    p = work;
    out->idat_data = p;
    out->idat_size = req->idat_size;
    p += req->idat_size;
    out->inflate_out = p;
    out->inflate_out_size = req->inflate_out_size;
    p += req->inflate_out_size;
    out->prev_row = p;
    out->prev_row_size = req->prev_row_size;
    p += req->prev_row_size;
    out->cur_row = p;
    out->cur_row_size = req->cur_row_size;
    p += req->cur_row_size;
    out->pass_row = p;
    out->pass_row_size = req->pass_row_size;
    return PNG89_OK;
}

int png89_zlib89_get_requirements(const png89_u8 *data, png89_u32 size, png89_u32 flags,
                                  Png89Info *out_info, Png89Requirements *out_req)
{
    return png89_get_requirements(data, size, flags, out_info, out_req);
}

int png89_zlib89_decode_indexed8(const png89_u8 *data, png89_u32 size,
                                 png89_u32 flags,
                                 Zlib89Scratch *inflate_scratch,
                                 png89_u8 *work, png89_u32 work_size,
                                 png89_u8 *out_pixels, png89_u32 out_pixels_size,
                                 png89_u8 out_pal_rgb[768], int *out_has_palette,
                                 Png89Info *out_info)
{
    Png89Info info;
    Png89Requirements req;
    Png89Scratch scratch;
    Png89InflateHooks hooks;
    Png89Zlib89Ctx ctx;
    int rc;

    if (!inflate_scratch || !work) return PNG89_EINVAL;
    rc = png89_get_requirements(data, size, flags, &info, &req);
    if (rc != PNG89_OK) return rc;
    rc = png89_zlib89_make_scratch(&req, work, work_size, &scratch);
    if (rc != PNG89_OK) return rc;
    ctx.scratch = inflate_scratch;
    hooks.inflate_zlib = png89_zlib89_inflate_cb;
    hooks.user = &ctx;
    rc = png89_decode_indexed8(data, size, flags, &hooks, &scratch,
                               out_pixels, out_pixels_size,
                               out_pal_rgb, out_has_palette,
                               out_info ? out_info : &info);
    return rc;
}

int png89_zlib89_decode_rgba8888(const png89_u8 *data, png89_u32 size,
                                 png89_u32 flags,
                                 Zlib89Scratch *inflate_scratch,
                                 png89_u8 *work, png89_u32 work_size,
                                 png89_u8 *out_rgba, png89_u32 out_rgba_size,
                                 Png89Info *out_info)
{
    Png89Info info;
    Png89Requirements req;
    Png89Scratch scratch;
    Png89InflateHooks hooks;
    Png89Zlib89Ctx ctx;
    int rc;

    if (!inflate_scratch || !work) return PNG89_EINVAL;
    rc = png89_get_requirements(data, size, flags, &info, &req);
    if (rc != PNG89_OK) return rc;
    rc = png89_zlib89_make_scratch(&req, work, work_size, &scratch);
    if (rc != PNG89_OK) return rc;
    ctx.scratch = inflate_scratch;
    hooks.inflate_zlib = png89_zlib89_inflate_cb;
    hooks.user = &ctx;
    rc = png89_decode_rgba8888(data, size, flags, &hooks, &scratch,
                               out_rgba, out_rgba_size,
                               out_info ? out_info : &info);
    return rc;
}
