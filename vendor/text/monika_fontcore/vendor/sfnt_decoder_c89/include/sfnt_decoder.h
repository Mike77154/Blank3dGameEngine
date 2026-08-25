#ifndef SFNT_DECODER_H
#define SFNT_DECODER_H

/*
   sfnt_decoder.h - tiny SFNT/OpenType/TrueType decoder core, C89.

   Design rules:
   - No malloc/free/realloc and no heap ownership.
   - No float/double. Fixed-point values use signed 16.16 integers.
   - Caller owns the font byte buffer and any output arrays.
   - Library does not include stdio/stdlib and performs no file I/O.

   Integer assumptions, checked at compile time in sfnt_decoder.c:
   unsigned char  = 8 bits
   unsigned short = 16 bits
   unsigned int   = 32 bits
*/

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char  sfnt_u8;
typedef signed char    sfnt_i8;
typedef unsigned short sfnt_u16;
typedef signed short   sfnt_i16;
typedef unsigned int   sfnt_u32;
typedef signed int     sfnt_i32;

typedef sfnt_i32 sfnt_fixed; /* 16.16 */

#ifndef SFNT_MAX_TABLES
#define SFNT_MAX_TABLES 128u
#endif

#define SFNT_TAG(a,b,c,d) \
    ((((sfnt_u32)(sfnt_u8)(a)) << 24) | (((sfnt_u32)(sfnt_u8)(b)) << 16) | \
     (((sfnt_u32)(sfnt_u8)(c)) << 8)  |  ((sfnt_u32)(sfnt_u8)(d)))

#define SFNT_TAG_TTCF SFNT_TAG('t','t','c','f')
#define SFNT_TAG_HEAD SFNT_TAG('h','e','a','d')
#define SFNT_TAG_MAXP SFNT_TAG('m','a','x','p')
#define SFNT_TAG_HHEA SFNT_TAG('h','h','e','a')
#define SFNT_TAG_HMTX SFNT_TAG('h','m','t','x')
#define SFNT_TAG_CMAP SFNT_TAG('c','m','a','p')
#define SFNT_TAG_LOCA SFNT_TAG('l','o','c','a')
#define SFNT_TAG_GLYF SFNT_TAG('g','l','y','f')
#define SFNT_TAG_NAME SFNT_TAG('n','a','m','e')
#define SFNT_TAG_OS2  SFNT_TAG('O','S','/','2')
#define SFNT_TAG_KERN SFNT_TAG('k','e','r','n')
#define SFNT_TAG_CFF  SFNT_TAG('C','F','F',' ')
#define SFNT_TAG_CFF2 SFNT_TAG('C','F','F','2')

#define SFNT_OK 0
#define SFNT_ERR_NULL          -1
#define SFNT_ERR_RANGE         -2
#define SFNT_ERR_BAD_MAGIC     -3
#define SFNT_ERR_TOO_MANY      -4
#define SFNT_ERR_NOT_FOUND     -5
#define SFNT_ERR_BAD_TABLE     -6
#define SFNT_ERR_UNSUPPORTED   -7
#define SFNT_ERR_TOO_SMALL     -8
#define SFNT_ERR_CHECKSUM      -9
#define SFNT_ERR_COMPOUND      -10
#define SFNT_ERR_SIMPLE        -11

typedef struct sfnt_table_record_s {
    sfnt_u32 tag;
    sfnt_u32 checksum;
    sfnt_u32 offset;
    sfnt_u32 length;
} sfnt_table_record;

typedef struct sfnt_table_ref_s {
    const sfnt_u8 *ptr;
    sfnt_u32 offset;
    sfnt_u32 length;
    sfnt_u32 checksum;
} sfnt_table_ref;

typedef struct sfnt_face_s {
    const sfnt_u8 *data;
    sfnt_u32 size;
    sfnt_u32 base_offset;

    sfnt_u32 sfnt_version;
    sfnt_u16 num_tables;
    sfnt_u16 search_range;
    sfnt_u16 entry_selector;
    sfnt_u16 range_shift;

    sfnt_table_record tables[SFNT_MAX_TABLES];

    sfnt_u8 has_head;
    sfnt_u8 has_maxp;
    sfnt_u8 has_hhea;
    sfnt_u8 has_hmtx;
    sfnt_u8 has_cmap;
    sfnt_u8 has_loca;
    sfnt_u8 has_glyf;
    sfnt_u8 has_name;
    sfnt_u8 has_os2;
    sfnt_u8 has_kern;
    sfnt_u8 has_cff;
    sfnt_u8 has_cff2;

    sfnt_u16 units_per_em;
    sfnt_u16 num_glyphs;
    sfnt_i16 index_to_loc_format;
    sfnt_i16 glyph_data_format;
    sfnt_i16 x_min;
    sfnt_i16 y_min;
    sfnt_i16 x_max;
    sfnt_i16 y_max;

    sfnt_i16 ascender;
    sfnt_i16 descender;
    sfnt_i16 line_gap;
    sfnt_u16 num_h_metrics;
} sfnt_face;

typedef struct sfnt_cmap_s {
    sfnt_u16 platform_id;
    sfnt_u16 encoding_id;
    sfnt_u32 subtable_offset; /* absolute font-file offset */
    sfnt_u32 length;
    sfnt_u16 format;
} sfnt_cmap;

typedef struct sfnt_hmetric_s {
    sfnt_u16 advance_width;
    sfnt_i16 left_side_bearing;
} sfnt_hmetric;

