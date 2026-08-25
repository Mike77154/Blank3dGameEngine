#include <stdio.h>
#include <string.h>
#include "psd89/psd89.h"

#define PSD_FILE_CAP (8U * 1024U * 1024U)
#define PSD_PLANE_CAP (4U * 1024U * 1024U)
#define PSD_RGBA_CAP (16U * 1024U * 1024U)
static unsigned char g_file[PSD_FILE_CAP];
static unsigned char g_p0[PSD_PLANE_CAP];
static unsigned char g_p1[PSD_PLANE_CAP];
static unsigned char g_p2[PSD_PLANE_CAP];
static unsigned char g_pa[PSD_PLANE_CAP];
static unsigned char g_discard[PSD_PLANE_CAP];
static unsigned char g_rgba[PSD_RGBA_CAP];

static void put_u32(FILE *f, unsigned int v)
{
    unsigned char b[4];
    b[0] = (unsigned char)(v & 255U);
    b[1] = (unsigned char)((v >> 8) & 255U);
    b[2] = (unsigned char)((v >> 16) & 255U);
    b[3] = (unsigned char)((v >> 24) & 255U);
    fwrite(b, 1U, 4U, f);
}

static int read_file(const char *path, unsigned int *out_size)
{
    FILE *f;
    unsigned int n;
    int ch;
    f = fopen(path, "rb");
    if (f == 0) return 0;
    n = (unsigned int)fread(g_file, 1U, PSD_FILE_CAP, f);
    if (ferror(f)) { fclose(f); return 0; }
    if (n == PSD_FILE_CAP) {
        ch = fgetc(f);
        if (ch != EOF) { fclose(f); return 0; }
    }
    fclose(f);
    *out_size = n;
    return n != 0U;
}

int main(int argc, char **argv)
{
    psd89_doc doc;
    psd89_memio mem;
    psd89_io io;
    psd89_u8 *planes[56];
    unsigned int size;
    unsigned int base;
    unsigned int alpha_index;
    unsigned int plane_bytes;
    unsigned int c;
    unsigned int x;
    unsigned int y;
    unsigned int row;
    unsigned int stride;
    unsigned int rgba_bytes;
    int rc;
    FILE *f;
    if (argc != 3) return 2;
    if (!read_file(argv[1], &size)) return 10;
    psd89_memio_init_read(&mem, g_file, (psd89_u32)size);
    psd89_memio_make_io(&mem, &io);
    rc = psd89_read(&doc, &io);
    if (rc != PSD89_OK) return 11;
    if (doc.width == 0U || doc.height == 0U) return 12;
    if (doc.width > PSD_PLANE_CAP / doc.height) return 13;
    plane_bytes = (unsigned int)(doc.width * doc.height);
    base = doc.color_mode == PSD89_MODE_GRAYSCALE ? 1U : 3U;
    if (doc.channels < base || doc.channels > 56U) return 14;
    if (plane_bytes > PSD_PLANE_CAP) return 15;
    stride = (unsigned int)doc.width * 4U;
    if (stride > PSD_RGBA_CAP / (unsigned int)doc.height) return 16;
    rgba_bytes = stride * (unsigned int)doc.height;
    if (rgba_bytes > PSD_RGBA_CAP) return 17;
    for (c = 0U; c < 56U; ++c) planes[c] = 0;
    planes[0] = g_p0;
    if (base >= 2U) planes[1] = g_p1;
    if (base >= 3U) planes[2] = g_p2;
    alpha_index = 0xFFFFFFFFU;
    if ((unsigned int)doc.channels > base && doc.merged_alpha_in_first_channel) {
        alpha_index = base;
        planes[alpha_index] = g_pa;
    }
    for (c = base; c < (unsigned int)doc.channels; ++c) {
        if (c != alpha_index) planes[c] = g_discard;
    }
    psd89_memio_init_read(&mem, g_file, (psd89_u32)size);
    psd89_memio_make_io(&mem, &io);
    rc = psd89_decode_composite_u8(&doc, &io, planes, (psd89_u32)doc.width);
    if (rc != PSD89_OK) return 18;
    for (y = 0U; y < (unsigned int)doc.height; ++y) {
        row = y * (unsigned int)doc.width;
        for (x = 0U; x < (unsigned int)doc.width; ++x) {
            unsigned char *d;
            unsigned char a;
            d = g_rgba + y * stride + x * 4U;
            a = alpha_index != 0xFFFFFFFFU ? g_pa[row + x] : 255U;
            if (base == 1U) {
                d[0] = g_p0[row + x]; d[1] = d[0]; d[2] = d[0]; d[3] = a;
            } else {
                d[0] = g_p0[row + x]; d[1] = g_p1[row + x]; d[2] = g_p2[row + x]; d[3] = a;
            }
        }
    }
    f = fopen(argv[2], "wb");
    if (f == 0) return 19;
    fwrite("IC0TEST1", 1U, 8U, f);
    put_u32(f, (unsigned int)doc.width);
    put_u32(f, (unsigned int)doc.height);
    put_u32(f, 1U);
    put_u32(f, 0U);
    put_u32(f, 0U);
    put_u32(f, stride);
    if (fwrite(g_rgba, 1U, rgba_bytes, f) != rgba_bytes) { fclose(f); return 20; }
    fclose(f);
    return 0;
}
