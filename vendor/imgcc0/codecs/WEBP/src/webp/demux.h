#ifndef GWP_WEBP_DEMUX_H_
#define GWP_WEBP_DEMUX_H_

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GWP_DEMUX_MAX_CHUNKS 128u
#define GWP_DEMUX_MAX_FRAMES 1024u

typedef struct GWPChunkInfo {
  GWPu32 fourcc;
  GWPu32 offset;
  GWPu32 size;
} GWPChunkInfo;

typedef enum GWPAnimBlendMethod {
  GWP_ANIM_BLEND = 0,
  GWP_ANIM_NO_BLEND = 1
} GWPAnimBlendMethod;

typedef enum GWPAnimDisposeMethod {
  GWP_ANIM_DISPOSE_NONE = 0,
  GWP_ANIM_DISPOSE_BACKGROUND = 1
} GWPAnimDisposeMethod;

typedef struct GWPFrameInfo {
  GWPu32 x;
  GWPu32 y;
  GWPu32 width;
  GWPu32 height;
  GWPu32 duration_ms;
  GWPu32 flags;
  GWPu8 blend_method;
  GWPu8 dispose_method;
  GWPu32 bitstream_fourcc;
  GWPData payload;
  GWPData alph_payload;
  GWPData bitstream_payload;
} GWPFrameInfo;

typedef struct GWPDemuxer {
  GWPBitstreamFeatures features;
  GWPChunkInfo chunks[GWP_DEMUX_MAX_CHUNKS];
  GWPu32 chunk_count;
  GWPFrameInfo frames[GWP_DEMUX_MAX_FRAMES];
  GWPu32 frame_count;
  GWPData vp8_payload;
  GWPData vp8l_payload;
  GWPData alph_payload;
  GWPData iccp_payload;
  GWPData exif_payload;
  GWPData xmp_payload;
  GWPu32 background_color;
  GWPu16 loop_count;
} GWPDemuxer;

GWPStatusCode GWPDemuxInit(GWPDemuxer* dmux);
GWPStatusCode GWPDemuxParse(GWPDemuxer* dmux, const GWPu8* data, GWPu32 size);
const GWPFrameInfo* GWPDemuxGetFrame(const GWPDemuxer* dmux, GWPu32 index);

#ifdef __cplusplus
}    /* extern "C" */
#endif

#endif  /* GWP_WEBP_DEMUX_H_ */
