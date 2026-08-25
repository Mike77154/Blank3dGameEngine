#ifndef PSD89_H
#define PSD89_H

#ifdef __cplusplus
extern "C" {
#endif

#include <limits.h>

#if UCHAR_MAX != 0xFFU
#error "psd89 requires 8-bit unsigned char"
#endif

#if USHRT_MAX != 0xFFFFU
#error "psd89 requires 16-bit unsigned short"
#endif

#if UINT_MAX != 0xFFFFFFFFU
#error "psd89 requires a 32-bit unsigned int"
#endif
typedef unsigned int psd89_u32;
typedef signed int   psd89_s32;

typedef unsigned char  psd89_u8;
typedef signed char    psd89_s8;
typedef unsigned short psd89_u16;
typedef signed short   psd89_s16;

typedef psd89_s32 psd89_fx16;
typedef psd89_s32 psd89_fx24;

#define PSD89_FX16_ONE ((psd89_fx16)0x00010000)
#define PSD89_FX24_ONE ((psd89_fx24)0x01000000)

#ifndef PSD89_MAX_LAYERS
#define PSD89_MAX_LAYERS 64
#endif

#ifndef PSD89_MAX_CHANNELS_PER_LAYER
#define PSD89_MAX_CHANNELS_PER_LAYER 8
#endif

#ifndef PSD89_MAX_IMAGE_RESOURCES
#define PSD89_MAX_IMAGE_RESOURCES 64
#endif

#ifndef PSD89_MAX_TAG_BLOCKS_PER_LAYER
#define PSD89_MAX_TAG_BLOCKS_PER_LAYER 16
#endif

#ifndef PSD89_MAX_GLOBAL_TAG_BLOCKS
#define PSD89_MAX_GLOBAL_TAG_BLOCKS 32
#endif

#ifndef PSD89_MAX_NAME_CHARS
#define PSD89_MAX_NAME_CHARS 63
#endif

#ifndef PSD89_MAX_RESOURCE_NAME_CHARS
#define PSD89_MAX_RESOURCE_NAME_CHARS 63
#endif

#define PSD89_MAX_BASE_CHANNELS 3

#ifndef PSD89_MAX_VECTOR_SUBPATHS
#define PSD89_MAX_VECTOR_SUBPATHS 8
#endif

#ifndef PSD89_MAX_VECTOR_KNOTS
#define PSD89_MAX_VECTOR_KNOTS 64
#endif

#ifndef PSD89_MAX_COMPOSE_ROW_BYTES
#define PSD89_MAX_COMPOSE_ROW_BYTES 30000U
#endif

#ifndef PSD89_MAX_VECTOR_SEGMENTS
#define PSD89_MAX_VECTOR_SEGMENTS 2048
#endif

#ifndef PSD89_MAX_GROUP_DEPTH
#define PSD89_MAX_GROUP_DEPTH 8
#endif

#ifndef PSD89_MAX_DESCRIPTOR_ITEMS
#define PSD89_MAX_DESCRIPTOR_ITEMS 16
#endif

#ifndef PSD89_MAX_DESCRIPTOR_NAME_CHARS
#define PSD89_MAX_DESCRIPTOR_NAME_CHARS 63
#endif

#ifndef PSD89_MAX_LRFX_COLOR_COMPONENTS
#define PSD89_MAX_LRFX_COLOR_COMPONENTS 5
#endif

#define PSD89_SIG_8BPS 0x38425053U
#define PSD89_SIG_8BIM 0x3842494DU

#define PSD89_FALSE 0
#define PSD89_TRUE  1

enum {
    PSD89_OK = 0,
    PSD89_E_IO = -1,
    PSD89_E_BAD_SIGNATURE = -2,
    PSD89_E_BAD_VERSION = -3,
    PSD89_E_BAD_RESERVED = -4,
    PSD89_E_UNSUPPORTED_DEPTH = -5,
    PSD89_E_UNSUPPORTED_MODE = -6,
    PSD89_E_UNSUPPORTED_CHANNELS = -7,
    PSD89_E_UNSUPPORTED_COMPRESSION = -8,
    PSD89_E_TRUNCATED = -9,
    PSD89_E_LIMIT = -10,
    PSD89_E_OVERFLOW = -11,
    PSD89_E_BAD_RESOURCE = -12,
    PSD89_E_BAD_LAYER = -13,
    PSD89_E_BAD_ARGUMENT = -14,
    PSD89_E_ROW_TOO_LARGE = -15,
    PSD89_E_BAD_STATE = -16,
    PSD89_E_RLE = -17
};

enum {
    PSD89_MODE_BITMAP = 0,
    PSD89_MODE_GRAYSCALE = 1,
    PSD89_MODE_INDEXED = 2,
    PSD89_MODE_RGB = 3,
    PSD89_MODE_CMYK = 4,
    PSD89_MODE_MULTICHANNEL = 7,
    PSD89_MODE_DUOTONE = 8,
    PSD89_MODE_LAB = 9
};

enum {
    PSD89_COMP_RAW = 0,
    PSD89_COMP_RLE = 1,
    PSD89_COMP_ZIP = 2,
    PSD89_COMP_ZIP_PRED = 3
};

enum {
    PSD89_CH_RED = 0,
    PSD89_CH_GREEN = 1,
    PSD89_CH_BLUE = 2,
    PSD89_CH_ALPHA = -1,
    PSD89_CH_LAYER_MASK = -2,
    PSD89_CH_REAL_LAYER_MASK = -3
};

typedef struct psd89_io {
    int (*read)(void *user, void *dst, psd89_u32 size);
    int (*write)(void *user, const void *src, psd89_u32 size);
    int (*seek)(void *user, psd89_u32 offset);
    psd89_u32 (*tell)(void *user);
    void *user;
} psd89_io;

typedef struct psd89_memio {
    psd89_u8 *data;
    psd89_u32 size;
    psd89_u32 capacity;
    psd89_u32 pos;
    int writable;
} psd89_memio;

typedef struct psd89_resource_ref {
    psd89_u16 id;
    char name[PSD89_MAX_RESOURCE_NAME_CHARS + 1];
    psd89_u32 raw_offset;
    psd89_u32 raw_size;
} psd89_resource_ref;

typedef struct psd89_tag_ref {
    char key[4];
    psd89_u32 raw_offset;
    psd89_u32 raw_size;
} psd89_tag_ref;

typedef struct psd89_layer_channel {
    psd89_s16 id;
    psd89_u16 compression;
    psd89_u32 data_length;
    psd89_u32 data_offset;
    const psd89_u8 *plane;
    psd89_u32 stride;
} psd89_layer_channel;

typedef struct psd89_layer_mask_ref {
    int present;
    psd89_u32 raw_offset;
    psd89_u32 raw_size;
} psd89_layer_mask_ref;

typedef struct psd89_layer_user_mask {
    psd89_u8 present;
    psd89_s32 top;
    psd89_s32 left;
    psd89_s32 bottom;
    psd89_s32 right;
    psd89_u8 default_color;
    psd89_u8 flags;
    psd89_u8 params_flags;
    psd89_u8 user_density_present;
    psd89_u8 user_density;
    psd89_u8 user_feather_present;
    psd89_fx16 user_feather;
    psd89_u8 vector_density_present;
    psd89_u8 vector_density;
    psd89_u8 vector_feather_present;
    psd89_fx16 vector_feather;
    psd89_u8 real_present;
    psd89_u8 real_flags;
    psd89_u8 real_background;
    psd89_s32 real_top;
    psd89_s32 real_left;
    psd89_s32 real_bottom;
    psd89_s32 real_right;
} psd89_layer_user_mask;

typedef struct psd89_vector_knot {
    psd89_u8 linked;
    psd89_fx24 preceding_v;
    psd89_fx24 preceding_h;
    psd89_fx24 anchor_v;
    psd89_fx24 anchor_h;
    psd89_fx24 leaving_v;
    psd89_fx24 leaving_h;
} psd89_vector_knot;

typedef struct psd89_vector_subpath {
    psd89_u8 closed;
    psd89_u16 knot_count;
    psd89_u16 first_knot;
} psd89_vector_subpath;

typedef struct psd89_vector_mask {
    psd89_u8 present;
    psd89_u8 use_vsms;
    psd89_u8 version;
    psd89_u8 flags;
    psd89_u8 invert;
    psd89_u8 not_link;
    psd89_u8 disabled;
    psd89_u8 path_fill_rule_present;
    psd89_u8 initial_fill_rule_present;
    psd89_u8 initial_fill_rule;
    psd89_u16 subpath_count;
    psd89_u16 knot_count;
    psd89_vector_subpath subpaths[PSD89_MAX_VECTOR_SUBPATHS];
    psd89_vector_knot knots[PSD89_MAX_VECTOR_KNOTS];
} psd89_vector_mask;

typedef struct psd89_descriptor_id {
    psd89_u8 is_fourcc;
    char fourcc[4];
    char text[PSD89_MAX_DESCRIPTOR_NAME_CHARS + 1];
} psd89_descriptor_id;

typedef struct psd89_descriptor_item {
    psd89_descriptor_id key;
    char type[4];
    psd89_u8 bool_valid;
    psd89_u8 bool_value;
    psd89_u8 int_valid;
    psd89_s32 int_value;
    psd89_u8 comp_valid;
    psd89_u32 comp_hi;
    psd89_u32 comp_lo;
    psd89_u8 fx_valid;
    psd89_fx16 fx_value;
    psd89_u8 unit_valid;
    char unit[4];
    psd89_u8 text_valid;
    char text[PSD89_MAX_DESCRIPTOR_NAME_CHARS + 1];
    psd89_u8 aux_text_valid;
    char aux_text[PSD89_MAX_DESCRIPTOR_NAME_CHARS + 1];
    psd89_u8 length_valid;
    psd89_u32 data_length;
    psd89_u8 list_count_valid;
    psd89_u32 list_count;
    psd89_u8 ref_count_valid;
    psd89_u32 ref_count;
    psd89_u8 nested_valid;
    psd89_u32 nested_item_count;
} psd89_descriptor_item;

typedef struct psd89_descriptor_summary {
    psd89_u8 parsed;
    psd89_u8 max_depth_seen;
    char name[PSD89_MAX_DESCRIPTOR_NAME_CHARS + 1];
    psd89_descriptor_id class_id;
    psd89_u32 item_count;
    psd89_u16 parsed_item_count;
    psd89_u16 parsed_list_count;
    psd89_u16 parsed_ref_count;
    psd89_u16 parsed_nested_count;
    psd89_descriptor_item items[PSD89_MAX_DESCRIPTOR_ITEMS];
} psd89_descriptor_summary;

typedef struct psd89_descriptor_blob {
    psd89_u32 version;
    const psd89_u8 *authored_raw;
    psd89_u32 authored_raw_size;
    psd89_descriptor_summary summary;
} psd89_descriptor_blob;

typedef struct psd89_lrfx_shadow {
    psd89_u8 present;
    psd89_u8 enabled;
    psd89_u8 use_global_angle;
    psd89_u8 native_color_present;
    psd89_u32 size;
    psd89_u32 version;
    psd89_s32 blur;
    psd89_s32 intensity;
    psd89_s32 angle;
    psd89_s32 distance;
    psd89_u16 color[PSD89_MAX_LRFX_COLOR_COMPONENTS];
    char blend_mode[4];
    psd89_u8 opacity;
    psd89_u16 native_color[PSD89_MAX_LRFX_COLOR_COMPONENTS];
} psd89_lrfx_shadow;

typedef struct psd89_lrfx_glow {
    psd89_u8 present;
    psd89_u8 enabled;
    psd89_u8 invert;
    psd89_u8 native_color_present;
    psd89_u32 size;
    psd89_u32 version;
    psd89_s32 blur;
    psd89_s32 intensity;
    psd89_u16 color[PSD89_MAX_LRFX_COLOR_COMPONENTS];
    char blend_mode[4];
    psd89_u8 opacity;
    psd89_u16 native_color[PSD89_MAX_LRFX_COLOR_COMPONENTS];
} psd89_lrfx_glow;

typedef struct psd89_lrfx_bevel {
    psd89_u8 present;
    psd89_u8 enabled;
    psd89_u8 use_global_angle;
    psd89_u8 up;
    psd89_u8 bevel_style;
    psd89_u8 real_colors_present;
    psd89_u32 size;
    psd89_u32 version;
    psd89_s32 angle;
    psd89_s32 strength;
    psd89_s32 blur;
    char highlight_blend_mode[4];
    char shadow_blend_mode[4];
    psd89_u16 highlight_color[PSD89_MAX_LRFX_COLOR_COMPONENTS];
    psd89_u16 shadow_color[PSD89_MAX_LRFX_COLOR_COMPONENTS];
    psd89_u8 highlight_opacity;
    psd89_u8 shadow_opacity;
    psd89_u16 real_highlight_color[PSD89_MAX_LRFX_COLOR_COMPONENTS];
    psd89_u16 real_shadow_color[PSD89_MAX_LRFX_COLOR_COMPONENTS];
} psd89_lrfx_bevel;

typedef struct psd89_lrfx_solid_fill {
    psd89_u8 present;
    psd89_u8 enabled;
    psd89_u32 size;
    psd89_u32 version;
    char blend_mode[4];
    psd89_u16 color[PSD89_MAX_LRFX_COLOR_COMPONENTS];
    psd89_u16 native_color[PSD89_MAX_LRFX_COLOR_COMPONENTS];
    psd89_u8 opacity;
} psd89_lrfx_solid_fill;

typedef struct psd89_lrfx {
    psd89_u8 present;
    psd89_u16 version;
    psd89_u16 effect_count;
    psd89_u8 common_state_present;
    psd89_u8 common_visible;
    psd89_u8 drop_shadow_present;
    psd89_u8 inner_shadow_present;
    psd89_u8 outer_glow_present;
    psd89_u8 inner_glow_present;
    psd89_u8 bevel_present;
    psd89_lrfx_shadow drop_shadow;
    psd89_lrfx_shadow inner_shadow;
    psd89_lrfx_glow outer_glow;
    psd89_lrfx_glow inner_glow;
    psd89_lrfx_bevel bevel;
    psd89_lrfx_solid_fill solid_fill;
} psd89_lrfx;

typedef struct psd89_type_tool {
    psd89_u8 present;
    psd89_u16 version;
    psd89_fx16 transform[6];
    psd89_u16 text_version;
    psd89_u32 text_descriptor_version;
    psd89_descriptor_blob text;
    psd89_u16 warp_version;
    psd89_u32 warp_descriptor_version;
    psd89_descriptor_blob warp;
    psd89_fx16 bounds[4];
} psd89_type_tool;

typedef struct psd89_text_engine_data {
    psd89_u8 present;
    psd89_u32 raw_offset;
    psd89_u32 raw_size;
    const psd89_u8 *authored_raw;
    psd89_u32 authored_raw_size;
} psd89_text_engine_data;

typedef struct psd89_object_effects {
    psd89_u8 present;
    psd89_u32 object_version;
    psd89_u32 descriptor_version;
    psd89_descriptor_blob descriptor;
} psd89_object_effects;

typedef struct psd89_smart_object {
    psd89_u8 present;
    char tag_key[4];
    char type[4];
    psd89_u32 version;
    psd89_u32 descriptor_version;
    psd89_descriptor_blob descriptor;
    psd89_u8 placed_info_present;
    char unique_id[PSD89_MAX_DESCRIPTOR_NAME_CHARS + 1];
    psd89_u32 page_number;
    psd89_u32 total_pages;
    psd89_u32 anti_alias_policy;
    psd89_u32 placed_layer_type;
    psd89_fx16 transform[8];
    psd89_u32 warp_version;
    psd89_u32 warp_descriptor_version;
    psd89_descriptor_blob warp_descriptor;
} psd89_smart_object;

typedef struct psd89_layer_blend_ranges_ref {
    psd89_u32 raw_offset;
    psd89_u32 raw_size;
} psd89_layer_blend_ranges_ref;

typedef struct psd89_layer {
    psd89_s32 top;
    psd89_s32 left;
    psd89_s32 bottom;
    psd89_s32 right;
    psd89_u16 channel_count;
    psd89_layer_channel channels[PSD89_MAX_CHANNELS_PER_LAYER];
    char blend_mode[4];
    psd89_u8 opacity;
    psd89_u8 clipping;
    psd89_u8 flags;
    psd89_u8 blend_clipped;
    psd89_u8 blend_clipped_present;
    psd89_u8 blend_interior;
    psd89_u8 blend_interior_present;
    psd89_u8 knockout;
    psd89_u8 knockout_present;
    psd89_u8 transparency_shapes_layer;
    psd89_u8 transparency_shapes_layer_present;
    psd89_u8 section_divider_type;
    psd89_u8 section_divider_present;
    psd89_u8 section_divider_subtype;
    psd89_u8 section_divider_subtype_present;
    char section_divider_blend_mode[4];
    char name[PSD89_MAX_NAME_CHARS + 1];
    psd89_layer_mask_ref mask_ref;
    psd89_layer_user_mask user_mask;
    psd89_vector_mask vector_mask;
    psd89_u8 layer_mask_global_present;
    psd89_u8 layer_mask_global;
    psd89_u8 vector_mask_global_present;
    psd89_u8 vector_mask_global;
    psd89_layer_blend_ranges_ref blend_ranges_ref;
    psd89_lrfx lrfx;
    psd89_type_tool type_tool;
    psd89_text_engine_data text_engine;
    psd89_object_effects object_effects;
    psd89_smart_object smart_object;
    psd89_u16 tag_count;
    psd89_tag_ref tags[PSD89_MAX_TAG_BLOCKS_PER_LAYER];
} psd89_layer;

typedef struct psd89_doc {
    psd89_u16 channels;
    psd89_u32 height;
    psd89_u32 width;
    psd89_u16 depth;
    psd89_u16 color_mode;
    int merged_alpha_in_first_channel;

    psd89_u16 layer_count;
    psd89_layer layers[PSD89_MAX_LAYERS];

    psd89_u16 resource_count;
    psd89_resource_ref resources[PSD89_MAX_IMAGE_RESOURCES];

    psd89_u16 global_tag_count;
    psd89_tag_ref global_tags[PSD89_MAX_GLOBAL_TAG_BLOCKS];

    psd89_u32 global_mask_raw_offset;
    psd89_u32 global_mask_raw_size;

    psd89_u32 composite_data_offset;
    psd89_u16 composite_compression;

    const psd89_u8 *composite_planes[56];
    psd89_u32 composite_stride;
    psd89_u16 composite_write_compression;

    int has_passthrough_source;
    psd89_io passthrough_source;
} psd89_doc;

typedef struct psd89_compose_options {
    int clear_output;
    int respect_hidden_layers;
    int layer_order_bottom_to_top;
    psd89_u8 backdrop_alpha;
    psd89_u8 backdrop_values[PSD89_MAX_BASE_CHANNELS];
} psd89_compose_options;

void psd89_doc_init(psd89_doc *doc);
const char *psd89_error_string(int code);

void psd89_descriptor_summary_init(psd89_descriptor_summary *summary);

psd89_fx16 psd89_fx16_from_int(int v);
int psd89_fx16_to_int(psd89_fx16 v);
psd89_fx16 psd89_fx16_mul(psd89_fx16 a, psd89_fx16 b);
psd89_fx16 psd89_fx16_div(psd89_fx16 a, psd89_fx16 b);
psd89_fx24 psd89_fx24_from_int(int v);
int psd89_fx24_to_int(psd89_fx24 v);
psd89_fx24 psd89_fx24_mul(psd89_fx24 a, psd89_fx24 b);

void psd89_memio_init_read(psd89_memio *m, const void *data, psd89_u32 size);
void psd89_memio_init_write(psd89_memio *m, void *data, psd89_u32 capacity);
void psd89_memio_make_io(psd89_memio *m, psd89_io *io);

int psd89_read(psd89_doc *doc, psd89_io *io);
int psd89_decode_composite_u8(const psd89_doc *doc, psd89_io *io, psd89_u8 **planes, psd89_u32 stride);
int psd89_decode_layer_channel_u8(const psd89_doc *doc, psd89_io *io, unsigned int layer_index, psd89_s16 channel_id, psd89_u8 *dst, psd89_u32 stride);

void psd89_compose_options_init(psd89_compose_options *opt);
psd89_u32 psd89_compose_max_row_bytes(const psd89_doc *doc);
int psd89_compose_layers_u8(const psd89_doc *doc,
                            psd89_io *io,
                            psd89_u8 **dst_color,
                            psd89_u8 *dst_alpha,
                            psd89_u32 dst_stride,
                            psd89_u8 **scratch_color,
                            psd89_u8 *scratch_alpha,
                            psd89_u32 scratch_stride,
                            const psd89_compose_options *opt);

typedef struct psd89_vector_segment {
    psd89_fx16 x0;
    psd89_fx16 y0;
    psd89_fx16 x1;
    psd89_fx16 y1;
} psd89_vector_segment;

typedef struct psd89_vector_flatten_options {
    psd89_fx16 flatness;
    psd89_u16 max_depth;
    int honor_handles;
} psd89_vector_flatten_options;

typedef struct psd89_vector_flatten_result {
    psd89_u16 segment_count;
    psd89_vector_segment segments[PSD89_MAX_VECTOR_SEGMENTS];
} psd89_vector_flatten_result;

enum {
    PSD89_ZIP_DIAG_OK = 0,
    PSD89_ZIP_DIAG_ZLIB_STATUS = -200,
    PSD89_ZIP_DIAG_BAD_PLANAR_SIZE = -201,
    PSD89_ZIP_DIAG_BAD_PREDICTED_SIZE = -202,
    PSD89_ZIP_DIAG_STREAM_MISMATCH = -203
};

typedef struct psd89_zip_diag {
    int code;
    int zlib_status;
    psd89_u16 compression;
    psd89_u32 rows;
    psd89_u32 cols;
    psd89_u16 channels;
    psd89_u32 compressed_bytes;
    psd89_u32 decoded_bytes;
    psd89_u32 expected_bytes;
} psd89_zip_diag;

void psd89_vector_flatten_options_init(psd89_vector_flatten_options *opt);
void psd89_vector_flatten_result_init(psd89_vector_flatten_result *out);
int psd89_vector_flatten_mask(psd89_u32 doc_width,
                              psd89_u32 doc_height,
                              const psd89_vector_mask *mask,
                              const psd89_vector_flatten_options *opt,
                              psd89_vector_flatten_result *out);
int psd89_vector_contains_point_px(const psd89_vector_mask *mask,
                                   const psd89_vector_flatten_result *flat,
                                   psd89_s32 px,
                                   psd89_s32 py);
psd89_u8 psd89_vector_coverage_u8(const psd89_vector_mask *mask,
                                  const psd89_vector_flatten_result *flat,
                                  psd89_s32 px,
                                  psd89_s32 py);

int psd89_mask_global_bool_parse(const psd89_u8 raw[4], psd89_u8 *value);
void psd89_mask_global_bool_write(psd89_u8 raw[4], psd89_u8 value);

void psd89_zip_diag_init(psd89_zip_diag *diag);
psd89_u32 psd89_zip_expected_planar_bytes(psd89_u32 rows,
                                              psd89_u32 cols,
                                              psd89_u16 channels);
int psd89_zip_diag_check_planar(psd89_u16 compression,
                                psd89_u32 rows,
                                psd89_u32 cols,
                                psd89_u16 channels,
                                psd89_u32 compressed_bytes,
                                psd89_u32 decoded_bytes,
                                int zlib_status,
                                psd89_zip_diag *diag);
const char *psd89_zip_diag_string(int code);

int psd89_write(psd89_io *io, const psd89_doc *doc);

#ifdef __cplusplus
}
#endif

#endif
