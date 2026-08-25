#ifndef GAFNYF_TGA_H
#define GAFNYF_TGA_H

/*
    gafnyf_tga.h - tiny TGA encoder/decoder for C89, no malloc, CC0-1.0.

    Core support:
      - decode image types 1, 2, 3, 9, 10, 11
      - encode grayscale, truecolor, and indexed TGA
      - raw and RLE image data
      - top/bottom and left/right origin bits
      - caller-owned buffers only
      - no floating point

    extended features:
      - TGA 2.0 footer reader
      - TGA 2.0 extension-area reader
      - scan-line table reader/writer
      - developer-directory and developer-field reader/writer
      - postage-stamp reader/decoder + optional auto/custom writer
      - color-correction table reader/writer
      - layout/section inspection helpers
      - RLE packet table readers
      - row-range decode helpers for giant images
      - rectangle decode helpers for giant images
      - exact encode-size measurement helpers
      - direct decode-to-channel-mask helpers
      - custom swizzle/deinterleave helpers
      - utility developer-tag payload builders/parsers
      - indexed encode with 16/24/32-bit on-disk palette entries
      - deeper forensic validation for extension/developer payloads
      - extension semantics helpers (alpha type, gamma, pixel aspect)
      - metadata patch helpers for existing TGA 2.0 files
      - retro-friendly packed 16-bit memory pixel formats + helpers
      - repair/canonicalize helpers for TGA 2.0 metadata remuxing
      - optional stdio FILE* helpers
      - optional encoder path that writes TGA 2.0 metadata
*/

#include <stddef.h>

#ifndef GAFNYF_TGA_NO_STDIO
#include <stdio.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define TGA_VERSION_MAJOR 3
#define TGA_VERSION_MINOR 0
#define TGA_VERSION_PATCH 0

typedef unsigned char  tga_u8;
typedef unsigned short tga_u16;
typedef unsigned long  tga_u32;

typedef size_t (*tga_read_fn)(void *user, void *dst, size_t bytes);
typedef size_t (*tga_write_fn)(void *user, const void *src, size_t bytes);

typedef struct tga_reader {
    tga_read_fn read;
    void *user;
} tga_reader;

typedef struct tga_writer {
    tga_write_fn write;
    void *user;
} tga_writer;

typedef enum tga_result {
    TGA_OK = 0,
    TGA_ERR_BAD_ARGUMENT = -1,
    TGA_ERR_IO = -2,
    TGA_ERR_BAD_MAGIC = -3,
    TGA_ERR_UNSUPPORTED = -4,
    TGA_ERR_TRUNCATED = -5,
    TGA_ERR_BAD_FORMAT = -6,
    TGA_ERR_OVERFLOW = -7,
    TGA_ERR_NEED_PALETTE_BUFFER = -8,
    TGA_ERR_PALETTE_RANGE = -9,
    TGA_ERR_CAPACITY = -10,
    TGA_ERR_NO_FOOTER = -11,
    TGA_ERR_NO_EXTENSION = -12,
    TGA_ERR_NO_DEVELOPER_AREA = -13,
    TGA_ERR_TOO_MANY_TAGS = -14,
    TGA_ERR_NOT_FOUND = -15,
    TGA_ERR_NO_SCAN_LINE_TABLE = -16,
    TGA_ERR_NO_POSTAGE_STAMP = -17,
    TGA_ERR_NO_COLOR_CORRECTION_TABLE = -18,
    TGA_ERR_INDEX_RANGE = -19
} tga_result;

typedef enum tga_pixel_format {
    TGA_PIXFMT_GRAY8 = 1,
    TGA_PIXFMT_RGB24 = 2,
    TGA_PIXFMT_BGR24 = 3,
    TGA_PIXFMT_RGBA32 = 4,
    TGA_PIXFMT_BGRA32 = 5,
    TGA_PIXFMT_INDEX8 = 6,
    TGA_PIXFMT_RGB565 = 7,
    TGA_PIXFMT_ARGB1555 = 8
} tga_pixel_format;

