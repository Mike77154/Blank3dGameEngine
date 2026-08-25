#ifndef GWP_WEBP_TYPES_H_
#define GWP_WEBP_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char  GWPu8;
typedef unsigned short GWPu16;
typedef unsigned int   GWPu32;
typedef signed int     GWPs32;
typedef int            GWPBool;

#define GWP_TRUE  1
#define GWP_FALSE 0

typedef struct GWPData {
  const GWPu8* bytes;
  GWPu32 size;
} GWPData;

typedef enum GWPStatusCode {
  GWP_STATUS_OK = 0,
  GWP_STATUS_INVALID_PARAM = 1,
  GWP_STATUS_TRUNCATED_DATA = 2,
  GWP_STATUS_BAD_SIGNATURE = 3,
  GWP_STATUS_BAD_DIMENSIONS = 4,
  GWP_STATUS_LIMIT_EXCEEDED = 5,
  GWP_STATUS_NOT_ENOUGH_OUTPUT = 6,
  GWP_STATUS_NOT_ENOUGH_SCRATCH = 7,
  GWP_STATUS_BITSTREAM_ERROR = 8,
  GWP_STATUS_PARSE_ERROR = 9,
  GWP_STATUS_UNSUPPORTED_FEATURE = 10,
  GWP_STATUS_UNSUPPORTED_FORMAT = 11,
  GWP_STATUS_NOT_IMPLEMENTED = 12
} GWPStatusCode;

typedef enum GWPBitstreamKind {
  GWP_BITSTREAM_UNKNOWN = 0,
  GWP_BITSTREAM_VP8 = 1,
  GWP_BITSTREAM_VP8L = 2,
  GWP_BITSTREAM_ANIMATION = 3
} GWPBitstreamKind;

typedef struct GWPBitstreamFeatures {
  GWPu32 width;
  GWPu32 height;
  GWPBool has_alpha;
  GWPBool has_animation;
  GWPBool has_icc;
  GWPBool has_exif;
  GWPBool has_xmp;
  GWPBitstreamKind format;
  GWPu32 frame_count;
} GWPBitstreamFeatures;

#ifdef __cplusplus
}    /* extern "C" */
#endif

#endif  /* GWP_WEBP_TYPES_H_ */
