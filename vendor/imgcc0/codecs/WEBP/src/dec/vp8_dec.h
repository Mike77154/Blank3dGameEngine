#ifndef GWP_DEC_VP8_DEC_H_
#define GWP_DEC_VP8_DEC_H_

#include "../webp/decode.h"
#include "vp8_frame.h"
#include "vp8_bool.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GWP_VP8_MAX_SEGMENTS 4u
#define GWP_VP8_MAX_TOKEN_PARTITIONS 8u
#define GWP_VP8_MB_FEATURE_TREE_PROBS 3u
#define GWP_VP8_BLOCK_CONTEXTS 4u
#define GWP_VP8_Y_MODE_COUNT 5u
#define GWP_VP8_UV_MODE_COUNT 4u
#define GWP_VP8_B_MODE_COUNT 10u
#define GWP_VP8_SUBBLOCKS_PER_MB 16u
#define GWP_VP8_COEFF_BLOCK_TYPES 4u
#define GWP_VP8_COEFF_BANDS 8u
#define GWP_VP8_PREV_COEFF_CONTEXTS 3u
#define GWP_VP8_ENTROPY_NODES 11u
#define GWP_VP8_TOKEN_COUNT 12u
#define GWP_VP8_MAX_MB_COLS (GWP_MAX_IMAGE_WIDTH / 16u)

typedef enum GWPVP8YMode {
  GWP_VP8_YMODE_DC = 0,
  GWP_VP8_YMODE_V = 1,
  GWP_VP8_YMODE_H = 2,
  GWP_VP8_YMODE_TM = 3,
  GWP_VP8_YMODE_B_PRED = 4
} GWPVP8YMode;

typedef enum GWPVP8UVMode {
  GWP_VP8_UVMODE_DC = 0,
  GWP_VP8_UVMODE_V = 1,
  GWP_VP8_UVMODE_H = 2,
  GWP_VP8_UVMODE_TM = 3
} GWPVP8UVMode;

typedef enum GWPVP8BMode {
  GWP_VP8_BMODE_DC = 0,
  GWP_VP8_BMODE_TM = 1,
  GWP_VP8_BMODE_VE = 2,
  GWP_VP8_BMODE_HE = 3,
  GWP_VP8_BMODE_LD = 4,
  GWP_VP8_BMODE_RD = 5,
  GWP_VP8_BMODE_VR = 6,
  GWP_VP8_BMODE_VL = 7,
  GWP_VP8_BMODE_HD = 8,
  GWP_VP8_BMODE_HU = 9
} GWPVP8BMode;

typedef struct GWPVP8SegmentHeader {
  GWPBool enabled;
  GWPBool update_map;
  GWPBool update_data;
  GWPBool absolute_delta;
  int quant_idx[GWP_VP8_MAX_SEGMENTS];
  int lf_level[GWP_VP8_MAX_SEGMENTS];
  GWPu8 tree_probs[GWP_VP8_MB_FEATURE_TREE_PROBS];
} GWPVP8SegmentHeader;

typedef struct GWPVP8LoopFilterHeader {
  GWPBool use_simple;
  GWPu8 level;
  GWPu8 sharpness;
  GWPBool delta_enabled;
  int ref_delta[GWP_VP8_BLOCK_CONTEXTS];
  int mode_delta[GWP_VP8_BLOCK_CONTEXTS];
} GWPVP8LoopFilterHeader;

typedef struct GWPVP8TokenPartitions {
  GWPu32 partition_count;
  GWPData partition[GWP_VP8_MAX_TOKEN_PARTITIONS];
} GWPVP8TokenPartitions;

typedef struct GWPVP8QuantHeader {
  GWPu8 q_index;
  int y1_dc_delta_q;
  int y2_dc_delta_q;
  int y2_ac_delta_q;
  int uv_dc_delta_q;
  int uv_ac_delta_q;
  GWPBool delta_update;
} GWPVP8QuantHeader;

