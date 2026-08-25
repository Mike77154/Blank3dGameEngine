#ifndef GWP_WEBP_ANIM_DECODE_H_
#define GWP_WEBP_ANIM_DECODE_H_

#include "decode.h"
#include "demux.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GWPAnimDecoderOptions {
  GWPPixelFormat pixel_format;
  GWPBool strict;
  GWPu32 max_width;
  GWPu32 max_height;
  void* scratch;
  GWPu32 scratch_size;
} GWPAnimDecoderOptions;

typedef struct GWPAnimInfo {
  GWPu32 canvas_width;
  GWPu32 canvas_height;
  GWPu32 frame_count;
  GWPu32 loop_count;
  GWPu32 background_color;
  GWPBool has_alpha;
  GWPBool has_icc;
  GWPBool has_exif;
  GWPBool has_xmp;
} GWPAnimInfo;

typedef struct GWPAnimDecoder {
  GWPDemuxer demux;
  GWPAnimDecoderOptions options;
  GWPu8* canvas_rgba;
  GWPu8* frame_rgba;
  GWPu8* nested_scratch;
  GWPu32 nested_scratch_size;
  GWPu32 canvas_stride;
  GWPu32 frame_index;
  GWPu32 next_timestamp_ms;
  GWPu8 bg_rgba[4];
  GWPu32 prev_x;
  GWPu32 prev_y;
  GWPu32 prev_w;
  GWPu32 prev_h;
  GWPu8 prev_dispose_method;
  GWPBool initialized;
} GWPAnimDecoder;

void GWPAnimDecoderOptionsInit(GWPAnimDecoderOptions* options);
GWPStatusCode GWPAnimDecoderInit(GWPAnimDecoder* dec,
                                 const GWPu8* data,
                                 GWPu32 data_size,
                                 const GWPAnimDecoderOptions* options);
GWPStatusCode GWPAnimDecoderGetInfo(const GWPAnimDecoder* dec, GWPAnimInfo* info);
GWPBool GWPAnimDecoderHasMoreFrames(const GWPAnimDecoder* dec);
GWPStatusCode GWPAnimDecoderGetNext(GWPAnimDecoder* dec,
                                    GWPu8* out_pixels,
                                    GWPu32 out_pixels_size,
                                    GWPu32 out_stride,
                                    GWPu32* out_timestamp_ms);
void GWPAnimDecoderReset(GWPAnimDecoder* dec);

#ifdef __cplusplus
}    /* extern "C" */
#endif

#endif  /* GWP_WEBP_ANIM_DECODE_H_ */
