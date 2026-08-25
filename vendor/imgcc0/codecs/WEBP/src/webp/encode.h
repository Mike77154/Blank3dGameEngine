#ifndef GWP_WEBP_ENCODE_H_
#define GWP_WEBP_ENCODE_H_

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum GWPRawPixelFormat {
  GWP_RAW_RGBA = 0,
  GWP_RAW_BGRA = 1,
  GWP_RAW_ARGB = 2
} GWPRawPixelFormat;

typedef enum GWPAlphaFilteringMode {
  GWP_ALPHA_FILTER_NONE = 0,
  GWP_ALPHA_FILTER_FAST = 1,
  GWP_ALPHA_FILTER_BEST = 2
} GWPAlphaFilteringMode;

typedef enum GWPEncodePreset {
  GWP_PRESET_DEFAULT = 0,
  GWP_PRESET_PHOTO = 1,
  GWP_PRESET_PICTURE = 2,
  GWP_PRESET_DRAWING = 3,
  GWP_PRESET_ICON = 4,
  GWP_PRESET_TEXT = 5
} GWPEncodePreset;

typedef struct GWPEncodeConfig {
  GWPBool lossless;
  GWPBool exact;
  GWPu32 quality;             /* 0..100 */
  GWPu32 method;              /* 0..6-ish */
  GWPBool use_sharp_yuv;
  GWPEncodePreset preset;
  GWPu32 near_lossless;       /* 0..100, 100 = off */
  GWPu32 alpha_quality;       /* cwebp alpha_q */
  GWPu32 filter_strength;     /* lossy filter strength hint (0..100) */
  GWPu32 vp8_partitions;      /* 0=auto, else 1/2/4/8 */
  GWPu32 vp8_segments;        /* 0=auto, else 1..4 */
  GWPBool vp8_enable_coeff_skip;
  GWPBool vp8_enable_prob_updates;
  GWPBool vp8_enable_subblock_modes;
  GWPBool vp8_enable_uv_modes;
  GWPBool vp8_enable_intra16_modes;
  GWPBool vp8_enable_ac_coeffs;
  GWPAlphaFilteringMode alpha_filtering;
  GWPBool allow_external_tools;   /* enables optional cwebp/img2webp bridge */
  const char* cwebp_path;         /* default: "cwebp" */
  const char* img2webp_path;      /* default: "img2webp" */
  const char* gif2webp_path;      /* default: "gif2webp" */
  GWPu8* output_buffer;
  GWPu32 output_buffer_size;
  void* scratch;
  GWPu32 scratch_size;
} GWPEncodeConfig;

typedef struct GWPVP8LBitstreamOptions {
  GWPBool exact;
  GWPu32 near_lossless;       /* 0..100 */
  GWPBool use_subtract_green;
  GWPBool use_color_cache;
  GWPu32 color_cache_bits;    /* 0 or 1..11 */
  GWPBool use_backrefs;       /* limited encoder: run-length via distance=1 */
} GWPVP8LBitstreamOptions;

void GWPEncodeConfigInit(GWPEncodeConfig* config);
void GWPVP8LBitstreamOptionsInit(GWPVP8LBitstreamOptions* options);
typedef struct GWPVP8LossyMacroblockStat {
  GWPu16 mean_y;
  GWPu16 variance_y;
  GWPu8 segment_id;
  GWPu8 filter_strength;
  GWPu8 has_alpha;
  GWPu8 reserved;
} GWPVP8LossyMacroblockStat;

typedef struct GWPVP8LossyPlan {
  GWPu32 width;
  GWPu32 height;
  GWPu32 quality;
  GWPu32 mb_cols;
  GWPu32 mb_rows;
  GWPu32 macroblock_count;
  GWPu32 average_luma;
  GWPu32 average_variance;
  GWPu32 suggested_segments;
  GWPu32 suggested_partitions;
  GWPu32 suggested_filter_strength;
  GWPu32 suggested_sharpness;
  GWPu32 alpha_macroblocks;
  GWPVP8LossyMacroblockStat* macroblocks;
  GWPu32 macroblocks_capacity;
} GWPVP8LossyPlan;

typedef struct GWPVP8IntraCell {
  GWPu16 x;
  GWPu16 y;
  GWPu8 width;
  GWPu8 height;
  GWPu8 segment_id;
  GWPu8 mode;
  GWPu8 qindex;
  GWPu8 alpha_present;
  GWPu8 r;
  GWPu8 g;
  GWPu8 b;
  GWPu8 a;
  GWPu8 y_avg;
  GWPu8 u_avg;
  GWPu8 v_avg;
  GWPu8 reserved;
} GWPVP8IntraCell;

typedef struct GWPVP8IntraEmitPlan {
  GWPu32 width;
  GWPu32 height;
  GWPu32 mb_cols;
  GWPu32 mb_rows;
  GWPu32 cell_count;
  GWPu32 cells_capacity;
  GWPu32 average_qindex;
  GWPu32 suggested_partitions;
  GWPu32 suggested_filter_strength;
  GWPVP8LossyPlan lossy_plan;
  GWPVP8IntraCell* cells;
} GWPVP8IntraEmitPlan;

