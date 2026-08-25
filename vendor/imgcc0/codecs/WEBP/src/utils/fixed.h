#ifndef GWP_UTILS_FIXED_H_
#define GWP_UTILS_FIXED_H_

#include "../webp/types.h"

#define GWP_FP_Q16_SHIFT 16
#define GWP_FP_ONE_Q16   ((GWPs32)1 << GWP_FP_Q16_SHIFT)

#define GWP_YUV_RV  91881
#define GWP_YUV_GU  22554
#define GWP_YUV_GV  46802
#define GWP_YUV_BU 116130

static GWPu8 GWPClamp8(GWPs32 v) {
  if (v < 0) return 0u;
  if (v > 255) return 255u;
  return (GWPu8)v;
}

#endif  /* GWP_UTILS_FIXED_H_ */
