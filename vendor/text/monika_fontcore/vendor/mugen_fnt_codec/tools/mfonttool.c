#include <stdio.h>
#include <string.h>

#include "mfont.h"
#include "fntpng.h"
#include "fntbump.h"
#include "pcxfnt.h"

static mft_u8 g_file_a[MFT_CFG_MAX_FILE_BUFFER];
static mft_u8 g_file_b[MFT_CFG_MAX_FILE_BUFFER];
static mft_u8 g_pixels_a[MFT_CFG_MAX_FILTERED];
static mft_u8 g_pixels_b[MFT_CFG_MAX_FILTERED];
static char g_text[MFT_CFG_MAX_TEXT];
static fntpng_workspace g_png_ws;

static int tool_ascii_ieq(const char *a, const char *b)
{
    while (*a != 0 && *b != 0) {
        char ca;
        char cb;
        ca = *a;
        cb = *b;
        if (ca >= 'A' && ca <= 'Z') {
            ca = (char)(ca - 'A' + 'a');
        }
        if (cb >= 'A' && cb <= 'Z') {
            cb = (char)(cb - 'A' + 'a');
        }
        if (ca != cb) {
            return 0;
        }
        ++a;
        ++b;
    }
    return (*a == 0 && *b == 0) ? 1 : 0;
}

static const char *tool_ext(const char *path)
{
    const char *dot;
    dot = strrchr(path, '.');
    return (dot != 0) ? (dot + 1) : "";
}

static void tool_image_init(mft_image *img, mft_u8 *pixels, mft_u32 cap)
{
    mft_image_reset(img);
    img->pixels = pixels;
    img->pixels_capacity = cap;
    img->stride = 0UL;
}

static mft_status tool_read_file(const char *path,
                                 mft_u8 *buf,
                                 mft_u32 cap,
                                 mft_u32 *out_size)
{
    FILE *fp;
    long size;
    size_t got;

    if (path == 0 || buf == 0 || out_size == 0) {
        return MFT_ERR_ARGS;
    }
    fp = fopen(path, "rb");
    if (fp == 0) {
        return MFT_ERR_IO;
    }
    if (fseek(fp, 0L, SEEK_END) != 0) {
        fclose(fp);
        return MFT_ERR_IO;
    }
    size = ftell(fp);
    if (size < 0L) {
        fclose(fp);
        return MFT_ERR_IO;
    }
    if ((mft_u32)size > cap) {
        fclose(fp);
        return MFT_ERR_CAPACITY;
    }
    if (fseek(fp, 0L, SEEK_SET) != 0) {
        fclose(fp);
        return MFT_ERR_IO;
    }
    got = fread(buf, 1U, (size_t)size, fp);
    fclose(fp);
    if (got != (size_t)size) {
        return MFT_ERR_IO;
    }
    *out_size = (mft_u32)size;
    return MFT_OK;
}

static mft_status tool_read_text_file(const char *path,
                                      char *buf,
                                      mft_u32 cap,
                                      mft_u32 *out_size)
{
    mft_status st;
    mft_u32 size;
    st = tool_read_file(path, (mft_u8 *)buf, cap - 1UL, &size);
    if (st != MFT_OK) {
        return st;
    }
    buf[size] = 0;
    *out_size = size;
    return MFT_OK;
}

static mft_status tool_write_file(const char *path,
                                  const void *buf,
                                  mft_u32 size)
{
    FILE *fp;
    size_t put;

    fp = fopen(path, "wb");
    if (fp == 0) {
        return MFT_ERR_IO;
    }
    put = fwrite(buf, 1U, (size_t)size, fp);
    fclose(fp);
    if (put != (size_t)size) {
        return MFT_ERR_IO;
    }
    return MFT_OK;
}

static mft_status tool_decode_image(mft_image *img,
                                    const char *path,
                                    const mft_u8 *data,
                                    mft_u32 size)
{
    const char *ext;
    ext = tool_ext(path);
    if (tool_ascii_ieq(ext, "png")) {
        return fntpng_decode(img, &g_png_ws, data, size);
    }
    if (tool_ascii_ieq(ext, "bmp")) {
        return fntbump_decode(img, data, size);
    }
    if (tool_ascii_ieq(ext, "pcx")) {
        return pcxfnt_decode(img, data, size);
    }
    return MFT_ERR_UNSUPPORTED;
}

