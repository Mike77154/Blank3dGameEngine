#include "rotorcraftmovement89.h"

void rotorcraftmovement89_step(const gveh_rotorcraft_cfg *cfg,
                               gveh_rotorcraft_state *state,
                               gveh_body *body,
                               gveh_basis basis,
                               const gveh_input *input,
                               gveh_fx dt)
{
    if (cfg == 0 || state == 0 || body == 0 || input == 0) return;
    gveh_rotorcraft_apply(cfg, state, body, basis, input, dt);
}
