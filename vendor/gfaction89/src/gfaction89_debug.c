#include "gfaction89_debug.h"
#include <stdio.h>

void gfa_debug_print_matrix(const GFA_World *world)
{
    int i;
    int j;
    GFA_Relation r;
    if (!world) return;
    printf("Faction matrix:\n");
    for (i = 0; i < world->faction_count; ++i) {
        for (j = 0; j < world->faction_count; ++j) {
            r = world->matrix[i][j];
            if (r.disposition != GFA_DISP_NEUTRAL || r.priority != 0 || (r.flags & ~GFA_FLAG_VALID) != 0) {
                printf("  %s -> %s = %s p=%d flags=0x%04x\n",
                    gfa_faction_name(world, i), gfa_faction_name(world, j),
                    gfa_disposition_name(r.disposition), r.priority, r.flags);
            }
        }
    }
}