static mft_status tool_encode_image(const char *path,
                                    mft_u8 *dst,
                                    mft_u32 *dst_size,
                                    const mft_image *img)
{
    const char *ext;
    ext = tool_ext(path);
    if (tool_ascii_ieq(ext, "png")) {
        return fntpng_encode(dst, dst_size, &g_png_ws, img);
    }
    if (tool_ascii_ieq(ext, "bmp")) {
        return fntbump_encode(dst, dst_size, img);
    }
    if (tool_ascii_ieq(ext, "pcx")) {
        return pcxfnt_encode(dst, dst_size, img);
    }
    return MFT_ERR_UNSUPPORTED;
}

static int tool_rgba_is_opaque(const mft_image *img)
{
    mft_u32 y;
    for (y = 0UL; y < img->height; ++y) {
        const mft_u8 *row;
        mft_u32 x;
        row = img->pixels + (y * img->stride);
        for (x = 0UL; x < img->width; ++x) {
            if (row[(x * 4UL) + 3UL] != 255U) {
                return 0;
            }
        }
    }
    return 1;
}

static mft_status tool_rgba_to_rgb(const mft_image *src,
                                   mft_image *dst,
                                   mft_u8 *pixels,
                                   mft_u32 cap)
{
    mft_u32 y;
    tool_image_init(dst, pixels, cap);
    dst->width = src->width;
    dst->height = src->height;
    dst->format = MFT_PIXFMT_RGB24;
    dst->stride = src->width * 3UL;
    if (dst->stride * dst->height > cap) {
        return MFT_ERR_CAPACITY;
    }
    for (y = 0UL; y < src->height; ++y) {
        const mft_u8 *s;
        mft_u8 *d;
        mft_u32 x;
        s = src->pixels + (y * src->stride);
        d = dst->pixels + (y * dst->stride);
        for (x = 0UL; x < src->width; ++x) {
            d[(x * 3UL) + 0UL] = s[(x * 4UL) + 0UL];
            d[(x * 3UL) + 1UL] = s[(x * 4UL) + 1UL];
            d[(x * 3UL) + 2UL] = s[(x * 4UL) + 2UL];
        }
    }
    return MFT_OK;
}

static mft_status tool_prepare_for_pcx(const mft_image **use_img,
                                       mft_image *tmp_img,
                                       const mft_image *src)
{
    mft_status st;
    if (src->format == MFT_PIXFMT_INDEX8 || src->format == MFT_PIXFMT_RGB24) {
        *use_img = src;
        return MFT_OK;
    }
    if (src->format == MFT_PIXFMT_RGBA32) {
        if (!tool_rgba_is_opaque(src)) {
            return MFT_ERR_UNSUPPORTED;
        }
        st = tool_rgba_to_rgb(src, tmp_img, g_pixels_b, (mft_u32)sizeof(g_pixels_b));
        if (st != MFT_OK) {
            return st;
        }
        *use_img = tmp_img;
        return MFT_OK;
    }
    return MFT_ERR_UNSUPPORTED;
}

static void tool_dump_image_info(const mft_image *img)
{
    const char *fmt;
    fmt = "unknown";
    if (img->format == MFT_PIXFMT_INDEX8) {
        fmt = "index8";
    } else if (img->format == MFT_PIXFMT_RGB24) {
        fmt = "rgb24";
    } else if (img->format == MFT_PIXFMT_RGBA32) {
        fmt = "rgba32";
    }
    printf("image: %lux%lu stride=%lu format=%s",
           (unsigned long)img->width,
           (unsigned long)img->height,
           (unsigned long)img->stride,
           fmt);
    if (img->format == MFT_PIXFMT_INDEX8) {
        printf(" palette=%lu", (unsigned long)img->palette_count);
    }
    printf("\n");
}

static void tool_usage(void)
{
    printf("mfonttool commands:\n");
    printf("  inspect-fnt <input.fnt>\n");
    printf("  unpack-fnt <input.fnt> <outbase>\n");
    printf("  pack-fnt <output.fnt> <font.txt> <image.(pcx|png|bmp)> [comment]\n");
    printf("  convert <input.(png|bmp|pcx)> <output.(png|bmp|pcx)>\n");
}

