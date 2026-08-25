#include "spacemovement89.h"

void spacemovement89_step(gveh_spacecraft_state *state,
                          const gveh_spacecraft_cfg *cfg,
                          gveh_body *body,
                          gveh_basis basis,
                          const gveh_input *input,
                          gveh_fx dt,
                          gveh_fx_queue *fxq)
{
    if (state == 0 || cfg == 0 || body == 0 || input == 0) return;
    gveh_spacecraft_apply(state, cfg, body, basis, input, dt, fxq);
}
