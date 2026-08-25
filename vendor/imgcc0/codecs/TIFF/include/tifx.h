/* SPDX-License-Identifier: CC0-1.0 */
#ifndef TIFX_H
#define TIFX_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef TIFX_MAX_STRIPS
#define TIFX_MAX_STRIPS 4096UL
#endif

#ifndef TIFX_MAX_TILES
#define TIFX_MAX_TILES 4096UL
#endif

#ifndef TIFX_MAX_SAMPLES
#define TIFX_MAX_SAMPLES 4U
#endif

#ifndef TIFX_MAX_COLORMAP_SHORTS
#define TIFX_MAX_COLORMAP_SHORTS 768U
#endif

#ifndef TIFX_MAX_PAGES
#define TIFX_MAX_PAGES 256UL
#endif

#ifndef TIFX_MAX_SUBIFDS
#define TIFX_MAX_SUBIFDS 256UL
#endif

#ifndef TIFX_MAX_SUBIFD_DEPTH
#define TIFX_MAX_SUBIFD_DEPTH 8UL
#endif

typedef long tifx_fixed;

#define TIFX_FP_SHIFT 16L
#define TIFX_FP_ONE ((tifx_fixed)(1L << TIFX_FP_SHIFT))
#define TIFX_FP_FROM_INT(value) ((tifx_fixed)((value) << TIFX_FP_SHIFT))
#define TIFX_FP_TO_INT_FLOOR(value) ((long)((value) >> TIFX_FP_SHIFT))

enum tifx_result {
    TIFX_OK = 0,
    TIFX_ERR_BAD_ARGUMENT = -1,
    TIFX_ERR_BAD_FORMAT = -2,
    TIFX_ERR_UNSUPPORTED = -3,
    TIFX_ERR_TRUNCATED = -4,
    TIFX_ERR_OVERFLOW = -5,
    TIFX_ERR_NO_SPACE = -6
};

enum tifx_pixel_format {
    TIFX_PIXEL_UNKNOWN = 0,
    TIFX_PIXEL_GRAY8 = 1,
    TIFX_PIXEL_RGB24 = 2,
    TIFX_PIXEL_BILEVEL = 3,
    TIFX_PIXEL_RGBA32 = 4
};

enum tifx_alpha_mode {
    TIFX_ALPHA_NONE = 0,
    TIFX_ALPHA_ASSOCIATED = 1,
    TIFX_ALPHA_UNASSOCIATED = 2
};

enum tifx_container_format {
    TIFX_CONTAINER_AUTO = 0,
    TIFX_CONTAINER_CLASSIC = 1,
    TIFX_CONTAINER_BIGTIFF = 2
};

enum tifx_storage_layout {
    TIFX_LAYOUT_STRIPS = 1,
    TIFX_LAYOUT_TILES = 2
};

enum tifx_subifd_style {
    TIFX_SUBIFD_STYLE_TREE = 0,
    TIFX_SUBIFD_STYLE_ADOBE_CHAIN = 1
};

enum tifx_predictor {
    TIFX_PREDICTOR_NONE = 1,
    TIFX_PREDICTOR_HORIZONTAL = 2
};

enum tifx_deflate_mode {
    TIFX_DEFLATE_AUTO = 0,
    TIFX_DEFLATE_STORED = 1,
    TIFX_DEFLATE_FIXED = 2,
    TIFX_DEFLATE_DYNAMIC = 3
};

typedef struct tifx_image_info {
    unsigned short container_format;
    unsigned short is_big_endian;
    unsigned short pixel_format;
    unsigned short compression;
    unsigned short photometric;
    unsigned short planar_config;
    unsigned short orientation;
    unsigned short fill_order;
    unsigned short resolution_unit;
    unsigned short samples_per_pixel;
    unsigned short alpha_mode;
    unsigned short predictor;
    unsigned short extra_samples_count;
    unsigned short bits_per_sample_count;
    unsigned short bits_per_sample[TIFX_MAX_SAMPLES];
    unsigned short extra_samples[TIFX_MAX_SAMPLES];
    unsigned short color_map_count;
    unsigned short color_map[TIFX_MAX_COLORMAP_SHORTS];
    unsigned short page_number[2];
    unsigned long new_subfile_type;
    unsigned long t4_options;
    unsigned long t6_options;
    unsigned long width;
    unsigned long height;
    unsigned long file_size;
    unsigned long first_ifd_offset;
    unsigned long current_ifd_offset;
    unsigned long next_ifd_offset;
    unsigned long parent_ifd_offset;
    unsigned long page_index;
    unsigned long page_count;
    unsigned long subifd_depth;
    unsigned long subifd_index;
    unsigned long subifd_count;
    unsigned long subifd_offsets[TIFX_MAX_SUBIFDS];
    unsigned long rows_per_strip;
    unsigned long strip_count;
    unsigned long strip_offsets[TIFX_MAX_STRIPS];
    unsigned long strip_byte_counts[TIFX_MAX_STRIPS];
    unsigned long tile_width;
    unsigned long tile_length;
    unsigned long tile_count;
    unsigned short storage_layout;
    unsigned short reserved0;
    unsigned long tile_offsets[TIFX_MAX_TILES];
    unsigned long tile_byte_counts[TIFX_MAX_TILES];
    tifx_fixed x_resolution;
    tifx_fixed y_resolution;
} tifx_image_info;

