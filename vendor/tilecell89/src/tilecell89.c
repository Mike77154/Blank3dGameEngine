#include "tilecell89.h"
#include <string.h>

#define TC89_ERR_NONE 0
#define TC89_ERR_ARGUMENT 1
#define TC89_ERR_CAPACITY 2
#define TC89_ERR_RANGE 3
#define TC89_ERR_LAYOUT 4

static void tc89_zero(void *ptr, unsigned int size)
{
    unsigned char *p;
    unsigned int i;
    if (!ptr) return;
    p = (unsigned char *)ptr;
    for (i = 0U; i < size; ++i) p[i] = 0U;
}

static int tc89_copy(char *dst, unsigned int cap, const char *src)
{
    unsigned int i;
    if (!dst || cap == 0U || !src) return 0;
    i = 0U;
    while (src[i] != '\0' && i + 1U < cap) {
        dst[i] = src[i];
        ++i;
    }
    if (src[i] != '\0') return 0;
    dst[i] = '\0';
    return 1;
}

void tc89_init(TileCell89 *ctx)
{
    tc89_u32 i;
    if (!ctx) return;
    tc89_zero(ctx, (unsigned int)sizeof(*ctx));
    for (i = 0U; i < TC89_MAX_MAP_TILES; ++i) ctx->map_tiles[i] = TC89_EMPTY_CELL;
}

tc89_id tc89_find_atlas(const TileCell89 *ctx, const char *name)
{
    tc89_id i;
    if (!ctx || !name) return TC89_INVALID_ID;
    for (i = 0U; i < ctx->atlas_count; ++i) {
        if (ctx->atlases[i].used && strcmp(ctx->atlases[i].name, name) == 0) return i;
    }
    return TC89_INVALID_ID;
}

tc89_id tc89_add_atlas(TileCell89 *ctx, const char *name, const char *image_request,
                       tc89_u32 image_width, tc89_u32 image_height,
                       tc89_u32 cell_width, tc89_u32 cell_height,
                       tc89_u32 margin_x, tc89_u32 margin_y,
                       tc89_u32 spacing_x, tc89_u32 spacing_y)
{
    tc89_id id;
    TC89_Atlas *a;
    tc89_u32 usable_w;
    tc89_u32 usable_h;
    if (!ctx || !name || !image_request || cell_width == 0U || cell_height == 0U) return TC89_INVALID_ID;
    id = tc89_find_atlas(ctx, name);
    if (id != TC89_INVALID_ID) return id;
    if (ctx->atlas_count >= TC89_MAX_ATLASES) {
        ctx->last_error = TC89_ERR_CAPACITY;
        return TC89_INVALID_ID;
    }
    if (image_width < margin_x * 2U + cell_width || image_height < margin_y * 2U + cell_height) {
        ctx->last_error = TC89_ERR_RANGE;
        return TC89_INVALID_ID;
    }
    id = ctx->atlas_count++;
    a = &ctx->atlases[id];
    tc89_zero(a, (unsigned int)sizeof(*a));
    if (!tc89_copy(a->name, TC89_NAME_CAP, name) ||
        !tc89_copy(a->image_request, TC89_REQUEST_CAP, image_request)) {
        --ctx->atlas_count;
        ctx->last_error = TC89_ERR_ARGUMENT;
        return TC89_INVALID_ID;
    }
    a->image_width = image_width;
    a->image_height = image_height;
    a->cell_width = cell_width;
    a->cell_height = cell_height;
    a->margin_x = margin_x;
    a->margin_y = margin_y;
    a->spacing_x = spacing_x;
    a->spacing_y = spacing_y;
    usable_w = image_width - margin_x * 2U;
    usable_h = image_height - margin_y * 2U;
    a->columns = (usable_w + spacing_x) / (cell_width + spacing_x);
    a->rows = (usable_h + spacing_y) / (cell_height + spacing_y);
    if (a->columns == 0U || a->rows == 0U) {
        --ctx->atlas_count;
        ctx->last_error = TC89_ERR_RANGE;
        return TC89_INVALID_ID;
    }
    a->used = 1U;
    return id;
}

int tc89_atlas_rect_xy(const TileCell89 *ctx, tc89_id atlas_id,
                       tc89_u32 cell_x, tc89_u32 cell_y, TC89_Rect *out_rect)
{
    const TC89_Atlas *a;
    tc89_u32 px;
    tc89_u32 py;
    if (!ctx || !out_rect || atlas_id >= ctx->atlas_count || !ctx->atlases[atlas_id].used) return 0;
    a = &ctx->atlases[atlas_id];
    if (cell_x >= a->columns || cell_y >= a->rows) return 0;
    px = a->margin_x + cell_x * (a->cell_width + a->spacing_x);
    py = a->margin_y + cell_y * (a->cell_height + a->spacing_y);
    if (px + a->cell_width > a->image_width || py + a->cell_height > a->image_height) return 0;
    out_rect->x = (tc89_s32)px;
    out_rect->y = (tc89_s32)py;
    out_rect->w = (tc89_s32)a->cell_width;
    out_rect->h = (tc89_s32)a->cell_height;
    return 1;
}

int tc89_atlas_rect_index(const TileCell89 *ctx, tc89_id atlas_id,
                          tc89_u32 index, TC89_Rect *out_rect)
{
    const TC89_Atlas *a;
    if (!ctx || atlas_id >= ctx->atlas_count) return 0;
    a = &ctx->atlases[atlas_id];
    if (!a->used || a->columns == 0U || index >= a->columns * a->rows) return 0;
    return tc89_atlas_rect_xy(ctx, atlas_id, index % a->columns, index / a->columns, out_rect);
}

