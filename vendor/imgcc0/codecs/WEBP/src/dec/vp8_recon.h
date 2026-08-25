#ifndef GWP_DEC_VP8_RECON_H_
#define GWP_DEC_VP8_RECON_H_

#include "vp8_dec.h"

#ifdef __cplusplus
extern "C" {
#endif

void GWPVP8Predict16x16(GWPVP8YMode mode,
                        const GWPu8* above,
                        const GWPu8* left,
                        GWPu8 top_left,
                        GWPu8 out[16 * 16]);

void GWPVP8Predict8x8(GWPVP8UVMode mode,
                      const GWPu8* above,
                      const GWPu8* left,
                      GWPu8 top_left,
                      GWPu8 out[8 * 8]);

void GWPVP8Predict4x4(GWPVP8BMode mode,
                      const GWPu8* above,
                      const GWPu8* left,
                      GWPu8 top_left,
                      GWPu8 out[4 * 4]);

void GWPVP8AddResidual4x4(const GWPu8* pred,
                          GWPu32 pred_stride,
                          const int residue[16],
                          GWPu8* dst,
                          GWPu32 dst_stride);

#ifdef __cplusplus
}    /* extern "C" */
#endif

#endif  /* GWP_DEC_VP8_RECON_H_ */
