#ifndef TILECELL89_H
#define TILECELL89_H

#include <limits.h>

#if UINT_MAX != 0xFFFFFFFFU
#error TileCell89 requires a 32-bit unsigned int target
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define TC89_VERSION_MAJOR 0
#define TC89_VERSION_MINOR 1
#define TC89_VERSION_PATCH 0

#ifndef TC89_MAX_ATLASES
#define TC89_MAX_ATLASES 32
#endif
#ifndef TC89_MAX_NAMED_CELLS
#define TC89_MAX_NAMED_CELLS 512
#endif
#ifndef TC89_MAX_MAPS
#define TC89_MAX_MAPS 32
#endif
#ifndef TC89_MAX_MAP_TILES
#define TC89_MAX_MAP_TILES 8192
#endif
#ifndef TC89_NAME_CAP
#define TC89_NAME_CAP 64
#endif
#ifndef TC89_REQUEST_CAP
#define TC89_REQUEST_CAP 260
#endif

#define TC89_INVALID_ID 0xFFFFU
#define TC89_EMPTY_CELL 0xFFFFU
#define TC89_CELL_FLIP_X 0x00000001U
#define TC89_CELL_FLIP_Y 0x00000002U

typedef unsigned short tc89_id;
typedef unsigned int tc89_u32;
typedef signed int tc89_s32;
typedef unsigned char tc89_u8;

typedef struct TC89_Rect_s {
    tc89_s32 x;
    tc89_s32 y;
    tc89_s32 w;
    tc89_s32 h;
} TC89_Rect;

typedef struct TC89_Atlas_s {
    char name[TC89_NAME_CAP];
    char image_request[TC89_REQUEST_CAP];
    tc89_u32 image_width;
    tc89_u32 image_height;
    tc89_u32 cell_width;
    tc89_u32 cell_height;
    tc89_u32 margin_x;
    tc89_u32 margin_y;
    tc89_u32 spacing_x;
    tc89_u32 spacing_y;
    tc89_u32 columns;
    tc89_u32 rows;
    tc89_u8 used;
} TC89_Atlas;

typedef struct TC89_Cell_s {
    char name[TC89_NAME_CAP];
    tc89_id atlas_id;
    tc89_u32 cell_x;
    tc89_u32 cell_y;
    tc89_u32 flags;
    tc89_u32 user_tag;
    tc89_u8 used;
} TC89_Cell;

typedef struct TC89_Map_s {
    char name[TC89_NAME_CAP];
    tc89_u32 width;
    tc89_u32 height;
    tc89_u32 first_tile;
    tc89_u32 tile_count;
    tc89_u8 used;
} TC89_Map;

typedef struct TileCell89_s {
    TC89_Atlas atlases[TC89_MAX_ATLASES];
    TC89_Cell cells[TC89_MAX_NAMED_CELLS];
    TC89_Map maps[TC89_MAX_MAPS];
    tc89_id map_tiles[TC89_MAX_MAP_TILES];
    tc89_id atlas_count;
    tc89_id cell_count;
    tc89_id map_count;
    tc89_u32 map_tile_count;
    int last_error;
} TileCell89;

void tc89_init(TileCell89 *ctx);
tc89_id tc89_add_atlas(TileCell89 *ctx, const char *name, const char *image_request,
                       tc89_u32 image_width, tc89_u32 image_height,
                       tc89_u32 cell_width, tc89_u32 cell_height,
                       tc89_u32 margin_x, tc89_u32 margin_y,
                       tc89_u32 spacing_x, tc89_u32 spacing_y);
tc89_id tc89_find_atlas(const TileCell89 *ctx, const char *name);
int tc89_atlas_rect_xy(const TileCell89 *ctx, tc89_id atlas_id,
                       tc89_u32 cell_x, tc89_u32 cell_y, TC89_Rect *out_rect);
int tc89_atlas_rect_index(const TileCell89 *ctx, tc89_id atlas_id,
                          tc89_u32 index, TC89_Rect *out_rect);

tc89_id tc89_add_named_cell(TileCell89 *ctx, const char *name, tc89_id atlas_id,
                            tc89_u32 cell_x, tc89_u32 cell_y,
                            tc89_u32 flags, tc89_u32 user_tag);
tc89_id tc89_find_named_cell(const TileCell89 *ctx, const char *name);
int tc89_named_cell_rect(const TileCell89 *ctx, tc89_id cell_id, TC89_Rect *out_rect);

tc89_id tc89_add_map(TileCell89 *ctx, const char *name, tc89_u32 width, tc89_u32 height);
int tc89_map_set(TileCell89 *ctx, tc89_id map_id, tc89_u32 x, tc89_u32 y, tc89_id cell_id);
tc89_id tc89_map_get(const TileCell89 *ctx, tc89_id map_id, tc89_u32 x, tc89_u32 y);

int tc89_collect_strip(const TileCell89 *ctx, tc89_id atlas_id,
                       tc89_u32 start_x, tc89_u32 start_y,
                       int step_x, int step_y, tc89_u32 count,
                       TC89_Rect *out_rects, tc89_u32 out_cap);
const char *tc89_error_string(int code);

#ifdef __cplusplus
}
#endif

#endif
