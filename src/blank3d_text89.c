#include "blank3d_text89.h"

#include <stdio.h>
#include <string.h>

static void b3d_text89_status(Blank3DText89 *text, const char *message)
{
    unsigned int i;
    if (!text) return;
    if (!message) message = "";
    i = 0U;
    while (message[i] && i + 1U < (unsigned int)sizeof(text->status)) {
        text->status[i] = message[i];
        ++i;
    }
    text->status[i] = '\0';
}

static void b3d_text89_copy_path(char *dst, unsigned int cap,
                                 const char *src)
{
    unsigned int i;
    if (!dst || cap == 0U) return;
    if (!src) src = "";
    i = 0U;
    while (src[i] && i + 1U < cap) {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
}

static unsigned long b3d_text89_read_file(const char *path,
                                           unsigned char *dst,
                                           unsigned long cap)
{
    FILE *fp;
    long size;
    size_t got;
    if (!path || !path[0] || !dst || cap == 0UL) return 0UL;
    fp = fopen(path, "rb");
    if (!fp) return 0UL;
    if (fseek(fp, 0L, SEEK_END) != 0) {
        fclose(fp);
        return 0UL;
    }
    size = ftell(fp);
    if (size <= 0L || (unsigned long)size > cap) {
        fclose(fp);
        return 0UL;
    }
    if (fseek(fp, 0L, SEEK_SET) != 0) {
        fclose(fp);
        return 0UL;
    }
    got = fread(dst, 1U, (size_t)size, fp);
    fclose(fp);
    if (got != (size_t)size) return 0UL;
    return (unsigned long)size;
}

void blank3d_text89_init(Blank3DText89 *text)
{
    if (!text) return;
    memset(text, 0, sizeof(*text));
    text->default_pixel_size = 7;
    mfc_scratch_init(&text->scratch,
                     text->sfnt_bytes, (mfc_u32)B3D_TEXT89_SFNT_BYTES,
                     text->work_bytes, (mfc_u32)B3D_TEXT89_WORK_BYTES);
    mfc_font_clear(&text->font);
    text->outline.points = text->outline_points;
    text->outline.max_points = (unsigned short)B3D_TEXT89_MAX_POINTS;
    text->outline.contours = text->outline_contours;
    text->outline.max_contours = (unsigned short)B3D_TEXT89_MAX_CONTOURS;
    gwt_outline_reset(&text->outline);
    gwt_atlas_init(&text->atlas,
                   text->atlas_pixels,
                   (unsigned short)B3D_TEXT89_ATLAS_W,
                   (unsigned short)B3D_TEXT89_ATLAS_H,
                   (unsigned short)B3D_TEXT89_ATLAS_W,
                   text->atlas_glyphs,
                   (unsigned short)B3D_TEXT89_ATLAS_GLYPHS);
    b3d_text89_status(text, "Monika FontCore ready; legacy 5x7 fallback active");
}

int blank3d_text89_load_file(Blank3DText89 *text, const char *path)
{
    unsigned long size;
    int rc;
    if (!text || !path || !path[0]) {
        if (text) b3d_text89_status(text, "No font path; legacy 5x7 fallback active");
        return 0;
    }
    text->loaded = 0;
    text->raster_ready = 0;
    text->source_size = 0UL;
    mfc_font_clear(&text->font);
    text->scratch.sfnt_len = 0UL;
    text->scratch.work_len = 0UL;

    size = b3d_text89_read_file(path, text->source_bytes,
                                B3D_TEXT89_FONT_BYTES);
    if (size == 0UL) {
        b3d_text89_status(text, "Font file missing, empty, or larger than static source buffer");
        return 0;
    }
    rc = mfc_open_memory(&text->font, text->source_bytes,
                         (mfc_u32)size, MFC_KIND_UNKNOWN,
                         &text->scratch);
    if (rc != MFC_OK) {
        b3d_text89_status(text, mfc_error_string(rc));
        return 0;
    }
    text->source_size = size;
    text->loaded = 1;
    text->raster_ready =
        ((text->font.features & MFC_FEATURE_TTF_GLYF) != 0UL) ? 1 : 0;
    b3d_text89_copy_path(text->font_path,
                         (unsigned int)sizeof(text->font_path), path);
    if (text->raster_ready)
        b3d_text89_status(text, "FontCore TTF/glyf raster provider online");
    else
        b3d_text89_status(text, "Font decoded; current HUD raster path requires TTF/glyf");
    return 1;
}

int blank3d_text89_measure(Blank3DText89 *text,
                           const char *utf8,
                           int pixel_size,
                           int *width_out,
                           int *height_out)
{
    int rc;
    if (!text || !utf8 || !width_out || !height_out || pixel_size <= 0)
        return 0;
    if (!text->loaded) return 0;
    rc = mfc_measure_utf8(&text->font, utf8, pixel_size,
                          width_out, height_out);
    return rc == MFC_OK ? 1 : 0;
}

int blank3d_text89_raster_rgba(Blank3DText89 *text,
                                const char *utf8,
                                int pixel_size,
                                unsigned long rgba,
                                const unsigned char **pixels_out,
                                int *width_out,
                                int *height_out)
{
    GWT_Bitmap dst;
    int measured_w;
    int measured_h;
    int out_w;
    int out_h;
    int baseline;
    int rc;
    unsigned long count;
    unsigned long i;
    unsigned int r;
    unsigned int g;
    unsigned int b;
    unsigned int a;
    unsigned int coverage;

    if (!text || !utf8 || !pixels_out || !width_out || !height_out ||
        pixel_size <= 0 || !text->raster_ready) return 0;
    if (!blank3d_text89_measure(text, utf8, pixel_size,
                                &measured_w, &measured_h)) return 0;

    out_w = measured_w + 8;
    out_h = measured_h + pixel_size + 8;
    if (out_w < 1) out_w = 1;
    if (out_h < pixel_size + 8) out_h = pixel_size + 8;
    if (out_w > (int)B3D_TEXT89_BITMAP_W ||
        out_h > (int)B3D_TEXT89_BITMAP_H) {
        b3d_text89_status(text, "Text raster exceeds static bitmap buffer");
        return 0;
    }

    gwt_atlas_reset(&text->atlas, 0U);
    gwt_outline_reset(&text->outline);
    rc = gwt_atlas_add_utf8(&text->font.ttf, &text->atlas, utf8,
                            pixel_size, 2U, &text->outline);
    if (rc != GWT_OK) {
        b3d_text89_status(text, gwt_error_string(rc));
        return 0;
    }

    memset(text->bitmap, 0, (size_t)out_w * (size_t)out_h);
    dst.pixels = text->bitmap;
    dst.width = (unsigned short)out_w;
    dst.height = (unsigned short)out_h;
    dst.stride = (unsigned short)out_w;
    baseline = pixel_size + 3;
    rc = mfc_ttf_draw_text_bitmap_utf8(&text->font, &text->atlas,
                                        &dst, utf8, pixel_size,
                                        3, baseline, 255U,
                                        text->layout,
                                        (unsigned short)B3D_TEXT89_LAYOUT_MAX);
    if (rc != MFC_OK) {
        b3d_text89_status(text, mfc_error_string(rc));
        return 0;
    }

    r = (unsigned int)((rgba >> 24) & 255UL);
    g = (unsigned int)((rgba >> 16) & 255UL);
    b = (unsigned int)((rgba >> 8) & 255UL);
    a = (unsigned int)(rgba & 255UL);
    count = (unsigned long)out_w * (unsigned long)out_h;
    for (i = 0UL; i < count; ++i) {
        coverage = (unsigned int)text->bitmap[i];
        text->rgba[i * 4UL] = (unsigned char)r;
        text->rgba[i * 4UL + 1UL] = (unsigned char)g;
        text->rgba[i * 4UL + 2UL] = (unsigned char)b;
        text->rgba[i * 4UL + 3UL] =
            (unsigned char)((coverage * a + 127U) / 255U);
    }

    *pixels_out = text->rgba;
    *width_out = out_w;
    *height_out = out_h;
    return 1;
}

int blank3d_text89_is_loaded(const Blank3DText89 *text)
{
    return text && text->loaded ? 1 : 0;
}

int blank3d_text89_is_raster_ready(const Blank3DText89 *text)
{
    return text && text->raster_ready ? 1 : 0;
}

const char *blank3d_text89_status(const Blank3DText89 *text)
{
    if (!text) return "Text provider unavailable";
    return text->status;
}
