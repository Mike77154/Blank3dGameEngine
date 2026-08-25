#ifndef OTF_C89_H
#define OTF_C89_H

/*
  monika_otf_decoder_c89
  C89, no malloc/free/realloc/heap required by this library.
  OTF decoder for OpenType fonts with CFF 1.0 outlines ('OTTO' sfnt flavor).

  Scope:
    - sfnt / OpenType table directory
    - common tables: head, maxp, hhea, hmtx, name, OS/2, cmap
    - cmap formats: 0, 4, 6, 12 (format 14 detection only)
    - CFF 1.0 parser: Header, Name INDEX, Top DICT, String INDEX, Global Subr INDEX,
      Charset, Encoding, CharStrings INDEX, Private DICT, Local Subr INDEX
    - Type 2 charstring interpreter: path extraction to fixed-point cubic outlines

  Non-goals in this version:
    - Full OpenType Layout shaping (GSUB/GPOS) is table-presence only.
    - CFF2 variations are decoded at the default master; blend deltas are consumed but not interpolated yet.
    - Rasterization/hint execution is skipped. Hints are counted/skipped.
*/

#ifdef __cplusplus
extern "C" {
#endif

#ifndef OTF_MAX_TABLES
#define OTF_MAX_TABLES 128
#endif
#ifndef OTF_MAX_NAME_RECORDS
#define OTF_MAX_NAME_RECORDS 128
#endif
#ifndef OTF_MAX_CMAP_SUBTABLES
#define OTF_MAX_CMAP_SUBTABLES 32
#endif
#ifndef OTF_MAX_CMAP4_SEGS
#define OTF_MAX_CMAP4_SEGS 4096
#endif
#ifndef OTF_MAX_CFF_STACK
#define OTF_MAX_CFF_STACK 513
#endif
#ifndef OTF_MAX_CFF_TRANSIENT
#define OTF_MAX_CFF_TRANSIENT 96
#endif
#ifndef OTF_MAX_SUBR_DEPTH
#define OTF_MAX_SUBR_DEPTH 16
#endif
#ifndef OTF_MAX_PATH_CMDS
#define OTF_MAX_PATH_CMDS 4096
#endif
#ifndef OTF_MAX_CFF_FD
#define OTF_MAX_CFF_FD 32
#endif
#ifndef OTF_MAX_CFF_VSTORE_ITEMS
#define OTF_MAX_CFF_VSTORE_ITEMS 32
#endif
#ifndef OTF_MAX_VAR_AXES
#define OTF_MAX_VAR_AXES 16
#endif
#ifndef OTF_MAX_VAR_REGIONS
#define OTF_MAX_VAR_REGIONS 256
#endif
#ifndef OTF_MAX_ITEM_REGION_INDEXES
#define OTF_MAX_ITEM_REGION_INDEXES 64
#endif
#ifndef OTF_MAX_AVAR_MAPS_PER_AXIS
#define OTF_MAX_AVAR_MAPS_PER_AXIS 16
#endif
#ifndef OTF_MAX_IVS_DATA
#define OTF_MAX_IVS_DATA 64
#endif
#ifndef OTF_MAX_MVAR_RECORDS
#define OTF_MAX_MVAR_RECORDS 96
#endif

#define OTF_TAG(a,b,c,d) (((unsigned long)(unsigned char)(a)<<24)|((unsigned long)(unsigned char)(b)<<16)|((unsigned long)(unsigned char)(c)<<8)|((unsigned long)(unsigned char)(d)))

#define OTF_OK 0
#define OTF_ERR_BAD_ARG -1
#define OTF_ERR_RANGE -2
#define OTF_ERR_BAD_MAGIC -3
#define OTF_ERR_UNSUPPORTED -4
#define OTF_ERR_OVERFLOW -5
#define OTF_ERR_MISSING_TABLE -6
#define OTF_ERR_BAD_CFF -7
#define OTF_ERR_BAD_CHARSTRING -8

/* 16.16 signed fixed point */
typedef long otf_fixed;
#define OTF_FIXED_ONE ((otf_fixed)65536L)
#define OTF_FIXED_FROM_INT(x) ((otf_fixed)((long)(x) << 16))
#define OTF_FIXED_TO_INT(x) ((int)((x) >> 16))

typedef struct otf_buf_s {
    const unsigned char *data;
    unsigned long size;
} otf_buf;

typedef struct otf_table_s {
    unsigned long tag;
    unsigned long checksum;
    unsigned long offset;
    unsigned long length;
} otf_table;

typedef struct otf_name_record_s {
    unsigned short platform_id;
    unsigned short encoding_id;
    unsigned short language_id;
    unsigned short name_id;
    unsigned short length;
    unsigned short offset;
} otf_name_record;

typedef struct otf_cmap_subtable_s {
    unsigned short platform_id;
    unsigned short encoding_id;
    unsigned long offset;
    unsigned short format;
} otf_cmap_subtable;

typedef struct otf_cmap4_s {
    int present;
    unsigned short seg_count;
    unsigned long table_offset;
    unsigned short end_code[OTF_MAX_CMAP4_SEGS];
    unsigned short start_code[OTF_MAX_CMAP4_SEGS];
    short id_delta[OTF_MAX_CMAP4_SEGS];
    unsigned short id_range_offset[OTF_MAX_CMAP4_SEGS];
    unsigned long id_range_offset_pos[OTF_MAX_CMAP4_SEGS];
} otf_cmap4;

typedef struct otf_cmap12_group_s {
    unsigned long start_char;
    unsigned long end_char;
    unsigned long start_glyph;
} otf_cmap12_group;

typedef struct otf_cff_index_s {
    unsigned long offset;
    unsigned short count;
    unsigned char off_size;
    unsigned long offsets_base;
    unsigned long data_base;
    unsigned long end;
} otf_cff_index;

typedef struct otf_cff_top_s {
    unsigned long charstrings_off;
    unsigned long charset_off;
    unsigned long encoding_off;
    unsigned long private_size;
    unsigned long private_off;
    unsigned long fd_array_off;
    unsigned long fd_select_off;
    unsigned long variation_store_off;
    short charstring_type;
    short ros_registry_sid;
    short ros_ordering_sid;
    short ros_supplement;
    int is_cid;
} otf_cff_top;

typedef struct otf_cff_priv_s {
    unsigned long subrs_off;
    short default_width_x;
    short vsindex;
    short nominal_width_x;
} otf_cff_priv;

typedef struct otf_var_axis_s {
    unsigned long tag;
    long min_value;       /* 16.16 user coordinate */
    long default_value;   /* 16.16 user coordinate */
    long max_value;       /* 16.16 user coordinate */
    long user_value;      /* 16.16 selected user coordinate */
    long norm_value;      /* 16.16 normalized coordinate after avar */
    unsigned short flags;
    unsigned short name_id;
} otf_var_axis;

typedef struct otf_avar_map_s {
    short from_coord; /* F2DOT14 */
    short to_coord;   /* F2DOT14 */
} otf_avar_map;

typedef struct otf_var_region_s {
    short start[OTF_MAX_VAR_AXES]; /* F2DOT14 */
    short peak[OTF_MAX_VAR_AXES];
    short end[OTF_MAX_VAR_AXES];
} otf_var_region;

typedef struct otf_cff_vstore_s {
    unsigned short axis_count;
    unsigned short region_count;
    otf_var_region regions[OTF_MAX_VAR_REGIONS];
    unsigned short item_count;
    unsigned short item_region_count[OTF_MAX_CFF_VSTORE_ITEMS];
    unsigned short item_region_index[OTF_MAX_CFF_VSTORE_ITEMS][OTF_MAX_ITEM_REGION_INDEXES];
    long item_scalar[OTF_MAX_CFF_VSTORE_ITEMS][OTF_MAX_ITEM_REGION_INDEXES]; /* 16.16 */
} otf_cff_vstore;

typedef struct otf_item_var_store_s {
    int present;
    unsigned long store_base;
    unsigned short axis_count;
    unsigned short region_count;
    otf_var_region regions[OTF_MAX_VAR_REGIONS];
    long region_scalar[OTF_MAX_VAR_REGIONS]; /* 16.16 */
    unsigned short data_count;
    unsigned long data_offset[OTF_MAX_IVS_DATA];
    unsigned short item_count[OTF_MAX_IVS_DATA];
    unsigned short word_delta_count[OTF_MAX_IVS_DATA];
    unsigned short region_index_count[OTF_MAX_IVS_DATA];
    unsigned short region_index[OTF_MAX_IVS_DATA][OTF_MAX_ITEM_REGION_INDEXES];
} otf_item_var_store;

typedef struct otf_metric_var_table_s {
    int present;
    unsigned long table_offset;
    unsigned long table_length;
    unsigned long map_advance;
    unsigned long map_side1;
    unsigned long map_side2;
    unsigned long map_vorg;
    otf_item_var_store store;
} otf_metric_var_table;

typedef struct otf_mvar_record_s {
    unsigned long value_tag;
    unsigned short outer_index;
    unsigned short inner_index;
} otf_mvar_record;

typedef struct otf_mvar_s {
    int present;
    unsigned short value_record_size;
    unsigned short value_record_count;
    otf_item_var_store store;
    otf_mvar_record records[OTF_MAX_MVAR_RECORDS];
} otf_mvar;

typedef struct otf_cff_s {
    int present;
    unsigned long table_offset;
    unsigned long table_length;
    unsigned char major;
    unsigned char minor;
    unsigned char hdr_size;
    unsigned char off_size;
    otf_cff_index name_index;
    otf_cff_index top_index;
    otf_cff_index string_index;
    otf_cff_index global_subr_index;
    otf_cff_index charstrings_index;
    otf_cff_index local_subr_index;
    otf_cff_index fd_array_index;
    otf_cff_index fd_local_subr_index[OTF_MAX_CFF_FD];
    otf_cff_top top;
    otf_cff_priv priv;
    otf_cff_top fd_top[OTF_MAX_CFF_FD];
    otf_cff_priv fd_priv[OTF_MAX_CFF_FD];
    unsigned short fd_count;
    unsigned short glyph_count;
    int is_cff2;
    unsigned short cff2_top_dict_size;
    unsigned short vstore_item_count;
    unsigned short vstore_region_count[OTF_MAX_CFF_VSTORE_ITEMS];
    otf_cff_vstore vstore;
} otf_cff;

typedef struct otf_font_s {
    otf_buf buf;
    unsigned long sfnt_version;
    unsigned short num_tables;
    unsigned short units_per_em;
    short index_to_loc_format;
    short glyph_data_format;
    unsigned short num_glyphs;
    short ascender;
    short descender;
    short line_gap;
    unsigned short number_of_hmetrics;
    short v_ascender;
    short v_descender;
    short v_line_gap;
    unsigned short number_of_vmetrics;
    unsigned short os2_version;
    short os2_typo_ascender;
    short os2_typo_descender;
    short os2_typo_line_gap;
    unsigned short weight_class;
    unsigned short width_class;
    otf_table tables[OTF_MAX_TABLES];
    unsigned short var_axis_count;
    otf_var_axis var_axes[OTF_MAX_VAR_AXES];
    unsigned short avar_map_count[OTF_MAX_VAR_AXES];
    otf_avar_map avar_maps[OTF_MAX_VAR_AXES][OTF_MAX_AVAR_MAPS_PER_AXIS];
    unsigned short name_count;
    unsigned long name_storage_offset;
    otf_name_record names[OTF_MAX_NAME_RECORDS];
    unsigned short cmap_count;
    otf_cmap_subtable cmaps[OTF_MAX_CMAP_SUBTABLES];
    otf_cmap4 cmap4;
    unsigned long cmap12_offset;
    unsigned long cmap12_groups;
    otf_cff cff;
    otf_metric_var_table hvar;
    otf_metric_var_table vvar;
    otf_mvar mvar;
} otf_font;

typedef enum otf_path_op_e {
    OTF_PATH_MOVE = 1,
    OTF_PATH_LINE = 2,
    OTF_PATH_CUBIC = 3,
    OTF_PATH_CLOSE = 4
} otf_path_op;

typedef struct otf_path_cmd_s {
    unsigned char op;
    otf_fixed x1, y1, x2, y2, x3, y3;
} otf_path_cmd;

typedef struct otf_glyph_path_s {
    otf_path_cmd cmds[OTF_MAX_PATH_CMDS];
    unsigned short count;
    short advance_width;
    short left_side_bearing;
    short advance_height;
    short top_side_bearing;
    short nominal_width;
    int overflow;
} otf_glyph_path;

int otf_parse(otf_font *font, const unsigned char *data, unsigned long size);
const otf_table *otf_find_table(const otf_font *font, unsigned long tag);
int otf_is_otf_cff(const otf_font *font);
int otf_is_otf_cff2(const otf_font *font);
int otf_has_table(const otf_font *font, unsigned long tag);
unsigned short otf_glyph_index_for_codepoint(const otf_font *font, unsigned long cp);
int otf_get_hmetric(const otf_font *font, unsigned short gid, unsigned short *advance, short *lsb);
int otf_get_hmetric_var(const otf_font *font, unsigned short gid, unsigned short *advance, short *lsb);
int otf_get_vmetric(const otf_font *font, unsigned short gid, unsigned short *advance_height, short *tsb);
int otf_get_vmetric_var(const otf_font *font, unsigned short gid, unsigned short *advance_height, short *tsb);
int otf_get_vorg_var(const otf_font *font, unsigned short gid, short default_y_origin, short *y_origin);
int otf_get_mvar_delta(const otf_font *font, unsigned long value_tag, long *delta_out);
int otf_get_name_ascii(const otf_font *font, unsigned short name_id, char *out, unsigned short out_cap);
int otf_decode_glyph_path(const otf_font *font, unsigned short gid, otf_glyph_path *path);
int otf_set_variation_axis(otf_font *font, unsigned long axis_tag, long user_value_16_16);
int otf_set_variation_axis_int(otf_font *font, unsigned long axis_tag, long user_value_integer);
void otf_reset_variations(otf_font *font);
int otf_update_cff2_variation_scalars(otf_font *font);
const char *otf_errstr(int code);

#ifdef __cplusplus
}
#endif

#endif
