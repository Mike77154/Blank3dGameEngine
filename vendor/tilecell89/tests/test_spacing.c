#include "tilecell89.h"
#include <stdio.h>
int main(void)
{
    TileCell89 tc;
    tc89_id a;
    TC89_Rect r;
    tc89_init(&tc);
    a = tc89_add_atlas(&tc, "spaced", "sheet", 38U, 38U, 8U, 8U, 2U, 2U, 2U, 2U);
    if (a == TC89_INVALID_ID) return 2;
    if (tc.atlases[a].columns != 3U || tc.atlases[a].rows != 3U) return 3;
    if (!tc89_atlas_rect_xy(&tc, a, 2U, 2U, &r)) return 4;
    if (r.x != 22 || r.y != 22 || r.w != 8 || r.h != 8) return 5;
    printf("TileCell89 spacing PASS\n");
    return 0;
}
