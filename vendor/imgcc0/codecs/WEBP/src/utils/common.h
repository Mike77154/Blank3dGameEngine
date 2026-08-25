#ifndef GWP_UTILS_COMMON_H_
#define GWP_UTILS_COMMON_H_

#include "../webp/types.h"

#define GWP_FOURCC(a, b, c, d) \
  ((GWPu32)(a) | ((GWPu32)(b) << 8) | ((GWPu32)(c) << 16) | ((GWPu32)(d) << 24))

#define GWP_FOURCC_RIFF GWP_FOURCC('R', 'I', 'F', 'F')
#define GWP_FOURCC_WEBP GWP_FOURCC('W', 'E', 'B', 'P')
#define GWP_FOURCC_VP8  GWP_FOURCC('V', 'P', '8', ' ')
#define GWP_FOURCC_VP8L GWP_FOURCC('V', 'P', '8', 'L')
#define GWP_FOURCC_VP8X GWP_FOURCC('V', 'P', '8', 'X')
#define GWP_FOURCC_ALPH GWP_FOURCC('A', 'L', 'P', 'H')
#define GWP_FOURCC_ANIM GWP_FOURCC('A', 'N', 'I', 'M')
#define GWP_FOURCC_ANMF GWP_FOURCC('A', 'N', 'M', 'F')
#define GWP_FOURCC_ICCP GWP_FOURCC('I', 'C', 'C', 'P')
#define GWP_FOURCC_EXIF GWP_FOURCC('E', 'X', 'I', 'F')
#define GWP_FOURCC_XMP  GWP_FOURCC('X', 'M', 'P', ' ')

#define GWP_ALIGN2(x) (((x) + 1u) & ~1u)
#define GWP_ARRAY_SIZE(a) ((GWPu32)(sizeof(a) / sizeof((a)[0])))

#define GWP_MAX_IMAGE_WIDTH  16384u
#define GWP_MAX_IMAGE_HEIGHT 16384u

#define GWP_VP8L_MAX_TRANSFORMS      4u
#define GWP_VP8L_MAX_GREEN_ALPHABET  (256u + 24u + (1u << 11))
#define GWP_VP8L_MAX_LITERAL_ALPHA   256u
#define GWP_VP8L_MAX_DISTANCE_CODES  40u
#define GWP_VP8L_MAX_CODE_LENGTHS    19u
#define GWP_VP8L_MAX_HUFF_BITS       15u
#define GWP_VP8L_CACHE_MAX_BITS      11u

GWPBool GWPAddU32(GWPu32 a, GWPu32 b, GWPu32* out);
GWPBool GWPMulU32(GWPu32 a, GWPu32 b, GWPu32* out);
void GWPZero(void* dst, GWPu32 size);
void GWPCopy(void* dst, const void* src, GWPu32 size);

#endif  /* GWP_UTILS_COMMON_H_ */
