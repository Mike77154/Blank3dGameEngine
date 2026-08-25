#ifndef GWP_UTILS_BIT_READER_H_
#define GWP_UTILS_BIT_READER_H_

#include "../webp/types.h"

typedef struct GWPBitReader {
  const GWPu8* data;
  GWPu32 size;
  GWPu32 byte_pos;
  GWPu32 bit_buf;
  int bit_count;
} GWPBitReader;

void GWPBitReaderInit(GWPBitReader* br, const GWPu8* data, GWPu32 size);
GWPBool GWPBitReaderPeekBits(GWPBitReader* br, int n, GWPu32* out_bits);
GWPBool GWPBitReaderSkipBits(GWPBitReader* br, int n);
GWPBool GWPBitReaderGetBits(GWPBitReader* br, int n, GWPu32* out_bits);
GWPBool GWPBitReaderGetBit(GWPBitReader* br, GWPu32* out_bit);
GWPu32 GWPBitReaderBytesConsumed(const GWPBitReader* br);

#endif  /* GWP_UTILS_BIT_READER_H_ */
