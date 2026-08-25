#ifndef GF_BITMAP_H
#define GF_BITMAP_H
#include "gf_config.h"

typedef struct GF_Bitmap {
    unsigned short w;
    unsigned short h;
    unsigned short pitch;
    unsigned char pixels[GF_MAX_BITMAP_H][GF_MAX_BITMAP_W / 8];
} GF_Bitmap;

typedef struct GF_Atlas {
    unsigned short w, h;
    unsigned short cursor_x, cursor_y, row_h;
    unsigned char pixels[GF_ATLAS_H][GF_ATLAS_W / 8];
} GF_Atlas;

void gf_bitmap_clear(GF_Bitmap *b, unsigned short w, unsigned short h);
void gf_bitmap_set(GF_Bitmap *b, int x, int y, int on);
int gf_bitmap_get(const GF_Bitmap *b, int x, int y);
void gf_bitmap_blit_or(GF_Bitmap *dst, const GF_Bitmap *src, int dx, int dy);
void gf_atlas_clear(GF_Atlas *a);
int gf_atlas_pack(GF_Atlas *a, const GF_Bitmap *b, unsigned short *out_x, unsigned short *out_y);
int gf_write_pbm(const char *path, const GF_Bitmap *b);
int gf_write_atlas_pbm(const char *path, const GF_Atlas *a);

#endif
