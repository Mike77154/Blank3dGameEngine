#ifndef GWP_DEC_VP8_PROBDATA_H_
#define GWP_DEC_VP8_PROBDATA_H_

#include "../webp/types.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const GWPu8 kGWPVP8CoeffUpdateProbs[4][8][3][11];
extern const GWPu8 kGWPVP8DefaultCoeffProbs[4][8][3][11];
extern const GWPu8 kGWPVP8KfYModeProbs[4];
extern const GWPu8 kGWPVP8KfUVModeProbs[3];
extern const GWPu8 kGWPVP8KfBModeProbs[10][10][9];
extern const GWPu8 kGWPVP8DefaultBModeProbs[9];
extern const GWPu8 kGWPVP8Cat1Probs[2];
extern const GWPu8 kGWPVP8Cat2Probs[3];
extern const GWPu8 kGWPVP8Cat3Probs[4];
extern const GWPu8 kGWPVP8Cat4Probs[5];
extern const GWPu8 kGWPVP8Cat5Probs[6];
extern const GWPu8 kGWPVP8Cat6Probs[12];
extern const GWPu8 kGWPVP8CoeffBands[16];
extern const GWPu8 kGWPVP8LeftContextIndex[25];
extern const GWPu8 kGWPVP8AboveContextIndex[25];
extern const GWPu8 kGWPVP8ZigZag[16];
extern const int kGWPVP8MbSegmentTree[6];
extern const int kGWPVP8KfYModeTree[8];
extern const int kGWPVP8YModeTree[8];
extern const int kGWPVP8UVModeTree[6];
extern const int kGWPVP8BModeTree[18];
extern const int kGWPVP8CoeffTree[22];
extern const GWPu8 kGWPVP8YModeToBMode[4];

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* GWP_DEC_VP8_PROBDATA_H_ */
