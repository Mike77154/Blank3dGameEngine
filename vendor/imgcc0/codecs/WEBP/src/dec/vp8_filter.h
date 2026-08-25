#ifndef GWP_DEC_VP8_FILTER_H_
#define GWP_DEC_VP8_FILTER_H_

#include "vp8_dec.h"

#ifdef __cplusplus
extern "C" {
#endif

int GWPVP8LoopFilterInteriorLimit(int filter_level, int sharpness_level);
int GWPVP8LoopFilterHevThreshold(int filter_level, GWPBool key_frame);
int GWPVP8LoopFilterMBEdgeLimit(int filter_level, int interior_limit);
int GWPVP8LoopFilterSubblockEdgeLimit(int filter_level, int interior_limit);

void GWPVP8SimpleFilterVerticalEdge(GWPu8* ptr,
                                    GWPu32 stride,
                                    int edge_limit,
                                    int count);
void GWPVP8SimpleFilterHorizontalEdge(GWPu8* ptr,
                                      GWPu32 stride,
                                      int edge_limit,
                                      int count);
void GWPVP8NormalMBFilterVerticalEdge(GWPu8* ptr,
                                      GWPu32 stride,
                                      int edge_limit,
                                      int interior_limit,
                                      int hev_threshold,
                                      int count);
void GWPVP8NormalMBFilterHorizontalEdge(GWPu8* ptr,
                                        GWPu32 stride,
                                        int edge_limit,
                                        int interior_limit,
                                        int hev_threshold,
                                        int count);
void GWPVP8NormalSubblockFilterVerticalEdge(GWPu8* ptr,
                                            GWPu32 stride,
                                            int edge_limit,
                                            int interior_limit,
                                            int hev_threshold,
                                            int count);
void GWPVP8NormalSubblockFilterHorizontalEdge(GWPu8* ptr,
                                              GWPu32 stride,
                                              int edge_limit,
                                              int interior_limit,
                                              int hev_threshold,
                                              int count);

#ifdef __cplusplus
}    /* extern "C" */
#endif

#endif  /* GWP_DEC_VP8_FILTER_H_ */
