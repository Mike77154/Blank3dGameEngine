#ifndef BLANK3D_TEXT89_H
#define BLANK3D_TEXT89_H

#include "monika_fontcore.h"

#ifdef __cplusplus
extern "C" {
#endif

#define B3D_TEXT89_FONT_BYTES       (1024UL * 1024UL)
#define B3D_TEXT89_SFNT_BYTES       (2UL * 1024UL * 1024UL)
#define B3D_TEXT89_WORK_BYTES       (2UL * 1024UL * 1024UL)
#define B3D_TEXT89_ATLAS_W          512U
#define B3D_TEXT89_ATLAS_H          512U
#define B3D_TEXT89_ATLAS_GLYPHS     192U
#define B3D_TEXT89_MAX_POINTS       4096U
#define B3D_TEXT89_MAX_CONTOURS     512U
#define B3D_TEXT89_LAYOUT_MAX       256U
#define B3D_TEXT89_BITMAP_W         1024U
#define B3D_TEXT89_BITMAP_H         160U

/*
   Blank3D host adapter for the vendored Monika FontCore grand decoder.
   All large memory is embedded here; the facade itself remains no-heap.
   File I/O is intentionally kept in this host adapter, not in FontCore.
*/
typedef struct Blank3DText89Tag {
    MFC_Font font;
    MFC_Scratch scratch;
    GWT_Outline outline;
    GWT_Atlas atlas;

    unsigned char source_bytes[B3D_TEXT89_FONT_BYTES];
    unsigned char sfnt_bytes[B3D_TEXT89_SFNT_BYTES];
    unsigned char work_bytes[B3D_TEXT89_WORK_BYTES];

    GWT_Point outline_points[B3D_TEXT89_MAX_POINTS];
    GWT_Contour outline_contours[B3D_TEXT89_MAX_CONTOURS];
    unsigned char atlas_pixels[B3D_TEXT89_ATLAS_W * B3D_TEXT89_ATLAS_H];
    GWT_AtlasGlyph atlas_glyphs[B3D_TEXT89_ATLAS_GLYPHS];
    GWT_LayoutGlyph layout[B3D_TEXT89_LAYOUT_MAX];

    unsigned char bitmap[B3D_TEXT89_BITMAP_W * B3D_TEXT89_BITMAP_H];
    unsigned char rgba[B3D_TEXT89_BITMAP_W * B3D_TEXT89_BITMAP_H * 4U];

    unsigned long source_size;
    int loaded;
    int raster_ready;
    int default_pixel_size;
    char font_path[192];
    char status[192];
} Blank3DText89;

void blank3d_text89_init(Blank3DText89 *text);
int blank3d_text89_load_file(Blank3DText89 *text, const char *path);
int blank3d_text89_measure(Blank3DText89 *text,
                           const char *utf8,
                           int pixel_size,
                           int *width_out,
                           int *height_out);
int blank3d_text89_raster_rgba(Blank3DText89 *text,
                                const char *utf8,
                                int pixel_size,
                                unsigned long rgba,
                                const unsigned char **pixels_out,
                                int *width_out,
                                int *height_out);
int blank3d_text89_is_loaded(const Blank3DText89 *text);
int blank3d_text89_is_raster_ready(const Blank3DText89 *text);
const char *blank3d_text89_status(const Blank3DText89 *text);

#ifdef __cplusplus
}
#endif

#endif
