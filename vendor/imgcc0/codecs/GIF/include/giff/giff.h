#ifndef GIFF_H
#define GIFF_H

#include "giff_config.h"
#include "giff_types.h"
#include "giff_fixed.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum giff_result {
    GIFF_OK = 0,
    GIFF_E_NEED_MORE_INPUT = 1,
    GIFF_E_DONE = 2,

    GIFF_E_INVALID_ARGUMENT = -1,
    GIFF_E_INVALID_SIGNATURE = -2,
    GIFF_E_UNSUPPORTED_VERSION = -3,
    GIFF_E_BAD_DIMENSIONS = -4,
    GIFF_E_BAD_COLOR_TABLE = -5,
    GIFF_E_BAD_BLOCK = -6,
    GIFF_E_BAD_LZW_CODE = -7,
    GIFF_E_TRUNCATED = -8,
    GIFF_E_NO_WORKSPACE = -9,
    GIFF_E_OUTPUT_OVERFLOW = -10,
    GIFF_E_UNSUPPORTED = -11,
    GIFF_E_INTERNAL = -12,
    GIFF_E_IO = -13
} giff_result;

typedef enum giff_output_mode {
    GIFF_OUTPUT_INDEXED = 0,
    GIFF_OUTPUT_RGBA8888 = 1
} giff_output_mode;

typedef enum giff_restore_previous_mode {
    GIFF_RESTORE_PREVIOUS_DISABLE = 0,
    GIFF_RESTORE_PREVIOUS_BOUNDS  = 1,
    GIFF_RESTORE_PREVIOUS_FULL    = 2
} giff_restore_previous_mode;

typedef enum giff_dither_mode {
    GIFF_DITHER_NONE = 0,
    GIFF_DITHER_ORDERED4X4 = 1,
    GIFF_DITHER_FLOYD_STEINBERG = 2
} giff_dither_mode;

typedef enum giff_quantizer_mode {
    GIFF_QUANTIZER_MEDIAN_CUT = 0,
    GIFF_QUANTIZER_OCTREE = 1
} giff_quantizer_mode;

typedef enum giff_lzw_strategy {
    GIFF_LZW_STRATEGY_DEFAULT = 0,
    GIFF_LZW_STRATEGY_COMPACT = 1,
    GIFF_LZW_STRATEGY_BALANCED = 2,
    GIFF_LZW_STRATEGY_AGGRESSIVE = 3
} giff_lzw_strategy;

typedef enum giff_palette_decision_mode {
    GIFF_PALETTE_DECISION_KEEP = 0,
    GIFF_PALETTE_DECISION_LOCAL = 1,
    GIFF_PALETTE_DECISION_TEMPORAL_REMAP = 2,
    GIFF_PALETTE_DECISION_SEGMENT_SHARE = 3
} giff_palette_decision_mode;

typedef enum giff_event_type {
    GIFF_EVENT_NONE = 0,
    GIFF_EVENT_INFO,
    GIFF_EVENT_FRAME_BEGIN,
    GIFF_EVENT_FRAME_ROW,
    GIFF_EVENT_FRAME_END,
    GIFF_EVENT_TRAILER,
    GIFF_EVENT_COMMENT,
    GIFF_EVENT_APPLICATION,
    GIFF_EVENT_ERROR
} giff_event_type;

typedef struct giff_io {
    void* user;
    int (*read)(void* user, giff_u8* dst, giff_u32* io_size);
    int (*write)(void* user, const giff_u8* src, giff_u32 size);
    int (*seek)(void* user, long offset, int origin);
} giff_io;

typedef struct giff_info {
    giff_u16 width;
    giff_u16 height;
    giff_u16 global_palette_entries;
    giff_u16 loop_count;
    giff_u8 version_major;
    giff_u8 version_minor;
    giff_u8 has_global_palette;
    giff_u8 background_index;
    giff_u8 aspect_byte;
    giff_u8 color_resolution_bits;
    giff_u8 global_palette_sorted;
} giff_info;

