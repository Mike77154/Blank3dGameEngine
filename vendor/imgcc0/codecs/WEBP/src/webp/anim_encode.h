#ifndef GWP_WEBP_ANIM_ENCODE_H_
#define GWP_WEBP_ANIM_ENCODE_H_

#include "mux.h"
#include "demux.h"
#include "encode.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GWP_ANIM_MAX_FRAMES GWP_DEMUX_MAX_FRAMES

typedef struct GWPAnimEncoderOptions {
  GWPBool force_vp8x;
  GWPBool snap_odd_offsets;
  GWPBool minimize_size;
  GWPBool allow_mixed;
  GWPu32 kmin;
  GWPu32 kmax;
  GWPu8* work_mem;
  GWPu32 work_mem_size;
} GWPAnimEncoderOptions;

typedef struct GWPAnimFrameSpec {
  GWPu32 x_offset;
  GWPu32 y_offset;
  GWPu32 duration_ms;
  GWPu8 blend_method;
  GWPu8 dispose_method;
} GWPAnimFrameSpec;

typedef struct GWPAnimEncoderFrame {
  GWPu32 x_offset;
  GWPu32 y_offset;
  GWPu32 width;
  GWPu32 height;
  GWPu32 duration_ms;
  GWPu8 blend_method;
  GWPu8 dispose_method;
  GWPBitstreamKind bitstream_kind;
  GWPu32 bitstream_fourcc;
  GWPBool has_alpha;
  GWPData alph_payload;
  GWPData bitstream_payload;
  GWPBool is_key_frame;
} GWPAnimEncoderFrame;

typedef struct GWPAnimEncoder {
  GWPu32 canvas_width;
  GWPu32 canvas_height;
  GWPAnimEncoderOptions options;
  GWPMuxAnimParams anim_params;
  GWPData iccp;
  GWPData exif;
  GWPData xmp;
  GWPBool has_alpha;
  GWPu32 frame_count;
  GWPu32 last_key_frame_distance;
  GWPu8* canvas_rgba;
  GWPu32 canvas_rgba_size;
  GWPBool canvas_valid;
  GWPu8* storage_bytes;
  GWPu32 storage_size;
  GWPu32 storage_used;
  GWPAnimEncoderFrame frames[GWP_ANIM_MAX_FRAMES];
} GWPAnimEncoder;

void GWPAnimEncoderOptionsInit(GWPAnimEncoderOptions* options);
void GWPAnimFrameSpecInit(GWPAnimFrameSpec* spec);
void GWPAnimEncoderInit(GWPAnimEncoder* enc,
                        GWPu32 canvas_width,
                        GWPu32 canvas_height,
                        const GWPAnimEncoderOptions* options);

GWPMuxError GWPAnimEncoderSetAnimationParams(GWPAnimEncoder* enc,
                                             const GWPMuxAnimParams* params);
GWPMuxError GWPAnimEncoderSetChunk(GWPAnimEncoder* enc,
                                   const char fourcc[4],
                                   const GWPData* data);

GWPMuxError GWPAnimEncoderAddFrameWebP(GWPAnimEncoder* enc,
                                       const GWPData* still_webp,
                                       const GWPAnimFrameSpec* spec);

GWPMuxError GWPAnimEncoderAddFrameBitstream(GWPAnimEncoder* enc,
                                            GWPBitstreamKind kind,
                                            const GWPData* bitstream,
                                            GWPu32 width,
                                            GWPu32 height,
                                            GWPBool has_alpha,
                                            const GWPData* alph_payload,
                                            const GWPAnimFrameSpec* spec);

GWPMuxError GWPAnimEncoderAddFramePixels(GWPAnimEncoder* enc,
                                         const GWPu8* pixels,
                                         GWPu32 stride,
                                         GWPRawPixelFormat pixel_format,
                                         const GWPAnimFrameSpec* spec,
                                         const GWPEncodeConfig* config);

GWPu32 GWPAnimEncoderEstimateSize(const GWPAnimEncoder* enc);
GWPMuxError GWPAnimEncoderAssemble(const GWPAnimEncoder* enc,
                                   GWPu8* out_buf,
                                   GWPu32 out_buf_size,
                                   GWPu32* out_size);

#ifdef __cplusplus
}    /* extern "C" */
#endif

#endif  /* GWP_WEBP_ANIM_ENCODE_H_ */