tc89_id tc89_find_named_cell(const TileCell89 *ctx, const char *name)
{
    tc89_id i;
    if (!ctx || !name) return TC89_INVALID_ID;
    for (i = 0U; i < ctx->cell_count; ++i) {
        if (ctx->cells[i].used && strcmp(ctx->cells[i].name, name) == 0) return i;
    }
    return TC89_INVALID_ID;
}

tc89_id tc89_add_named_cell(TileCell89 *ctx, const char *name, tc89_id atlas_id,
                            tc89_u32 cell_x, tc89_u32 cell_y,
                            tc89_u32 flags, tc89_u32 user_tag)
{
    tc89_id id;
    TC89_Rect rect;
    TC89_Cell *c;
    if (!ctx || !name || !tc89_atlas_rect_xy(ctx, atlas_id, cell_x, cell_y, &rect)) return TC89_INVALID_ID;
    id = tc89_find_named_cell(ctx, name);
    if (id != TC89_INVALID_ID) return id;
    if (ctx->cell_count >= TC89_MAX_NAMED_CELLS) {
        ctx->last_error = TC89_ERR_CAPACITY;
        return TC89_INVALID_ID;
    }
    id = ctx->cell_count++;
    c = &ctx->cells[id];
    tc89_zero(c, (unsigned int)sizeof(*c));
    if (!tc89_copy(c->name, TC89_NAME_CAP, name)) {
        --ctx->cell_count;
        return TC89_INVALID_ID;
    }
    c->atlas_id = atlas_id;
    c->cell_x = cell_x;
    c->cell_y = cell_y;
    c->flags = flags;
    c->user_tag = user_tag;
    c->used = 1U;
    return id;
}

int tc89_named_cell_rect(const TileCell89 *ctx, tc89_id cell_id, TC89_Rect *out_rect)
{
    const TC89_Cell *c;
    if (!ctx || cell_id >= ctx->cell_count || !ctx->cells[cell_id].used) return 0;
    c = &ctx->cells[cell_id];
    return tc89_atlas_rect_xy(ctx, c->atlas_id, c->cell_x, c->cell_y, out_rect);
}

tc89_id tc89_add_map(TileCell89 *ctx, const char *name, tc89_u32 width, tc89_u32 height)
{
    tc89_u32 count;
    tc89_u32 i;
    tc89_id id;
    TC89_Map *m;
    if (!ctx || !name || width == 0U || height == 0U) return TC89_INVALID_ID;
    if (width > TC89_MAX_MAP_TILES / height) return TC89_INVALID_ID;
    count = width * height;
    if (ctx->map_count >= TC89_MAX_MAPS || ctx->map_tile_count + count > TC89_MAX_MAP_TILES) {
        ctx->last_error = TC89_ERR_CAPACITY;
        return TC89_INVALID_ID;
    }
    id = ctx->map_count++;
    m = &ctx->maps[id];
    tc89_zero(m, (unsigned int)sizeof(*m));
    if (!tc89_copy(m->name, TC89_NAME_CAP, name)) {
        --ctx->map_count;
        return TC89_INVALID_ID;
    }
    m->width = width;
    m->height = height;
    m->first_tile = ctx->map_tile_count;
    m->tile_count = count;
    m->used = 1U;
    for (i = 0U; i < count; ++i) ctx->map_tiles[ctx->map_tile_count + i] = TC89_EMPTY_CELL;
    ctx->map_tile_count += count;
    return id;
}

int tc89_map_set(TileCell89 *ctx, tc89_id map_id, tc89_u32 x, tc89_u32 y, tc89_id cell_id)
{
    TC89_Map *m;
    if (!ctx || map_id >= ctx->map_count || !ctx->maps[map_id].used) return 0;
    m = &ctx->maps[map_id];
    if (x >= m->width || y >= m->height) return 0;
    if (cell_id != TC89_EMPTY_CELL && (cell_id >= ctx->cell_count || !ctx->cells[cell_id].used)) return 0;
    ctx->map_tiles[m->first_tile + y * m->width + x] = cell_id;
    return 1;
}

tc89_id tc89_map_get(const TileCell89 *ctx, tc89_id map_id, tc89_u32 x, tc89_u32 y)
{
    const TC89_Map *m;
    if (!ctx || map_id >= ctx->map_count || !ctx->maps[map_id].used) return TC89_EMPTY_CELL;
    m = &ctx->maps[map_id];
    if (x >= m->width || y >= m->height) return TC89_EMPTY_CELL;
    return ctx->map_tiles[m->first_tile + y * m->width + x];
}

int tc89_collect_strip(const TileCell89 *ctx, tc89_id atlas_id,
                       tc89_u32 start_x, tc89_u32 start_y,
                       int step_x, int step_y, tc89_u32 count,
                       TC89_Rect *out_rects, tc89_u32 out_cap)
{
    tc89_u32 i;
    int x;
    int y;
    if (!ctx || !out_rects || count > out_cap) return 0;
    x = (int)start_x;
    y = (int)start_y;
    for (i = 0U; i < count; ++i) {
        if (x < 0 || y < 0 || !tc89_atlas_rect_xy(ctx, atlas_id, (tc89_u32)x, (tc89_u32)y, &out_rects[i])) return 0;
        x += step_x;
        y += step_y;
    }
    return 1;
}

const char *tc89_error_string(int code)
{
    if (code == TC89_ERR_NONE) return "ok";
    if (code == TC89_ERR_ARGUMENT) return "invalid argument";
    if (code == TC89_ERR_CAPACITY) return "capacity exhausted";
    if (code == TC89_ERR_RANGE) return "cell/atlas range error";
    if (code == TC89_ERR_LAYOUT) return "layout error";
    return "unknown error";
}