typedef struct tga_info {
    unsigned id_length;
    unsigned color_map_type;
    unsigned image_type;
    unsigned color_map_origin;
    unsigned color_map_length;
    unsigned color_map_entry_bits;
    unsigned x_origin;
    unsigned y_origin;
    unsigned width;
    unsigned height;
    unsigned pixel_depth;
    unsigned descriptor;
    int is_rle;
    int is_color_mapped;
    int is_grayscale;
    int file_top_origin;
    int file_right_to_left;
    int has_footer;
} tga_info;

typedef struct tga_footer {
    int present;
    tga_u32 extension_offset;
    tga_u32 developer_offset;
} tga_footer;

typedef struct tga_extension {
    unsigned size;
    char author_name[41];
    char author_comment[325];
    unsigned stamp_month;
    unsigned stamp_day;
    unsigned stamp_year;
    unsigned stamp_hour;
    unsigned stamp_minute;
    unsigned stamp_second;
    char job_name[41];
    unsigned job_hour;
    unsigned job_minute;
    unsigned job_second;
    char software_id[41];
    unsigned software_version_number;
    unsigned software_version_letter;
    tga_u32 key_color;
    unsigned pixel_numerator;
    unsigned pixel_denominator;
    unsigned gamma_numerator;
    unsigned gamma_denominator;
    tga_u32 color_correction_offset;
    tga_u32 postage_stamp_offset;
    tga_u32 scan_line_offset;
    unsigned attributes_type;
} tga_extension;

#define TGA_ATTRIBUTES_TYPE_NONE               0u
#define TGA_ATTRIBUTES_TYPE_UNDEFINED_IGNORE   1u
#define TGA_ATTRIBUTES_TYPE_UNDEFINED_RETAIN   2u
#define TGA_ATTRIBUTES_TYPE_ALPHA              3u
#define TGA_ATTRIBUTES_TYPE_PREMULTIPLIED      4u

typedef struct tga_developer_tag {
    unsigned tag;
    tga_u32 data_offset;
    tga_u32 data_size;
} tga_developer_tag;

typedef struct tga_developer_field {
    unsigned tag;
    const void *data;
    tga_u32 data_size;
} tga_developer_field;

#define TGA_COLOR_CORRECTION_ENTRY_COUNT 256u

typedef struct tga_color_correction_entry {
    tga_u16 alpha;
    tga_u16 red;
    tga_u16 green;
    tga_u16 blue;
} tga_color_correction_entry;

typedef struct tga_postage_stamp_info {
    unsigned width;
    unsigned height;
    unsigned pixel_depth;
    unsigned sample_bytes;
    size_t data_size;
    size_t block_size;
} tga_postage_stamp_info;

typedef struct tga_postage_stamp_source {
    const void *pixels;
    unsigned width;
    unsigned height;
    unsigned stride_bytes;
    tga_pixel_format pixel_format;
    int input_top_origin;
} tga_postage_stamp_source;

typedef struct tga_inspect {
    tga_info info;
    tga_footer footer;
    tga_extension extension;
    tga_postage_stamp_info postage_stamp_info;

    size_t file_size;

    size_t image_id_offset;
    size_t image_id_size;

    size_t color_map_offset;
    size_t color_map_size;

    size_t image_data_offset;
    size_t image_data_size;

    size_t extension_area_offset;
    size_t extension_area_size;

    size_t scan_line_table_offset;
    size_t scan_line_table_size;

    size_t postage_stamp_offset;
    size_t postage_stamp_size;

    size_t color_correction_offset;
    size_t color_correction_size;

    size_t developer_directory_offset;
    size_t developer_directory_size;

    size_t developer_payload_offset;
    size_t developer_payload_size;

    size_t footer_offset;
    size_t footer_size;

    unsigned developer_tag_count;

    int is_tga2;
    int has_extension;
    int has_scan_line_table;
    int has_postage_stamp;
    int has_color_correction_table;
    int has_developer_directory;
    int has_developer_payload;
} tga_inspect;

typedef struct tga_packet_info {
    size_t header_offset;
    size_t payload_offset;
    size_t payload_size;
    size_t packet_size;
    unsigned packet_index;
    unsigned pixel_count;
    unsigned first_pixel_index;
    unsigned first_saved_row;
    unsigned first_saved_col;
    unsigned last_saved_row;
    unsigned last_saved_col;
    int is_rle;
    int crosses_scanline;
} tga_packet_info;