typedef struct giff_frame_info {
    giff_u16 left;
    giff_u16 top;
    giff_u16 width;
    giff_u16 height;
    giff_u16 delay_cs;
    giff_u16 local_palette_entries;
    giff_u8 interlaced;
    giff_u8 has_local_palette;
    giff_u8 transparency_index;
    giff_u8 has_transparency;
    giff_u8 disposal_method;
} giff_frame_info;

typedef struct giff_event {
    giff_event_type type;
    giff_result code;
    giff_info info;
    giff_frame_info frame;
    giff_u16 row_index;
    const giff_u8* indexed_row;
    const giff_rgba8* rgba_row;
    const giff_u8* text_bytes;
    giff_u32 text_size;
} giff_event;

typedef struct giff_decoder_config {
    giff_u16 max_width;
    giff_u16 max_height;
    giff_u8 strict_mode;
    giff_u8 output_mode;
    giff_u8 restore_previous_mode;
    giff_u8 capture_comments;
    giff_u32 user_canvas_stride;
    giff_u8* user_canvas_rgba;
    giff_u32 user_previous_stride;
    giff_u8* user_previous_rgba;
} giff_decoder_config;

typedef struct giff_encoder_config {
    giff_u16 max_width;
    giff_u16 max_height;
    giff_u16 global_palette_entries;
    giff_u8 reserve_transparent;
    giff_u8 dither_mode;
    giff_u8 quantizer_hist_bits;
    giff_u8 quantizer_mode;
    giff_u8 enable_cross_frame_cluster;
    giff_u8 enable_temporal_remap;
    giff_u8 lzw_band_rows;
    giff_u8 temporal_decay_shift;
    giff_u8 temporal_window_frames;
    giff_u8 enable_segment_sharing;
    giff_u8 segment_max_frames;
    giff_u8 segment_similarity_q8_threshold;
    giff_u32 user_indexed_stride;
} giff_encoder_config;

typedef struct giff_decoder {
    giff_decoder_config cfg;
    giff_io io;
    void* workspace;
    giff_u32 workspace_size;

    giff_u8 inline_scratch[GIFF_INLINE_SCRATCH_BYTES];

    giff_u16* lzw_prefix;
    giff_u8*  lzw_suffix;
    giff_u8*  lzw_stack;
    giff_u8*  subblock;
    giff_u8*  row_indexed;
    giff_u8*  frame_indexed;
    giff_u8*  canvas_rgba;
    giff_u8*  previous_rgba;
    giff_u8*  canvas_indexed;
    giff_u8*  previous_indexed;
    giff_u8*  stream_buf;

    giff_info info;
    giff_frame_info frame;
    giff_frame_info previous_frame;

    giff_rgb8 global_palette[256];
    giff_rgb8 local_palette[256];

    const giff_u8* stream_data;
    const giff_u8* parse_ptr;
    const giff_u8* parse_end;

    giff_u32 canvas_stride;
    giff_u32 previous_stride;
    giff_u32 canvas_bytes;
    giff_u32 previous_bytes;
    giff_u32 input_offset;
    giff_u32 stream_size;
    giff_u32 stream_base_offset;
    giff_u32 events_emitted;
    giff_u32 frames_seen;
    giff_u32 stream_buf_cap;
    giff_u32 stream_buf_pos;
    giff_u32 stream_buf_fill;

    giff_u16 pending_delay_cs;
    giff_u16 emit_row_cursor;
    giff_u16 emit_row_count;

    giff_u8 header_bytes[13];
    giff_u8 capture_size;
    giff_u8 header_filled;
    giff_u8 started;
    giff_u8 finished;
    giff_u8 last_result;
    giff_u8 pending_transparency_index;
    giff_u8 pending_has_transparency;
    giff_u8 pending_disposal_method;
    giff_u8 have_previous_frame;
    giff_u8 info_emitted;
    giff_u8 event_phase;
    giff_u8 trailer_emitted;
    giff_u8 source_mode;
    giff_u8 stream_eof;
    giff_u8 pad1;
} giff_decoder;

