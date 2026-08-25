#ifndef SPACEMOVEMENT89_H
#define SPACEMOVEMENT89_H

#include "gveh_spacecraft.h"

void spacemovement89_step(gveh_spacecraft_state *state,
                          const gveh_spacecraft_cfg *cfg,
                          gveh_body *body,
                          gveh_basis basis,
                          const gveh_input *input,
                          gveh_fx dt,
                          gveh_fx_queue *fxq);

#endif
