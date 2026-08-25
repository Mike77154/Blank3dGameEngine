#include <stdio.h>
#include <string.h>
#include "png_decoder.h"
#include "png_mem89.h"
#include "giffany_dds.h"
#include "bmp/bmp.h"
#include "pcx/pcx.h"

#define TEST_FILE_CAP (8U * 1024U * 1024U)
#define TEST_RGBA_CAP (16U * 1024U * 1024U)
static unsigned char g_file[TEST_FILE_CAP];
static unsigned char g_rgba[TEST_RGBA_CAP];

static void put_u32(FILE *f, unsigned int v)
{
    unsigned char b[4];
    b[0] = (unsigned char)(v & 255U);
    b[1] = (unsigned char)((v >> 8) & 255U);
    b[2] = (unsigned char)((v >> 16) & 255U);
    b[3] = (unsigned char)((v >> 24) & 255U);
    fwrite(b, 1U, 4U, f);
}

static unsigned int delay_ms(unsigned short num, unsigned short den)
{
    unsigned int d;
    unsigned int n;
    d = den == 0U ? 100U : (unsigned int)den;
    n = (unsigned int)num * 1000U;
    return (n + d / 2U) / d;
}

static int read_file(const char *path, unsigned int *out_size)
{
    FILE *f;
    unsigned int n;
    int ch;
    f = fopen(path, "rb");
    if (!f) return 0;
    n = (unsigned int)fread(g_file, 1U, TEST_FILE_CAP, f);
    if (ferror(f)) { fclose(f); return 0; }
    if (n == TEST_FILE_CAP) {
        ch = fgetc(f);
        if (ch != EOF) { fclose(f); return 0; }
    }
    fclose(f);
    *out_size = n;
    return n != 0U;
}

static int begin_dump(FILE **pf, const char *out, unsigned int w, unsigned int h,
                      unsigned int frames, unsigned int loop)
{
    FILE *f;
    f = fopen(out, "wb");
    if (!f) return 0;
    fwrite("IC0TEST1", 1U, 8U, f);
    put_u32(f, w);
    put_u32(f, h);
    put_u32(f, frames);
    put_u32(f, loop);
    *pf = f;
    return 1;
}

static int dump_frame(FILE *f, unsigned int delay, unsigned int stride,
                      unsigned int h, const unsigned char *pixels)
{
    unsigned int bytes;
    bytes = stride * h;
    put_u32(f, delay);
    put_u32(f, stride);
    return fwrite(pixels, 1U, bytes, f) == bytes;
}

static int run_png(const char *in, const char *out, int apng)
{
    unsigned int size;
    png_decode_options opt;
    int rc;
    FILE *f;
    if (!read_file(in, &size)) return 10;
    png_mem89_reset();
    png_decode_options_init(&opt);
    opt.output_format = PNG_OUTPUT_RGBA8;
    opt.transform_flags = PNG_DEC_TRANSFORM_NONE;
    opt.strict_trailing_data = 1;
    if (!apng) {
        png_image img;
        memset(&img, 0, sizeof(img));
        rc = png_decode_memory_ex_zlib(g_file, size, &opt, &img);
        if (rc != PNG_DEC_OK) return 20;
        if (!begin_dump(&f, out, img.width, img.height, 1U, 0U)) { png_free_image(&img); return 21; }
        if (!dump_frame(f, 0U, img.pixel_rowbytes, img.height, img.pixels)) { fclose(f); png_free_image(&img); return 22; }
        fclose(f);
        png_free_image(&img);
    } else {
        png_apng a;
        unsigned int i;
        memset(&a, 0, sizeof(a));
        rc = png_decode_apng_memory_ex_zlib(g_file, size, &opt, &a);
        if (rc != PNG_DEC_OK) return 23;
        if (!begin_dump(&f, out, a.width, a.height, a.frame_count, a.num_plays)) { png_free_apng(&a); return 24; }
        for (i = 0U; i < a.frame_count; ++i) {
            if (!dump_frame(f, delay_ms(a.frames[i].control.delay_num, a.frames[i].control.delay_den),
                            a.frames[i].pixel_rowbytes, a.height, a.frames[i].pixels)) {
                fclose(f); png_free_apng(&a); return 25;
            }
        }
        fclose(f);
        png_free_apng(&a);
    }
    png_mem89_reset();
    return 0;
}

