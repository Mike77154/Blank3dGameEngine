#ifndef GWP_DEC_VP8_MODES_H_
#define GWP_DEC_VP8_MODES_H_

#include "vp8_dec.h"
#include "vp8_bool.h"

#ifdef __cplusplus
extern "C" {
#endif

GWPStatusCode GWPVP8ReadKeyFrameMacroblockHeader(
    GWPVP8BoolDecoder* br,
    const GWPVP8SegmentHeader* segment,
    const GWPVP8EntropyHeader* entropy,
    GWPu8* above_pred,
    GWPu8 left_pred[4],
    GWPu32 col_base,
    GWPu32* out_segment_id,
    GWPBool* out_skip_coeff,
    int* out_y_mode,
    int* out_uv_mode,
    GWPu8 out_b_modes[16]);

GWPStatusCode GWPVP8ParseKeyFrameModeSummary(GWPVP8BoolDecoder* br,
                                             const GWPVP8SegmentHeader* segment,
                                             const GWPVP8EntropyHeader* entropy,
                                             GWPu32 mb_cols,
                                             GWPu32 mb_rows,
                                             GWPVP8ModeSummary* summary);

#ifdef __cplusplus
}    /* extern "C" */
#endif

#endif  /* GWP_DEC_VP8_MODES_H_ */
