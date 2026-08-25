#ifndef TANKMOVEMENT89_H
#define TANKMOVEMENT89_H

#include "gveh_tank4.h"
#include "gveh_fx.h"

void tankmovement89_step(const gveh_tank4_cfg *cfg,
                         gveh_tank4_state *state,
                         gveh_body *body,
                         gveh_basis basis,
                         const gveh_input *input,
                         gveh_fx grounded_ratio,
                         gveh_fx speed_abs,
                         gveh_fx dt,
                         gveh_i32 tick,
                         gveh_fx_queue *fxq);
void tankmovement89_post_integrate(gveh_body *body, gveh_u16 class_flags);

#endif
