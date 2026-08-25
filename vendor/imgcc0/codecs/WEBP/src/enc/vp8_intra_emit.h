#ifndef GWP_ENC_VP8_INTRA_EMIT_H_
#define GWP_ENC_VP8_INTRA_EMIT_H_

#include "../webp/encode.h"

#ifdef __cplusplus
extern "C" {
#endif

void GWPVP8IntraEmitPlanInit(GWPVP8IntraEmitPlan* plan);
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

#ifdef __cplusplus
}    /* extern \"C\" */
#endif

#endif  /* GWP_ENC_VP8_INTRA_EMIT_H_ */
