#include <stdio.h>
#include <string.h>

#include "monika_fontcore.h"
#include "pcxfnt.h"
#include "fntpng.h"

static mft_u8 g_fnt_file[MFT_CFG_MAX_FILE_BUFFER];
static mft_u8 g_pcx_file[MFT_CFG_MAX_FILE_BUFFER];
static char   g_text_file[MFT_CFG_MAX_TEXT];
static mft_u8 g_sheet_pixels[MFT_CFG_MAX_FILTERED];
static mft_u8 g_out_pixels[MFT_CFG_MAX_FILTERED];
static mft_u8 g_png_file[MFT_CFG_MAX_FILE_BUFFER];
static fntpng_workspace g_png_ws;

static mft_status read_file(const char *path, mft_u8 *buf, mft_u32 cap, mft_u32 *out_size)
{
    FILE *fp;
    long size;
    size_t got;
    if (path == 0 || buf == 0 || out_size == 0) return MFT_ERR_ARGS;
    fp = fopen(path, "rb");
    if (fp == 0) return MFT_ERR_IO;
    if (fseek(fp, 0L, SEEK_END) != 0) { fclose(fp); return MFT_ERR_IO; }
    size = ftell(fp);
    if (size < 0L) { fclose(fp); return MFT_ERR_IO; }
    if ((mft_u32)size > cap) { fclose(fp); return MFT_ERR_CAPACITY; }
    if (fseek(fp, 0L, SEEK_SET) != 0) { fclose(fp); return MFT_ERR_IO; }
    got = fread(buf, 1U, (size_t)size, fp);
    fclose(fp);
    if (got != (size_t)size) return MFT_ERR_IO;
    *out_size = (mft_u32)size;
    return MFT_OK;
}

static mft_status write_file(const char *path, const mft_u8 *buf, mft_u32 size)
{
    FILE *fp;
    size_t put;
    if (path == 0 || buf == 0) return MFT_ERR_ARGS;
    fp = fopen(path, "wb");
    if (fp == 0) return MFT_ERR_IO;
    put = fwrite(buf, 1U, (size_t)size, fp);
    fclose(fp);
    if (put != (size_t)size) return MFT_ERR_IO;
    return MFT_OK;
}

static void image_reset_with_pixels(mft_image *img, mft_u8 *pixels, mft_u32 cap)
{
    mft_image_reset(img);
    img->pixels = pixels;
    img->pixels_capacity = cap;
}

static void copy_palette(mft_image *dst, const mft_image *src)
{
    mft_u32 i;
    dst->palette_count = src->palette_count;
    for (i = 0UL; i < 256UL; ++i) {
        dst->palette[i] = src->palette[i];
    }
    if (dst->palette_count == 0UL) {
        dst->palette_count = 2UL;
        dst->palette[0].r = 0U; dst->palette[0].g = 0U; dst->palette[0].b = 0U; dst->palette[0].a = 255U;
        dst->palette[1].r = 255U; dst->palette[1].g = 255U; dst->palette[1].b = 255U; dst->palette[1].a = 255U;
    }
}

static void clear_index8(mft_image *img, mft_u8 index)
{
    mft_u32 y;
    for (y = 0UL; y < img->height; ++y) {
        mft_u32 x;
        mft_u8 *row;
        row = img->pixels + (y * img->stride);
        for (x = 0UL; x < img->width; ++x) row[x] = index;
    }
}

static void put_scaled_pixel(mft_image *dst, mft_s32 x, mft_s32 y, int scale, mft_u8 v)
{
    int sy;
    int sx;
    if (v == 0U) return;
    for (sy = 0; sy < scale; ++sy) {
        mft_s32 yy;
        yy = y + sy;
        if (yy < 0 || (mft_u32)yy >= dst->height) continue;
        for (sx = 0; sx < scale; ++sx) {
            mft_s32 xx;
            xx = x + sx;
            if (xx < 0 || (mft_u32)xx >= dst->width) continue;
            dst->pixels[((mft_u32)yy * dst->stride) + (mft_u32)xx] = v;
        }
    }
}

