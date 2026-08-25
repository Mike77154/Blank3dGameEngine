#ifndef GVEH_DEBUG_H
#define GVEH_DEBUG_H

#include <stdio.h>
#include "gveh_types.h"

struct gveh_vehicle_s;

void gveh_debug_print(FILE *f, const struct gveh_vehicle_s *v, gveh_i32 tick);

#endif
