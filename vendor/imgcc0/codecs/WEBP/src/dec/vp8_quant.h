#ifndef GWP_DEC_VP8_QUANT_H_
#define GWP_DEC_VP8_QUANT_H_

#include "vp8_dec.h"

#ifdef __cplusplus
extern "C" {
#endif

void GWPVP8InitDequantFactors(const GWPVP8SegmentHeader* seg,
                              const GWPVP8QuantHeader* quant,
                              GWPVP8DequantFactors out_factors[GWP_VP8_MAX_SEGMENTS]);

#ifdef __cplusplus
}    /* extern "C" */
#endif

#endif  /* GWP_DEC_VP8_QUANT_H_ */
