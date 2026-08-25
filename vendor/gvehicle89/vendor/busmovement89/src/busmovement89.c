#include "busmovement89.h"

void busmovement89_step(gveh_surface *surfaces,
                        gveh_world_i *world,
                        gveh_profile *profile,
                        gveh_body *body,
                        gveh_basis basis,
                        const gveh_input *input,
                        gveh_fx dt,
                        gveh_i32 *grounded_count_out,
                        gveh_fx *grounded_ratio_out,
                        gveh_fx_queue *fxq)
{
    carmovement89_step(surfaces, world, profile, body, basis, input, dt,
                       grounded_count_out, grounded_ratio_out, fxq);
}