typedef struct tifx_write_params {
    unsigned short container_format;
    unsigned short pixel_format;
    unsigned short compression;
    unsigned short photometric;
    unsigned short alpha_mode;
    unsigned short planar_config;
    unsigned short fill_order;
    unsigned short subifd_style;
    unsigned short predictor;
    unsigned short deflate_mode;
    unsigned short resolution_unit;
    unsigned long width;
    unsigned long height;
    unsigned long stride;
    unsigned long rows_per_strip;
    unsigned long tile_width;
    unsigned long tile_length;
    unsigned long t4_options;
    unsigned long t6_options;
    unsigned long new_subfile_type;
    tifx_fixed x_resolution;
    tifx_fixed y_resolution;
    const unsigned char *pixels;
} tifx_write_params;

typedef struct tifx_tiff_node tifx_tiff_node;
typedef tifx_tiff_node tifx_bigtiff_node;

struct tifx_tiff_node {
    tifx_write_params image;
    const tifx_tiff_node *children;
    unsigned long child_count;
};

void tifx_image_info_init(tifx_image_info *info);
void tifx_write_params_init(tifx_write_params *params);

const char *tifx_strerror(int code);

tifx_fixed tifx_fp_from_ratio(unsigned long numerator, unsigned long denominator);
unsigned long tifx_fp_to_rational_numerator(tifx_fixed value);
unsigned long tifx_fp_to_rational_denominator(void);

int tifx_bigtiff_available(void);

int tifx_parse_memory(tifx_image_info *info, const void *data, unsigned long size);
int tifx_parse_classic_memory(tifx_image_info *info, const void *data, unsigned long size);
int tifx_parse_classic_page_memory(tifx_image_info *info,
                                   const void *data,
                                   unsigned long size,
                                   unsigned long page_index);
int tifx_parse_classic_node_memory(tifx_image_info *info,
                                   const void *data,
                                   unsigned long size,
                                   unsigned long page_index,
                                   const unsigned long *subifd_path,
                                   unsigned long subifd_path_length);
int tifx_classic_page_count_memory(const void *data,
                                   unsigned long size,
                                   unsigned long *out_page_count);
int tifx_classic_subifd_count_memory(const void *data,
                                     unsigned long size,
                                     unsigned long page_index,
                                     const unsigned long *subifd_path,
                                     unsigned long subifd_path_length,
                                     unsigned long *out_subifd_count);
int tifx_parse_bigtiff_memory(tifx_image_info *info, const void *data, unsigned long size);
int tifx_parse_bigtiff_page_memory(tifx_image_info *info,
                                   const void *data,
                                   unsigned long size,
                                   unsigned long page_index);
int tifx_parse_bigtiff_node_memory(tifx_image_info *info,
                                   const void *data,
                                   unsigned long size,
                                   unsigned long page_index,
                                   const unsigned long *subifd_path,
                                   unsigned long subifd_path_length);
int tifx_bigtiff_page_count_memory(const void *data,
                                   unsigned long size,
                                   unsigned long *out_page_count);
int tifx_bigtiff_subifd_count_memory(const void *data,
                                     unsigned long size,
                                     unsigned long page_index,
                                     const unsigned long *subifd_path,
                                     unsigned long subifd_path_length,
                                     unsigned long *out_subifd_count);

unsigned long tifx_decode_workspace_size(const tifx_image_info *info);
unsigned long tifx_decode_buffer_size(const tifx_image_info *info,
                                      unsigned long *out_stride);
int tifx_decode_memory(const tifx_image_info *info,
                       const void *data,
                       unsigned long size,
                       void *dst,
                       unsigned long dst_size,
                       unsigned long dst_stride,
                       void *workspace,
                       unsigned long workspace_size);

unsigned long tifx_write_buffer_size(const tifx_write_params *params);
unsigned long tifx_write_classic_buffer_size(const tifx_write_params *params);
unsigned long tifx_write_classic_tree_buffer_size(const tifx_tiff_node *pages,
                                                  unsigned long page_count);
unsigned long tifx_write_bigtiff_buffer_size(const tifx_write_params *params);
unsigned long tifx_write_bigtiff_pages_buffer_size(const tifx_write_params *pages,
                                                   unsigned long page_count);
unsigned long tifx_write_bigtiff_tree_buffer_size(const tifx_tiff_node *pages,
                                                  unsigned long page_count);
int tifx_write_memory(void *dst,
                      unsigned long dst_size,
                      const tifx_write_params *params,
                      unsigned long *written_size);
int tifx_write_classic_memory(void *dst,
                              unsigned long dst_size,
                              const tifx_write_params *params,
                              unsigned long *written_size);
int tifx_write_classic_tree_memory(void *dst,
                                   unsigned long dst_size,
                                   const tifx_tiff_node *pages,
                                   unsigned long page_count,
                                   unsigned long *written_size);
int tifx_write_bigtiff_memory(void *dst,
                              unsigned long dst_size,
                              const tifx_write_params *params,
                              unsigned long *written_size);
int tifx_write_bigtiff_pages_memory(void *dst,
                                    unsigned long dst_size,
                                    const tifx_write_params *pages,
                                    unsigned long page_count,
                                    unsigned long *written_size);
int tifx_write_bigtiff_tree_memory(void *dst,
                                   unsigned long dst_size,
                                   const tifx_tiff_node *pages,
                                   unsigned long page_count,
                                   unsigned long *written_size);

#ifdef __cplusplus
}
#endif

#endif
