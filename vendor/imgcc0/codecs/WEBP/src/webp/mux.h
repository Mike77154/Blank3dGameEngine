#ifndef GWP_WEBP_MUX_H_
#define GWP_WEBP_MUX_H_

#include "types.h"
#include "mux_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GWPMux {
  GWPData image_data;
  GWPBitstreamKind image_kind;
  GWPu32 width;
  GWPu32 height;
  GWPBool has_alpha;
  GWPBool force_vp8x;
  GWPData iccp;
  GWPData exif;
  GWPData xmp;
} GWPMux;

void GWPMuxInit(GWPMux* mux);

GWPMuxError GWPMuxSetImage(GWPMux* mux,
                           const GWPData* image_data,
                           GWPBitstreamKind image_kind,
                           GWPu32 width,
                           GWPu32 height,
                           GWPBool has_alpha);

GWPMuxError GWPMuxSetChunk(GWPMux* mux, const char fourcc[4], const GWPData* data);

GWPu32 GWPMuxEstimateSize(const GWPMux* mux);

GWPMuxError GWPMuxAssemble(const GWPMux* mux,
                           GWPu8* out_buf,
                           GWPu32 out_buf_size,
                           GWPu32* out_size);

#ifdef __cplusplus
}    /* extern "C" */
#endif

#endif  /* GWP_WEBP_MUX_H_ */
