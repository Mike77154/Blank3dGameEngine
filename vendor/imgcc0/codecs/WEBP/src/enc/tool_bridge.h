#ifndef GWP_ENC_TOOL_BRIDGE_H_
#define GWP_ENC_TOOL_BRIDGE_H_

#include "../webp/encode.h"

GWPStatusCode GWPEncodeViaCWebP(const GWPu8* pixels,
                                GWPu32 width,
                                GWPu32 height,
                                GWPu32 stride,
                                GWPRawPixelFormat pixel_format,
                                const GWPEncodeConfig* config,
                                GWPu32* out_size);

#endif  /* GWP_ENC_TOOL_BRIDGE_H_ */
