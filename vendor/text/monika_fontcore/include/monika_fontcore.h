#ifndef MONIKA_FONTCORE_H
#define MONIKA_FONTCORE_H

/*
   Monika FontCore Grand Decoder
   C89 facade over the vendored no-heap font decoders.

   Hard rules for this facade:
   - no malloc/free/realloc/calloc
   - no float/double
   - caller owns every large byte buffer
   - no file I/O in src/monika_fontcore.c

   The full upstream/vendor libraries are preserved under vendor/.
*/

#ifdef __cplusplus
extern "C" {
#endif

#include "sfnt_decoder.h"
#include "glyphwifettf.h"
#include "otf_c89.h"
#include "woff1_decoder.h"
#include "w2f.h"
#include "gmyy_spritefont.h"
#include "mfont.h"
#include "giffy_hb.h"

#define MFC_VERSION_MAJOR 0
#define MFC_VERSION_MINOR 1
#define MFC_VERSION_PATCH 0

#define MFC_OK 0
#define MFC_ERR_NULL              -1
#define MFC_ERR_RANGE             -2
#define MFC_ERR_UNKNOWN_FORMAT    -3
#define MFC_ERR_OUTPUT_TOO_SMALL  -4
#define MFC_ERR_DECODE            -5
#define MFC_ERR_UNSUPPORTED       -6
#define MFC_ERR_VENDOR            -7
#define MFC_ERR_BAD_ARG           -8

#define MFC_FEATURE_SFNT       0x00000001ul
#define MFC_FEATURE_TTF_GLYF   0x00000002ul
#define MFC_FEATURE_OTF_CFF    0x00000004ul
#define MFC_FEATURE_CFF2       0x00000008ul
#define MFC_FEATURE_WOFF1      0x00000010ul
#define MFC_FEATURE_WOFF2      0x00000020ul
#define MFC_FEATURE_EOT_SCAN   0x00000040ul
#define MFC_FEATURE_GMYY       0x00000080ul
#define MFC_FEATURE_MUGEN_TEXT 0x00000100ul
#define MFC_FEATURE_MUGEN_FNT  0x00000200ul
#define MFC_FEATURE_GIFFY_HB   0x00000400ul
#define MFC_FEATURE_KERN       0x00000800ul
#define MFC_FEATURE_HVAR       0x00001000ul
#define MFC_FEATURE_VVAR       0x00002000ul
#define MFC_FEATURE_MVAR       0x00004000ul

#define MFC_NAME_MAX 128

#define MFC_TAG(a,b,c,d) ((((unsigned long)(unsigned char)(a))<<24)|(((unsigned long)(unsigned char)(b))<<16)|(((unsigned long)(unsigned char)(c))<<8)|((unsigned long)(unsigned char)(d)))

typedef unsigned char  mfc_u8;
typedef unsigned short mfc_u16;
typedef signed short   mfc_i16;
typedef unsigned long  mfc_u32;
typedef signed long    mfc_i32;
typedef long           mfc_fixed; /* 16.16 */

typedef enum MFC_FontKindTag {
    MFC_KIND_UNKNOWN = 0,
    MFC_KIND_SFNT_TTF = 1,
    MFC_KIND_SFNT_OTF = 2,
    MFC_KIND_SFNT_TTC = 3,
    MFC_KIND_WOFF1 = 4,
    MFC_KIND_WOFF2 = 5,
    MFC_KIND_EOT = 6,
    MFC_KIND_GMYY_SPRITE_YY = 7,
    MFC_KIND_GMYY_GML_CALL = 8,
    MFC_KIND_MUGEN_TEXT = 9,
    MFC_KIND_MUGEN_FNT = 10
} MFC_FontKind;

typedef struct MFC_ScratchTag {
    mfc_u8 *sfnt_bytes;
    mfc_u32 sfnt_cap;
    mfc_u32 sfnt_len;

    mfc_u8 *work_bytes;
    mfc_u32 work_cap;
    mfc_u32 work_len;
} MFC_Scratch;

typedef struct MFC_FontTag {
    int requested_kind;
    int source_kind;
    int effective_kind;
    int last_error;
    unsigned long features;

    const mfc_u8 *source_bytes;
    mfc_u32 source_size;
    const mfc_u8 *font_bytes;
    mfc_u32 font_size;

    sfnt_face sfnt;
    GWT_Font ttf;
    otf_font otf;

    GMYY_Sprite gmyy_sprite;
    GMYY_FontCall gmyy_call;
    GMYY_SpriteFont gmyy_font;

    mft_font_text mugen_text;
    mft_fnt_header mugen_header;

    char name[MFC_NAME_MAX];
} MFC_Font;

typedef struct MFC_GlyphMetricsTag {
    unsigned long codepoint;
    unsigned short glyph_id;
    int advance;
    int left_side_bearing;
    short x_min;
    short y_min;
    short x_max;
    short y_max;
} MFC_GlyphMetrics;

void mfc_scratch_init(MFC_Scratch *scratch,
                      mfc_u8 *sfnt_bytes,
                      mfc_u32 sfnt_cap,
                      mfc_u8 *work_bytes,
                      mfc_u32 work_cap);

void mfc_font_clear(MFC_Font *font);
int mfc_detect_kind(const void *bytes, mfc_u32 size);
const char *mfc_kind_name(int kind);
const char *mfc_error_string(int code);

int mfc_open_memory(MFC_Font *font,
                    const void *bytes,
                    mfc_u32 size,
                    int kind_hint,
                    MFC_Scratch *scratch);

int mfc_open_gmyy_spritefont(MFC_Font *font,
                             const char *sprite_yy_text,
                             const char *font_call_gml_text);

int mfc_open_mugen_text(MFC_Font *font,
                        const char *font_text,
                        mfc_u32 text_size);

int mfc_lookup_glyph(const MFC_Font *font,
                     unsigned long codepoint,
                     unsigned short *glyph_id_out);

int mfc_get_glyph_metrics(const MFC_Font *font,
                          unsigned long codepoint,
                          MFC_GlyphMetrics *out_metrics);

int mfc_measure_utf8(const MFC_Font *font,
                     const char *text,
                     int pixel_size,
                     int *width_out,
                     int *height_out);

int mfc_ttf_build_atlas_ascii(const MFC_Font *font,
                              GWT_Atlas *atlas,
                              int pixel_size,
                              unsigned char samples_log2,
                              GWT_Outline *scratch_outline);

int mfc_ttf_draw_text_bitmap_utf8(const MFC_Font *font,
                                  const GWT_Atlas *atlas,
                                  GWT_Bitmap *dst,
                                  const char *text,
                                  int pixel_size,
                                  int x,
                                  int y,
                                  unsigned char color,
                                  GWT_LayoutGlyph *layout_scratch,
                                  unsigned short max_layout);

int mfc_shape_static_utf8(const ghb_static_font *static_font,
                          ghb_buffer *buffer,
                          const char *text,
                          int byte_count,
                          const ghb_feature *features,
                          int feature_count);

#ifdef __cplusplus
}
#endif

#endif
