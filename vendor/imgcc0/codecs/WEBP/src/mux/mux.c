#include "../webp/mux.h"
#include "../utils/common.h"
#include "../utils/endian.h"

static GWPu32 GWPChunkDiskSize(GWPu32 payload_size) {
  return 8u + GWP_ALIGN2(payload_size);
}

void GWPMuxInit(GWPMux* mux) {
  if (mux == 0) return;
  GWPZero(mux, (GWPu32)sizeof(*mux));
}

GWPMuxError GWPMuxSetImage(GWPMux* mux,
                           const GWPData* image_data,
                           GWPBitstreamKind image_kind,
                           GWPu32 width,
                           GWPu32 height,
                           GWPBool has_alpha) {
  if (mux == 0 || image_data == 0 || image_data->bytes == 0) return GWP_MUX_INVALID_ARGUMENT;
  if (image_kind != GWP_BITSTREAM_VP8 && image_kind != GWP_BITSTREAM_VP8L) {
    return GWP_MUX_INVALID_ARGUMENT;
  }
  if (width == 0u || height == 0u) return GWP_MUX_INVALID_ARGUMENT;
  mux->image_data = *image_data;
  mux->image_kind = image_kind;
  mux->width = width;
  mux->height = height;
  mux->has_alpha = has_alpha;
  return GWP_MUX_OK;
}

GWPMuxError GWPMuxSetChunk(GWPMux* mux, const char fourcc[4], const GWPData* data) {
  GWPu32 tag;
  if (mux == 0 || fourcc == 0 || data == 0 || data->bytes == 0) return GWP_MUX_INVALID_ARGUMENT;
  tag = GWP_FOURCC(fourcc[0], fourcc[1], fourcc[2], fourcc[3]);
  switch (tag) {
    case GWP_FOURCC_ICCP:
      mux->iccp = *data;
      break;
    case GWP_FOURCC_EXIF:
      mux->exif = *data;
      break;
    case GWP_FOURCC_XMP:
      mux->xmp = *data;
      break;
    default:
      return GWP_MUX_UNSUPPORTED;
  }
  return GWP_MUX_OK;
}

GWPu32 GWPMuxEstimateSize(const GWPMux* mux) {
  GWPu32 total;
  GWPBool need_vp8x;
  if (mux == 0 || mux->image_data.bytes == 0) return 0u;
  need_vp8x = mux->force_vp8x || mux->iccp.bytes != 0 || mux->exif.bytes != 0 ||
              mux->xmp.bytes != 0 || mux->has_alpha;
  total = 12u;
  if (need_vp8x) total += GWPChunkDiskSize(10u);
  if (mux->iccp.bytes != 0) total += GWPChunkDiskSize(mux->iccp.size);
  total += GWPChunkDiskSize(mux->image_data.size);
  if (mux->exif.bytes != 0) total += GWPChunkDiskSize(mux->exif.size);
  if (mux->xmp.bytes != 0) total += GWPChunkDiskSize(mux->xmp.size);
  return total;
}

static GWPMuxError GWPWriteChunk(GWPu8** dst,
                                 GWPu32* remain,
                                 GWPu32 fourcc,
                                 const GWPData* data) {
  GWPu32 disk_size;
  GWPu8* p;
  if (dst == 0 || *dst == 0 || remain == 0 || data == 0) return GWP_MUX_INVALID_ARGUMENT;
  disk_size = GWPChunkDiskSize(data->size);
  if (*remain < disk_size) return GWP_MUX_NOT_ENOUGH_OUTPUT;
  p = *dst;
  GWPWriteLE32(p + 0, fourcc);
  GWPWriteLE32(p + 4, data->size);
  GWPCopy(p + 8, data->bytes, data->size);
  if ((data->size & 1u) != 0u) p[8u + data->size] = 0u;
  *dst += disk_size;
  *remain -= disk_size;
  return GWP_MUX_OK;
}

GWPMuxError GWPMuxAssemble(const GWPMux* mux,
                           GWPu8* out_buf,
                           GWPu32 out_buf_size,
                           GWPu32* out_size) {
  GWPu32 total;
  GWPu32 remain;
  GWPu8* p;
  GWPBool need_vp8x;
  GWPData chunk_data;
  GWPu8 vp8x_payload[10];

  if (out_size != 0) *out_size = 0u;
  if (mux == 0 || out_buf == 0 || out_size == 0) return GWP_MUX_INVALID_ARGUMENT;
  if (mux->image_data.bytes == 0) return GWP_MUX_INVALID_ARGUMENT;

  total = GWPMuxEstimateSize(mux);
  if (total == 0u) return GWP_MUX_INVALID_ARGUMENT;
  if (out_buf_size < total) return GWP_MUX_NOT_ENOUGH_OUTPUT;

  need_vp8x = mux->force_vp8x || mux->iccp.bytes != 0 || mux->exif.bytes != 0 ||
              mux->xmp.bytes != 0 || mux->has_alpha;

  GWPWriteLE32(out_buf + 0, GWP_FOURCC_RIFF);
  GWPWriteLE32(out_buf + 4, total - 8u);
  GWPWriteLE32(out_buf + 8, GWP_FOURCC_WEBP);

  p = out_buf + 12u;
  remain = out_buf_size - 12u;

  if (need_vp8x) {
    GWPZero(vp8x_payload, 10u);
    if (mux->iccp.bytes != 0) vp8x_payload[0] |= 0x20u;
    if (mux->has_alpha) vp8x_payload[0] |= 0x10u;
    if (mux->exif.bytes != 0) vp8x_payload[0] |= 0x08u;
    if (mux->xmp.bytes != 0) vp8x_payload[0] |= 0x04u;
    GWPWriteLE24(vp8x_payload + 4, mux->width - 1u);
    GWPWriteLE24(vp8x_payload + 7, mux->height - 1u);
    chunk_data.bytes = vp8x_payload;
    chunk_data.size = 10u;
    if (GWPWriteChunk(&p, &remain, GWP_FOURCC_VP8X, &chunk_data) != GWP_MUX_OK) {
      return GWP_MUX_NOT_ENOUGH_OUTPUT;
    }
  }

  if (mux->iccp.bytes != 0) {
    if (GWPWriteChunk(&p, &remain, GWP_FOURCC_ICCP, &mux->iccp) != GWP_MUX_OK) {
      return GWP_MUX_NOT_ENOUGH_OUTPUT;
    }
  }

  if (mux->image_kind == GWP_BITSTREAM_VP8L) {
    if (GWPWriteChunk(&p, &remain, GWP_FOURCC_VP8L, &mux->image_data) != GWP_MUX_OK) {
      return GWP_MUX_NOT_ENOUGH_OUTPUT;
    }
  } else {
    if (GWPWriteChunk(&p, &remain, GWP_FOURCC_VP8, &mux->image_data) != GWP_MUX_OK) {
      return GWP_MUX_NOT_ENOUGH_OUTPUT;
    }
  }

  if (mux->exif.bytes != 0) {
    if (GWPWriteChunk(&p, &remain, GWP_FOURCC_EXIF, &mux->exif) != GWP_MUX_OK) {
      return GWP_MUX_NOT_ENOUGH_OUTPUT;
    }
  }

  if (mux->xmp.bytes != 0) {
    if (GWPWriteChunk(&p, &remain, GWP_FOURCC_XMP, &mux->xmp) != GWP_MUX_OK) {
      return GWP_MUX_NOT_ENOUGH_OUTPUT;
    }
  }

  *out_size = total;
  return GWP_MUX_OK;
}
