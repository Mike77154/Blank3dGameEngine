#include "w3d89.h"
#include <stdio.h>

static unsigned char g_mem[65536UL];

int main(void)
{
    w3d_world_config cfg;
    w3d_world world;
    w3d_u16 visible[8];
    int n;
    int i;
    w3d_aabb door;

    w3d_world_default_config(&cfg);
    cfg.max_cells = 8U;
    cfg.max_cell_hash_entries = 19U;
    cfg.max_entities = 8U;
    cfg.max_portals = 8U;
    cfg.cell_size = w3d_fp_from_int(8);
    if (w3d_world_init(&world, &cfg, g_mem, sizeof(g_mem), 0, 0) != W3D_OK) return 2;

    w3d_world_define_cell(&world, 0, 0, 0, W3D_CELL_FLAG_ROOM | W3D_CELL_FLAG_INTERIOR, W3D_LAYER_ALL, 10UL);
    w3d_world_define_cell(&world, 1, 0, 0, W3D_CELL_FLAG_ROOM | W3D_CELL_FLAG_INTERIOR, W3D_LAYER_ALL, 11UL);
    w3d_world_define_cell(&world, 2, 0, 0, W3D_CELL_FLAG_ROOM | W3D_CELL_FLAG_INTERIOR, W3D_LAYER_ALL, 12UL);

    door = w3d_aabb_make(w3d_v3_make(w3d_fp_from_int(8), 0, 0), w3d_v3_make(w3d_fp_from_int(8), w3d_fp_from_int(2), w3d_fp_from_int(3)));
    w3d_world_define_portal(&world, 0U, 1U, door, W3D_PORTAL_OPEN, W3D_LAYER_ALL, W3D_PHASE_ALL, 101UL);
    w3d_world_define_portal(&world, 1U, 2U, door, 0UL, W3D_LAYER_ALL, W3D_PHASE_ALL, 102UL);

    n = w3d_world_query_visible_cells_from(&world, 0U, 4U, W3D_LAYER_ALL, visible, 8U);
    printf("visible before opening second door: ");
    for (i = 0; i < n; ++i) printf("%u ", (unsigned)visible[i]);
    printf("\n");

    w3d_world_set_portal_open(&world, 1U, 1);
    n = w3d_world_query_visible_cells_from(&world, 0U, 4U, W3D_LAYER_ALL, visible, 8U);
    printf("visible after opening second door: ");
    for (i = 0; i < n; ++i) printf("%u ", (unsigned)visible[i]);
    printf("\n");
    return 0;
}
