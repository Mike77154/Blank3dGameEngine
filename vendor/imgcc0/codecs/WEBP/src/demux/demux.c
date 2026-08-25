#include "../webp/demux.h"
#include "../utils/common.h"
#include "../utils/endian.h"

static GWPStatusCode GWPProbeVP8(const GWPu8* data,
                                 GWPu32 size,
                                 GWPu32* width,
                                 GWPu32* height) {
  if (data == 0 || width == 0 || height == 0) return GWP_STATUS_INVALID_PARAM;
  if (size < 10u) return GWP_STATUS_TRUNCATED_DATA;
  if (GWPReadLE24(data + 3) != 0x2a019du) return GWP_STATUS_BITSTREAM_ERROR;
  *width = (GWPReadLE16(data + 6) & 0x3fffu);
  *height = (GWPReadLE16(data + 8) & 0x3fffu);
  if (*width == 0u || *height == 0u) return GWP_STATUS_BAD_DIMENSIONS;
  return GWP_STATUS_OK;
}

static GWPStatusCode GWPProbeVP8L(const GWPu8* data,
                                  GWPu32 size,
                                  GWPu32* width,
                                  GWPu32* height,
                                  GWPBool* has_alpha) {
  GWPu32 bits;
  if (data == 0 || width == 0 || height == 0 || has_alpha == 0) {
    return GWP_STATUS_INVALID_PARAM;
  }
  if (size < 5u) return GWP_STATUS_TRUNCATED_DATA;
  if (data[0] != 0x2fu) return GWP_STATUS_BITSTREAM_ERROR;
  bits = GWPReadLE32(data + 1);
  *width = (bits & 0x3fffu) + 1u;
  *height = ((bits >> 14) & 0x3fffu) + 1u;
  *has_alpha = ((bits >> 28) & 1u) ? GWP_TRUE : GWP_FALSE;
  if (((bits >> 29) & 7u) != 0u) return GWP_STATUS_BITSTREAM_ERROR;
  if (*width == 0u || *height == 0u) return GWP_STATUS_BAD_DIMENSIONS;
  return GWP_STATUS_OK;
}

GWPStatusCode GWPDemuxInit(GWPDemuxer* dmux) {
  if (dmux == 0) return GWP_STATUS_INVALID_PARAM;
  GWPZero(dmux, (GWPu32)sizeof(*dmux));
  return GWP_STATUS_OK;
}

static GWPStatusCode GWPParseVP8X(GWPDemuxer* dmux,
                                  const GWPu8* payload,
                                  GWPu32 size) {
  GWPu32 flags;
  if (dmux == 0 || payload == 0) return GWP_STATUS_INVALID_PARAM;
  if (size < 10u) return GWP_STATUS_TRUNCATED_DATA;
  flags = payload[0];
  dmux->features.has_icc = (flags & 0x20u) ? GWP_TRUE : GWP_FALSE;
  dmux->features.has_alpha = (flags & 0x10u) ? GWP_TRUE : GWP_FALSE;
  dmux->features.has_exif = (flags & 0x08u) ? GWP_TRUE : GWP_FALSE;
  dmux->features.has_xmp = (flags & 0x04u) ? GWP_TRUE : GWP_FALSE;
  dmux->features.has_animation = (flags & 0x02u) ? GWP_TRUE : GWP_FALSE;
  dmux->features.width = GWPReadLE24(payload + 4) + 1u;
  dmux->features.height = GWPReadLE24(payload + 7) + 1u;
  if (dmux->features.width == 0u || dmux->features.height == 0u) {
    return GWP_STATUS_BAD_DIMENSIONS;
  }
  return GWP_STATUS_OK;
}

static GWPBool GWPFrameFitsCanvas(const GWPDemuxer* dmux, const GWPFrameInfo* frame) {
  if (dmux == 0 || frame == 0) return GWP_FALSE;
  if (frame->x > dmux->features.width) return GWP_FALSE;
  if (frame->y > dmux->features.height) return GWP_FALSE;
  if (frame->width > dmux->features.width - frame->x) return GWP_FALSE;
  if (frame->height > dmux->features.height - frame->y) return GWP_FALSE;
  return GWP_TRUE;
}

