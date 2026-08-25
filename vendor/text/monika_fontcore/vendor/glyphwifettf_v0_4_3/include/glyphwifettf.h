#ifndef GLYPHWIFETTF_H
#define GLYPHWIFETTF_H

/*
   GlyphWifeTTF v0.4.3
   C89, no malloc/realloc/free, no heap ownership, no float/double.
   Caller owns all buffers. Fixed point: 16.16 for transformed coordinates.
*/

#ifdef __cplusplus
extern "C" {
#endif

#define GWT_VERSION_MAJOR 0
#define GWT_VERSION_MINOR 4
#define GWT_VERSION_PATCH 3

#define GWT_OK 0
#define GWT_ERR_BAD_ARG -1
#define GWT_ERR_BAD_FONT -2
#define GWT_ERR_TABLE_MISSING -3
#define GWT_ERR_UNSUPPORTED -4
#define GWT_ERR_RANGE -5
#define GWT_ERR_BUFFER_FULL -6
#define GWT_ERR_RECURSION_LIMIT -7

#define GWT_TAG(a,b,c,d) ((((unsigned long)(unsigned char)(a))<<24)|(((unsigned long)(unsigned char)(b))<<16)|(((unsigned long)(unsigned char)(c))<<8)|((unsigned long)(unsigned char)(d)))

#define GWT_FP_SHIFT 16
#define GWT_FP_ONE 65536L
#define GWT_FP_HALF 32768L
#define GWT_POINT_ON_CURVE 1

/* Set GWT_USE_LONG_LONG=0 before compiling for a stricter C89 fallback.
   Default keeps wider intermediate math for accuracy on modern C compilers. */
#ifndef GWT_USE_LONG_LONG
#define GWT_USE_LONG_LONG 1
#endif

#define GWT_NAME_COPYRIGHT 0
#define GWT_NAME_FAMILY 1
#define GWT_NAME_SUBFAMILY 2
#define GWT_NAME_UNIQUE_ID 3
#define GWT_NAME_FULL 4
#define GWT_NAME_VERSION 5
#define GWT_NAME_POSTSCRIPT 6

#define GWT_LAYOUT_WRAP_WORDS 1
#define GWT_LAYOUT_SKIP_MISSING 2

#define GWT_ALIGN_LEFT 0
#define GWT_ALIGN_CENTER 1
#define GWT_ALIGN_RIGHT 2

#ifndef GWT_MAX_RECURSION
#define GWT_MAX_RECURSION 8
#endif

typedef long gwt_fixed; /* 16.16 */

typedef struct GWT_Table {
    unsigned long tag;
    unsigned long offset;
    unsigned long length;
} GWT_Table;

typedef struct GWT_Font {
    const unsigned char *data;
    unsigned long size;
    unsigned short num_tables;

    GWT_Table table_head;
    GWT_Table table_maxp;
    GWT_Table table_hhea;
    GWT_Table table_hmtx;
    GWT_Table table_cmap;
    GWT_Table table_loca;
    GWT_Table table_glyf;
    GWT_Table table_name;
    GWT_Table table_kern;

    unsigned short units_per_em;
    short index_to_loc_format;
    short x_min, y_min, x_max, y_max;
    unsigned short num_glyphs;
    short ascent, descent, line_gap;
    unsigned short num_hmetrics;

    unsigned long cmap_fmt4_offset;
    unsigned long cmap_fmt12_offset;
} GWT_Font;

typedef struct GWT_Point {
    gwt_fixed x;
    gwt_fixed y;
    unsigned char flags; /* GWT_POINT_ON_CURVE if original/current point is on-curve */
    unsigned short contour;
} GWT_Point;

typedef struct GWT_Contour {
    unsigned short first_point;
    unsigned short point_count;
} GWT_Contour;

typedef struct GWT_Outline {
    GWT_Point *points;
    unsigned short max_points;
    unsigned short point_count;

    GWT_Contour *contours;
    unsigned short max_contours;
    unsigned short contour_count;

    short x_min, y_min, x_max, y_max;
} GWT_Outline;

typedef struct GWT_GlyphMetrics {
    int advance_width;
    int left_side_bearing;
    short x_min, y_min, x_max, y_max;
} GWT_GlyphMetrics;

typedef struct GWT_Segment {
    unsigned char type; /* 0=move, 1=line, 2=quad, 3=close */
    gwt_fixed x0, y0;
    gwt_fixed x1, y1;
    gwt_fixed x2, y2;
} GWT_Segment;

typedef struct GWT_SegmentBuffer {
    GWT_Segment *segments;
    unsigned short max_segments;
    unsigned short count;
} GWT_SegmentBuffer;

typedef struct GWT_Bitmap {
    unsigned char *pixels;
    unsigned short width;
    unsigned short height;
    unsigned short stride;
} GWT_Bitmap;

typedef struct GWT_AtlasGlyph {
    unsigned long codepoint;
    unsigned short glyph_id;
    unsigned short x, y, w, h;
    int advance_px;
    int bearing_x_px;
    int bearing_y_px;
} GWT_AtlasGlyph;

typedef struct GWT_Atlas {
    unsigned char *pixels;
    unsigned short width, height, stride;
    GWT_AtlasGlyph *glyphs;
    unsigned short max_glyphs;
    unsigned short glyph_count;
    unsigned short pen_x, pen_y, row_h;
} GWT_Atlas;

typedef struct GWT_LayoutGlyph {
    unsigned long codepoint;
    unsigned short glyph_id;
    int x; /* pixels */
    int y; /* pixels baseline */
    int advance_px;
} GWT_LayoutGlyph;

typedef struct GWT_TextQuad {
    unsigned long codepoint;
    unsigned short glyph_id;
    int dst_x;
    int dst_y;
    unsigned short src_x;
    unsigned short src_y;
    unsigned short w;
    unsigned short h;
    int advance_px;
} GWT_TextQuad;

typedef struct GWT_TextMetrics {
    int width;
    int height;
    int line_count;
    int ascent_px;
    int descent_px;
    int line_gap_px;
} GWT_TextMetrics;

/* SVG export helpers. Writer returns 0 on success. No heap ownership. */
typedef int (*GWT_SvgWriteFn)(void *user, const char *text);

typedef struct GWT_SvgOptions {
    short padding_units;
    unsigned char flip_y;
    unsigned char include_metrics;
    unsigned char include_control_points;
    const char *fill;
    const char *stroke;
} GWT_SvgOptions;

int gwt_font_init(GWT_Font *font, const unsigned char *data, unsigned long size);

const char *gwt_error_string(int code);
int gwt_get_name_ascii(const GWT_Font *font, unsigned short name_id, char *out, unsigned short max_out);

void gwt_bitmap_clear(GWT_Bitmap *bmp, unsigned char value);
int gwt_rasterize_outline_aa(const GWT_Outline *outline, gwt_fixed scale, GWT_Bitmap *bmp, int origin_x_px, int origin_y_px, unsigned char fill_value, unsigned char samples_log2);

int gwt_atlas_add_codepoint_aa(const GWT_Font *font, GWT_Atlas *atlas, unsigned long codepoint, int pixel_size, unsigned char samples_log2, GWT_Outline *scratch_outline);
int gwt_atlas_add_range(const GWT_Font *font, GWT_Atlas *atlas, unsigned long first_codepoint, unsigned long last_codepoint, int pixel_size, unsigned char samples_log2, GWT_Outline *scratch_outline);
int gwt_atlas_add_ascii(const GWT_Font *font, GWT_Atlas *atlas, int pixel_size, unsigned char samples_log2, GWT_Outline *scratch_outline);

int gwt_measure_utf8(const GWT_Font *font, const char *text, int pixel_size, int *out_width, int *out_height);
int gwt_layout_box_utf8(const GWT_Font *font, const char *text, int pixel_size, int x, int y, int max_width_px, int line_height_px, unsigned short flags, GWT_LayoutGlyph *out, unsigned short max_out, unsigned short *out_count);
int gwt_find_table(const GWT_Font *font, unsigned long tag, GWT_Table *out_table);
int gwt_codepoint_to_glyph(const GWT_Font *font, unsigned long codepoint, unsigned short *glyph_id);
int gwt_get_glyph_metrics(const GWT_Font *font, unsigned short glyph_id, GWT_GlyphMetrics *metrics);
int gwt_get_kerning(const GWT_Font *font, unsigned short left_gid, unsigned short right_gid, int *kern_units);

void gwt_outline_reset(GWT_Outline *outline);
int gwt_decode_glyph_outline(const GWT_Font *font, unsigned short glyph_id, GWT_Outline *outline);
int gwt_outline_to_segments(const GWT_Outline *outline, GWT_SegmentBuffer *buf);

int gwt_svg_write_path_d(const GWT_Outline *outline, GWT_SegmentBuffer *segments, GWT_SvgWriteFn writer, void *user, unsigned char flip_y);
int gwt_svg_write_glyph(const GWT_Font *font, unsigned long codepoint, GWT_Outline *scratch_outline, GWT_SegmentBuffer *scratch_segments, GWT_SvgWriteFn writer, void *user, const GWT_SvgOptions *options);

/* Fixed scale: pixels_per_em_fp = (pixel_size << 16). */
gwt_fixed gwt_make_scale(const GWT_Font *font, int pixel_size);
gwt_fixed gwt_units_to_pixels(gwt_fixed scale, int units);
int gwt_rasterize_outline(const GWT_Outline *outline, gwt_fixed scale, GWT_Bitmap *bmp, int origin_x_px, int origin_y_px, unsigned char fill_value);

void gwt_atlas_init(GWT_Atlas *atlas, unsigned char *pixels, unsigned short w, unsigned short h, unsigned short stride, GWT_AtlasGlyph *glyphs, unsigned short max_glyphs);
int gwt_atlas_add_codepoint(const GWT_Font *font, GWT_Atlas *atlas, unsigned long codepoint, int pixel_size, GWT_Outline *scratch_outline);
const GWT_AtlasGlyph *gwt_atlas_find(const GWT_Atlas *atlas, unsigned long codepoint);

void gwt_atlas_reset(GWT_Atlas *atlas, unsigned char clear_value);
int gwt_atlas_add_utf8(const GWT_Font *font, GWT_Atlas *atlas, const char *text, int pixel_size, unsigned char samples_log2, GWT_Outline *scratch_outline);
int gwt_build_text_quads_utf8(const GWT_Atlas *atlas, const GWT_LayoutGlyph *layout, unsigned short layout_count, GWT_TextQuad *out, unsigned short max_out, unsigned short *out_count);
int gwt_build_text_quads_box_utf8(const GWT_Font *font, const GWT_Atlas *atlas, const char *text, int pixel_size, int x, int y, int max_width_px, int line_height_px, unsigned short flags, GWT_LayoutGlyph *layout_scratch, unsigned short max_layout, GWT_TextQuad *out, unsigned short max_out, unsigned short *out_count);
int gwt_get_text_metrics(const GWT_Font *font, const char *text, int pixel_size, GWT_TextMetrics *out);
int gwt_bitmap_blit_alpha(GWT_Bitmap *dst, int dst_x, int dst_y, const unsigned char *src, unsigned short src_w, unsigned short src_h, unsigned short src_stride, unsigned char color);
int gwt_draw_text_bitmap_utf8(const GWT_Font *font, const GWT_Atlas *atlas, GWT_Bitmap *dst, const char *text, int pixel_size, int x, int y, unsigned char color, GWT_LayoutGlyph *layout_scratch, unsigned short max_layout);

unsigned long gwt_utf8_next(const char **text);
int gwt_layout_utf8(const GWT_Font *font, const char *text, int pixel_size, int x, int y, GWT_LayoutGlyph *out, unsigned short max_out, unsigned short *out_count);

#ifdef __cplusplus
}
#endif

#endif
