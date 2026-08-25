#ifndef GWP_ENC_VP8_LOSSY_PLAN_H_
#define GWP_ENC_VP8_LOSSY_PLAN_H_

#include "../webp/encode.h"

GWPu32 GWPEstimateVP8LossyPlanScratch(GWPu32 width, GWPu32 height);
GWPStatusCode GWPVP8BuildLossyPlan(const GWPu8* pixels,
                                   GWPu32 width,
                                   GWPu32 height,
                                   GWPu32 stride,
                                   GWPRawPixelFormat pixel_format,
                                   const GWPEncodeConfig* config,
                                   void* scratch,
                                   GWPu32 scratch_size,
                                   GWPVP8LossyPlan* out_plan);

#endif  /* GWP_ENC_VP8_LOSSY_PLAN_H_ */