static int draw_char(const mft_image *sheet,
                     const mft_font_text *font,
                     mft_image *dst,
                     unsigned char ch,
                     mft_s32 x,
                     mft_s32 y,
                     int scale)
{
    const mft_glyph *g;
    mft_s32 sx;
    mft_s32 sy;
    mft_s32 w;
    mft_s32 h;
    if (ch == ' ') return (font->size_w + font->spacing_x) * scale;
    g = &font->glyphs[(mft_u8)ch];
    if (!g->present) {
        g = &font->glyphs[(mft_u8)'?'];
        if (!g->present) return (font->size_w + font->spacing_x) * scale;
    }
    w = g->width;
    h = font->size_h;
    if (w <= 0) w = font->size_w;
    if (h <= 0) h = 8;
    for (sy = 0; sy < h; ++sy) {
        for (sx = 0; sx < w; ++sx) {
            mft_s32 src_x;
            mft_s32 src_y;
            mft_u8 pix;
            src_x = g->offset + sx;
            src_y = sy;
            if (src_x < 0 || src_y < 0) continue;
            if ((mft_u32)src_x >= sheet->width || (mft_u32)src_y >= sheet->height) continue;
            pix = sheet->pixels[((mft_u32)src_y * sheet->stride) + (mft_u32)src_x];
            put_scaled_pixel(dst, x + (sx * scale), y + (sy * scale), scale, pix);
        }
    }
    return (w + font->spacing_x) * scale;
}

static void draw_string(const mft_image *sheet,
                        const mft_font_text *font,
                        mft_image *dst,
                        const char *s,
                        mft_s32 x,
                        mft_s32 y,
                        int scale)
{
    mft_s32 pen_x;
    mft_s32 pen_y;
    int line_advance;
    pen_x = x;
    pen_y = y;
    line_advance = (font->size_h + font->spacing_y) * scale + 4;
    while (*s != 0) {
        unsigned char ch;
        ch = (unsigned char)*s;
        if (ch == '\n') {
            pen_x = x;
            pen_y += line_advance;
        } else {
            pen_x += draw_char(sheet, font, dst, ch, pen_x, pen_y, scale);
        }
        ++s;
    }
}


static int compute_demo_scale(const mft_font_text *font)
{
    int i;
    mft_s32 max_w;
    max_w = font->size_w;
    for (i = 0; i < 256; ++i) {
        if (font->glyphs[i].present && font->glyphs[i].width > max_w) {
            max_w = font->glyphs[i].width;
        }
    }
    if (max_w > 32 || font->size_h > 24) return 1;
    if (max_w > 16 || font->size_h > 14) return 2;
    return 4;
}

static mft_s32 glyph_advance_for_sample(const mft_font_text *font, unsigned char ch)
{
    const mft_glyph *g;
    mft_s32 w;
    g = &font->glyphs[(mft_u8)ch];
    if (!g->present) return font->size_w + font->spacing_x;
    w = g->width;
    if (w <= 0) w = font->size_w;
    return w + font->spacing_x;
}

static void append_sample_char(char *dst, mft_u32 cap, mft_u32 *pos, char ch)
{
    if (*pos + 1UL < cap) {
        dst[*pos] = ch;
        *pos = *pos + 1UL;
        dst[*pos] = 0;
    }
}

static void build_glyph_sample(const mft_font_text *font, char *dst, mft_u32 cap, int scale)
{
    int pass;
    mft_u32 pos;
    mft_s32 pen;
    int printed;
    if (cap == 0UL) return;
    dst[0] = 0;
    pos = 0UL;
    pen = 0;
    printed = 0;
    for (pass = 0; pass < 4; ++pass) {
        int i;
        for (i = 32; i < 127; ++i) {
            mft_s32 adv;
            if (!font->glyphs[i].present) continue;
            if (i == 32) continue;
            adv = glyph_advance_for_sample(font, (unsigned char)i) * scale;
            if (pen > 0 && pen + adv > 390) {
                append_sample_char(dst, cap, &pos, '\n');
                pen = 0;
            }
            append_sample_char(dst, cap, &pos, (char)i);
            pen += adv;
            printed++;
        }
        if (printed > 0) {
            append_sample_char(dst, cap, &pos, '\n');
            pen = 0;
        }
    }
    if (printed == 0) {
        append_sample_char(dst, cap, &pos, '?');
        append_sample_char(dst, cap, &pos, '\n');
    }
}

