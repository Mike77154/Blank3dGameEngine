#include "blank3d_text89.h"

#include <stdio.h>
#include <string.h>

#define OUT_W 920
#define OUT_H 260

static Blank3DText89 g_text;
static unsigned char g_rgb[OUT_W * OUT_H * 3];

static void fill(unsigned char r, unsigned char g, unsigned char b)
{
    unsigned long i;
    for (i = 0UL; i < (unsigned long)OUT_W * (unsigned long)OUT_H; ++i) {
        g_rgb[i * 3UL] = r;
        g_rgb[i * 3UL + 1UL] = g;
        g_rgb[i * 3UL + 2UL] = b;
    }
}

static void rect(int x, int y, int w, int h,
                 unsigned char r, unsigned char g, unsigned char b)
{
    int xx;
    int yy;
    unsigned long at;
    for (yy = 0; yy < h; ++yy) {
        if (y + yy < 0 || y + yy >= OUT_H) continue;
        for (xx = 0; xx < w; ++xx) {
            if (x + xx < 0 || x + xx >= OUT_W) continue;
            at = ((unsigned long)(y + yy) * OUT_W + (unsigned long)(x + xx)) * 3UL;
            g_rgb[at] = r;
            g_rgb[at + 1UL] = g;
            g_rgb[at + 2UL] = b;
        }
    }
}

static int draw_text(int x, int y, const char *s, int px, unsigned long rgba)
{
    const unsigned char *src;
    int w;
    int h;
    int xx;
    int yy;
    unsigned long si;
    unsigned long di;
    unsigned int a;
    unsigned int inv;
    if (!blank3d_text89_raster_rgba(&g_text, s, px, rgba, &src, &w, &h))
        return 0;
    for (yy = 0; yy < h; ++yy) {
        if (y + yy < 0 || y + yy >= OUT_H) continue;
        for (xx = 0; xx < w; ++xx) {
            if (x + xx < 0 || x + xx >= OUT_W) continue;
            si = ((unsigned long)yy * (unsigned long)w + (unsigned long)xx) * 4UL;
            a = src[si + 3UL];
            if (a == 0U) continue;
            inv = 255U - a;
            di = ((unsigned long)(y + yy) * OUT_W + (unsigned long)(x + xx)) * 3UL;
            g_rgb[di] = (unsigned char)(((unsigned int)src[si] * a +
                                         (unsigned int)g_rgb[di] * inv + 127U) / 255U);
            g_rgb[di + 1UL] = (unsigned char)(((unsigned int)src[si + 1UL] * a +
                                               (unsigned int)g_rgb[di + 1UL] * inv + 127U) / 255U);
            g_rgb[di + 2UL] = (unsigned char)(((unsigned int)src[si + 2UL] * a +
                                               (unsigned int)g_rgb[di + 2UL] * inv + 127U) / 255U);
        }
    }
    return 1;
}

static int write_ppm(const char *path)
{
    FILE *fp;
    size_t bytes;
    fp = fopen(path, "wb");
    if (!fp) return 0;
    fprintf(fp, "P6\n%d %d\n255\n", OUT_W, OUT_H);
    bytes = fwrite(g_rgb, 1U, sizeof(g_rgb), fp);
    fclose(fp);
    return bytes == sizeof(g_rgb) ? 1 : 0;
}

int main(int argc, char **argv)
{
    const char *font;
    const char *out;
    int mw;
    int mh;
    font = argc > 1 ? argv[1] : "";
    out = argc > 2 ? argv[2] : "text89_preview.ppm";
    blank3d_text89_init(&g_text);
    if (!blank3d_text89_load_file(&g_text, font)) {
        fprintf(stderr, "%s\n", blank3d_text89_status(&g_text));
        return 2;
    }
    if (!blank3d_text89_measure(&g_text, "PISTOL 06 / 024", 30, &mw, &mh))
        return 3;
    fill(10U, 12U, 15U);
    rect(40, 35, 840, 175, 20U, 23U, 28U);
    rect(42, 37, 4, 171, 150U, 160U, 170U);
    if (!draw_text(72, 55, "PISTOL", 26, 0xDCE4ECFFUL)) return 4;
    if (!draw_text(72, 94, "06 / 024", 42, 0xFFFFFFFFUL)) return 5;
    if (!draw_text(72, 154, "ACTIVE RELOAD  -  WEAPONS / SFX / MASTER", 18,
                   0xA8B4C0FFUL)) return 6;
    if (!write_ppm(out)) return 7;
    printf("FontCore host preview PASS: %s %dx%d measure=%dx%d\n",
           out, OUT_W, OUT_H, mw, mh);
    return 0;
}