static GWPStatusCode GWPParseFrameSubchunks(GWPDemuxer* dmux, GWPFrameInfo* frame) {
  GWPu32 pos;
  if (dmux == 0 || frame == 0) return GWP_STATUS_INVALID_PARAM;
  pos = 0u;
  while (pos + 8u <= frame->payload.size) {
    const GWPu8* chunk = frame->payload.bytes + pos;
    GWPu32 fourcc = GWPReadLE32(chunk + 0);
    GWPu32 size = GWPReadLE32(chunk + 4);
    GWPu32 total;
    if (!GWPAddU32(8u, GWP_ALIGN2(size), &total)) return GWP_STATUS_LIMIT_EXCEEDED;
    if (pos + total > frame->payload.size) return GWP_STATUS_TRUNCATED_DATA;
    if (fourcc == GWP_FOURCC_ALPH) {
      if (frame->alph_payload.bytes != 0) return GWP_STATUS_PARSE_ERROR;
      frame->alph_payload.bytes = chunk + 8u;
      frame->alph_payload.size = size;
      dmux->features.has_alpha = GWP_TRUE;
    } else if (fourcc == GWP_FOURCC_VP8 || fourcc == GWP_FOURCC_VP8L) {
      GWPu32 bit_w, bit_h;
      if (frame->bitstream_payload.bytes != 0) return GWP_STATUS_PARSE_ERROR;
      frame->bitstream_fourcc = fourcc;
      frame->bitstream_payload.bytes = chunk + 8u;
      frame->bitstream_payload.size = size;
      bit_w = 0u;
      bit_h = 0u;
      if (fourcc == GWP_FOURCC_VP8) {
        GWPStatusCode st = GWPProbeVP8(frame->bitstream_payload.bytes,
                                       frame->bitstream_payload.size,
                                       &bit_w, &bit_h);
        if (st != GWP_STATUS_OK) return st;
      } else {
        GWPBool bit_alpha = GWP_FALSE;
        GWPStatusCode st = GWPProbeVP8L(frame->bitstream_payload.bytes,
                                        frame->bitstream_payload.size,
                                        &bit_w, &bit_h,
                                        &bit_alpha);
        if (st != GWP_STATUS_OK) return st;
        if (bit_alpha) dmux->features.has_alpha = GWP_TRUE;
      }
      if (bit_w != frame->width || bit_h != frame->height) return GWP_STATUS_PARSE_ERROR;
    }
    pos += total;
  }
  if (frame->bitstream_payload.bytes == 0) return GWP_STATUS_PARSE_ERROR;
  return GWP_STATUS_OK;
}