typedef enum tga_index_format {
    TGA_INDEX_U8 = 1,
    TGA_INDEX_U16 = 2
} tga_index_format;

typedef struct tga_palette_info {
    unsigned first_index;
    unsigned entry_count;
    unsigned entry_bits;
} tga_palette_info;

typedef struct tga_decode_indexed_params {
    void *indices;
    unsigned stride_bytes;
    int output_top_origin;
    int normalize_indices;
    tga_index_format index_format;
    tga_u8 *palette_rgba;
    unsigned palette_capacity;
} tga_decode_indexed_params;

typedef enum tga_section_kind {
    TGA_SECTION_HEADER = 1,
    TGA_SECTION_IMAGE_ID = 2,
    TGA_SECTION_COLOR_MAP = 3,
    TGA_SECTION_IMAGE_DATA = 4,
    TGA_SECTION_EXTENSION_AREA = 5,
    TGA_SECTION_COLOR_CORRECTION_TABLE = 6,
    TGA_SECTION_POSTAGE_STAMP = 7,
    TGA_SECTION_SCAN_LINE_TABLE = 8,
    TGA_SECTION_DEVELOPER_DIRECTORY = 9,
    TGA_SECTION_DEVELOPER_FIELD = 10,
    TGA_SECTION_FOOTER = 11
} tga_section_kind;

typedef struct tga_section {
    unsigned kind;
    unsigned tag;
    size_t offset;
    size_t size;
} tga_section;

#define TGA_FORENSIC_BLOCK_OVERLAP                0x0001u
#define TGA_FORENSIC_BLOCK_GAP                    0x0002u
#define TGA_FORENSIC_DUPLICATE_TAG                0x0004u
#define TGA_FORENSIC_ZERO_SIZE_TAG                0x0008u
#define TGA_FORENSIC_SCANLINE_BACKWARD            0x0010u
#define TGA_FORENSIC_SCANLINE_OUT_OF_IMAGE        0x0020u
#define TGA_FORENSIC_RLE_CROSSES_SCANLINE         0x0040u
#define TGA_FORENSIC_TRAILING_BYTES               0x0080u
#define TGA_FORENSIC_EXTENSION_SIZE_MISMATCH      0x0100u
#define TGA_FORENSIC_BAD_COLOR_CORRECTION_REF     0x0200u
#define TGA_FORENSIC_BAD_POSTAGE_STAMP_REF        0x0400u
#define TGA_FORENSIC_BAD_SCAN_LINE_REF            0x0800u
#define TGA_FORENSIC_DEVELOPER_FIELD_OUT_OF_FILE  0x1000u
#define TGA_FORENSIC_DEVELOPER_FIELD_OVERLAP      0x2000u
#define TGA_FORENSIC_UTILITY_PAYLOAD_MISMATCH     0x4000u
#define TGA_FORENSIC_UTILITY_TEXT_NOT_TERMINATED  0x8000u
#define TGA_FORENSIC_BAD_ATTRIBUTES_TYPE          0x10000u
#define TGA_FORENSIC_ATTRIBUTE_TYPE_MISMATCH      0x20000u
#define TGA_FORENSIC_BAD_TIMESTAMP                0x40000u
#define TGA_FORENSIC_BAD_JOB_TIME                 0x80000u
#define TGA_FORENSIC_BAD_PIXEL_ASPECT             0x100000u
#define TGA_FORENSIC_BAD_GAMMA                    0x200000u

typedef struct tga_forensic_report {
    tga_inspect inspect;
    unsigned section_count;
    unsigned overlap_count;
    unsigned gap_count;
    size_t gap_bytes;
    size_t trailing_bytes;
    unsigned duplicate_developer_tag_count;
    unsigned zero_size_developer_tag_count;
    unsigned backward_scan_line_count;
    unsigned out_of_image_scan_line_count;
    unsigned rle_packet_count;
    unsigned rle_cross_scanline_count;
    unsigned extension_size_mismatch_count;
    unsigned bad_color_correction_ref_count;
    unsigned bad_postage_stamp_ref_count;
    unsigned bad_scan_line_ref_count;
    unsigned developer_field_out_of_file_count;
    unsigned developer_field_overlap_count;
    unsigned utility_payload_mismatch_count;
    unsigned utility_text_not_terminated_count;
    unsigned bad_attributes_type_count;
    unsigned attribute_type_mismatch_count;
    unsigned bad_timestamp_count;
    unsigned bad_job_time_count;
    unsigned bad_pixel_aspect_count;
    unsigned bad_gamma_count;
    unsigned flags;
} tga_forensic_report;

