#include "vp8_yuv.h"

#include "../utils/fixed.h"

void GWPVP8YUV420RowToPacked(const GWPu8* y_row,
                             const GWPu8* u_row,
                             const GWPu8* v_row,
                             GWPu8* dst,
                             GWPu32 width,
                             GWPPixelFormat fmt) {
  GWPu32 x;
  if (y_row == 0 || u_row == 0 || v_row == 0 || dst == 0) return;
  for (x = 0u; x < width; ++x) {
    GWPs32 y;
    GWPs32 u;
    GWPs32 v;
    GWPs32 c;
    GWPu8 r8;
    GWPu8 g8;
    GWPu8 b8;
    y = (GWPs32)y_row[x];
    u = (GWPs32)u_row[x >> 1] - 128;
    v = (GWPs32)v_row[x >> 1] - 128;
    c = y << 16;
    r8 = GWPClamp8((c + GWP_YUV_RV * v + 32768) >> 16);
    g8 = GWPClamp8((c - GWP_YUV_GU * u - GWP_YUV_GV * v + 32768) >> 16);
    b8 = GWPClamp8((c + GWP_YUV_BU * u + 32768) >> 16);
    if (fmt == GWP_PIXFMT_BGRA) {
      dst[4u * x + 0u] = b8;
      dst[4u * x + 1u] = g8;
      dst[4u * x + 2u] = r8;
      dst[4u * x + 3u] = 255u;
    } else if (fmt == GWP_PIXFMT_ARGB) {
      dst[4u * x + 0u] = 255u;
      dst[4u * x + 1u] = r8;
      dst[4u * x + 2u] = g8;
      dst[4u * x + 3u] = b8;
    } else {
      dst[4u * x + 0u] = r8;
      dst[4u * x + 1u] = g8;
      dst[4u * x + 2u] = b8;
      dst[4u * x + 3u] = 255u;
    }
  }
}
