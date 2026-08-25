#ifndef GWP_WEBP_MUX_TYPES_H_
#define GWP_WEBP_MUX_TYPES_H_

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum GWPMuxError {
  GWP_MUX_OK = 0,
  GWP_MUX_INVALID_ARGUMENT = 1,
  GWP_MUX_BAD_DATA = 2,
  GWP_MUX_NOT_ENOUGH_DATA = 3,
  GWP_MUX_NOT_ENOUGH_OUTPUT = 4,
  GWP_MUX_UNSUPPORTED = 5
} GWPMuxError;

typedef enum GWPChunkId {
  GWP_CHUNK_UNKNOWN = 0,
  GWP_CHUNK_VP8X,
  GWP_CHUNK_ICCP,
  GWP_CHUNK_ANIM,
  GWP_CHUNK_ANMF,
  GWP_CHUNK_ALPH,
  GWP_CHUNK_IMAGE,
  GWP_CHUNK_EXIF,
  GWP_CHUNK_XMP
} GWPChunkId;

typedef struct GWPMuxAnimParams {
  GWPu32 bgcolor;
  GWPu16 loop_count;
} GWPMuxAnimParams;

#ifdef __cplusplus
}    /* extern "C" */
#endif

#endif  /* GWP_WEBP_MUX_TYPES_H_ */