GWPStatusCode GWPDemuxParse(GWPDemuxer* dmux, const GWPu8* data, GWPu32 size) {
  GWPu32 riff_size;
  GWPu32 pos;
  GWPu32 i;
  GWPStatusCode st;
  GWPu32 stage;

  if (dmux == 0 || data == 0) return GWP_STATUS_INVALID_PARAM;
  GWPDemuxInit(dmux);
  if (size < 12u) return GWP_STATUS_TRUNCATED_DATA;
  if (GWPReadLE32(data) != GWP_FOURCC_RIFF) return GWP_STATUS_BAD_SIGNATURE;
  riff_size = GWPReadLE32(data + 4);
  if (GWPReadLE32(data + 8) != GWP_FOURCC_WEBP) return GWP_STATUS_BAD_SIGNATURE;
  if (riff_size + 8u > size) return GWP_STATUS_TRUNCATED_DATA;

  pos = 12u;
  while (pos + 8u <= size) {
    GWPu32 fourcc;
    GWPu32 chunk_size;
    GWPu32 chunk_total;
    if (dmux->chunk_count >= GWP_DEMUX_MAX_CHUNKS) return GWP_STATUS_LIMIT_EXCEEDED;
    fourcc = GWPReadLE32(data + pos);
    chunk_size = GWPReadLE32(data + pos + 4u);
    if (!GWPAddU32(8u, GWP_ALIGN2(chunk_size), &chunk_total)) return GWP_STATUS_LIMIT_EXCEEDED;
    if (pos + chunk_total > size) return GWP_STATUS_TRUNCATED_DATA;
    dmux->chunks[dmux->chunk_count].fourcc = fourcc;
    dmux->chunks[dmux->chunk_count].offset = pos + 8u;
    dmux->chunks[dmux->chunk_count].size = chunk_size;
    ++dmux->chunk_count;
    pos += chunk_total;
  }

  stage = 0u;
  for (i = 0u; i < dmux->chunk_count; ++i) {
    const GWPChunkInfo* c = &dmux->chunks[i];
    const GWPu8* payload = data + c->offset;
    switch (c->fourcc) {
      case GWP_FOURCC_VP8X:
        if (stage > 0u) return GWP_STATUS_PARSE_ERROR;
        st = GWPParseVP8X(dmux, payload, c->size);
        if (st != GWP_STATUS_OK) return st;
        stage = 1u;
        break;
      case GWP_FOURCC_ICCP:
        if (stage > 2u) return GWP_STATUS_PARSE_ERROR;
        dmux->iccp_payload.bytes = payload;
        dmux->iccp_payload.size = c->size;
        dmux->features.has_icc = GWP_TRUE;
        if (stage < 2u) stage = 2u;
        break;
      case GWP_FOURCC_ANIM:
        if (stage > 3u) return GWP_STATUS_PARSE_ERROR;
        if (c->size < 6u) return GWP_STATUS_PARSE_ERROR;
        dmux->background_color = GWPReadLE32(payload);
        dmux->loop_count = GWPReadLE16(payload + 4);
        dmux->features.has_animation = GWP_TRUE;
        stage = 3u;
        break;
      case GWP_FOURCC_ANMF:
        if (dmux->frame_count >= GWP_DEMUX_MAX_FRAMES) return GWP_STATUS_LIMIT_EXCEEDED;
        if (c->size < 16u) return GWP_STATUS_PARSE_ERROR;
        dmux->frames[dmux->frame_count].x = 2u * GWPReadLE24(payload + 0);
        dmux->frames[dmux->frame_count].y = 2u * GWPReadLE24(payload + 3);
        dmux->frames[dmux->frame_count].width = GWPReadLE24(payload + 6) + 1u;
        dmux->frames[dmux->frame_count].height = GWPReadLE24(payload + 9) + 1u;
        dmux->frames[dmux->frame_count].duration_ms = GWPReadLE24(payload + 12);
        dmux->frames[dmux->frame_count].flags = payload[15];
        dmux->frames[dmux->frame_count].dispose_method = (GWPu8)(payload[15] & 1u);
        dmux->frames[dmux->frame_count].blend_method = (GWPu8)((payload[15] >> 1) & 1u);
        dmux->frames[dmux->frame_count].payload.bytes = payload + 16;
        dmux->frames[dmux->frame_count].payload.size = c->size - 16u;
        st = GWPParseFrameSubchunks(dmux, &dmux->frames[dmux->frame_count]);
        if (st != GWP_STATUS_OK) return st;
        if (!GWPFrameFitsCanvas(dmux, &dmux->frames[dmux->frame_count])) return GWP_STATUS_BAD_DIMENSIONS;
        ++dmux->frame_count;
        dmux->features.has_animation = GWP_TRUE;
        stage = 4u;
        break;
      case GWP_FOURCC_ALPH:
        if (stage > 5u) return GWP_STATUS_PARSE_ERROR;
        dmux->alph_payload.bytes = payload;
        dmux->alph_payload.size = c->size;
        dmux->features.has_alpha = GWP_TRUE;
        if (stage < 5u) stage = 5u;
        break;
      case GWP_FOURCC_VP8:
        if (stage > 6u) return GWP_STATUS_PARSE_ERROR;
        dmux->vp8_payload.bytes = payload;
        dmux->vp8_payload.size = c->size;
        dmux->features.format = GWP_BITSTREAM_VP8;
        if (dmux->features.width == 0u || dmux->features.height == 0u) {
          st = GWPProbeVP8(payload, c->size, &dmux->features.width, &dmux->features.height);
          if (st != GWP_STATUS_OK) return st;
        }
        stage = 6u;
        break;
      case GWP_FOURCC_VP8L:
        if (stage > 6u) return GWP_STATUS_PARSE_ERROR;
        dmux->vp8l_payload.bytes = payload;
        dmux->vp8l_payload.size = c->size;
        dmux->features.format = GWP_BITSTREAM_VP8L;
        {
          GWPBool alpha = GWP_FALSE;
          if (dmux->features.width == 0u || dmux->features.height == 0u) {
            st = GWPProbeVP8L(payload, c->size, &dmux->features.width, &dmux->features.height, &alpha);
            if (st != GWP_STATUS_OK) return st;
          } else {
            GWPu32 tmp_w;
            GWPu32 tmp_h;
            st = GWPProbeVP8L(payload, c->size, &tmp_w, &tmp_h, &alpha);
            if (st != GWP_STATUS_OK) return st;
          }
          if (alpha) dmux->features.has_alpha = GWP_TRUE;
        }
        stage = 6u;
        break;
      case GWP_FOURCC_EXIF:
        dmux->exif_payload.bytes = payload;
        dmux->exif_payload.size = c->size;
        dmux->features.has_exif = GWP_TRUE;
        break;
      case GWP_FOURCC_XMP:
        dmux->xmp_payload.bytes = payload;
        dmux->xmp_payload.size = c->size;
        dmux->features.has_xmp = GWP_TRUE;
        break;
      default:
        break;
    }
  }

  if (dmux->features.width > GWP_MAX_IMAGE_WIDTH ||
      dmux->features.height > GWP_MAX_IMAGE_HEIGHT) {
    return GWP_STATUS_BAD_DIMENSIONS;
  }
  if (dmux->features.has_animation) {
    if (dmux->frame_count == 0u) return GWP_STATUS_PARSE_ERROR;
    dmux->features.format = GWP_BITSTREAM_ANIMATION;
    dmux->features.frame_count = dmux->frame_count;
  }
  if (dmux->features.width == 0u || dmux->features.height == 0u) {
    return GWP_STATUS_PARSE_ERROR;
  }
  if (dmux->features.format == GWP_BITSTREAM_UNKNOWN && !dmux->features.has_animation) {
    return GWP_STATUS_PARSE_ERROR;
  }
  return GWP_STATUS_OK;
}

const GWPFrameInfo* GWPDemuxGetFrame(const GWPDemuxer* dmux, GWPu32 index) {
  if (dmux == 0) return 0;
  if (index >= dmux->frame_count) return 0;
  return &dmux->frames[index];
}
