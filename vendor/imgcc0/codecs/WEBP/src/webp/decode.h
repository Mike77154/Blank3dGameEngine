#ifndef GWP_WEBP_DECODE_H_
#define GWP_WEBP_DECODE_H_

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum GWPPixelFormat {
  GWP_PIXFMT_RGBA = 0,
  GWP_PIXFMT_BGRA = 1,
  GWP_PIXFMT_ARGB = 2
} GWPPixelFormat;

typedef struct GWPDecoderOptions {
  GWPPixelFormat pixel_format;
  GWPBool strict;
  GWPu32 max_width;
  GWPu32 max_height;
  GWPu8* output_buffer;
  GWPu32 output_buffer_size;
  GWPu32 output_stride;
  void* scratch;
  GWPu32 scratch_size;
} GWPDecoderOptions;

GWPStatusCode GWPGetFeatures(const GWPu8* data,
                             GWPu32 data_size,
                             GWPBitstreamFeatures* features);

GWPStatusCode GWPDecode(const GWPu8* data,
                        GWPu32 data_size,
                        const GWPDecoderOptions* options,
                        GWPBitstreamFeatures* out_features);

const char* GWPStatusToString(GWPStatusCode status);

#ifdef __cplusplus
}    /* extern "C" */
#endif

#endif  /* GWP_WEBP_DECODE_H_ */