typedef struct sfnt_glyph_header_s {
    sfnt_i16 number_of_contours;
    sfnt_i16 x_min;
    sfnt_i16 y_min;
    sfnt_i16 x_max;
    sfnt_i16 y_max;
} sfnt_glyph_header;

typedef struct sfnt_point_s {
    sfnt_i16 x;
    sfnt_i16 y;
    sfnt_u8 on_curve;
    sfnt_u8 end_contour;
    sfnt_u8 raw_flags;
} sfnt_point;

typedef struct sfnt_outline_s {
    sfnt_u16 contour_count;
    sfnt_u16 point_count;
    sfnt_u16 instruction_length;
    sfnt_i16 x_min;
    sfnt_i16 y_min;
    sfnt_i16 x_max;
    sfnt_i16 y_max;
} sfnt_outline;

typedef struct sfnt_component_s {
    sfnt_u16 flags;
    sfnt_u16 glyph_index;
    sfnt_i16 arg1;
    sfnt_i16 arg2;
    sfnt_fixed a;
    sfnt_fixed b;
    sfnt_fixed c;
    sfnt_fixed d;
} sfnt_component;

typedef int (*sfnt_component_visitor)(void *user, const sfnt_component *component);

typedef struct sfnt_name_record_s {
    sfnt_u16 platform_id;
    sfnt_u16 encoding_id;
    sfnt_u16 language_id;
    sfnt_u16 name_id;
    const sfnt_u8 *bytes;
    sfnt_u16 length;
} sfnt_name_record;

typedef struct sfnt_os2_metrics_s {
    sfnt_u16 version;
    sfnt_i16 x_avg_char_width;
    sfnt_u16 us_weight_class;
    sfnt_u16 us_width_class;
    sfnt_u16 fs_type;
    sfnt_i16 s_typo_ascender;
    sfnt_i16 s_typo_descender;
    sfnt_i16 s_typo_line_gap;
    sfnt_u16 us_win_ascent;
    sfnt_u16 us_win_descent;
    sfnt_i16 sx_height;
    sfnt_i16 s_cap_height;
    sfnt_u16 us_default_char;
    sfnt_u16 us_break_char;
    sfnt_u16 us_max_context;
} sfnt_os2_metrics;

/* Basic byte helpers. They return zero if p is NULL. */
sfnt_u16 sfnt_read_u16(const sfnt_u8 *p);
sfnt_i16 sfnt_read_i16(const sfnt_u8 *p);
sfnt_u32 sfnt_read_u32(const sfnt_u8 *p);
sfnt_i32 sfnt_read_i32(const sfnt_u8 *p);
void sfnt_tag_to_chars(sfnt_u32 tag, char out5[5]);

/* Open one font. sfnt_open() also accepts a TTC collection and opens index 0. */
int sfnt_open(sfnt_face *face, const void *font_bytes, sfnt_u32 font_size);
int sfnt_open_ttc(sfnt_face *face, const void *font_bytes, sfnt_u32 font_size, sfnt_u32 font_index);
int sfnt_open_at(sfnt_face *face, const void *font_bytes, sfnt_u32 font_size, sfnt_u32 offset);

/* Table access and validation. */
int sfnt_find_table(const sfnt_face *face, sfnt_u32 tag, sfnt_table_ref *out_ref);
int sfnt_table_index(const sfnt_face *face, sfnt_u32 tag);
sfnt_u32 sfnt_table_checksum(const sfnt_u8 *bytes, sfnt_u32 length);
int sfnt_validate_table_checksum(const sfnt_face *face, sfnt_u32 tag);
int sfnt_validate_font_checksum(const sfnt_face *face);
int sfnt_validate_directory_bounds(const sfnt_face *face);

/* cmap: Unicode codepoint -> glyph id. */
int sfnt_select_cmap(const sfnt_face *face, sfnt_cmap *out_cmap);
int sfnt_cmap_lookup(const sfnt_face *face, const sfnt_cmap *cmap, sfnt_u32 codepoint, sfnt_u16 *out_gid);
int sfnt_lookup_glyph(const sfnt_face *face, sfnt_u32 codepoint, sfnt_u16 *out_gid);

/* Metrics and glyph table helpers. */
int sfnt_get_hmetric(const sfnt_face *face, sfnt_u16 glyph_id, sfnt_hmetric *out_metric);
int sfnt_get_glyph_data(const sfnt_face *face, sfnt_u16 glyph_id, sfnt_table_ref *out_ref);
int sfnt_get_glyph_header(const sfnt_face *face, sfnt_u16 glyph_id, sfnt_glyph_header *out_header);
int sfnt_decode_simple_glyph(const sfnt_face *face, sfnt_u16 glyph_id, sfnt_point *points, sfnt_u16 max_points, sfnt_outline *out_outline);
int sfnt_visit_compound_glyph(const sfnt_face *face, sfnt_u16 glyph_id, sfnt_component_visitor visitor, void *user);

/* name and OS/2 helpers. Name bytes are returned as raw table slices. */
sfnt_u16 sfnt_name_count(const sfnt_face *face);
int sfnt_get_name_record(const sfnt_face *face, sfnt_u16 index, sfnt_name_record *out_record);
int sfnt_get_os2_metrics(const sfnt_face *face, sfnt_os2_metrics *out_metrics);

#ifdef __cplusplus
}
#endif

#endif /* SFNT_DECODER_H */
