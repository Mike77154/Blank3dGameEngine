#ifndef GWP_DEC_VP8_BOOL_H_
#define GWP_DEC_VP8_BOOL_H_

#include "../webp/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GWPVP8BoolDecoder {
  const GWPu8* input;
  GWPu32 input_len;
  GWPu32 total_len;
  GWPu32 range;
  GWPu32 value;
  int bit_count;
  GWPBool error;
} GWPVP8BoolDecoder;

void GWPVP8BoolInit(GWPVP8BoolDecoder* br,
                    const GWPu8* data,
                    GWPu32 data_size);
GWPBool GWPVP8BoolGet(GWPVP8BoolDecoder* br,
                      int probability,
                      GWPu32* out_bit);
GWPBool GWPVP8BoolGetBit(GWPVP8BoolDecoder* br, GWPu32* out_bit);
GWPBool GWPVP8BoolGetUInt(GWPVP8BoolDecoder* br,
                          int bits,
                          GWPu32* out_value);
GWPBool GWPVP8BoolGetInt(GWPVP8BoolDecoder* br,
                         int bits,
                         int* out_value);
GWPBool GWPVP8BoolMaybeGetInt(GWPVP8BoolDecoder* br,
                              int bits,
                              int* out_value);
GWPu32 GWPVP8BoolBytesTouched(const GWPVP8BoolDecoder* br);
GWPBool GWPVP8BoolHasError(const GWPVP8BoolDecoder* br);

#ifdef __cplusplus
}    /* extern "C" */
#endif

#endif  /* GWP_DEC_VP8_BOOL_H_ */