typedef struct giff_encoder {
    giff_encoder_config cfg;
    giff_io io;
    void* workspace;
    giff_u32 workspace_size;

    giff_u8 inline_scratch[GIFF_INLINE_SCRATCH_BYTES];

    giff_u16* hash_prefix;
    giff_u8*  hash_suffix;
    giff_u16* hash_code;
    giff_u8*  hash_used;
    giff_u8*  packet_buf;
    giff_u8*  row_indexed;
    giff_u8*  frame_indexed;
    giff_u32* quant_hist;
    giff_u16* quant_bins;
    giff_u16* quant_bins_tmp;
    giff_s32* dither_curr;
    giff_s32* dither_next;
    void* oct_nodes;
    giff_u8*  lzw_code_len;
    giff_rgb8* temporal_window_palette;
    giff_u32* temporal_window_hist;

    giff_info stream_info;
    giff_frame_info frame;

    giff_rgb8 global_palette[256];
    giff_rgb8 local_palette[256];
    giff_rgb8 temporal_palette[256];
    giff_rgb8 prev_palette[256];
    giff_rgb8 segment_palette[256];
    giff_u32 temporal_weights[256];
    giff_u32 segment_hist[256];

    giff_u32 frames_written;
    giff_u32 frame_cost_keep;
    giff_u32 frame_cost_local;
    giff_u32 frame_cost_temporal;
    giff_u32 frame_cost_segment;
    giff_u32 frame_cost_chosen;
    giff_u32 output_offset;
    giff_u32 lzw_bitbuf;

    giff_u16 global_palette_entries;
    giff_u16 local_palette_entries;
    giff_u16 frame_rows_written;
    giff_u16 lzw_clear_code;
    giff_u16 lzw_end_code;
    giff_u16 lzw_next_code;
    giff_u16 lzw_prefix_code;
    giff_u16 frame_stride;
    giff_u16 temporal_entries;
    giff_u16 prev_palette_entries;
    giff_u16 temporal_similarity_q8;
    giff_u16 segment_palette_entries;
    giff_u16 segment_similarity_q8;
    giff_u16 segment_frame_count;
    giff_u16 frame_band_rows;
    giff_u16 frame_band_count;
    giff_u16 lzw_band_clears;
    giff_u16 lzw_band_switches;
    giff_u16 temporal_window_similarity_q8;

    giff_u8 started;
    giff_u8 finished;
    giff_u8 last_result;
    giff_u8 frame_active;

    giff_u8 lzw_min_code_size;
    giff_u8 lzw_code_size;
    giff_u8 lzw_bits_in_buf;
    giff_u8 packet_size;
    giff_u8 global_palette_set;
    giff_u8 local_palette_set;
    giff_u8 global_has_transparency;
    giff_u8 local_has_transparency;
    giff_u8 global_transparency_index;
    giff_u8 local_transparency_index;
    giff_u8 lzw_have_prefix;
    giff_u16 current_palette_entries;
    giff_u16 octree_map_entries;
    giff_u16 frame_used_colors;
    giff_u16 frame_encoded_palette_entries;
    giff_u16 frame_entropy_q8;
    giff_u16 frame_reset_threshold;
    giff_u16 frame_periodic_clear;
    giff_u16 frame_stall_threshold;
    giff_u16 lzw_inputs_since_clear;
    giff_u16 lzw_codes_since_clear;
    giff_u16 lzw_stall_run;
    giff_u16 lzw_clear_count;
    giff_u8 interlace_buffered;
    giff_u8 octree_map_valid;
    giff_u8 octree_map_is_local;
    giff_u8 octree_map_has_transparency;
    giff_u8 octree_map_transparency_index;
    giff_u8 frame_sorted_local_palette;
    giff_u8 frame_auto_local_palette;
    giff_u8 frame_sparse_local_remap;
    giff_u8 lzw_strategy;
    giff_u8 lzw_prefix_len;
    giff_u8 temporal_valid;
    giff_u8 temporal_has_transparency;
    giff_u8 temporal_transparency_index;
    giff_u8 prev_palette_valid;
    giff_u8 prev_has_transparency;
    giff_u8 prev_transparency_index;
    giff_u8 frame_temporal_palette;
    giff_u8 frame_clustered_palette;
    giff_u8 band_lzw_tuned;
    giff_u8 temporal_window_count;
    giff_u8 temporal_window_head;
    giff_u8 frame_window_used;
    giff_u8 frame_cost_mode;
    giff_u8 segment_valid;
    giff_u8 segment_has_transparency;
    giff_u8 segment_transparency_index;
    giff_u8 frame_segment_shared;
    giff_u8 frame_segment_break;
    giff_u16 temporal_window_entries[GIFF_TEMPORAL_WINDOW_MAX];
    giff_u8 temporal_window_has_transparency[GIFF_TEMPORAL_WINDOW_MAX];
    giff_u8 temporal_window_transparency_index[GIFF_TEMPORAL_WINDOW_MAX];
} giff_encoder;