static int tool_cmd_inspect_fnt(const char *path)
{
    mft_status st;
    mft_u32 fnt_size;
    mft_u32 pcx_size;
    mft_u32 text_size;
    mft_fnt_header hdr;
    mft_font_text font;
    mft_image img;
    char comment[MFT_FNT_COMMENT_SIZE + 1U];

    st = tool_read_file(path, g_file_a, (mft_u32)sizeof(g_file_a), &fnt_size);
    if (st != MFT_OK) {
        fprintf(stderr, "read error: %s\n", mft_status_string(st));
        return 1;
    }
    pcx_size = (mft_u32)sizeof(g_file_b);
    text_size = (mft_u32)sizeof(g_text);
    st = mft_fnt_extract(g_file_a, fnt_size, g_file_b, &pcx_size, g_text, &text_size, &hdr);
    if (st != MFT_OK) {
        fprintf(stderr, "extract error: %s\n", mft_status_string(st));
        return 1;
    }

    mft_fnt_comment_to_cstr(comment, (mft_u32)sizeof(comment), &hdr);
    printf("signature: %.12s\n", hdr.signature);
    printf("version: %u.%u\n", (unsigned)hdr.ver_hi, (unsigned)hdr.ver_lo);
    printf("pcx offset: %lu size: %lu\n",
           (unsigned long)hdr.pcx_offset,
           (unsigned long)hdr.pcx_size);
    printf("text offset: %lu size: %lu\n",
           (unsigned long)hdr.text_offset,
           (unsigned long)hdr.text_size);
    printf("comment: %s\n", comment);

    st = mft_font_text_parse(&font, g_text, text_size + 1UL);
    if (st == MFT_OK) {
        printf("font type: %s\n", (font.type == MFT_FONT_VARIABLE) ? "Variable" : "Fixed");
        printf("offset: %ld,%ld\n", (long)font.offset_x, (long)font.offset_y);
        printf("size: %ld,%ld\n", (long)font.size_w, (long)font.size_h);
        printf("spacing: %ld,%ld\n", (long)font.spacing_x, (long)font.spacing_y);
        printf("colors: %ld\n", (long)font.colors);
        printf("sprites: %s\n", font.sprites);
        printf("glyphs: %lu\n", (unsigned long)font.glyph_count);
    } else {
        printf("text parse: %s\n", mft_status_string(st));
    }

    tool_image_init(&img, g_pixels_a, (mft_u32)sizeof(g_pixels_a));
    st = pcxfnt_decode(&img, g_file_b, pcx_size);
    if (st == MFT_OK) {
        tool_dump_image_info(&img);
    } else {
        printf("pcx decode: %s\n", mft_status_string(st));
    }

    return 0;
}

static int tool_cmd_unpack_fnt(const char *path, const char *outbase)
{
    mft_status st;
    mft_u32 fnt_size;
    mft_u32 pcx_size;
    mft_u32 text_size;
    char out_pcx[MFT_CFG_MAX_PATH * 2];
    char out_txt[MFT_CFG_MAX_PATH * 2];

    st = tool_read_file(path, g_file_a, (mft_u32)sizeof(g_file_a), &fnt_size);
    if (st != MFT_OK) {
        fprintf(stderr, "read error: %s\n", mft_status_string(st));
        return 1;
    }
    pcx_size = (mft_u32)sizeof(g_file_b);
    text_size = (mft_u32)sizeof(g_text);
    st = mft_fnt_extract(g_file_a, fnt_size, g_file_b, &pcx_size, g_text, &text_size, 0);
    if (st != MFT_OK) {
        fprintf(stderr, "extract error: %s\n", mft_status_string(st));
        return 1;
    }

    sprintf(out_pcx, "%s.pcx", outbase);
    sprintf(out_txt, "%s.txt", outbase);
    st = tool_write_file(out_pcx, g_file_b, pcx_size);
    if (st != MFT_OK) {
        fprintf(stderr, "write error: %s\n", mft_status_string(st));
        return 1;
    }
    st = tool_write_file(out_txt, g_text, text_size);
    if (st != MFT_OK) {
        fprintf(stderr, "write error: %s\n", mft_status_string(st));
        return 1;
    }
    printf("wrote %s and %s\n", out_pcx, out_txt);
    return 0;
}

