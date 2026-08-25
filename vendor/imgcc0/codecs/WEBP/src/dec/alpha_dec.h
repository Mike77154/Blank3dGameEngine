#ifndef GWP_DEC_ALPHA_DEC_H_
#define GWP_DEC_ALPHA_DEC_H_

#include "../webp/decode.h"

GWPStatusCode GWPDecodeAlphaChunk(const GWPu8* data,
                                  GWPu32 data_size,
                                  GWPu32 width,
                                  GWPu32 height,
                                  GWPPixelFormat pixel_format,
                                  GWPu8* output_buffer,
                                  GWPu32 output_stride,
                                  void* scratch,
                                  GWPu32 scratch_size);

#endif  /* GWP_DEC_ALPHA_DEC_H_ */
