#ifndef ROTORCRAFTMOVEMENT89_H
#define ROTORCRAFTMOVEMENT89_H

#include "gveh_rotorcraft.h"

void rotorcraftmovement89_step(const gveh_rotorcraft_cfg *cfg,
                               gveh_rotorcraft_state *state,
                               gveh_body *body,
                               gveh_basis basis,
                               const gveh_input *input,
                               gveh_fx dt);

#endif
