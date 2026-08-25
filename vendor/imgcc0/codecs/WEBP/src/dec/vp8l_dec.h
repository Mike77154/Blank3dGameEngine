#ifndef GWP_DEC_VP8L_DEC_H_
#define GWP_DEC_VP8L_DEC_H_

#include "../webp/decode.h"
#include "../utils/arena.h"

GWPStatusCode GWPDecodeVP8L(const GWPu8* data,
                            GWPu32 data_size,
                            const GWPDecoderOptions* options,
                            GWPBitstreamFeatures* out_features);

GWPStatusCode GWPDecodeVP8LAlphaImage(const GWPu8* data,
                                      GWPu32 data_size,
                                      GWPu32 width,
                                      GWPu32 height,
                                      void* scratch,
                                      GWPu32 scratch_size,
                                      GWPu8* out_alpha);

#endif  /* GWP_DEC_VP8L_DEC_H_ */
