#include "bit_reader.h"

void GWPBitReaderInit(GWPBitReader* br, const GWPu8* data, GWPu32 size) {
  if (br == 0) return;
  br->data = data;
  br->size = size;
  br->byte_pos = 0u;
  br->bit_buf = 0u;
  br->bit_count = 0;
}

static void GWPBitReaderFill(GWPBitReader* br, int need) {
  while (br->bit_count < need && br->byte_pos < br->size) {
    br->bit_buf |= ((GWPu32)br->data[br->byte_pos++]) << br->bit_count;
    br->bit_count += 8;
  }
}

GWPBool GWPBitReaderPeekBits(GWPBitReader* br, int n, GWPu32* out_bits) {
  GWPu32 mask;
  if (br == 0 || out_bits == 0) return GWP_FALSE;
  if (n < 0 || n > 24) return GWP_FALSE;
  if (n == 0) {
    *out_bits = 0u;
    return GWP_TRUE;
  }
  GWPBitReaderFill(br, n);
  mask = (((GWPu32)1u) << n) - 1u;
  *out_bits = br->bit_buf & mask;
  return GWP_TRUE;
}

GWPBool GWPBitReaderSkipBits(GWPBitReader* br, int n) {
  if (br == 0) return GWP_FALSE;
  if (n < 0 || n > 24) return GWP_FALSE;
  if (n == 0) return GWP_TRUE;
  GWPBitReaderFill(br, n);
  if (br->bit_count < n) return GWP_FALSE;
  br->bit_buf >>= n;
  br->bit_count -= n;
  return GWP_TRUE;
}

GWPBool GWPBitReaderGetBits(GWPBitReader* br, int n, GWPu32* out_bits) {
  if (!GWPBitReaderPeekBits(br, n, out_bits)) return GWP_FALSE;
  return GWPBitReaderSkipBits(br, n);
}

GWPBool GWPBitReaderGetBit(GWPBitReader* br, GWPu32* out_bit) {
  return GWPBitReaderGetBits(br, 1, out_bit);
}

GWPu32 GWPBitReaderBytesConsumed(const GWPBitReader* br) {
  if (br == 0) return 0u;
  return br->byte_pos;
}