#define TGA_CHANNEL_RED   0x01u
#define TGA_CHANNEL_GREEN 0x02u
#define TGA_CHANNEL_BLUE  0x04u
#define TGA_CHANNEL_ALPHA 0x08u

typedef enum tga_swizzle_source {
    TGA_SWIZZLE_ZERO = 0,
    TGA_SWIZZLE_ONE = 1,
    TGA_SWIZZLE_RED = 2,
    TGA_SWIZZLE_GREEN = 3,
    TGA_SWIZZLE_BLUE = 4,
    TGA_SWIZZLE_ALPHA = 5,
    TGA_SWIZZLE_LUMA = 6
} tga_swizzle_source;

typedef struct tga_swizzle {
    unsigned output_components;
    unsigned source[4];
} tga_swizzle;

typedef struct tga_swizzle_copy_params {
    const void *src_pixels;
    unsigned width;
    unsigned height;
    unsigned src_stride_bytes;
    tga_pixel_format src_pixel_format;
    int src_top_origin;
    void *dst_pixels;
    unsigned dst_stride_bytes;
    int dst_top_origin;
    tga_swizzle swizzle;
} tga_swizzle_copy_params;

typedef struct tga_decode_channels_params {
    void *pixels;
    unsigned stride_bytes;
    int output_top_origin;
    tga_u8 *palette_rgba;
    unsigned palette_capacity;
    unsigned channel_mask;
} tga_decode_channels_params;

typedef struct tga_decode_swizzle_params {
    void *pixels;
    unsigned stride_bytes;
    int output_top_origin;
    tga_u8 *palette_rgba;
    unsigned palette_capacity;
    tga_swizzle swizzle;
} tga_decode_swizzle_params;

#define TGA_UTIL_TAG_TEXT_UTF8   0x4701u
#define TGA_UTIL_TAG_U32         0x4702u
#define TGA_UTIL_TAG_U32_PAIR    0x4703u
#define TGA_UTIL_TAG_RECT_U32    0x4704u
#define TGA_UTIL_TAG_FLAGS_U32   0x4705u
#define TGA_UTIL_TAG_FOURCC      0x4706u

typedef struct tga_utility_u32_pair {
    tga_u32 first;
    tga_u32 second;
} tga_utility_u32_pair;

typedef struct tga_utility_rect {
    tga_u32 x;
    tga_u32 y;
    tga_u32 width;
    tga_u32 height;
} tga_utility_rect;

typedef struct tga_decode_rows_params {
    void *pixels;
    unsigned stride_bytes;
    tga_pixel_format pixel_format;
    int output_top_origin;
    tga_u8 *palette_rgba;
    unsigned palette_capacity;
    unsigned first_row;
    unsigned row_count;
} tga_decode_rows_params;

typedef struct tga_decode_rect_params {
    void *pixels;
    unsigned stride_bytes;
    tga_pixel_format pixel_format;
    int output_top_origin;
    tga_u8 *palette_rgba;
    unsigned palette_capacity;
    unsigned first_col;
    unsigned col_count;
    unsigned first_row;
    unsigned row_count;
} tga_decode_rect_params;

typedef struct tga_decode_params {
    void *pixels;
    unsigned stride_bytes;
    tga_pixel_format pixel_format;
    int output_top_origin;
    tga_u8 *palette_rgba;
    unsigned palette_capacity;
} tga_decode_params;

