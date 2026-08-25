#ifndef GWP_UTILS_ENDIAN_H_
#define GWP_UTILS_ENDIAN_H_

#include "../webp/types.h"

static GWPu16 GWPReadLE16(const GWPu8* p) {
  return (GWPu16)((GWPu16)p[0] | ((GWPu16)p[1] << 8));
}

static GWPu32 GWPReadLE24(const GWPu8* p) {
  return (GWPu32)p[0] | ((GWPu32)p[1] << 8) | ((GWPu32)p[2] << 16);
}

static GWPu32 GWPReadLE32(const GWPu8* p) {
  return (GWPu32)p[0] |
         ((GWPu32)p[1] << 8) |
         ((GWPu32)p[2] << 16) |
         ((GWPu32)p[3] << 24);
}

static void GWPWriteLE16(GWPu8* p, GWPu16 v) {
  p[0] = (GWPu8)(v & 0xffu);
  p[1] = (GWPu8)((v >> 8) & 0xffu);
}

static void GWPWriteLE24(GWPu8* p, GWPu32 v) {
  p[0] = (GWPu8)(v & 0xffu);
  p[1] = (GWPu8)((v >> 8) & 0xffu);
  p[2] = (GWPu8)((v >> 16) & 0xffu);
}

static void GWPWriteLE32(GWPu8* p, GWPu32 v) {
  p[0] = (GWPu8)(v & 0xffu);
  p[1] = (GWPu8)((v >> 8) & 0xffu);
  p[2] = (GWPu8)((v >> 16) & 0xffu);
  p[3] = (GWPu8)((v >> 24) & 0xffu);
}

#endif  /* GWP_UTILS_ENDIAN_H_ */