static int run_dds(const char *in, const char *out)
{
    unsigned int size;
    gdds_image img;
    gdds_parse_options opt;
    gdds_result rc;
    FILE *f;
    if (!read_file(in, &size)) return 30;
    memset(&img, 0, sizeof(img));
    opt = gdds_parse_options_default();
    opt.mode = GDDS_PARSE_MODE_STRICT;
    rc = gdds_decode_memory_ex(g_file, size, &opt, &img);
    if (rc != GDDS_RESULT_OK) return 31;
    if (!begin_dump(&f, out, img.width, img.height, 1U, 0U)) return 32;
    if (!dump_frame(f, 0U, img.width * 4U, img.height, img.pixels)) { fclose(f); return 33; }
    fclose(f);
    gdds_image_release(&img);
    return 0;
}

static int run_bmp(const char *in, const char *out)
{
    unsigned int size;
    bmp_image img;
    bmp_limits lim;
    unsigned int need;
    unsigned int stride;
    int rc;
    FILE *f;
    if (!read_file(in, &size)) return 40;
    memset(&img, 0, sizeof(img));
    bmp_limits_default(&lim);
    lim.max_input_bytes = size;
    rc = bmp_parse_memory_with_limits(g_file, size, &lim, &img);
    if (rc != BMP_OK) return 41;
    rc = bmp_calc_rgba32_buffer_size(&img, &need);
    if (rc != BMP_OK || need > TEST_RGBA_CAP) return 42;
    stride = (unsigned int)img.meta.width * 4U;
    rc = bmp_decode_to_rgba32_with_limits(&img, &lim, g_rgba, stride);
    if (rc != BMP_OK) return 43;
    if (!begin_dump(&f, out, (unsigned int)img.meta.width, (unsigned int)img.meta.height, 1U, 0U)) return 44;
    if (!dump_frame(f, 0U, stride, (unsigned int)img.meta.height, g_rgba)) { fclose(f); return 45; }
    fclose(f);
    return 0;
}

static int run_pcx(const char *in, const char *out)
{
    unsigned int size;
    PCXImage img;
    PCXFileInfo info;
    PCXDecodeLimits lim;
    unsigned int x, y, stride, need;
    int rc;
    FILE *f;
    if (!read_file(in, &size)) return 50;
    pcx_image_init(&img);
    memset(&info, 0, sizeof(info));
    pcx_decode_limits_default(&lim);
    lim.maxInputBytes = size;
    rc = pcx_load_memory_ex(g_file, size, &img, &info, PCX_PARSE_FLAG_STRICT, &lim);
    if (rc != PCX_OK) return 51;
    stride = (unsigned int)img.width * 4U;
    need = stride * (unsigned int)img.height;
    if (img.channels != 3 || need > TEST_RGBA_CAP) { pcx_image_release(&img); return 52; }
    for (y = 0U; y < (unsigned int)img.height; ++y) {
        for (x = 0U; x < (unsigned int)img.width; ++x) {
            const unsigned char *s;
            unsigned char *d;
            s = img.pixels + (y * (unsigned int)img.width + x) * 3U;
            d = g_rgba + y * stride + x * 4U;
            d[0] = s[0]; d[1] = s[1]; d[2] = s[2]; d[3] = 255U;
        }
    }
    if (!begin_dump(&f, out, (unsigned int)img.width, (unsigned int)img.height, 1U, 0U)) { pcx_image_release(&img); return 53; }
    if (!dump_frame(f, 0U, stride, (unsigned int)img.height, g_rgba)) { fclose(f); pcx_image_release(&img); return 54; }
    fclose(f);
    pcx_image_release(&img);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 4) return 2;
    if (strcmp(argv[1], "png") == 0) return run_png(argv[2], argv[3], 0);
    if (strcmp(argv[1], "apng") == 0) return run_png(argv[2], argv[3], 1);
    if (strcmp(argv[1], "dds") == 0) return run_dds(argv[2], argv[3]);
    if (strcmp(argv[1], "bmp") == 0) return run_bmp(argv[2], argv[3]);
    if (strcmp(argv[1], "pcx") == 0) return run_pcx(argv[2], argv[3]);
    return 3;
}
