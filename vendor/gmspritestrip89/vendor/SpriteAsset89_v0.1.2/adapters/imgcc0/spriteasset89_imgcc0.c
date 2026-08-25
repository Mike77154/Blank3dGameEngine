#include "spriteasset89_imgcc0.h"

static void sa89_i_zero(void *p, unsigned int n)
{
    unsigned char *b;
    unsigned int i;
    if (!p) return;
    b = (unsigned char *)p;
    for (i = 0U; i < n; ++i) b[i] = 0U;
}

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

static int sa89_i_load_handle(SA89_Imgcc0Adapter *a, sa89_u32 handle)
{
    imgcc0_open_options opt;
    SA89_Imgcc0Entry *e;
    if (!a || handle == 0U || handle > SA89_IMGCC0_MAX_HANDLES) return 0;
    e = &a->entries[handle - 1U];
    if (!e->used) return 0;
    if (a->loaded && a->current_handle == handle) return 1;
    imgcc0_image_reset(&a->image);
    imgcc0_open_options_init(&opt);
    opt.output_buffer = a->output_buffer;
    opt.output_buffer_size = a->output_size;
    opt.temp_buffer = a->temp_buffer;
    opt.temp_buffer_size = a->temp_size;
    opt.file_buffer = a->file_buffer;
    opt.file_buffer_size = a->file_size;
    if (imgcc0_open_file(e->path, &opt, &a->image) != IMGCC0_OK || !a->image.ok) {
        a->loaded = 0;
        a->current_handle = 0U;
        return 0;
    }
    e->width = a->image.width;
    e->height = a->image.height;
    e->frames = a->image.frame_count;
    a->loaded = 1;
    a->current_handle = handle;
    return 1;
}

static sa89_u32 sa89_i_find_or_add(SA89_Imgcc0Adapter *a, const char *path)
{
    sa89_u32 i;
    sa89_u32 slot;
    if (!a || !path) return 0U;
    for (i = 0U; i < SA89_IMGCC0_MAX_HANDLES; ++i) {
        if (a->entries[i].used && sa89_i_same(a->entries[i].path, path)) return i + 1U;
    }
    slot = SA89_IMGCC0_MAX_HANDLES;
    for (i = 0U; i < SA89_IMGCC0_MAX_HANDLES; ++i) {
        if (!a->entries[i].used) { slot = i; break; }
    }
    if (slot >= SA89_IMGCC0_MAX_HANDLES) return 0U;
    sa89_i_zero(&a->entries[slot], (unsigned int)sizeof(a->entries[slot]));
    sa89_i_copy(a->entries[slot].path, SA89_PATH_CAP, path);
    a->entries[slot].used = 1U;
    if (slot + 1U > a->entry_count) a->entry_count = slot + 1U;
    return slot + 1U;
}

static int sa89_i_acquire(void *user, const char *path,
                          sa89_u32 *out_handle, sa89_u32 *out_width,
                          sa89_u32 *out_height, sa89_u32 *out_frames)
{
    SA89_Imgcc0Adapter *a;
    SA89_Imgcc0Entry *e;
    sa89_u32 handle;
    a = (SA89_Imgcc0Adapter *)user;
    if (!a || !path || !out_handle || !out_width || !out_height || !out_frames) return SA89_PROVIDER_ERROR;
    handle = sa89_i_find_or_add(a, path);
    if (handle == 0U) return SA89_PROVIDER_ERROR;
    e = &a->entries[handle - 1U];
    if (e->width == 0U || e->height == 0U || e->frames == 0U) {
        if (!sa89_i_load_handle(a, handle)) return SA89_PROVIDER_ERROR;
    }
    *out_handle = handle;
    *out_width = e->width;
    *out_height = e->height;
    *out_frames = e->frames;
    return SA89_PROVIDER_HANDLED;
}

static int sa89_i_get_frame(void *user, sa89_u32 handle,
                            sa89_u32 frame_index, SA89_ImageView *out_view)
{
    SA89_Imgcc0Adapter *a;
    imgcc0_frame *f;
    a = (SA89_Imgcc0Adapter *)user;
    if (!a || !out_view || handle == 0U || handle > SA89_IMGCC0_MAX_HANDLES) return SA89_PROVIDER_ERROR;
    if (!a->entries[handle - 1U].used) return SA89_PROVIDER_ERROR;
    if (!sa89_i_load_handle(a, handle)) return SA89_PROVIDER_ERROR;
    if (frame_index >= a->image.frame_count) return SA89_PROVIDER_ERROR;
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
    SA89_Imgcc0Entry *e;
    a = (SA89_Imgcc0Adapter *)user;
    if (!a || handle == 0U || handle > SA89_IMGCC0_MAX_HANDLES) return;
    e = &a->entries[handle - 1U];
    if (!e->used) return;
    sa89_i_zero(e, (unsigned int)sizeof(*e));
    if (a->loaded && a->current_handle == handle) {
        imgcc0_image_reset(&a->image);
        a->loaded = 0;
        a->current_handle = 0U;
    }
}

void sa89_imgcc0_init(SA89_Imgcc0Adapter *ctx,
                      void *output_buffer, imgcc0_u32 output_size,
                      void *temp_buffer, imgcc0_u32 temp_size,
                      void *file_buffer, imgcc0_u32 file_size)
{
    if (!ctx) return;
    sa89_i_zero(ctx, (unsigned int)sizeof(*ctx));
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
