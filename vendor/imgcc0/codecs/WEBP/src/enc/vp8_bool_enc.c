#include "vp8_bool_enc.h"

static void GWPVP8BoolAddOneToOutput(GWPu8* q, GWPu8* start) {
  if (q == 0 || start == 0) return;
  while (q > start) {
    --q;
    if (*q == 255u) {
      *q = 0u;
    } else {
      ++*q;
      return;
    }
  }
  if (q == start) {
    if (*q == 255u) *q = 0u;
    else ++*q;
  }
}

void GWPVP8BoolEncInit(GWPVP8BoolEncoder* bw, GWPu8* dst, GWPu32 capacity) {
  if (bw == 0) return;
  bw->output = dst;
  bw->start = dst;
  bw->capacity = capacity;
  bw->range = 255u;
  bw->bottom = 0u;
  bw->bit_count = 24;
  bw->error = (dst == 0) ? GWP_TRUE : GWP_FALSE;
}

void GWPVP8BoolEncWrite(GWPVP8BoolEncoder* bw, int probability, int bit) {
  GWPu32 split;
  if (bw == 0 || bw->error) return;
  if (probability <= 0 || probability >= 256) {
    bw->error = GWP_TRUE;
    return;
  }
  split = 1u + (((bw->range - 1u) * (GWPu32)probability) >> 8);
  if (bit) {
    bw->bottom += split;
    bw->range -= split;
  } else {
    bw->range = split;
  }
  while (bw->range < 128u) {
    bw->range <<= 1;
    if ((bw->bottom & 0x80000000u) != 0u) {
      GWPVP8BoolAddOneToOutput(bw->output, bw->start);
    }
    bw->bottom <<= 1;
    --bw->bit_count;
    if (bw->bit_count == 0) {
      if ((GWPu32)(bw->output - bw->start) >= bw->capacity) {
        bw->error = GWP_TRUE;
        return;
      }
      *bw->output++ = (GWPu8)(bw->bottom >> 24);
      bw->bottom &= 0x00ffffffu;
      bw->bit_count = 8;
    }
  }
}

void GWPVP8BoolEncWriteBit(GWPVP8BoolEncoder* bw, int bit) {
  GWPVP8BoolEncWrite(bw, 128, bit ? 1 : 0);
}

void GWPVP8BoolEncWriteUInt(GWPVP8BoolEncoder* bw, GWPu32 value, int bits) {
  int bit;
  if (bw == 0) return;
  for (bit = bits - 1; bit >= 0; --bit) {
    GWPVP8BoolEncWriteBit(bw, (int)((value >> bit) & 1u));
  }
}

void GWPVP8BoolEncWriteMaybeInt(GWPVP8BoolEncoder* bw, int value, int bits) {
  GWPu32 abs_value;
  if (bw == 0) return;
  if (value == 0) {
    GWPVP8BoolEncWriteBit(bw, 0);
    return;
  }
  GWPVP8BoolEncWriteBit(bw, 1);
  abs_value = (value < 0) ? (GWPu32)(-value) : (GWPu32)value;
  GWPVP8BoolEncWriteUInt(bw, abs_value, bits);
  GWPVP8BoolEncWriteBit(bw, value < 0 ? 1 : 0);
}

void GWPVP8BoolEncFlush(GWPVP8BoolEncoder* bw) {
  int c;
  GWPu32 v;
  if (bw == 0 || bw->error) return;
  c = bw->bit_count;
  v = bw->bottom;
  if ((v & (1u << (32 - c))) != 0u) {
    GWPVP8BoolAddOneToOutput(bw->output, bw->start);
  }
  v <<= (c & 7);
  c >>= 3;
  while (--c >= 0) v <<= 8;
  c = 4;
  while (--c >= 0) {
    if ((GWPu32)(bw->output - bw->start) >= bw->capacity) {
      bw->error = GWP_TRUE;
      return;
    }
    *bw->output++ = (GWPu8)(v >> 24);
    v <<= 8;
  }
}

GWPu32 GWPVP8BoolEncSize(const GWPVP8BoolEncoder* bw) {
  if (bw == 0 || bw->start == 0 || bw->output == 0) return 0u;
  return (GWPu32)(bw->output - bw->start);
}

GWPBool GWPVP8BoolEncOk(const GWPVP8BoolEncoder* bw) {
  return (bw != 0 && !bw->error) ? GWP_TRUE : GWP_FALSE;
}
