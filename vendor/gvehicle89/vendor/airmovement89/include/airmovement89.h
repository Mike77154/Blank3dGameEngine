#ifndef AIRMOVEMENT89_H
#define AIRMOVEMENT89_H

#include "gveh_airframe.h"
#include "gveh_airassist.h"
#include "gveh_rotorcraft.h"

void airmovement89_step(const gveh_airframe_cfg *cfg,
                        gveh_airframe_state *state,
                        gveh_body *body,
                        gveh_basis basis,
                        const gveh_input *input,
                        gveh_fx dt);
void airmovement89_apply_assist(const gveh_airassist_cfg *cfg,
                                gveh_body *body,
                                const gveh_airframe_state *airframe,
                                const gveh_rotorcraft_state *rotorcraft,
                                const gveh_input *input);

#endif
