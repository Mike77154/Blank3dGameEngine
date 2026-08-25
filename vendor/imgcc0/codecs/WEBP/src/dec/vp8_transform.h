#ifndef GWP_DEC_VP8_TRANSFORM_H_
#define GWP_DEC_VP8_TRANSFORM_H_

#include "../webp/types.h"

#ifdef __cplusplus
extern "C" {
#endif

void GWPVP8DequantizeBlock(const int coeffs[16],
                           int dc_q,
                           int ac_q,
                           int out[16]);

void GWPVP8InverseWalsh4x4(const int input[16], int output[16]);
void GWPVP8InverseWalsh4x4DC(int dc, int output[16]);
void GWPVP8InverseDCT4x4(const int input[16], int output[16]);
void GWPVP8InverseDCT4x4DC(int dc, int output[16]);

#ifdef __cplusplus
}    /* extern "C" */
#endif

#endif  /* GWP_DEC_VP8_TRANSFORM_H_ */