void GWPVP8LossyPlanInit(GWPVP8LossyPlan* plan);
void GWPVP8IntraEmitPlanInit(GWPVP8IntraEmitPlan* plan);
typedef struct GWPVP8NativeEncodeStats {
  GWPu32 width;
  GWPu32 height;
  GWPu32 mb_cols;
  GWPu32 mb_rows;
  GWPu32 q_index;
  GWPu32 filter_level;
  GWPu32 sharpness;
  GWPu32 first_partition_size;
  GWPu32 token_partition_size;
  GWPu32 token_partition_count;
  GWPu32 segment_count;
  GWPu32 skipped_macroblocks;
  GWPu32 coeff_prob_updates;
  GWPu32 y_mode_mask;
  GWPu32 b_mode_mask;
  GWPu32 uv_mode_mask;
  GWPu32 y2_blocks_non_zero;
  GWPu32 y_blocks_non_zero;
  GWPu32 uv_blocks_non_zero;
  GWPu32 macroblocks;
} GWPVP8NativeEncodeStats;

void GWPVP8NativeEncodeStatsInit(GWPVP8NativeEncodeStats* stats);


GWPStatusCode GWPEncodePixels(const GWPu8* pixels,
                              GWPu32 width,
                              GWPu32 height,
                              GWPu32 stride,
                              GWPRawPixelFormat pixel_format,
                              const GWPEncodeConfig* config,
                              GWPu32* out_size);

/* Raw lossless bitstream helper, mainly for animation assembly paths. */
GWPStatusCode GWPEncodeVP8Bitstream(const GWPu8* pixels,
                                    GWPu32 width,
                                    GWPu32 height,
                                    GWPu32 stride,
                                    GWPRawPixelFormat pixel_format,
                                    const GWPEncodeConfig* config,
                                    GWPu8* out_buf,
                                    GWPu32 out_buf_size,
                                    GWPu32* out_size,
                                    GWPVP8NativeEncodeStats* out_stats);

GWPStatusCode GWPEncodeVP8LBitstream(const GWPu8* pixels,
                                     GWPu32 width,
                                     GWPu32 height,
                                     GWPu32 stride,
                                     GWPRawPixelFormat pixel_format,
                                     GWPBool exact,
                                     GWPu8* out_buf,
                                     GWPu32 out_buf_size,
                                     GWPu32* out_size,
                                     GWPBool* out_has_alpha);

GWPStatusCode GWPEncodeVP8LBitstreamEx(const GWPu8* pixels,
                                       GWPu32 width,
                                       GWPu32 height,
                                       GWPu32 stride,
                                       GWPRawPixelFormat pixel_format,
                                       const GWPVP8LBitstreamOptions* options,
                                       GWPu8* out_buf,
                                       GWPu32 out_buf_size,
                                       GWPu32* out_size,
                                       GWPBool* out_has_alpha);

/* Conservative helpers for callers provisioning static buffers. */
GWPu32 GWPEstimateLosslessBitstreamSize(GWPu32 width, GWPu32 height);
GWPu32 GWPEstimateNativeVP8BitstreamSize(GWPu32 width, GWPu32 height);
GWPu32 GWPEstimateWebPSizeFromPixels(GWPu32 width, GWPu32 height);
GWPu32 GWPEstimateLossyWebPSizeFromPixels(GWPu32 width, GWPu32 height);
GWPu32 GWPEstimateVP8LossyPlanScratch(GWPu32 width, GWPu32 height);
GWPu32 GWPEstimateVP8IntraEmitScratch(GWPu32 width, GWPu32 height);
GWPu32 GWPEstimateNativeLossyScratch(GWPu32 width, GWPu32 height);

GWPStatusCode GWPBuildVP8IntraEmitPlan(const GWPu8* pixels,
                                       GWPu32 width,
                                       GWPu32 height,
                                       GWPu32 stride,
                                       GWPRawPixelFormat pixel_format,
                                       const GWPEncodeConfig* config,
                                       void* scratch,
                                       GWPu32 scratch_size,
                                       GWPVP8IntraEmitPlan* out_plan);

GWPStatusCode GWPEncodeLossyNativeProxy(const GWPu8* pixels,
                                        GWPu32 width,
                                        GWPu32 height,
                                        GWPu32 stride,
                                        GWPRawPixelFormat pixel_format,
                                        const GWPEncodeConfig* config,
                                        GWPu32* out_size);

GWPStatusCode GWPAnalyzeVP8LossyPlan(const GWPu8* pixels,
                                     GWPu32 width,
                                     GWPu32 height,
                                     GWPu32 stride,
                                     GWPRawPixelFormat pixel_format,
                                     const GWPEncodeConfig* config,
                                     void* scratch,
                                     GWPu32 scratch_size,
                                     GWPVP8LossyPlan* out_plan);

#ifdef __cplusplus
}    /* extern "C" */
#endif

#endif  /* GWP_WEBP_ENCODE_H_ */