const char* giff_result_string(giff_result code);

giff_u32 giff_decoder_workspace_size(const giff_decoder_config* cfg);
giff_u32 giff_encoder_workspace_size(const giff_encoder_config* cfg);

giff_result giff_decoder_init(
    giff_decoder* dec,
    const giff_decoder_config* cfg,
    void* workspace,
    giff_u32 workspace_size
);

giff_result giff_encoder_init(
    giff_encoder* enc,
    const giff_encoder_config* cfg,
    void* workspace,
    giff_u32 workspace_size
);

void giff_decoder_reset(giff_decoder* dec);
void giff_encoder_reset(giff_encoder* enc);

void giff_decoder_attach_io(giff_decoder* dec, const giff_io* io);
void giff_encoder_attach_io(giff_encoder* enc, const giff_io* io);

giff_result giff_decoder_inspect_header(
    const giff_u8* bytes,
    giff_u32 size,
    giff_info* out_info
);

giff_result giff_decoder_feed(
    giff_decoder* dec,
    const giff_u8* data,
    giff_u32 size,
    giff_u32* consumed
);

void giff_decoder_finish_input(giff_decoder* dec);

giff_result giff_decoder_begin_memory(
    giff_decoder* dec,
    const giff_u8* data,
    giff_u32 size
);

giff_result giff_decoder_begin_io(giff_decoder* dec);

giff_result giff_decoder_next_event(
    giff_decoder* dec,
    giff_event* out_event
);

giff_result giff_decoder_decode_memory(
    giff_decoder* dec,
    const giff_u8* data,
    giff_u32 size
);

giff_result giff_decoder_decode_io(giff_decoder* dec);

giff_result giff_encoder_set_global_palette(
    giff_encoder* enc,
    const giff_rgb8* palette,
    giff_u16 entries
);

giff_result giff_encoder_set_local_palette(
    giff_encoder* enc,
    const giff_rgb8* palette,
    giff_u16 entries
);

giff_result giff_encoder_build_palette_from_rgba(
    giff_encoder* enc,
    const giff_rgba8* pixels,
    giff_u16 width,
    giff_u16 height,
    giff_u32 stride,
    giff_u8 to_local_palette,
    giff_u16 max_entries
);

giff_result giff_encoder_begin(
    giff_encoder* enc,
    const giff_info* stream_info
);

giff_result giff_encoder_begin_frame(
    giff_encoder* enc,
    const giff_frame_info* frame
);

giff_result giff_encoder_write_indexed_rows(
    giff_encoder* enc,
    const giff_u8* rows,
    giff_u16 row_count,
    giff_u32 stride
);

giff_result giff_encoder_end_frame(giff_encoder* enc);

giff_result giff_encoder_write_indexed_frame(
    giff_encoder* enc,
    const giff_frame_info* frame,
    const giff_u8* pixels,
    giff_u32 stride
);

giff_result giff_encoder_write_rgba_frame(
    giff_encoder* enc,
    const giff_frame_info* frame,
    const giff_rgba8* pixels,
    giff_u32 stride,
    giff_u8 use_local_palette
);

giff_result giff_encoder_end(giff_encoder* enc);

#ifdef __cplusplus
}
#endif

#endif
