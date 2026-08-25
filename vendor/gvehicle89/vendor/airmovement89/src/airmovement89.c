#include "airmovement89.h"

void airmovement89_step(const gveh_airframe_cfg *cfg,
                        gveh_airframe_state *state,
                        gveh_body *body,
                        gveh_basis basis,
                        const gveh_input *input,
                        gveh_fx dt)
{
    if (cfg == 0 || state == 0 || body == 0 || input == 0) return;
    gveh_airframe_apply(cfg, state, body, basis, input, dt);
}

void airmovement89_apply_assist(const gveh_airassist_cfg *cfg,
                                gveh_body *body,
                                const gveh_airframe_state *airframe,
                                const gveh_rotorcraft_state *rotorcraft,
                                const gveh_input *input)
{
    if (cfg == 0 || body == 0 || airframe == 0 || rotorcraft == 0 || input == 0) return;
    gveh_airassist_apply(cfg, body, airframe, rotorcraft, input);
}