#define TGA_CANONICALIZE_KEEP_EXTENSION        0x0001u
#define TGA_CANONICALIZE_KEEP_DEVELOPER_AREA   0x0002u
#define TGA_CANONICALIZE_KEEP_COLOR_CORRECTION 0x0004u
#define TGA_CANONICALIZE_KEEP_POSTAGE_STAMP    0x0008u
#define TGA_CANONICALIZE_KEEP_SCAN_LINE_TABLE  0x0010u
#define TGA_CANONICALIZE_SYNTHESIZE_EXTENSION  0x0020u
#define TGA_CANONICALIZE_FORCE_FOOTER          0x0040u
#define TGA_CANONICALIZE_NORMALIZE_EXTENSION   0x0080u
#define TGA_CANONICALIZE_DEFAULT (\
    TGA_CANONICALIZE_KEEP_EXTENSION | \
    TGA_CANONICALIZE_KEEP_DEVELOPER_AREA | \
    TGA_CANONICALIZE_KEEP_COLOR_CORRECTION | \
    TGA_CANONICALIZE_KEEP_POSTAGE_STAMP | \
    TGA_CANONICALIZE_KEEP_SCAN_LINE_TABLE | \
    TGA_CANONICALIZE_SYNTHESIZE_EXTENSION | \
    TGA_CANONICALIZE_FORCE_FOOTER | \
    TGA_CANONICALIZE_NORMALIZE_EXTENSION)

typedef struct tga_canonicalize_options {
    unsigned flags;
    const tga_extension *extension_override;
} tga_canonicalize_options;

typedef struct tga_encode_params {
    const void *pixels;
    unsigned width;
    unsigned height;
    unsigned stride_bytes;
    tga_pixel_format pixel_format;
    int input_top_origin;

    int rle;
    int write_top_origin;
    int write_right_to_left;
    unsigned file_pixel_depth;

    const void *palette;
    unsigned palette_count;
    unsigned palette_first_index;
    tga_pixel_format palette_format;
    unsigned palette_file_entry_bits;

    const tga_u8 *image_id;
    unsigned image_id_length;
    int write_footer;
} tga_encode_params;

typedef struct tga_encode_params_ex {
    tga_encode_params image;
    const tga_extension *extension;
    int write_scan_line_table;
    const tga_color_correction_entry *color_correction_table;
    int write_postage_stamp;
    unsigned postage_stamp_max_width;
    unsigned postage_stamp_max_height;
    const tga_postage_stamp_source *postage_stamp_source;
    const tga_developer_field *developer_fields;
    unsigned developer_field_count;
} tga_encode_params_ex;

typedef struct tga_mem_reader {
    const tga_u8 *data;
    size_t size;
    size_t pos;
} tga_mem_reader;

typedef struct tga_mem_writer {
    tga_u8 *data;
    size_t capacity;
    size_t pos;
    int overflowed;
} tga_mem_writer;

#ifndef GAFNYF_TGA_NO_STDIO
typedef struct tga_file_reader {
    FILE *fp;
} tga_file_reader;

typedef struct tga_file_writer {
    FILE *fp;
} tga_file_writer;
#endif

unsigned tga_bytes_per_pixel(tga_pixel_format fmt);
const char *tga_result_string(int rc);
const char *tga_section_kind_string(unsigned kind);
const char *tga_attributes_type_string(unsigned attributes_type);

unsigned tga_default_attributes_type_from_info(const tga_info *info);
unsigned tga_default_attributes_type_from_encode_params(const tga_encode_params *params);
int tga_extension_pixel_aspect_q16(const tga_extension *extension, tga_u32 *out_q16);
int tga_extension_gamma_q16(const tga_extension *extension, tga_u32 *out_q16);
int tga_extension_set_pixel_aspect_q16(tga_extension *extension, tga_u32 q16);
int tga_extension_set_gamma_q16(tga_extension *extension, tga_u32 q16);

tga_u16 tga_pack_rgb565(tga_u8 r, tga_u8 g, tga_u8 b);
tga_u16 tga_pack_argb1555(tga_u8 a, tga_u8 r, tga_u8 g, tga_u8 b);
void tga_unpack_rgb565(tga_u16 pixel,
                       tga_u8 *out_r,
                       tga_u8 *out_g,
                       tga_u8 *out_b);
void tga_unpack_argb1555(tga_u16 pixel,
                         tga_u8 *out_a,
                         tga_u8 *out_r,
                         tga_u8 *out_g,
                         tga_u8 *out_b);

unsigned tga_channel_count_from_mask(unsigned mask);
int tga_swizzle_from_channel_mask(unsigned mask, tga_swizzle *out_swizzle);
int tga_swizzle_copy(const tga_swizzle_copy_params *params);

