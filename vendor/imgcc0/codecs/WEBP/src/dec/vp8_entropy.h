#ifndef GWP_DEC_VP8_ENTROPY_H_
#define GWP_DEC_VP8_ENTROPY_H_

#include "vp8_dec.h"
#include "vp8_bool.h"

#ifdef __cplusplus
extern "C" {
#endif

void GWPVP8InitEntropyDefaults(GWPVP8EntropyHeader* entropy,
                               GWPBool key_frame);

GWPStatusCode GWPVP8ParseEntropyHeader(GWPVP8BoolDecoder* br,
                                       GWPBool key_frame,
                                       GWPVP8EntropyHeader* entropy);

#ifdef __cplusplus
}    /* extern "C" */
#endif

#endif  /* GWP_DEC_VP8_ENTROPY_H_ */
