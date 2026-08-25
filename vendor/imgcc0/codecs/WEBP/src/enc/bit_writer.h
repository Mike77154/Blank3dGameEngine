#ifndef GWP_ENC_BIT_WRITER_H_
#define GWP_ENC_BIT_WRITER_H_

#include "../webp/types.h"

typedef struct GWPBitWriter {
  GWPu8* dst;
  GWPu32 capacity;
  GWPu32 byte_pos;
  GWPu32 bit_buf;
  int bit_count;
  GWPBool error;
} GWPBitWriter;

void GWPBitWriterInit(GWPBitWriter* bw, GWPu8* dst, GWPu32 capacity);
void GWPBitWriterPutBits(GWPBitWriter* bw, GWPu32 bits, int nbits);
void GWPBitWriterFlush(GWPBitWriter* bw);
GWPu32 GWPBitWriterSize(const GWPBitWriter* bw);
GWPBool GWPBitWriterOk(const GWPBitWriter* bw);

#endif  /* GWP_ENC_BIT_WRITER_H_ */
