#include "tankmovement89.h"
#include "gveh_tag.h"

void tankmovement89_step(const gveh_tank4_cfg *cfg,
                         gveh_tank4_state *state,
                         gveh_body *body,
                         gveh_basis basis,
                         const gveh_input *input,
                         gveh_fx grounded_ratio,
                         gveh_fx speed_abs,
                         gveh_fx dt,
                         gveh_i32 tick,
                         gveh_fx_queue *fxq)
{
    if (cfg == 0 || state == 0 || body == 0 || input == 0) return;
    gveh_tank4_step(cfg, state, body, basis, input, grounded_ratio, speed_abs, dt, tick, fxq);
}

void tankmovement89_post_integrate(gveh_body *body, gveh_u16 class_flags)
{
    if (body == 0) return;
    if ((class_flags & GVEH_CLASS_TANKSIM) != 0u && body->pos.y < gveh_fx_from_int(2)) {
        body->pos.y = gveh_fx_from_int(2);
        if (body->vel.y < 0) body->vel.y = 0;
    }
}
