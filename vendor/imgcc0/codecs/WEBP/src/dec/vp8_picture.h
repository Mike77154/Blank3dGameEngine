#ifndef GWP_DEC_VP8_PICTURE_H_
#define GWP_DEC_VP8_PICTURE_H_

#include "vp8_dec.h"
#include "../utils/arena.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GWPVP8FrameBuffer {
  GWPu32 width;
  GWPu32 height;
  GWPu32 mb_cols;
  GWPu32 mb_rows;
  GWPu32 y_stride;
  GWPu32 y_height;
  GWPu32 uv_stride;
  GWPu32 uv_height;
  GWPu8* y;
  GWPu8* u;
  GWPu8* v;
  GWPu8* mb_segment_ids;
  GWPu8* mb_y_modes;
  GWPu8* mb_has_coeffs;
  GWPu8* mb_filter_levels;
} GWPVP8FrameBuffer;

GWPStatusCode GWPVP8FrameBufferInit(const GWPVP8ControlHeader* header,
                                    const GWPDecoderOptions* options,
                                    GWPArena* arena,
                                    GWPVP8FrameBuffer* fb);
void GWPVP8FrameBufferFillDefaults(GWPVP8FrameBuffer* fb);
void GWPVP8FrameBufferFilter(const GWPVP8ControlHeader* header,
                             GWPVP8FrameBuffer* fb);
void GWPVP8FrameBufferPack(const GWPVP8FrameBuffer* fb,
                           const GWPDecoderOptions* options);


GWPStatusCode GWPDecodeVP8WithState(const GWPu8* data,
                                    GWPu32 data_size,
                                    const GWPDecoderOptions* options,
                                    GWPBitstreamFeatures* out_features,
                                    GWPVP8ControlHeader* out_control,
                                    GWPVP8FrameBuffer* out_fb);


#ifdef __cplusplus
}    /* extern "C" */
#endif

#endif  /* GWP_DEC_VP8_PICTURE_H_ */