typedef struct GWPVP8ReferenceHeader {
  GWPBool refresh_gf;
  GWPBool refresh_arf;
  GWPu8 copy_gf;
  GWPu8 copy_arf;
  GWPu8 sign_bias_golden;
  GWPu8 sign_bias_altref;
  GWPBool refresh_entropy;
  GWPBool refresh_last;
} GWPVP8ReferenceHeader;

typedef struct GWPVP8DequantFactors {
  int quant_idx;
  int y1_dc;
  int y1_ac;
  int y2_dc;
  int y2_ac;
  int uv_dc;
  int uv_ac;
} GWPVP8DequantFactors;

typedef struct GWPVP8EntropyHeader {
  GWPu8 coeff_probs[GWP_VP8_COEFF_BLOCK_TYPES][GWP_VP8_COEFF_BANDS]
                  [GWP_VP8_PREV_COEFF_CONTEXTS][GWP_VP8_ENTROPY_NODES];
  GWPu32 coeff_update_count;
  GWPBool coeff_skip_enabled;
  GWPu8 coeff_skip_prob;
  GWPu8 y_mode_probs[4];
  GWPu8 uv_mode_probs[3];
} GWPVP8EntropyHeader;

typedef struct GWPVP8ModeSummary {
  GWPu32 macroblock_count;
  GWPu32 skipped_macroblocks;
  GWPu32 bpred_macroblocks;
  GWPu32 segment_hist[GWP_VP8_MAX_SEGMENTS];
  GWPu32 y_mode_hist[GWP_VP8_Y_MODE_COUNT];
  GWPu32 uv_mode_hist[GWP_VP8_UV_MODE_COUNT];
  GWPu32 b_mode_hist[GWP_VP8_B_MODE_COUNT];
} GWPVP8ModeSummary;

typedef struct GWPVP8ResidualSummary {
  GWPu32 macroblock_count;
  GWPu32 skipped_macroblocks;
  GWPu32 y2_macroblocks;
  GWPu32 token_partition_count;
  GWPu32 coeff_block_count;
  GWPu32 blocks_with_coeffs;
  GWPu32 y_blocks_with_coeffs;
  GWPu32 uv_blocks_with_coeffs;
  GWPu32 y2_blocks_with_coeffs;
  GWPu32 nonzero_coeff_count;
  GWPu32 max_abs_coeff;
  GWPu32 token_hist[GWP_VP8_TOKEN_COUNT];
  GWPu32 token_partition_bytes_touched[GWP_VP8_MAX_TOKEN_PARTITIONS];
} GWPVP8ResidualSummary;

typedef struct GWPVP8ControlHeader {
  GWPVP8FrameHeader frame;
  GWPu32 mb_cols;
  GWPu32 mb_rows;
  GWPu8 color_space;
  GWPu8 clamping_type;
  GWPVP8SegmentHeader segment;
  GWPVP8LoopFilterHeader loop_filter;
  GWPVP8TokenPartitions token_partitions;
  GWPVP8QuantHeader quant;
  GWPVP8ReferenceHeader reference;
  GWPVP8EntropyHeader entropy;
  GWPVP8ModeSummary mode_summary;
  GWPVP8ResidualSummary residual_summary;
  GWPVP8DequantFactors dequant[GWP_VP8_MAX_SEGMENTS];
  GWPu32 control_header_bytes_touched;
  GWPu32 entropy_header_bytes_touched;
  GWPu32 part0_bytes_touched;
} GWPVP8ControlHeader;

GWPStatusCode GWPVP8ParseControlHeader(const GWPu8* data,
                                       GWPu32 data_size,
                                       GWPVP8ControlHeader* header);

GWPStatusCode GWPVP8ParseControlHeaderWithModeState(const GWPu8* data,
                                                    GWPu32 data_size,
                                                    GWPVP8ControlHeader* header,
                                                    GWPVP8BoolDecoder* out_mode_br);

GWPStatusCode GWPDecodeVP8Stub(const GWPu8* data,
                               GWPu32 data_size,
                               const GWPDecoderOptions* options,
                               GWPBitstreamFeatures* out_features);

#ifdef __cplusplus
}    /* extern "C" */
#endif

#endif  /* GWP_DEC_VP8_DEC_H_ */