size_t tga_utility_text_size(const char *text);
int tga_make_utility_text_field(tga_developer_field *out_field,
                                unsigned tag,
                                const char *text,
                                void *payload,
                                size_t payload_capacity,
                                size_t *out_payload_size);
int tga_make_utility_u32_field(tga_developer_field *out_field,
                               unsigned tag,
                               tga_u32 value,
                               void *payload,
                               size_t payload_capacity);
int tga_make_utility_u32_pair_field(tga_developer_field *out_field,
                                    unsigned tag,
                                    const tga_utility_u32_pair *value,
                                    void *payload,
                                    size_t payload_capacity);
int tga_make_utility_rect_field(tga_developer_field *out_field,
                                unsigned tag,
                                const tga_utility_rect *value,
                                void *payload,
                                size_t payload_capacity);
int tga_make_utility_flags_field(tga_developer_field *out_field,
                                 unsigned tag,
                                 tga_u32 value,
                                 void *payload,
                                 size_t payload_capacity);
int tga_make_utility_fourcc_field(tga_developer_field *out_field,
                                  unsigned tag,
                                  const char fourcc[4],
                                  void *payload,
                                  size_t payload_capacity);

int tga_parse_utility_text(const void *payload, size_t payload_size,
                           char *dst,
                           size_t dst_capacity,
                           size_t *out_text_size);
int tga_parse_utility_u32(const void *payload, size_t payload_size,
                          tga_u32 *out_value);
int tga_parse_utility_u32_pair(const void *payload, size_t payload_size,
                               tga_utility_u32_pair *out_value);
int tga_parse_utility_rect(const void *payload, size_t payload_size,
                           tga_utility_rect *out_value);
int tga_parse_utility_fourcc(const void *payload, size_t payload_size,
                             char out_fourcc[4]);

int tga_probe_memory(const void *src, size_t src_size, tga_info *out_info);
int tga_read_image_id_memory(const void *src, size_t src_size,
                             void *dst,
                             size_t dst_capacity,
                             size_t *out_id_size);
int tga_read_color_map_memory(const void *src, size_t src_size,
                              void *dst,
                              size_t dst_capacity,
                              size_t *out_color_map_size);
int tga_decode_color_map_rgba(tga_reader *reader,
                              tga_u8 *palette_rgba,
                              unsigned palette_capacity,
                              tga_palette_info *out_palette_info,
                              tga_info *out_info);
int tga_decode_color_map_rgba_memory(const void *src, size_t src_size,
                                     tga_u8 *palette_rgba,
                                     unsigned palette_capacity,
                                     tga_palette_info *out_palette_info,
                                     tga_info *out_info);
int tga_read_image_data_memory(const void *src, size_t src_size,
                               void *dst,
                               size_t dst_capacity,
                               size_t *out_image_data_size);
int tga_inspect_memory(const void *src, size_t src_size, tga_inspect *out_inspect);
int tga_list_sections_memory(const void *src, size_t src_size,
                             tga_section *sections,
                             unsigned section_capacity,
                             unsigned *out_section_count);
int tga_forensics_memory(const void *src, size_t src_size,
                         tga_forensic_report *out_report);
int tga_canonicalize_size_memory(const void *src,
                                 size_t src_size,
                                 const tga_canonicalize_options *options,
                                 size_t *out_size);
int tga_canonicalize_memory(void *dst,
                            size_t dst_capacity,
                            size_t *dst_size,
                            const void *src,
                            size_t src_size,
                            const tga_canonicalize_options *options);
int tga_repair_tga2_size_memory(const void *src, size_t src_size, size_t *out_size);
int tga_repair_tga2_memory(void *dst,
                           size_t dst_capacity,
                           size_t *dst_size,
                           const void *src,
                           size_t src_size);

int tga_read_footer_memory(const void *src, size_t src_size, tga_footer *out_footer);
int tga_read_extension_memory(const void *src, size_t src_size, tga_extension *out_extension);
int tga_patch_extension_attributes_type_memory(void *src, size_t src_size,
                                              unsigned attributes_type);
int tga_patch_extension_pixel_aspect_q16_memory(void *src, size_t src_size,
                                                tga_u32 q16);
