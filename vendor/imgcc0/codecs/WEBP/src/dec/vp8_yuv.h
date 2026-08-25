#ifndef GWP_DEC_VP8_YUV_H_
#define GWP_DEC_VP8_YUV_H_

#include "../webp/decode.h"

#ifdef __cplusplus
extern "C" {
#endif

void GWPVP8YUV420RowToPacked(const GWPu8* y_row,
                             const GWPu8* u_row,
                             const GWPu8* v_row,
                             GWPu8* dst,
                             GWPu32 width,
                             GWPPixelFormat fmt);

#ifdef __cplusplus
}    /* extern "C" */
#endif

#endif  /* GWP_DEC_VP8_YUV_H_ */
