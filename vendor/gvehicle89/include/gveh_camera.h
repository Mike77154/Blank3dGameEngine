#ifndef GVEH_CAMERA_H
#define GVEH_CAMERA_H

#include "gveh_math.h"

typedef struct gveh_camera_s {
    gveh_vec3 pos;
    gveh_vec3 look;
    gveh_fx dist;
    gveh_fx height;
    gveh_fx lag;
    gveh_fx fov;
} gveh_camera;

void gveh_camera_default(gveh_camera *c);
void gveh_camera_update(gveh_camera *c, gveh_vec3 veh_pos, gveh_basis veh_basis, gveh_fx speed, gveh_fx dt);

#endif