int tga_patch_extension_gamma_q16_memory(void *src, size_t src_size,
                                         tga_u32 q16);
int tga_read_scan_line_table_memory(const void *src, size_t src_size,
                                    tga_u32 *offsets,
                                    unsigned offset_capacity,
                                    unsigned *out_offset_count);
int tga_read_color_correction_table_memory(const void *src, size_t src_size,
                                           tga_color_correction_entry *table,
                                           unsigned table_capacity);
int tga_read_postage_stamp_info_memory(const void *src, size_t src_size,
                                       tga_postage_stamp_info *out_info);
int tga_read_postage_stamp_memory(const void *src, size_t src_size,
                                  void *dst,
                                  size_t dst_capacity,
                                  size_t *out_stamp_size,
                                  tga_postage_stamp_info *out_info);
int tga_decode_postage_stamp_memory(const void *src, size_t src_size,
                                    const tga_decode_params *params,
                                    tga_info *out_info);
int tga_read_developer_directory_memory(const void *src, size_t src_size,
                                        tga_developer_tag *tags,
                                        unsigned tag_capacity,
                                        unsigned *out_tag_count);
int tga_find_developer_tag_memory(const void *src, size_t src_size,
                                  unsigned tag,
                                  tga_developer_tag *out_tag);
int tga_read_developer_field_memory(const void *src, size_t src_size,
                                    const tga_developer_tag *tag,
                                    void *dst,
                                    size_t dst_capacity,
                                    size_t *out_field_size);
int tga_read_developer_tag_memory(const void *src, size_t src_size,
                                  unsigned tag,
                                  void *dst,
                                  size_t dst_capacity,
                                  size_t *out_field_size);

int tga_read_rle_packet_table_memory(const void *src, size_t src_size,
                                     tga_packet_info *packets,
                                     unsigned packet_capacity,
                                     unsigned *out_packet_count);
int tga_decode_rows_memory(const void *src, size_t src_size,
                           const tga_decode_rows_params *params,
                           tga_info *out_info);
int tga_decode_rect_memory(const void *src, size_t src_size,
                           const tga_decode_rect_params *params,
                           tga_info *out_info);
int tga_decode_indexed(tga_reader *reader,
                       const tga_decode_indexed_params *params,
                       tga_palette_info *out_palette_info,
                       tga_info *out_info);
int tga_decode_indexed_memory(const void *src, size_t src_size,
                              const tga_decode_indexed_params *params,
                              tga_palette_info *out_palette_info,
                              tga_info *out_info);

int tga_decode_channels(tga_reader *reader,
                        const tga_decode_channels_params *params,
                        tga_info *out_info);
int tga_decode_channels_memory(const void *src, size_t src_size,
                               const tga_decode_channels_params *params,
                               tga_info *out_info);
int tga_decode_swizzle(tga_reader *reader,
                       const tga_decode_swizzle_params *params,
                       tga_info *out_info);
int tga_decode_swizzle_memory(const void *src, size_t src_size,
                              const tga_decode_swizzle_params *params,
                              tga_info *out_info);

int tga_decode(tga_reader *reader, const tga_decode_params *params, tga_info *out_info);
int tga_decode_memory(const void *src, size_t src_size,
                      const tga_decode_params *params, tga_info *out_info);

unsigned long tga_encode_max_size(const tga_encode_params *params);
int tga_encode_size(const tga_encode_params *params, size_t *out_size);
int tga_encode(tga_writer *writer, const tga_encode_params *params);
int tga_encode_memory(void *dst, size_t dst_capacity, size_t *dst_size,
                      const tga_encode_params *params);

unsigned long tga_encode_ex_max_size(const tga_encode_params_ex *params);
int tga_encode_ex_size(const tga_encode_params_ex *params, size_t *out_size);
int tga_encode_ex(tga_writer *writer, const tga_encode_params_ex *params);
int tga_encode_ex_memory(void *dst, size_t dst_capacity, size_t *dst_size,
                         const tga_encode_params_ex *params);

size_t tga_mem_read(void *user, void *dst, size_t bytes);
size_t tga_mem_write(void *user, const void *src, size_t bytes);

#ifndef GAFNYF_TGA_NO_STDIO
size_t tga_file_read(void *user, void *dst, size_t bytes);
size_t tga_file_write(void *user, const void *src, size_t bytes);

