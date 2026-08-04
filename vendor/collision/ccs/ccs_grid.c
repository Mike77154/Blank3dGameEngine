#include "ccs_grid.h"

ccs_grid_cell ccs_grid[CCS_GRID_DIM][CCS_GRID_DIM];
int           ccs_grid_next[CCS_GRID_MAX_OBJECTS];

void ccs_grid_clear(void)
{
    int x;
    int z;
    int i;

    for (z = 0; z < CCS_GRID_DIM; ++z) {
        for (x = 0; x < CCS_GRID_DIM; ++x) {
            ccs_grid[z][x].head = -1;
        }
    }

    for (i = 0; i < CCS_GRID_MAX_OBJECTS; ++i)
        ccs_grid_next[i] = -1;
}

void ccs_grid_insert_aabb(int id, const ccs_aabb* aabb)
{
    ccs_vec3 c;
    int wx;
    int wz;
    int gx;
    int gz;

    if (!aabb)
        return;

    if (id < 0 || id >= CCS_GRID_MAX_OBJECTS)
        return;

    /* Inserción simple: celda del centro del AABB */
    c = ccs_aabb_center(*aabb);

    wx = (int)ccs_fixed_to_int(c.x);
    wz = (int)ccs_fixed_to_int(c.z);

    gx = (wx / CCS_GRID_SIZE) + CCS_GRID_BIAS;
    gz = (wz / CCS_GRID_SIZE) + CCS_GRID_BIAS;

    if (gx < 0 || gz < 0 || gx >= CCS_GRID_DIM || gz >= CCS_GRID_DIM)
        return;

    ccs_grid_next[id] = ccs_grid[gz][gx].head;
    ccs_grid[gz][gx].head = id;
}
