#include "tilecell89.h"

int main(void)
{
    TileCell89 tc;
    tc89_id atlas;
    tc89_id grass;
    tc89_id map;
    tc89_init(&tc);
    atlas = tc89_add_atlas(&tc, "world", "world_tiles", 256U, 256U, 16U, 16U, 0U, 0U, 0U, 0U);
    grass = tc89_add_named_cell(&tc, "grass", atlas, 0U, 0U, 0U, 0U);
    map = tc89_add_map(&tc, "room", 20U, 15U);
    tc89_map_set(&tc, map, 3U, 4U, grass);
    return 0;
}