static int tool_cmd_pack_fnt(const char *out_fnt,
                             const char *text_path,
                             const char *image_path,
                             const char *comment)
{
    mft_status st;
    mft_u32 text_size;
    mft_u32 image_size;
    mft_u32 pcx_size;
    const char *ext;
    mft_font_text font;

    st = tool_read_text_file(text_path, g_text, (mft_u32)sizeof(g_text), &text_size);
    if (st != MFT_OK) {
        fprintf(stderr, "text read error: %s\n", mft_status_string(st));
        return 1;
    }
    st = mft_font_text_parse(&font, g_text, text_size + 1UL);
    if (st != MFT_OK) {
        fprintf(stderr, "font text parse error: %s\n", mft_status_string(st));
        return 1;
    }

    ext = tool_ext(image_path);
    st = tool_read_file(image_path, g_file_a, (mft_u32)sizeof(g_file_a), &image_size);
    if (st != MFT_OK) {
        fprintf(stderr, "image read error: %s\n", mft_status_string(st));
        return 1;
    }

    if (tool_ascii_ieq(ext, "pcx")) {
        pcx_size = image_size;
        memcpy(g_file_b, g_file_a, (size_t)image_size);
    } else {
        mft_image src_img;
        mft_image tmp_img;
        const mft_image *use_img;

        tool_image_init(&src_img, g_pixels_a, (mft_u32)sizeof(g_pixels_a));
        tool_image_init(&tmp_img, g_pixels_b, (mft_u32)sizeof(g_pixels_b));
        st = tool_decode_image(&src_img, image_path, g_file_a, image_size);
        if (st != MFT_OK) {
            fprintf(stderr, "decode error: %s\n", mft_status_string(st));
            return 1;
        }
        st = tool_prepare_for_pcx(&use_img, &tmp_img, &src_img);
        if (st != MFT_OK) {
            fprintf(stderr, "pcx conversion error: %s\n", mft_status_string(st));
            return 1;
        }
        pcx_size = (mft_u32)sizeof(g_file_b);
        st = pcxfnt_encode(g_file_b, &pcx_size, use_img);
        if (st != MFT_OK) {
            fprintf(stderr, "pcx encode error: %s\n", mft_status_string(st));
            return 1;
        }
    }

    image_size = (mft_u32)sizeof(g_file_a);
    st = mft_fnt_pack(g_file_a,
                      &image_size,
                      g_file_b,
                      pcx_size,
                      g_text,
                      text_size,
                      comment,
                      1U,
                      0U);
    if (st != MFT_OK) {
        fprintf(stderr, "fnt pack error: %s\n", mft_status_string(st));
        return 1;
    }
    st = tool_write_file(out_fnt, g_file_a, image_size);
    if (st != MFT_OK) {
        fprintf(stderr, "write error: %s\n", mft_status_string(st));
        return 1;
    }
    printf("wrote %s\n", out_fnt);
    return 0;
}

static int tool_cmd_convert(const char *in_path, const char *out_path)
{
    mft_status st;
    mft_u32 in_size;
    mft_u32 out_size;
    mft_image src_img;
    mft_image tmp_img;
    const mft_image *use_img;

    st = tool_read_file(in_path, g_file_a, (mft_u32)sizeof(g_file_a), &in_size);
    if (st != MFT_OK) {
        fprintf(stderr, "read error: %s\n", mft_status_string(st));
        return 1;
    }

    tool_image_init(&src_img, g_pixels_a, (mft_u32)sizeof(g_pixels_a));
    tool_image_init(&tmp_img, g_pixels_b, (mft_u32)sizeof(g_pixels_b));
    st = tool_decode_image(&src_img, in_path, g_file_a, in_size);
    if (st != MFT_OK) {
        fprintf(stderr, "decode error: %s\n", mft_status_string(st));
        return 1;
    }

    use_img = &src_img;
    if (tool_ascii_ieq(tool_ext(out_path), "pcx")) {
        st = tool_prepare_for_pcx(&use_img, &tmp_img, &src_img);
        if (st != MFT_OK) {
            fprintf(stderr, "pcx conversion error: %s\n", mft_status_string(st));
            return 1;
        }
    }

    out_size = (mft_u32)sizeof(g_file_b);
    st = tool_encode_image(out_path, g_file_b, &out_size, use_img);
    if (st != MFT_OK) {
        fprintf(stderr, "encode error: %s\n", mft_status_string(st));
        return 1;
    }
    st = tool_write_file(out_path, g_file_b, out_size);
    if (st != MFT_OK) {
        fprintf(stderr, "write error: %s\n", mft_status_string(st));
        return 1;
    }
    printf("wrote %s\n", out_path);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        tool_usage();
        return 1;
    }
    if (tool_ascii_ieq(argv[1], "inspect-fnt")) {
        if (argc != 3) {
            tool_usage();
            return 1;
        }
        return tool_cmd_inspect_fnt(argv[2]);
    }
    if (tool_ascii_ieq(argv[1], "unpack-fnt")) {
        if (argc != 4) {
            tool_usage();
            return 1;
        }
        return tool_cmd_unpack_fnt(argv[2], argv[3]);
    }
    if (tool_ascii_ieq(argv[1], "pack-fnt")) {
        if (argc != 5 && argc != 6) {
            tool_usage();
            return 1;
        }
        return tool_cmd_pack_fnt(argv[2], argv[3], argv[4], (argc == 6) ? argv[5] : 0);
    }
    if (tool_ascii_ieq(argv[1], "convert")) {
        if (argc != 4) {
            tool_usage();
            return 1;
        }
        return tool_cmd_convert(argv[2], argv[3]);
    }

    tool_usage();
    return 1;
}
