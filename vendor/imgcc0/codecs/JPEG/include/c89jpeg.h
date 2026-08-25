#ifndef C89JPEG_H
#define C89JPEG_H

#include <limits.h>

#if defined(__cplusplus)
extern "C" {
#endif

#if ULONG_MAX < 0xFFFFFFFFUL
#error "c89jpeg requires unsigned long to be at least 32 bits"
#endif

#define C89JPEG_VERSION_MAJOR 1
#define C89JPEG_VERSION_MINOR 2
#define C89JPEG_VERSION_PATCH 0

#define C89JPEG_MAX_COMPONENTS 3
#define C89JPEG_MAX_QUANT_TABLES 4
#define C89JPEG_MAX_HUFF_CLASSES 2
#define C89JPEG_MAX_HUFF_TABLES 4
#define C89JPEG_MAX_BLOCKS_PER_MCU 10
#define C89JPEG_HUFF_SYMBOLS 257
#define C89JPEG_MAX_PROGRESSIVE_SCANS 64

typedef unsigned char c89jpeg_u8;
typedef unsigned short c89jpeg_u16;
typedef signed short c89jpeg_i16;
typedef unsigned long c89jpeg_u32;
typedef signed long c89jpeg_i32;

typedef enum c89jpeg_status_tag {
    C89JPEG_SUSPENDED = 1,
    C89JPEG_OK = 0,
    C89JPEG_ERR_BAD_ARG = -1,
    C89JPEG_ERR_SHORT_BUFFER = -2,
    C89JPEG_ERR_UNSUPPORTED = -3,
    C89JPEG_ERR_CORRUPT = -4,
    C89JPEG_ERR_LIMIT = -5,
    C89JPEG_ERR_IO = -6
} c89jpeg_status;

typedef enum c89jpeg_pixel_format_tag {
    C89JPEG_PIXFMT_GRAY8 = 1,
    C89JPEG_PIXFMT_RGB24 = 3
} c89jpeg_pixel_format;

typedef enum c89jpeg_subsampling_tag {
    C89JPEG_SUBSAMP_444 = 0,
    C89JPEG_SUBSAMP_422 = 1,
    C89JPEG_SUBSAMP_420 = 2
} c89jpeg_subsampling;

typedef enum c89jpeg_decode_format_tag {
    C89JPEG_DECODE_NATIVE = 0,
    C89JPEG_DECODE_GRAY8 = 1,
    C89JPEG_DECODE_RGB24 = 3
} c89jpeg_decode_format;

typedef enum c89jpeg_upsampling_mode_tag {
    C89JPEG_UPSAMPLE_NEAREST = 0,
    C89JPEG_UPSAMPLE_LINEAR = 1
} c89jpeg_upsampling_mode;

typedef enum c89jpeg_huffman_mode_tag {
    C89JPEG_HUFFMAN_DEFAULT = 0,
    C89JPEG_HUFFMAN_OPTIMAL = 1,
    C89JPEG_HUFFMAN_CUSTOM = 2
} c89jpeg_huffman_mode;

typedef enum c89jpeg_resume_output_mode_tag {
    C89JPEG_RESUME_OUTPUT_MEMORY = 0,
    C89JPEG_RESUME_OUTPUT_SINK = 1
} c89jpeg_resume_output_mode;

typedef enum c89jpeg_roi_mode_tag {
    C89JPEG_ROI_DISABLED = 0,
    C89JPEG_ROI_ALIGN_TO_IMCU = 1,
    C89JPEG_ROI_STRICT = 2
} c89jpeg_roi_mode;

typedef struct c89jpeg_rect_tag {
    c89jpeg_u16 x;
    c89jpeg_u16 y;
    c89jpeg_u16 width;
    c89jpeg_u16 height;
} c89jpeg_rect;

typedef int (*c89jpeg_write_fn)(void *user, const c89jpeg_u8 *data, c89jpeg_u32 size);
typedef int (*c89jpeg_read_fn)(void *user, c89jpeg_u8 *data, c89jpeg_u32 capacity, c89jpeg_u32 *out_read);
typedef int (*c89jpeg_row_sink_fn)(void *user, c89jpeg_u32 row_index, const c89jpeg_u8 *row, c89jpeg_u32 row_size);
typedef int (*c89jpeg_row_read_fn)(void *user, c89jpeg_u32 row_index, c89jpeg_u8 *row, c89jpeg_u32 row_size);

typedef struct c89jpeg_mem_dest_tag {
    c89jpeg_u8 *data;
    c89jpeg_u32 capacity;
    c89jpeg_u32 size;
} c89jpeg_mem_dest;

typedef struct c89jpeg_mem_src_tag {
    const c89jpeg_u8 *data;
    c89jpeg_u32 size;
    c89jpeg_u32 offset;
    c89jpeg_u32 chunk_limit;
} c89jpeg_mem_src;

typedef struct c89jpeg_component_info_tag {
    c89jpeg_u8 id;
    c89jpeg_u8 h_samp;
    c89jpeg_u8 v_samp;
    c89jpeg_u8 quant_table;
    c89jpeg_u8 dc_table;
    c89jpeg_u8 ac_table;
} c89jpeg_component_info;

typedef struct c89jpeg_image_info_tag {
    c89jpeg_u16 width;
    c89jpeg_u16 height;
    c89jpeg_u8 precision;
    c89jpeg_u8 components;
    c89jpeg_u8 max_h_samp;
    c89jpeg_u8 max_v_samp;
    c89jpeg_u8 is_jfif;
    c89jpeg_u8 density_units;
    c89jpeg_u16 density_x;
    c89jpeg_u16 density_y;
    c89jpeg_u16 restart_interval;
    c89jpeg_u8 progressive;
    c89jpeg_u8 arithmetic;
    c89jpeg_component_info component[C89JPEG_MAX_COMPONENTS];
} c89jpeg_image_info;

typedef struct c89jpeg_encode_params_tag {
    c89jpeg_u16 width;
    c89jpeg_u16 height;
    const c89jpeg_u8 *pixels;
    c89jpeg_u32 stride_bytes;
    c89jpeg_pixel_format pixel_format;
    c89jpeg_subsampling subsampling;
    int quality;
    c89jpeg_u16 restart_interval;
    int emit_jfif;
    c89jpeg_u8 density_units;
    c89jpeg_u16 density_x;
    c89jpeg_u16 density_y;
    const c89jpeg_u8 *quant_luma;
    const c89jpeg_u8 *quant_chroma;
    c89jpeg_huffman_mode huffman_mode;
    const struct c89jpeg_huffman_table_tag *custom_dc_luma;
    const struct c89jpeg_huffman_table_tag *custom_ac_luma;
    const struct c89jpeg_huffman_table_tag *custom_dc_chroma;
    const struct c89jpeg_huffman_table_tag *custom_ac_chroma;
} c89jpeg_encode_params;

typedef struct c89jpeg_encode_source_params_tag {
    c89jpeg_u16 width;
    c89jpeg_u16 height;
    c89jpeg_u32 stride_bytes;
    c89jpeg_pixel_format pixel_format;
    c89jpeg_subsampling subsampling;
    int quality;
    c89jpeg_u16 restart_interval;
    int emit_jfif;
    c89jpeg_u8 density_units;
    c89jpeg_u16 density_x;
    c89jpeg_u16 density_y;
    const c89jpeg_u8 *quant_luma;
    const c89jpeg_u8 *quant_chroma;
    c89jpeg_huffman_mode huffman_mode;
    const struct c89jpeg_huffman_table_tag *custom_dc_luma;
    const struct c89jpeg_huffman_table_tag *custom_ac_luma;
    const struct c89jpeg_huffman_table_tag *custom_dc_chroma;
    const struct c89jpeg_huffman_table_tag *custom_ac_chroma;
    c89jpeg_row_read_fn read_row_fn;
    void *read_user;
    c89jpeg_u8 *row_cache;
    c89jpeg_u32 row_cache_size;
} c89jpeg_encode_source_params;

typedef struct c89jpeg_decode_params_tag {
    const c89jpeg_u8 *data;
    c89jpeg_u32 size;
    c89jpeg_u8 *out_pixels;
    c89jpeg_u32 out_capacity;
    c89jpeg_u32 out_stride;
    c89jpeg_decode_format out_format;
    c89jpeg_upsampling_mode upsampling;
    c89jpeg_rect roi;
    c89jpeg_roi_mode roi_mode;
    void *progressive_workspace;
    c89jpeg_u32 progressive_workspace_size;
} c89jpeg_decode_params;

typedef struct c89jpeg_decode_sink_params_tag {
    const c89jpeg_u8 *data;
    c89jpeg_u32 size;
    c89jpeg_u8 *strip_buffer;
    c89jpeg_u32 strip_buffer_size;
    c89jpeg_decode_format out_format;
    c89jpeg_upsampling_mode upsampling;
    c89jpeg_rect roi;
    c89jpeg_roi_mode roi_mode;
    void *progressive_workspace;
    c89jpeg_u32 progressive_workspace_size;
} c89jpeg_decode_sink_params;

typedef struct c89jpeg_decode_source_params_tag {
    c89jpeg_read_fn read_fn;
    void *read_user;
    c89jpeg_u8 *input_buffer;
    c89jpeg_u32 input_buffer_size;
    c89jpeg_u8 *out_pixels;
    c89jpeg_u32 out_capacity;
    c89jpeg_u32 out_stride;
    c89jpeg_decode_format out_format;
    c89jpeg_upsampling_mode upsampling;
    c89jpeg_rect roi;
    c89jpeg_roi_mode roi_mode;
} c89jpeg_decode_source_params;

typedef struct c89jpeg_decode_source_sink_params_tag {
    c89jpeg_read_fn read_fn;
    void *read_user;
    c89jpeg_u8 *input_buffer;
    c89jpeg_u32 input_buffer_size;
    c89jpeg_u8 *strip_buffer;
    c89jpeg_u32 strip_buffer_size;
    c89jpeg_decode_format out_format;
    c89jpeg_upsampling_mode upsampling;
    c89jpeg_rect roi;
    c89jpeg_roi_mode roi_mode;
} c89jpeg_decode_source_sink_params;

typedef struct c89jpeg_decode_resume_params_tag {
    c89jpeg_u8 *input_storage;
    c89jpeg_u32 input_capacity;
    c89jpeg_u8 *out_pixels;
    c89jpeg_u32 out_capacity;
    c89jpeg_u32 out_stride;
    c89jpeg_u8 *strip_buffer;
    c89jpeg_u32 strip_buffer_size;
    c89jpeg_decode_format out_format;
    c89jpeg_upsampling_mode upsampling;
    c89jpeg_rect roi;
    c89jpeg_roi_mode roi_mode;
    c89jpeg_resume_output_mode output_mode;
} c89jpeg_decode_resume_params;

typedef struct c89jpeg_huffman_table_tag {
    c89jpeg_u8 present;
    c89jpeg_u8 bits[16];
    c89jpeg_u8 vals[256];
    c89jpeg_i32 mincode[17];
    c89jpeg_i32 maxcode[18];
    c89jpeg_i32 valptr[17];
    c89jpeg_u16 ehufco[256];
    c89jpeg_u8 ehufsi[256];
    c89jpeg_u16 count;
} c89jpeg_huffman_table;

typedef struct c89jpeg_tables_tag {
    c89jpeg_u16 quant[C89JPEG_MAX_QUANT_TABLES][64];
    c89jpeg_u8 quant_present[C89JPEG_MAX_QUANT_TABLES];
    c89jpeg_huffman_table huff[C89JPEG_MAX_HUFF_CLASSES][C89JPEG_MAX_HUFF_TABLES];
    c89jpeg_u16 restart_interval;
} c89jpeg_tables;

typedef struct c89jpeg_component_tag {
    c89jpeg_u8 id;
    c89jpeg_u8 h_samp;
    c89jpeg_u8 v_samp;
    c89jpeg_u8 tq;
    c89jpeg_u8 dc_table;
    c89jpeg_u8 ac_table;
    c89jpeg_u16 width_in_blocks;
    c89jpeg_u16 height_in_blocks;
    c89jpeg_i32 pred;
    c89jpeg_u8 scan_index;
} c89jpeg_component;

typedef struct c89jpeg_encoder_tag {
    c89jpeg_u16 qtable[2][64];
    c89jpeg_huffman_table huff_dc[2];
    c89jpeg_huffman_table huff_ac[2];
    c89jpeg_component comp[C89JPEG_MAX_COMPONENTS];
    c89jpeg_i32 dct_tmp[64];
    c89jpeg_i32 dct_work[64];
    c89jpeg_i32 coeff[64];
    c89jpeg_i16 qcoeff[64];
    c89jpeg_u32 huff_freq_dc[2][C89JPEG_HUFF_SYMBOLS];
    c89jpeg_u32 huff_freq_ac[2][C89JPEG_HUFF_SYMBOLS];
    c89jpeg_u32 emitted_size;
    c89jpeg_status last_error;
} c89jpeg_encoder;

typedef struct c89jpeg_decoder_tag {
    c89jpeg_image_info info;
    c89jpeg_u16 quant[C89JPEG_MAX_QUANT_TABLES][64];
    c89jpeg_u8 quant_present[C89JPEG_MAX_QUANT_TABLES];
    c89jpeg_huffman_table huff[C89JPEG_MAX_HUFF_CLASSES][C89JPEG_MAX_HUFF_TABLES];
    c89jpeg_component comp[C89JPEG_MAX_COMPONENTS];
    c89jpeg_component *scan_comp[C89JPEG_MAX_COMPONENTS];
    c89jpeg_u8 scan_count;
    c89jpeg_u8 scan_started;
    c89jpeg_u8 scan_ss;
    c89jpeg_u8 scan_se;
    c89jpeg_u8 scan_ah;
    c89jpeg_u8 scan_al;
    c89jpeg_u32 entropy_offset;
    c89jpeg_i32 blocks[C89JPEG_MAX_BLOCKS_PER_MCU][64];
    c89jpeg_i32 idct_tmp[64];
    c89jpeg_u8 samples[C89JPEG_MAX_BLOCKS_PER_MCU][64];
    c89jpeg_status last_error;
} c89jpeg_decoder;

typedef struct c89jpeg_decoder_resume_tag {
    c89jpeg_decoder *decoder;
    c89jpeg_u8 *input_storage;
    c89jpeg_u32 input_capacity;
    c89jpeg_u32 input_size;
    c89jpeg_u8 final_input;
    c89jpeg_u8 headers_ready;
    c89jpeg_u8 prepared;
    c89jpeg_u8 finished;
    c89jpeg_resume_output_mode output_mode;
    c89jpeg_u8 *out_pixels;
    c89jpeg_u32 out_capacity;
    c89jpeg_u32 out_stride;
    c89jpeg_u8 *strip_buffer;
    c89jpeg_u32 strip_buffer_size;
    c89jpeg_decode_format out_format;
    c89jpeg_upsampling_mode upsampling;
    c89jpeg_rect requested_roi;
    c89jpeg_roi_mode roi_mode;
    c89jpeg_rect region;
    c89jpeg_u32 row_bytes;
    c89jpeg_u32 strip_need;
    c89jpeg_u8 out_channels;
    c89jpeg_u16 mcu_cols;
    c89jpeg_u16 mcu_rows;
    c89jpeg_u16 mcu_w;
    c89jpeg_u16 mcu_h;
    c89jpeg_u16 my;
    c89jpeg_u16 mx;
    c89jpeg_u16 restart_left;
    c89jpeg_u8 next_rst;
    c89jpeg_u32 scan_pos;
    c89jpeg_u32 scan_acc;
    int scan_bits;
    int scan_unread_marker;
    c89jpeg_i32 comp_pred[C89JPEG_MAX_COMPONENTS];
    c89jpeg_image_info info;
    c89jpeg_status last_error;
} c89jpeg_decoder_resume;

typedef struct c89jpeg_encode_resume_params_tag {
    c89jpeg_u8 *staging_buffer;
    c89jpeg_u32 staging_capacity;
    c89jpeg_u32 emit_chunk_size;
    int abbreviated;
    const c89jpeg_tables *tables;
} c89jpeg_encode_resume_params;

typedef struct c89jpeg_encode_source_resume_params_tag {
    c89jpeg_u16 width;
    c89jpeg_u16 height;
    c89jpeg_u32 stride_bytes;
    c89jpeg_pixel_format pixel_format;
    c89jpeg_subsampling subsampling;
    int quality;
    c89jpeg_u16 restart_interval;
    int emit_jfif;
    c89jpeg_u8 density_units;
    c89jpeg_u16 density_x;
    c89jpeg_u16 density_y;
    const c89jpeg_u8 *quant_luma;
    const c89jpeg_u8 *quant_chroma;
    c89jpeg_huffman_mode huffman_mode;
    const struct c89jpeg_huffman_table_tag *custom_dc_luma;
    const struct c89jpeg_huffman_table_tag *custom_ac_luma;
    const struct c89jpeg_huffman_table_tag *custom_dc_chroma;
    const struct c89jpeg_huffman_table_tag *custom_ac_chroma;
    c89jpeg_u8 *row_window;
    c89jpeg_u32 row_window_size;
    c89jpeg_u8 *replay_buffer;
    c89jpeg_u32 replay_buffer_size;
    c89jpeg_u8 *staging_buffer;
    c89jpeg_u32 staging_capacity;
    c89jpeg_u32 emit_chunk_size;
    int abbreviated;
    const c89jpeg_tables *tables;
} c89jpeg_encode_source_resume_params;

typedef struct c89jpeg_encoder_resume_tag {
    c89jpeg_encoder *encoder;
    c89jpeg_u8 *staging_buffer;
    c89jpeg_u32 staging_capacity;
    c89jpeg_u32 staging_size;
    c89jpeg_u32 emit_offset;
    c89jpeg_u32 emit_chunk_size;
    c89jpeg_u8 abbreviated;
    c89jpeg_u8 prepared;
    c89jpeg_u8 finished;
    c89jpeg_status last_error;
    c89jpeg_encode_params params;
    const c89jpeg_tables *tables;
    c89jpeg_u8 phase;
    c89jpeg_u8 components;
    c89jpeg_u8 max_h;
    c89jpeg_u8 max_v;
    c89jpeg_u8 ci;
    c89jpeg_u8 bx;
    c89jpeg_u8 by;
    c89jpeg_u8 rst_index;
    c89jpeg_u16 mcu_w;
    c89jpeg_u16 mcu_h;
    c89jpeg_u16 mcu_cols;
    c89jpeg_u16 mcu_rows;
    c89jpeg_u16 mcu_x;
    c89jpeg_u16 mcu_y;
    c89jpeg_u16 restart_count;
    c89jpeg_u16 restart_interval;
    c89jpeg_u32 total_emitted;
    c89jpeg_u32 exact_total_size;
    c89jpeg_u8 total_size_known;
    c89jpeg_u32 bw_acc;
    int bw_bits;
    c89jpeg_u8 source_mode;
    c89jpeg_u8 source_input_final;
    c89jpeg_u8 source_window_padded;
    c89jpeg_u8 source_replay_enabled;
    c89jpeg_u8 source_replay_ready;
    c89jpeg_u16 source_base_y;
    c89jpeg_u16 source_rows_loaded;
    c89jpeg_u16 source_next_row;
    c89jpeg_u8 *source_row_window;
    c89jpeg_u32 source_row_window_size;
    c89jpeg_u8 *source_replay_buffer;
    c89jpeg_u32 source_replay_buffer_size;
} c89jpeg_encoder_resume;

typedef struct c89jpeg_tables_trainer_tag {
    c89jpeg_tables base_tables;
    c89jpeg_u32 dc_freq[2][C89JPEG_HUFF_SYMBOLS];
    c89jpeg_u32 ac_freq[2][C89JPEG_HUFF_SYMBOLS];
    c89jpeg_u8 prepared;
    c89jpeg_pixel_format pixel_format;
    c89jpeg_subsampling subsampling;
    c89jpeg_u8 component_count;
    c89jpeg_u32 image_count;
    c89jpeg_status last_error;
} c89jpeg_tables_trainer;

typedef struct c89jpeg_tables_session_tag {
    c89jpeg_tables tables;
    c89jpeg_u8 tables_ready;
    c89jpeg_u8 tables_emitted;
    c89jpeg_pixel_format pixel_format;
    c89jpeg_subsampling subsampling;
    c89jpeg_u8 component_count;
    c89jpeg_u32 image_count;
    c89jpeg_status last_error;
} c89jpeg_tables_session;

const char *c89jpeg_status_string(c89jpeg_status status);
void c89jpeg_mem_dest_init(c89jpeg_mem_dest *dest, c89jpeg_u8 *buffer, c89jpeg_u32 capacity);
int c89jpeg_mem_dest_write(void *user, const c89jpeg_u8 *data, c89jpeg_u32 size);
void c89jpeg_mem_src_init(c89jpeg_mem_src *src, const c89jpeg_u8 *buffer, c89jpeg_u32 size);
void c89jpeg_mem_src_set_chunk_limit(c89jpeg_mem_src *src, c89jpeg_u32 chunk_limit);
int c89jpeg_mem_src_read(void *user, c89jpeg_u8 *data, c89jpeg_u32 capacity, c89jpeg_u32 *out_read);

c89jpeg_u32 c89jpeg_encoder_max_output_size(c89jpeg_u16 width, c89jpeg_u16 height, c89jpeg_pixel_format fmt);
c89jpeg_u32 c89jpeg_encoder_source_row_cache_size(c89jpeg_u16 width, c89jpeg_pixel_format fmt, c89jpeg_subsampling subsampling, c89jpeg_u32 stride_bytes);
c89jpeg_u32 c89jpeg_decoder_row_stride(c89jpeg_u16 width, c89jpeg_decode_format fmt, c89jpeg_u8 source_components);
c89jpeg_u32 c89jpeg_decoder_output_size(c89jpeg_u16 width, c89jpeg_u16 height, c89jpeg_decode_format fmt, c89jpeg_u8 source_components);
c89jpeg_u32 c89jpeg_decoder_progressive_workspace_size(const c89jpeg_image_info *info);
c89jpeg_u32 c89jpeg_decoder_sink_strip_size(const c89jpeg_image_info *info, c89jpeg_decode_format fmt);
c89jpeg_u32 c89jpeg_decoder_sink_strip_size_region(const c89jpeg_image_info *info, const c89jpeg_rect *region, c89jpeg_decode_format fmt);
void c89jpeg_decoder_imcu_size(const c89jpeg_image_info *info, c89jpeg_u16 *out_width, c89jpeg_u16 *out_height);
c89jpeg_status c89jpeg_resolve_roi(const c89jpeg_image_info *info, const c89jpeg_rect *request, c89jpeg_roi_mode mode, c89jpeg_rect *resolved);

c89jpeg_status c89jpeg_huffman_table_init(c89jpeg_huffman_table *tab, const c89jpeg_u8 *bits, const c89jpeg_u8 *vals, c89jpeg_u16 count);
c89jpeg_status c89jpeg_huffman_table_init_std(c89jpeg_huffman_table *tab, int table_class, int chroma);

void c89jpeg_tables_init(c89jpeg_tables *tables);
c89jpeg_status c89jpeg_tables_prepare(c89jpeg_encoder *enc, const c89jpeg_encode_params *params, c89jpeg_tables *tables);
c89jpeg_status c89jpeg_tables_prepare_source(c89jpeg_encoder *enc, const c89jpeg_encode_source_params *params, c89jpeg_tables *tables);
c89jpeg_status c89jpeg_tables_load(c89jpeg_tables *tables, const c89jpeg_u8 *data, c89jpeg_u32 size);
c89jpeg_status c89jpeg_tables_extract(c89jpeg_tables *tables, const c89jpeg_u8 *data, c89jpeg_u32 size);
c89jpeg_status c89jpeg_write_tables_sink(const c89jpeg_tables *tables, c89jpeg_write_fn write_fn, void *write_user, c89jpeg_u32 *bytes_written);
c89jpeg_status c89jpeg_write_tables_memory(const c89jpeg_tables *tables, c89jpeg_u8 *out_buf, c89jpeg_u32 out_capacity, c89jpeg_u32 *out_size);

void c89jpeg_encoder_init(c89jpeg_encoder *enc);
void c89jpeg_decoder_init(c89jpeg_decoder *dec);

c89jpeg_status c89jpeg_encode_sink(c89jpeg_encoder *enc, const c89jpeg_encode_params *params, c89jpeg_write_fn write_fn, void *write_user, c89jpeg_u32 *bytes_written);
c89jpeg_status c89jpeg_encode_memory(c89jpeg_encoder *enc, const c89jpeg_encode_params *params, c89jpeg_u8 *out_buf, c89jpeg_u32 out_capacity, c89jpeg_u32 *out_size);
c89jpeg_status c89jpeg_encode_abbreviated_sink(c89jpeg_encoder *enc, const c89jpeg_encode_params *params, const c89jpeg_tables *tables, c89jpeg_write_fn write_fn, void *write_user, c89jpeg_u32 *bytes_written);
c89jpeg_status c89jpeg_encode_abbreviated_memory(c89jpeg_encoder *enc, const c89jpeg_encode_params *params, const c89jpeg_tables *tables, c89jpeg_u8 *out_buf, c89jpeg_u32 out_capacity, c89jpeg_u32 *out_size);
c89jpeg_status c89jpeg_encode_source_sink(c89jpeg_encoder *enc, const c89jpeg_encode_source_params *params, c89jpeg_write_fn write_fn, void *write_user, c89jpeg_u32 *bytes_written);
c89jpeg_status c89jpeg_encode_source_memory(c89jpeg_encoder *enc, const c89jpeg_encode_source_params *params, c89jpeg_u8 *out_buf, c89jpeg_u32 out_capacity, c89jpeg_u32 *out_size);
c89jpeg_status c89jpeg_encode_abbreviated_source_sink(c89jpeg_encoder *enc, const c89jpeg_encode_source_params *params, const c89jpeg_tables *tables, c89jpeg_write_fn write_fn, void *write_user, c89jpeg_u32 *bytes_written);
c89jpeg_status c89jpeg_encode_abbreviated_source_memory(c89jpeg_encoder *enc, const c89jpeg_encode_source_params *params, const c89jpeg_tables *tables, c89jpeg_u8 *out_buf, c89jpeg_u32 out_capacity, c89jpeg_u32 *out_size);

c89jpeg_status c89jpeg_probe(c89jpeg_decoder *dec, const c89jpeg_u8 *data, c89jpeg_u32 size, c89jpeg_image_info *out_info);
c89jpeg_status c89jpeg_probe_source(c89jpeg_decoder *dec, c89jpeg_read_fn read_fn, void *read_user, c89jpeg_u8 *input_buffer, c89jpeg_u32 input_buffer_size, c89jpeg_image_info *out_info);
c89jpeg_status c89jpeg_probe_abbreviated(c89jpeg_decoder *dec, const c89jpeg_tables *tables, const c89jpeg_u8 *data, c89jpeg_u32 size, c89jpeg_image_info *out_info);
c89jpeg_status c89jpeg_probe_abbreviated_source(c89jpeg_decoder *dec, const c89jpeg_tables *tables, c89jpeg_read_fn read_fn, void *read_user, c89jpeg_u8 *input_buffer, c89jpeg_u32 input_buffer_size, c89jpeg_image_info *out_info);
c89jpeg_status c89jpeg_decode(c89jpeg_decoder *dec, const c89jpeg_decode_params *params, c89jpeg_image_info *out_info);
c89jpeg_status c89jpeg_decode_sink(c89jpeg_decoder *dec, const c89jpeg_decode_sink_params *params, c89jpeg_row_sink_fn row_fn, void *row_user, c89jpeg_image_info *out_info);
c89jpeg_status c89jpeg_decode_source(c89jpeg_decoder *dec, const c89jpeg_decode_source_params *params, c89jpeg_image_info *out_info);
c89jpeg_status c89jpeg_decode_source_sink(c89jpeg_decoder *dec, const c89jpeg_decode_source_sink_params *params, c89jpeg_row_sink_fn row_fn, void *row_user, c89jpeg_image_info *out_info);
c89jpeg_status c89jpeg_decode_abbreviated(c89jpeg_decoder *dec, const c89jpeg_tables *tables, const c89jpeg_decode_params *params, c89jpeg_image_info *out_info);
c89jpeg_status c89jpeg_decode_abbreviated_sink(c89jpeg_decoder *dec, const c89jpeg_tables *tables, const c89jpeg_decode_sink_params *params, c89jpeg_row_sink_fn row_fn, void *row_user, c89jpeg_image_info *out_info);
c89jpeg_status c89jpeg_decode_abbreviated_source(c89jpeg_decoder *dec, const c89jpeg_tables *tables, const c89jpeg_decode_source_params *params, c89jpeg_image_info *out_info);
c89jpeg_status c89jpeg_decode_abbreviated_source_sink(c89jpeg_decoder *dec, const c89jpeg_tables *tables, const c89jpeg_decode_source_sink_params *params, c89jpeg_row_sink_fn row_fn, void *row_user, c89jpeg_image_info *out_info);

void c89jpeg_decoder_resume_init(c89jpeg_decoder_resume *state);
c89jpeg_status c89jpeg_decoder_resume_begin(c89jpeg_decoder_resume *state, c89jpeg_decoder *dec, const c89jpeg_decode_resume_params *params);
c89jpeg_status c89jpeg_decoder_resume_feed(c89jpeg_decoder_resume *state, const c89jpeg_u8 *data, c89jpeg_u32 size, int final_chunk);
c89jpeg_status c89jpeg_decoder_resume_run(c89jpeg_decoder_resume *state, c89jpeg_row_sink_fn row_fn, void *row_user, c89jpeg_image_info *out_info);
int c89jpeg_decoder_resume_is_finished(const c89jpeg_decoder_resume *state);

void c89jpeg_encoder_resume_init(c89jpeg_encoder_resume *state);
c89jpeg_status c89jpeg_encoder_resume_begin(c89jpeg_encoder_resume *state, c89jpeg_encoder *enc, const c89jpeg_encode_params *params, const c89jpeg_encode_resume_params *resume_params, c89jpeg_u32 *out_total_size);
c89jpeg_status c89jpeg_encoder_resume_begin_source(c89jpeg_encoder_resume *state, c89jpeg_encoder *enc, const c89jpeg_encode_source_resume_params *params);
c89jpeg_status c89jpeg_encoder_resume_feed_rows(c89jpeg_encoder_resume *state, const c89jpeg_u8 *rows, c89jpeg_u32 row_count, c89jpeg_u32 row_stride, c89jpeg_u32 *rows_accepted);
void c89jpeg_encoder_resume_finish_input(c89jpeg_encoder_resume *state);
c89jpeg_status c89jpeg_encoder_resume_run(c89jpeg_encoder_resume *state, c89jpeg_write_fn write_fn, void *write_user, c89jpeg_u32 *bytes_written);
c89jpeg_status c89jpeg_encoder_resume_pull(c89jpeg_encoder_resume *state, c89jpeg_u8 *out_buf, c89jpeg_u32 out_capacity, c89jpeg_u32 *out_size);
int c89jpeg_encoder_resume_is_finished(const c89jpeg_encoder_resume *state);
int c89jpeg_encoder_resume_needs_input(const c89jpeg_encoder_resume *state);
c89jpeg_u32 c89jpeg_encoder_resume_source_rows_needed(const c89jpeg_encoder_resume *state);
c89jpeg_u32 c89jpeg_encoder_resume_source_rows_accepted(const c89jpeg_encoder_resume *state);
c89jpeg_u32 c89jpeg_encoder_resume_total_size(const c89jpeg_encoder_resume *state);
c89jpeg_u32 c89jpeg_encoder_resume_remaining(const c89jpeg_encoder_resume *state);
c89jpeg_u32 c89jpeg_encoder_resume_min_output_buffer(void);
c89jpeg_u32 c89jpeg_encoder_resume_replay_buffer_size(c89jpeg_u16 width, c89jpeg_u16 height, c89jpeg_u32 stride_bytes);

void c89jpeg_tables_trainer_init(c89jpeg_tables_trainer *trainer);
c89jpeg_status c89jpeg_tables_trainer_prepare(c89jpeg_tables_trainer *trainer, c89jpeg_encoder *enc, const c89jpeg_encode_params *params);
c89jpeg_status c89jpeg_tables_trainer_add_image(c89jpeg_tables_trainer *trainer, c89jpeg_encoder *enc, const c89jpeg_encode_params *params);
c89jpeg_status c89jpeg_tables_trainer_build(c89jpeg_tables_trainer *trainer, c89jpeg_tables *tables);
c89jpeg_status c89jpeg_tables_trainer_prepare_source(c89jpeg_tables_trainer *trainer, c89jpeg_encoder *enc, const c89jpeg_encode_source_params *params);
c89jpeg_status c89jpeg_tables_trainer_add_image_source(c89jpeg_tables_trainer *trainer, c89jpeg_encoder *enc, const c89jpeg_encode_source_params *params);

void c89jpeg_tables_session_init(c89jpeg_tables_session *session);
c89jpeg_status c89jpeg_tables_session_prepare(c89jpeg_tables_session *session, c89jpeg_encoder *enc, const c89jpeg_encode_params *params);
c89jpeg_status c89jpeg_tables_session_set(c89jpeg_tables_session *session, const c89jpeg_tables *tables, c89jpeg_pixel_format pixel_format, c89jpeg_subsampling subsampling);
c89jpeg_status c89jpeg_tables_session_check_image(const c89jpeg_tables_session *session, const c89jpeg_encode_params *params);
c89jpeg_status c89jpeg_tables_session_check_image_source(const c89jpeg_tables_session *session, const c89jpeg_encode_source_params *params);
c89jpeg_status c89jpeg_tables_session_write_tables_sink(c89jpeg_tables_session *session, c89jpeg_write_fn write_fn, void *write_user, c89jpeg_u32 *bytes_written);
c89jpeg_status c89jpeg_tables_session_write_tables_memory(c89jpeg_tables_session *session, c89jpeg_u8 *out_buf, c89jpeg_u32 out_capacity, c89jpeg_u32 *out_size);
c89jpeg_status c89jpeg_tables_session_begin_image(c89jpeg_tables_session *session, c89jpeg_encoder_resume *resume, c89jpeg_encoder *enc, const c89jpeg_encode_params *params, const c89jpeg_encode_resume_params *resume_params, c89jpeg_u32 *out_total_size);
c89jpeg_status c89jpeg_tables_session_begin_image_source(c89jpeg_tables_session *session, c89jpeg_encoder_resume *resume, c89jpeg_encoder *enc, const c89jpeg_encode_source_resume_params *params);
c89jpeg_status c89jpeg_tables_session_prepare_trained(c89jpeg_tables_session *session, c89jpeg_tables_trainer *trainer, c89jpeg_tables *out_tables);
c89jpeg_status c89jpeg_tables_session_encode_image_source_sink(c89jpeg_tables_session *session, c89jpeg_encoder *enc, const c89jpeg_encode_source_params *params, c89jpeg_write_fn write_fn, void *write_user, c89jpeg_u32 *bytes_written);
c89jpeg_status c89jpeg_tables_session_encode_image_source_memory(c89jpeg_tables_session *session, c89jpeg_encoder *enc, const c89jpeg_encode_source_params *params, c89jpeg_u8 *out_buf, c89jpeg_u32 out_capacity, c89jpeg_u32 *out_size);

#if defined(__cplusplus)
}
#endif

#endif
