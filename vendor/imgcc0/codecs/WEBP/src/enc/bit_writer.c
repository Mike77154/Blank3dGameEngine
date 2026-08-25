#include "bit_writer.h"

void GWPBitWriterInit(GWPBitWriter* bw, GWPu8* dst, GWPu32 capacity) {
  if (bw == 0) return;
  bw->dst = dst;
  bw->capacity = capacity;
  bw->byte_pos = 0u;
  bw->bit_buf = 0u;
  bw->bit_count = 0;
  bw->error = GWP_FALSE;
}

void GWPBitWriterPutBits(GWPBitWriter* bw, GWPu32 bits, int nbits) {
  GWPu32 mask;
  if (bw == 0 || bw->error) return;
  if (nbits < 0 || nbits > 24) {
    bw->error = GWP_TRUE;
    return;
  }
  if (nbits == 0) return;
  mask = (((GWPu32)1u) << nbits) - 1u;
  bw->bit_buf |= (bits & mask) << bw->bit_count;
  bw->bit_count += nbits;
  while (bw->bit_count >= 8) {
    if (bw->byte_pos >= bw->capacity) {
      bw->error = GWP_TRUE;
      return;
    }
    bw->dst[bw->byte_pos++] = (GWPu8)(bw->bit_buf & 0xffu);
    bw->bit_buf >>= 8;
    bw->bit_count -= 8;
  }
}

void GWPBitWriterFlush(GWPBitWriter* bw) {
  if (bw == 0 || bw->error) return;
  while (bw->bit_count > 0) {
    if (bw->byte_pos >= bw->capacity) {
      bw->error = GWP_TRUE;
      return;
    }
    bw->dst[bw->byte_pos++] = (GWPu8)(bw->bit_buf & 0xffu);
    bw->bit_buf >>= 8;
    bw->bit_count -= (bw->bit_count >= 8) ? 8 : bw->bit_count;
  }
}

GWPu32 GWPBitWriterSize(const GWPBitWriter* bw) {
  if (bw == 0) return 0u;
  return bw->byte_pos;
}

GWPBool GWPBitWriterOk(const GWPBitWriter* bw) {
  return (bw != 0 && !bw->error) ? GWP_TRUE : GWP_FALSE;
}
