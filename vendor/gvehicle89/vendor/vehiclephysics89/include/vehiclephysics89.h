#ifndef VEHICLEPHYSICS89_H
#define VEHICLEPHYSICS89_H

#include "gveh_body.h"
#include "gveh_masspoint.h"
#include "gveh_world.h"

void vehiclephysics89_apply_gravity(gveh_body *body, gveh_fx mass, gveh_fx gravity);
void vehiclephysics89_apply_masspoints(gveh_body *body,
                                       const gveh_masspoint *masspoints,
                                       gveh_i32 masspoint_count,
                                       gveh_world_i *world,
                                       gveh_basis basis,
                                       gveh_fx gravity,
                                       gveh_fx dt);
void vehiclephysics89_apply_linear_drag(gveh_body *body, gveh_fx speed_forward);
void vehiclephysics89_integrate_with_retention(gveh_body *body, gveh_fx dt,
                                                 gveh_fx velocity_retention);
void vehiclephysics89_integrate(gveh_body *body, gveh_fx dt);

#endif
