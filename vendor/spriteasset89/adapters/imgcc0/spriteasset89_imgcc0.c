#include "spriteasset89_imgcc0.h"

static void sa89_i_copy(char *dst, unsigned int cap, const char *src)
{
    unsigned int i;
    i = 0U;
    if (!dst || cap == 0U) return;
    if (src) while (src[i] && i + 1U < cap) { dst[i] = src[i]; ++i; }
    dst[i] = '\0';
}

static int sa89_i_same(const char *a, const char *b)
{
    unsigned int i;
    if (!a || !b) return 0;
    i = 0U;
    while (a[i] && b[i] && a[i] == b[i]) ++i;
    return a[i] == b[i];
}

static int sa89_i_acquire(void *user, const char *path,
                          sa89_u32 *out_handle, sa89_u32 *out_width,
                          sa89_u32 *out_height, sa89_u32 *out_frames)
{
    SA89_Imgcc0Adapter *a;
    imgcc0_open_options opt;
    a = (SA89_Imgcc0Adapter *)user;
    if (!a || !path || !out_handle || !out_width || !out_height || !out_frames) return SA89_PROVIDER_ERROR;
    if (!a->loaded || !sa89_i_same(a->current_path, path)) {
        imgcc0_image_reset(&a->image);
        imgcc0_open_options_init(&opt);
        opt.output_buffer = a->output_buffer;
        opt.output_buffer_size = a->output_size;
        opt.temp_buffer = a->temp_buffer;
        opt.temp_buffer_size = a->temp_size;
        opt.file_buffer = a->file_buffer;
        opt.file_buffer_size = a->file_size;
        if (imgcc0_open_file(path, &opt, &a->image) != IMGCC0_OK || !a->image.ok) {
            a->loaded = 0;
            return SA89_PROVIDER_ERROR;
        }
        sa89_i_copy(a->current_path, SA89_PATH_CAP, path);
        a->loaded = 1;
    }
    *out_handle = 1U;
    *out_width = a->image.width;
    *out_height = a->image.height;
    *out_frames = a->image.frame_count;
    return SA89_PROVIDER_HANDLED;
}

static int sa89_i_get_frame(void *user, sa89_u32 handle,
                            sa89_u32 frame_index, SA89_ImageView *out_view)
{
    SA89_Imgcc0Adapter *a;
    imgcc0_frame *f;
    a = (SA89_Imgcc0Adapter *)user;
    if (!a || !out_view || handle != 1U || !a->loaded || frame_index >= a->image.frame_count) return SA89_PROVIDER_ERROR;
    f = &a->image.frames[frame_index];
    out_view->pixels = f->pixels;
    out_view->width = f->width;
    out_view->height = f->height;
    out_view->stride = f->stride;
    return SA89_PROVIDER_HANDLED;
}

static void sa89_i_release(void *user, sa89_u32 handle)
{
    SA89_Imgcc0Adapter *a;
    a = (SA89_Imgcc0Adapter *)user;
    if (!a || handle != 1U) return;
    imgcc0_image_reset(&a->image);
    a->loaded = 0;
    a->current_path[0] = '\0';
}

void sa89_imgcc0_init(SA89_Imgcc0Adapter *ctx,
                      void *output_buffer, imgcc0_u32 output_size,
                      void *temp_buffer, imgcc0_u32 temp_size,
                      void *file_buffer, imgcc0_u32 file_size)
{
    unsigned char *p;
    unsigned int i;
    if (!ctx) return;
    p = (unsigned char *)ctx;
    for (i = 0U; i < (unsigned int)sizeof(*ctx); ++i) p[i] = 0U;
    ctx->output_buffer = output_buffer;
    ctx->output_size = output_size;
    ctx->temp_buffer = temp_buffer;
    ctx->temp_size = temp_size;
    ctx->file_buffer = file_buffer;
    ctx->file_size = file_size;
    imgcc0_image_init(&ctx->image);
}

void sa89_imgcc0_make_provider(SA89_Imgcc0Adapter *ctx, SA89_ImageProvider *out_provider)
{
    if (!out_provider) return;
    out_provider->acquire = sa89_i_acquire;
    out_provider->get_frame = sa89_i_get_frame;
    out_provider->release = sa89_i_release;
    out_provider->user = ctx;
}
