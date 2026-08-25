#ifndef GMYY_SPRITEFONT_H
#define GMYY_SPRITEFONT_H

/*
   gmyy_spritefont_decoder_v3
   C89, no malloc/free/realloc, no heap ownership, no float/double.

   Scope:
   - GameMaker-style .yy GMSprite decoder for sprite frames/layers.
   - GML scanner for font_add_sprite() / font_add_sprite_ext().
   - Sprite-font glyph table builder.
   - Proportional alpha scanner for already-decoded RGBA, BMP, and TGA.

   PNG note:
   Real PNG pixels are DEFLATE-compressed, so this module does not own a full
   PNG decoder. Use gmyy_font_apply_proportional_rgba_provider() with your
   existing no-heap image loader / renderer upload path. This keeps this
   module C89/no-heap and 1:1 with the GameMaker sprite-font behavior.
*/

#ifdef __cplusplus
extern "C" {
#endif

#define GMYY_OK 0
#define GMYY_ERR (-1)
#define GMYY_WARN_TRUNC 1

#define GMYY_MAX_NAME       96
#define GMYY_MAX_PATH       256
#define GMYY_MAX_FRAMES     1024
#define GMYY_MAX_LAYERS     16
#define GMYY_MAX_GLYPHS     2048
#define GMYY_MAX_MAP_BYTES  4096

#define GMYY_FONT_CALL_ADD        1
#define GMYY_FONT_CALL_ADD_EXT    2

#define GMYY_FX_SHIFT 16
#define GMYY_FX_ONE   65536L

#define GMYY_SCAN_THRESHOLD_DEFAULT 1

/* Flags for scanners */
#define GMYY_SCAN_USE_ALPHA     1
#define GMYY_SCAN_USE_COLORKEY  2
#define GMYY_SCAN_INVERT_Y      4

typedef long GMYY_Fixed;

typedef struct GMYY_Status {
    int code;
    int warnings;
    char message[128];
} GMYY_Status;

typedef struct GMYY_Frame {
    char name[GMYY_MAX_NAME];
    char image_path[GMYY_MAX_PATH];
    int x;
    int y;
    int w;
    int h;
} GMYY_Frame;

typedef struct GMYY_Layer {
    char name[GMYY_MAX_NAME];
} GMYY_Layer;

typedef struct GMYY_Sprite {
    char name[GMYY_MAX_NAME];
    char resource_type[GMYY_MAX_NAME];
    int width;
    int height;
    int xorigin;
    int yorigin;
    int bbox_left;
    int bbox_top;
    int bbox_right;
    int bbox_bottom;
    int frame_count;
    int layer_count;
    GMYY_Frame frames[GMYY_MAX_FRAMES];
    GMYY_Layer layers[GMYY_MAX_LAYERS];
} GMYY_Sprite;

typedef struct GMYY_FontCall {
    int kind;
    char out_name[GMYY_MAX_NAME];
    char sprite_name[GMYY_MAX_NAME];
    unsigned long first_codepoint;
    char string_map[GMYY_MAX_MAP_BYTES];
    int proportional;
    int separation;
} GMYY_FontCall;

typedef struct GMYY_Glyph {
    unsigned long codepoint;
    int frame_index;
    int x;
    int y;
    int w;
    int h;
    int xoffset;
    int yoffset;
    int xadvance;
    GMYY_Fixed fxadvance;
    int trim_left;
    int trim_top;
    int trim_right;
    int trim_bottom;
    int has_trim;
} GMYY_Glyph;

typedef struct GMYY_SpriteFont {
    char name[GMYY_MAX_NAME];
    char sprite_name[GMYY_MAX_NAME];
    int proportional;
    int separation;
    int space_width;
    int line_height;
    int baseline;
    unsigned long missing_codepoint;
    int glyph_count;
    GMYY_Glyph glyphs[GMYY_MAX_GLYPHS];
    int ascii_lut[256];
} GMYY_SpriteFont;

typedef struct GMYY_Bounds {
    int found;
    int left;
    int top;
    int right;
    int bottom;
    int width;
    int height;
} GMYY_Bounds;

typedef struct GMYY_ImageInfo {
    int width;
    int height;
    int bpp;
    int top_down;
    int has_alpha;
} GMYY_ImageInfo;

/* Caller-owned RGBA provider. Must not allocate unless caller wants to. */
typedef int (*GMYY_RGBAProvider)(
    const char* path,
    unsigned char* rgba_out,
    long rgba_cap,
    int* width_out,
    int* height_out,
    int* stride_out,
    void* user
);

void gmyy_status_clear(GMYY_Status* st);
void gmyy_sprite_clear(GMYY_Sprite* s);
void gmyy_font_call_clear(GMYY_FontCall* c);
void gmyy_font_clear(GMYY_SpriteFont* f);

int gmyy_decode_sprite_yy(const char* yy_text, GMYY_Sprite* out, GMYY_Status* st);
int gmyy_decode_font_call_gml(const char* gml_text, GMYY_FontCall* out, GMYY_Status* st);
int gmyy_build_spritefont(const GMYY_Sprite* sprite, const GMYY_FontCall* call, GMYY_SpriteFont* out, GMYY_Status* st);

const GMYY_Glyph* gmyy_font_find_glyph(const GMYY_SpriteFont* font, unsigned long cp);
int gmyy_text_measure(const GMYY_SpriteFont* font, const char* utf8, int* w_out, int* h_out);

/* Alpha scanners */
int gmyy_scan_rgba_bounds(const unsigned char* rgba, int width, int height, int stride, int threshold, GMYY_Bounds* out);
int gmyy_scan_bmp_bounds(const unsigned char* data, long size, int threshold, unsigned long color_key_rgb, int flags, GMYY_Bounds* out, GMYY_ImageInfo* info);
int gmyy_scan_tga_bounds(const unsigned char* data, long size, int threshold, unsigned long color_key_rgb, int flags, GMYY_Bounds* out, GMYY_ImageInfo* info);
int gmyy_scan_image_bounds_by_ext(const char* path, const unsigned char* data, long size, int threshold, unsigned long color_key_rgb, int flags, GMYY_Bounds* out, GMYY_ImageInfo* info);

/* Apply prop=true widths using RGBA provider. Caller supplies static scratch buffer. */
int gmyy_font_apply_proportional_rgba_provider(
    GMYY_SpriteFont* font,
    const GMYY_Sprite* sprite,
    GMYY_RGBAProvider provider,
    unsigned char* scratch_rgba,
    long scratch_cap,
    int threshold,
    void* user,
    GMYY_Status* st
);

/* Apply bounds computed elsewhere to one glyph. */
int gmyy_font_apply_glyph_bounds(GMYY_SpriteFont* font, int glyph_index, const GMYY_Bounds* b);

/* Helpers */
unsigned long gmyy_utf8_decode_one(const char** p);
GMYY_Fixed gmyy_int_to_fx(int v);
int gmyy_fx_to_int_ceil(GMYY_Fixed v);

#ifdef __cplusplus
}
#endif

#endif
