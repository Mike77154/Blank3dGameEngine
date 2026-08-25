#ifndef GWP_ENC_VP8L_ENC_H_
#define GWP_ENC_VP8L_ENC_H_

#include "../webp/encode.h"

GWPStatusCode GWPVP8LEncodeImage(const GWPu8* pixels,
                                 GWPu32 width,
                                 GWPu32 height,
                                 GWPu32 stride,
                                 GWPRawPixelFormat pixel_format,
                                 GWPBool exact,
                                 GWPu8* out_buf,
                                 GWPu32 out_buf_size,
                                 GWPu32* out_size,
                                 GWPBool* out_has_alpha);

GWPStatusCode GWPVP8LEncodeImageEx(const GWPu8* pixels,
                                   GWPu32 width,
                                   GWPu32 height,
                                   GWPu32 stride,
                                   GWPRawPixelFormat pixel_format,
                                   const GWPVP8LBitstreamOptions* options,
                                   GWPu8* out_buf,
                                   GWPu32 out_buf_size,
                                   GWPu32* out_size,
                                   GWPBool* out_has_alpha);

#endif  /* GWP_ENC_VP8L_ENC_H_ */
