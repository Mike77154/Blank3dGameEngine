#ifndef BUSMOVEMENT89_H
#define BUSMOVEMENT89_H

#include "carmovement89.h"

/*
   Baseline long/heavy wheeled policy. Phase 1 deliberately delegates to the
   deterministic car wheel solver. Future bus-only articulation/body-roll
   policy can be added here without changing gvehicle89.
*/
void busmovement89_step(gveh_surface *surfaces,
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
