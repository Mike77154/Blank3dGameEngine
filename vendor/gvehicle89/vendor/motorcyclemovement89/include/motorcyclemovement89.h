#ifndef MOTORCYCLEMOVEMENT89_H
#define MOTORCYCLEMOVEMENT89_H

#include "carmovement89.h"

/*
   Phase-1 two-wheel specialization seam. It reuses the existing wheel solver
   but does not pretend to implement lean/counter-steer/balance yet.
*/
void motorcyclemovement89_step(gveh_surface *surfaces,
                               gveh_world_i *world,
                               gveh_profile *profile,
                               gveh_body *body,
                               gveh_basis basis,
                               const gveh_input *input,
                               gveh_fx dt,
                               gveh_i32 *grounded_count_out,
                               gveh_fx *grounded_ratio_out,
                               gveh_fx_queue *fxq);

#endif