int tga_probe_file(FILE *fp, tga_info *out_info);
int tga_read_image_id_file(FILE *fp,
                           void *dst,
                           size_t dst_capacity,
                           size_t *out_id_size);
int tga_read_color_map_file(FILE *fp,
                            void *dst,
                            size_t dst_capacity,
                            size_t *out_color_map_size);
int tga_decode_color_map_rgba_file(FILE *fp,
                                   tga_u8 *palette_rgba,
                                   unsigned palette_capacity,
                                   tga_palette_info *out_palette_info,
                                   tga_info *out_info);
int tga_read_image_data_file(FILE *fp,
                             void *dst,
                             size_t dst_capacity,
                             size_t *out_image_data_size);
int tga_inspect_file(FILE *fp, tga_inspect *out_inspect);
int tga_list_sections_file(FILE *fp,
                           tga_section *sections,
                           unsigned section_capacity,
                           unsigned *out_section_count);
int tga_forensics_file(FILE *fp, tga_forensic_report *out_report);
int tga_canonicalize_file(FILE *dst,
                          FILE *src,
                          const tga_canonicalize_options *options);
int tga_repair_tga2_file(FILE *dst, FILE *src);

int tga_read_footer_file(FILE *fp, tga_footer *out_footer);
int tga_read_extension_file(FILE *fp, tga_extension *out_extension);
int tga_patch_extension_attributes_type_file(FILE *fp, unsigned attributes_type);
int tga_patch_extension_pixel_aspect_q16_file(FILE *fp, tga_u32 q16);
int tga_patch_extension_gamma_q16_file(FILE *fp, tga_u32 q16);
int tga_read_scan_line_table_file(FILE *fp, tga_u32 *offsets,
                                  unsigned offset_capacity,
                                  unsigned *out_offset_count);
int tga_read_color_correction_table_file(FILE *fp,
                                         tga_color_correction_entry *table,
                                         unsigned table_capacity);
int tga_read_postage_stamp_info_file(FILE *fp,
                                     tga_postage_stamp_info *out_info);
int tga_read_postage_stamp_file(FILE *fp,
                                void *dst,
                                size_t dst_capacity,
                                size_t *out_stamp_size,
                                tga_postage_stamp_info *out_info);
int tga_decode_postage_stamp_file(FILE *fp,
                                  const tga_decode_params *params,
                                  tga_info *out_info);
int tga_read_developer_directory_file(FILE *fp,
                                      tga_developer_tag *tags,
                                      unsigned tag_capacity,
                                      unsigned *out_tag_count);
int tga_find_developer_tag_file(FILE *fp, unsigned tag, tga_developer_tag *out_tag);
int tga_read_developer_field_file(FILE *fp,
                                  const tga_developer_tag *tag,
                                  void *dst,
                                  size_t dst_capacity,
                                  size_t *out_field_size);
int tga_read_developer_tag_file(FILE *fp, unsigned tag,
                                void *dst,
                                size_t dst_capacity,
                                size_t *out_field_size);
int tga_read_rle_packet_table_file(FILE *fp,
                                   tga_packet_info *packets,
                                   unsigned packet_capacity,
                                   unsigned *out_packet_count);
int tga_decode_rows_file(FILE *fp,
                         const tga_decode_rows_params *params,
                         tga_info *out_info);
int tga_decode_rect_file(FILE *fp,
                         const tga_decode_rect_params *params,
                         tga_info *out_info);
int tga_decode_indexed_file(FILE *fp,
                            const tga_decode_indexed_params *params,
                            tga_palette_info *out_palette_info,
                            tga_info *out_info);
int tga_decode_channels_file(FILE *fp,
                             const tga_decode_channels_params *params,
                             tga_info *out_info);
int tga_decode_swizzle_file(FILE *fp,
                            const tga_decode_swizzle_params *params,
                            tga_info *out_info);
int tga_decode_file(FILE *fp, const tga_decode_params *params, tga_info *out_info);
int tga_encode_file(FILE *fp, const tga_encode_params *params);
int tga_encode_ex_file(FILE *fp, const tga_encode_params_ex *params);
#endif

#ifdef __cplusplus
}
#endif

#endif