int main(int argc, char **argv)
{
    const char *in_path;
    const char *out_path;
    mft_u32 fnt_size;
    mft_u32 pcx_size;
    mft_u32 text_size;
    mft_u32 png_size;
    mft_status st;
    int r;
    int kind;
    MFC_Font bin_font;
    MFC_Font txt_font;
    mft_fnt_header header;
    mft_image sheet;
    mft_image out;
    char comment[64];
    char sample[512];
    int demo_scale;

    if (argc < 3) {
        printf("usage: proof_mugen_fnt_png input.fnt output.png\n");
        return 2;
    }
    in_path = argv[1];
    out_path = argv[2];

    st = read_file(in_path, g_fnt_file, (mft_u32)sizeof(g_fnt_file), &fnt_size);
    if (st != MFT_OK) { printf("read: %s\n", mft_status_string(st)); return 1; }

    kind = mfc_detect_kind(g_fnt_file, fnt_size);
    printf("detect: %s\n", mfc_kind_name(kind));
    r = mfc_open_memory(&bin_font, g_fnt_file, fnt_size, MFC_KIND_UNKNOWN, 0);
    printf("open-binary: %s features=0x%08lx\n", mfc_error_string(r), bin_font.features);
    if (r != MFC_OK) return 1;

    pcx_size = (mft_u32)sizeof(g_pcx_file);
    text_size = (mft_u32)sizeof(g_text_file) - 1UL;
    st = mft_fnt_extract(g_fnt_file, fnt_size, g_pcx_file, &pcx_size, g_text_file, &text_size, &header);
    if (st != MFT_OK) { printf("extract: %s\n", mft_status_string(st)); return 1; }
    g_text_file[text_size] = 0;
    mft_fnt_comment_to_cstr(comment, (mft_u32)sizeof(comment), &header);
    printf("extract: pcx=%lu text=%lu comment='%s'\n",
           (unsigned long)pcx_size, (unsigned long)text_size, comment);

    r = mfc_open_mugen_text(&txt_font, g_text_file, text_size);
    printf("open-text: %s glyphs=%lu size=%ldx%ld spacing=%ld,%ld type=%ld\n",
           mfc_error_string(r),
           (unsigned long)txt_font.mugen_text.glyph_count,
           (long)txt_font.mugen_text.size_w,
           (long)txt_font.mugen_text.size_h,
           (long)txt_font.mugen_text.spacing_x,
           (long)txt_font.mugen_text.spacing_y,
           (long)txt_font.mugen_text.type);
    if (r != MFC_OK) return 1;

    image_reset_with_pixels(&sheet, g_sheet_pixels, (mft_u32)sizeof(g_sheet_pixels));
    st = pcxfnt_decode(&sheet, g_pcx_file, pcx_size);
    if (st != MFT_OK) { printf("pcx-decode: %s\n", mft_status_string(st)); return 1; }
    printf("sheet: %lux%lu stride=%lu format=%ld palette=%lu\n",
           (unsigned long)sheet.width, (unsigned long)sheet.height,
           (unsigned long)sheet.stride, (long)sheet.format,
           (unsigned long)sheet.palette_count);

    image_reset_with_pixels(&out, g_out_pixels, (mft_u32)sizeof(g_out_pixels));
    out.width = 420UL;
    out.height = 140UL;
    out.stride = out.width;
    out.format = MFT_PIXFMT_INDEX8;
    copy_palette(&out, &sheet);
    clear_index8(&out, 0U);

    demo_scale = compute_demo_scale(&txt_font.mugen_text);
    build_glyph_sample(&txt_font.mugen_text, sample, (mft_u32)sizeof(sample), demo_scale);
    printf("sample-scale: %d sample='%s'\n", demo_scale, sample);
    draw_string(&sheet, &txt_font.mugen_text, &out, sample, 12, 12, demo_scale);

    png_size = (mft_u32)sizeof(g_png_file);
    st = fntpng_encode(g_png_file, &png_size, &g_png_ws, &out);
    if (st != MFT_OK) { printf("png-encode: %s\n", mft_status_string(st)); return 1; }
    st = write_file(out_path, g_png_file, png_size);
    if (st != MFT_OK) { printf("write: %s\n", mft_status_string(st)); return 1; }
    printf("wrote: %s bytes=%lu\n", out_path, (unsigned long)png_size);
    return 0;
}
