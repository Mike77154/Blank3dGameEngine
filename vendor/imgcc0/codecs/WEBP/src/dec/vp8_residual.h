#ifndef GWP_DEC_VP8_RESIDUAL_H_
#define GWP_DEC_VP8_RESIDUAL_H_

#include "vp8_dec.h"
#include "vp8_bool.h"

#ifdef __cplusplus
extern "C" {
#endif

GWPStatusCode GWPVP8ProbeResidualFromState(const GWPVP8ControlHeader* header,
                                           GWPVP8BoolDecoder* mode_br,
                                           GWPVP8ResidualSummary* summary);

#ifdef __cplusplus
}    /* extern "C" */
#endif

#endif  /* GWP_DEC_VP8_RESIDUAL_H_ */
