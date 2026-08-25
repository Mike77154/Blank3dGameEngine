#include "gf_bitmap.h"
#include <stdio.h>

void gf_bitmap_clear(GF_Bitmap *b, unsigned short w, unsigned short h) {
    int y, x;
    if (w > GF_MAX_BITMAP_W) w = GF_MAX_BITMAP_W;
    if (h > GF_MAX_BITMAP_H) h = GF_MAX_BITMAP_H;
    b->w = w; b->h = h; b->pitch = GF_MAX_BITMAP_W / 8;
    for (y = 0; y < GF_MAX_BITMAP_H; ++y) for (x = 0; x < GF_MAX_BITMAP_W / 8; ++x) b->pixels[y][x] = 0;
}
void gf_bitmap_set(GF_Bitmap *b, int x, int y, int on) {
    unsigned char mask;
    if (x < 0 || y < 0 || x >= (int)b->w || y >= (int)b->h) return;
    mask = (unsigned char)(0x80 >> (x & 7));
    if (on) b->pixels[y][x >> 3] |= mask; else b->pixels[y][x >> 3] &= (unsigned char)~mask;
}
int gf_bitmap_get(const GF_Bitmap *b, int x, int y) {
    if (x < 0 || y < 0 || x >= (int)b->w || y >= (int)b->h) return 0;
    return (b->pixels[y][x >> 3] & (0x80 >> (x & 7))) ? 1 : 0;
}

void gf_bitmap_blit_or(GF_Bitmap *dst, const GF_Bitmap *src, int dx, int dy) {
    int x, y;
    if (!dst || !src) return;
    for (y = 0; y < (int)src->h; ++y) {
        for (x = 0; x < (int)src->w; ++x) {
            if (gf_bitmap_get(src, x, y)) gf_bitmap_set(dst, dx + x, dy + y, 1);
        }
    }
}

void gf_atlas_clear(GF_Atlas *a) {
    int y, x; a->w = GF_ATLAS_W; a->h = GF_ATLAS_H; a->cursor_x = 0; a->cursor_y = 0; a->row_h = 0;
    for (y = 0; y < GF_ATLAS_H; ++y) for (x = 0; x < GF_ATLAS_W / 8; ++x) a->pixels[y][x] = 0;
}
static void gf_atlas_set(GF_Atlas *a, int x, int y, int on) {
    unsigned char mask;
    if (x < 0 || y < 0 || x >= GF_ATLAS_W || y >= GF_ATLAS_H) return;
    mask = (unsigned char)(0x80 >> (x & 7));
    if (on) a->pixels[y][x >> 3] |= mask; else a->pixels[y][x >> 3] &= (unsigned char)~mask;
}
int gf_atlas_pack(GF_Atlas *a, const GF_Bitmap *b, unsigned short *out_x, unsigned short *out_y) {
    int x, y;
    if (b->w > a->w || b->h > a->h) return -1;
    if (a->cursor_x + b->w + 1 > a->w) { a->cursor_x = 0; a->cursor_y = (unsigned short)(a->cursor_y + a->row_h + 1); a->row_h = 0; }
    if (a->cursor_y + b->h + 1 > a->h) return -1;
    *out_x = a->cursor_x; *out_y = a->cursor_y;
    for (y = 0; y < (int)b->h; ++y) for (x = 0; x < (int)b->w; ++x) if (gf_bitmap_get(b, x, y)) gf_atlas_set(a, a->cursor_x + x, a->cursor_y + y, 1);
    a->cursor_x = (unsigned short)(a->cursor_x + b->w + 1);
    if (b->h > a->row_h) a->row_h = b->h;
    return 0;
}
int gf_write_pbm(const char *path, const GF_Bitmap *b) {
    FILE *fp; int x, y; fp = fopen(path, "wb"); if (!fp) return -1;
    fprintf(fp, "P1\n%d %d\n", (int)b->w, (int)b->h);
    for (y = 0; y < (int)b->h; ++y) { for (x = 0; x < (int)b->w; ++x) fprintf(fp, "%d ", gf_bitmap_get(b, x, y)); fprintf(fp, "\n"); }
    fclose(fp); return 0;
}
int gf_write_atlas_pbm(const char *path, const GF_Atlas *a) {
    FILE *fp; int x, y; fp = fopen(path, "wb"); if (!fp) return -1;
    fprintf(fp, "P1\n%d %d\n", (int)a->w, (int)a->h);
    for (y = 0; y < (int)a->h; ++y) { for (x = 0; x < (int)a->w; ++x) fprintf(fp, "%d ", (a->pixels[y][x >> 3] & (0x80 >> (x & 7))) ? 1 : 0); fprintf(fp, "\n"); }
    fclose(fp); return 0;
}
