#include "tilecell89.h"
#include <stdio.h>

int main(void)
{
    TileCell89 ctx;
    tc89_id atlas;
    tc89_id fire0;
    tc89_id fire1;
    tc89_id map;
    TC89_Rect r;
    TC89_Rect strip[4];
    tc89_init(&ctx);
    atlas = tc89_add_atlas(&ctx, "fire", "fire_sheet", 128U, 64U, 16U, 16U, 0U, 0U, 0U, 0U);
    if (atlas == TC89_INVALID_ID) return 2;
    if (!tc89_atlas_rect_index(&ctx, atlas, 10U, &r)) return 3;
    if (r.x != 32 || r.y != 16 || r.w != 16 || r.h != 16) return 4;
    fire0 = tc89_add_named_cell(&ctx, "fire0", atlas, 0U, 0U, 0U, 100U);
    fire1 = tc89_add_named_cell(&ctx, "fire1", atlas, 1U, 0U, TC89_CELL_FLIP_X, 101U);
    if (fire0 == TC89_INVALID_ID || fire1 == TC89_INVALID_ID) return 5;
    map = tc89_add_map(&ctx, "demo", 4U, 2U);
    if (map == TC89_INVALID_ID) return 6;
    if (!tc89_map_set(&ctx, map, 0U, 0U, fire0) || !tc89_map_set(&ctx, map, 1U, 0U, fire1)) return 7;
    if (tc89_map_get(&ctx, map, 1U, 0U) != fire1) return 8;
    if (!tc89_collect_strip(&ctx, atlas, 0U, 1U, 1, 0, 4U, strip, 4U)) return 9;
    if (strip[3].x != 48 || strip[3].y != 16) return 10;
    printf("TileCell89 PASS atlases=%u cells=%u maps=%u\n",
           (unsigned)ctx.atlas_count, (unsigned)ctx.cell_count, (unsigned)ctx.map_count);
    return 0;
}
