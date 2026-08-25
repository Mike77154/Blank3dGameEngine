#ifndef BLANK3D_KATANA_MESH_H
#define BLANK3D_KATANA_MESH_H

#include "../vendor/giffany_shapes3d/g3d_shapes.h"

#ifdef __cplusplus
extern "C" {
#endif

#define B3D_KATANA_RENDER_VERTEX_CAPACITY 4096
#define B3D_KATANA_RENDER_INDEX_CAPACITY  4096

int blank3d_katana_mesh_build(g3d_mesh *destination,
                              g3d_vertex *vertices,
                              unsigned short vertex_capacity,
                              g3d_index *indices,
                              unsigned short index_capacity,
                              unsigned short preset_index,
                              int thickness_percent,
                              int width_percent,
                              int length_percent);

#ifdef __cplusplus
}
#endif

#endif
