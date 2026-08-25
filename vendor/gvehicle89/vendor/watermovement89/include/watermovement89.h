#ifndef WATERMOVEMENT89_H
#define WATERMOVEMENT89_H

#include "gveh_tag.h"
#include "gveh_world.h"
#include "gveh_fx.h"

void watermovement89_step(gveh_world_i *world,
                          gveh_profile *profile,
                          gveh_body *body,
                          gveh_basis basis,
                          const gveh_input *input,
                          gveh_i32 tick,
                          gveh_fx dt,
                          gveh_fx_queue *fxq);

#endif
