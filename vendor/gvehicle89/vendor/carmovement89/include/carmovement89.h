#ifndef CARMOVEMENT89_H
#define CARMOVEMENT89_H

#include "gveh_tag.h"
#include "gveh_world.h"
#include "gveh_fx.h"

void carmovement89_step(gveh_surface *surfaces,
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
