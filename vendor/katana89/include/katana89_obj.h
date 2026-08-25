#ifndef KATANA89_OBJ_H
#define KATANA89_OBJ_H
#include "katana89.h"
#include <stdio.h>
int km89_write_obj(FILE *obj, FILE *mtl, const char *mtl_name, const km89_mesh *mesh, const km89_palette *palette);
#endif
