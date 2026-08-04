#ifndef GPRIMITIVE89_OBJ_H
#define GPRIMITIVE89_OBJ_H

#include <stdio.h>

#include "gprimitive89.h"

#ifdef __cplusplus
extern "C" {
#endif

int gp89_write_obj(FILE *file, const gp89_mesh *mesh, const char *object_name);

#ifdef __cplusplus
}
#endif

#endif
