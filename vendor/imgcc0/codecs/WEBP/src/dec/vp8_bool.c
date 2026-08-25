#include "vp8_bool.h"

void GWPVP8BoolInit(GWPVP8BoolDecoder* br,
                    const GWPu8* data,
                    GWPu32 data_size) {
  if (br == 0) return;
  br->input = 0;
  br->input_len = 0u;
  br->total_len = data_size;
  br->range = 255u;
  br->value = 0u;
  br->bit_count = 0;
  br->error = GWP_FALSE;
  if (data == 0) {
    br->error = GWP_TRUE;
    return;
  }
  if (data_size >= 2u) {
    br->value = ((GWPu32)data[0] << 8) | (GWPu32)data[1];
    br->input = data + 2;
    br->input_len = data_size - 2u;
  } else {
    br->input = data + data_size;
    br->input_len = 0u;
    br->error = GWP_TRUE;
  }
}

GWPBool GWPVP8BoolGet(GWPVP8BoolDecoder* br,
                      int probability,
                      GWPu32* out_bit) {
  GWPu32 split;
  GWPu32 split_shifted;
  GWPu32 bit;
  if (br == 0 || out_bit == 0) return GWP_FALSE;
  if (probability < 0 || probability > 255) return GWP_FALSE;
  if (br->error) return GWP_FALSE;

  split = 1u + (((br->range - 1u) * (GWPu32)probability) >> 8);
  split_shifted = split << 8;
  if (br->value >= split_shifted) {
    bit = 1u;
    br->range -= split;
    br->value -= split_shifted;
  } else {
    bit = 0u;
    br->range = split;
  }

  while (br->range < 128u) {
    br->value <<= 1;
    br->range <<= 1;
    ++br->bit_count;
    if (br->bit_count == 8) {
      br->bit_count = 0;
      if (br->input_len == 0u) {
        br->error = GWP_TRUE;
        return GWP_FALSE;
      }
      br->value |= (GWPu32)(*br->input++);
      --br->input_len;
    }
  }

  *out_bit = bit;
  return GWP_TRUE;
}

GWPBool GWPVP8BoolGetBit(GWPVP8BoolDecoder* br, GWPu32* out_bit) {
  return GWPVP8BoolGet(br, 128, out_bit);
}

GWPBool GWPVP8BoolGetUInt(GWPVP8BoolDecoder* br,
                          int bits,
                          GWPu32* out_value) {
  GWPu32 value;
  int bit;
  GWPu32 one_bit;
  if (br == 0 || out_value == 0) return GWP_FALSE;
  if (bits < 0 || bits > 31) return GWP_FALSE;
  value = 0u;
  for (bit = bits - 1; bit >= 0; --bit) {
    if (!GWPVP8BoolGetBit(br, &one_bit)) return GWP_FALSE;
    value |= one_bit << bit;
  }
  *out_value = value;
  return GWP_TRUE;
}

GWPBool GWPVP8BoolGetInt(GWPVP8BoolDecoder* br,
                         int bits,
                         int* out_value) {
  GWPu32 value;
  GWPu32 sign;
  if (br == 0 || out_value == 0) return GWP_FALSE;
  if (!GWPVP8BoolGetUInt(br, bits, &value)) return GWP_FALSE;
  if (!GWPVP8BoolGetBit(br, &sign)) return GWP_FALSE;
  *out_value = sign ? -(int)value : (int)value;
  return GWP_TRUE;
}

GWPBool GWPVP8BoolMaybeGetInt(GWPVP8BoolDecoder* br,
                              int bits,
                              int* out_value) {
  GWPu32 has_update;
  if (br == 0 || out_value == 0) return GWP_FALSE;
  if (!GWPVP8BoolGetBit(br, &has_update)) return GWP_FALSE;
  if (!has_update) {
    *out_value = 0;
    return GWP_TRUE;
  }
  return GWPVP8BoolGetInt(br, bits, out_value);
}

GWPu32 GWPVP8BoolBytesTouched(const GWPVP8BoolDecoder* br) {
  if (br == 0) return 0u;
  return br->total_len - br->input_len;
}

GWPBool GWPVP8BoolHasError(const GWPVP8BoolDecoder* br) {
  if (br == 0) return GWP_TRUE;
  return br->error;
}
