#ifndef GWP_ENC_VP8_BOOL_ENC_H_
#define GWP_ENC_VP8_BOOL_ENC_H_

#include "../webp/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GWPVP8BoolEncoder {
  GWPu8* output;
  GWPu8* start;
  GWPu32 capacity;
  GWPu32 range;
  GWPu32 bottom;
  int bit_count;
  GWPBool error;
} GWPVP8BoolEncoder;

void GWPVP8BoolEncInit(GWPVP8BoolEncoder* bw, GWPu8* dst, GWPu32 capacity);
void GWPVP8BoolEncWrite(GWPVP8BoolEncoder* bw, int probability, int bit);
void GWPVP8BoolEncWriteBit(GWPVP8BoolEncoder* bw, int bit);
void GWPVP8BoolEncWriteUInt(GWPVP8BoolEncoder* bw, GWPu32 value, int bits);
void GWPVP8BoolEncWriteMaybeInt(GWPVP8BoolEncoder* bw, int value, int bits);
void GWPVP8BoolEncFlush(GWPVP8BoolEncoder* bw);
GWPu32 GWPVP8BoolEncSize(const GWPVP8BoolEncoder* bw);
GWPBool GWPVP8BoolEncOk(const GWPVP8BoolEncoder* bw);

#ifdef __cplusplus
}    /* extern "C" */
#endif

#endif  /* GWP_ENC_VP8_BOOL_ENC_H_ */
