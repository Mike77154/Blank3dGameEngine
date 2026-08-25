#include "w3d89.h"
#include <stdio.h>

static void w3d_zero(void *p, w3d_u32 n)
{
    w3d_u8 *b;
    w3d_u32 i;
    b = (w3d_u8*)p;
    for (i = 0UL; i < n; ++i) b[i] = 0U;
}

static w3d_u32 w3d_size_align(w3d_u32 size)
{
    return (size + 3UL) & ~3UL;
}

w3d_fp w3d_fp_from_int(w3d_i32 v)
{
    return (w3d_fp)(v * W3D_FP_ONE);
}

w3d_i32 w3d_fp_to_int_floor(w3d_fp v)
{
    if (v >= 0) return v / W3D_FP_ONE;
    return -(((-v) + W3D_FP_ONE - 1) / W3D_FP_ONE);
}

w3d_fp w3d_fp_abs(w3d_fp v)
{
    return (v < 0) ? -v : v;
}

static w3d_i32 w3d_floor_div(w3d_i32 a, w3d_i32 b)
{
    if (b <= 0) return 0;
    if (a >= 0) return a / b;
    return -(((-a) + b - 1) / b);
}

static w3d_fp w3d_max3_fp(w3d_fp a, w3d_fp b, w3d_fp c)
{
    w3d_fp m;
    m = a;
    if (b > m) m = b;
    if (c > m) m = c;
    return m;
}

w3d_v3 w3d_v3_make(w3d_fp x, w3d_fp y, w3d_fp z)
{
    w3d_v3 v;
    v.x = x;
    v.y = y;
    v.z = z;
    return v;
}

w3d_aabb w3d_aabb_make(w3d_v3 minv, w3d_v3 maxv)
{
    w3d_aabb b;
    b.minv = minv;
    b.maxv = maxv;
    return b;
}

w3d_aabb w3d_aabb_translate(w3d_aabb b, w3d_v3 p)
{
    w3d_aabb r;
    r.minv.x = b.minv.x + p.x;
    r.minv.y = b.minv.y + p.y;
    r.minv.z = b.minv.z + p.z;
    r.maxv.x = b.maxv.x + p.x;
    r.maxv.y = b.maxv.y + p.y;
    r.maxv.z = b.maxv.z + p.z;
    return r;
}

int w3d_aabb_overlap(w3d_aabb a, w3d_aabb b)
{
    if (a.maxv.x < b.minv.x) return 0;
    if (a.minv.x > b.maxv.x) return 0;
    if (a.maxv.y < b.minv.y) return 0;
    if (a.minv.y > b.maxv.y) return 0;
    if (a.maxv.z < b.minv.z) return 0;
    if (a.minv.z > b.maxv.z) return 0;
    return 1;
}

static w3d_u32 w3d_hash_coords(w3d_i16 cx, w3d_i16 cy, w3d_i16 cz)
{
    w3d_u32 x;
    w3d_u32 y;
    w3d_u32 z;
    x = (w3d_u32)((w3d_u16)cx);
    y = (w3d_u32)((w3d_u16)cy);
    z = (w3d_u32)((w3d_u16)cz);
    return (x * 73856093UL) ^ (y * 19349663UL) ^ (z * 83492791UL);
}

static void w3d_default_cell_desc(w3d_cell_desc *d)
{
    if (!d) return;
    w3d_zero(d, (w3d_u32)sizeof(*d));
    d->layer_mask = W3D_LAYER_ALL;
    d->phase_mask = W3D_PHASE_DEFAULT;
    d->difficulty_mask = W3D_DIFF_ALL;
    d->service_mask = W3D_SERVICE_ALL;
    d->proxy_distance = 0;
}

void w3d_world_default_config(w3d_world_config *cfg)
{
    if (!cfg) return;
    w3d_zero(cfg, (w3d_u32)sizeof(*cfg));
    cfg->max_cells = 128U;
    cfg->max_entities = 512U;
    cfg->max_stream_sources = 4U;
    cfg->max_events = 128U;
    cfg->max_cell_hash_entries = 257U;
    cfg->max_portals = 64U;
    cfg->cell_size = w3d_fp_from_int(64);
    cfg->origin = w3d_v3_make(0, 0, 0);
    cfg->auto_commit_streaming = 1U;
    cfg->active_layer_mask = W3D_LAYER_ALL;
    cfg->active_phase_mask = W3D_PHASE_ALL;
    cfg->active_difficulty_mask = W3D_DIFF_ALL;
    cfg->budget.max_load_requests_per_tick = 8U;
    cfg->budget.max_unload_requests_per_tick = 8U;
    cfg->budget.max_activate_requests_per_tick = 16U;
    cfg->budget.max_deactivate_requests_per_tick = 16U;
}

static void w3d_normalize_config(w3d_world_config *out_cfg, const w3d_world_config *in_cfg)
{
    w3d_world_default_config(out_cfg);
    if (!in_cfg) return;
    *out_cfg = *in_cfg;
    if (out_cfg->max_cell_hash_entries == 0U) {
        out_cfg->max_cell_hash_entries = (w3d_u16)(out_cfg->max_cells * 2U + 1U);
        if (out_cfg->max_cell_hash_entries < out_cfg->max_cells) out_cfg->max_cell_hash_entries = out_cfg->max_cells;
    }
    if (out_cfg->active_layer_mask == 0UL) out_cfg->active_layer_mask = W3D_LAYER_ALL;
    if (out_cfg->active_phase_mask == 0UL) out_cfg->active_phase_mask = W3D_PHASE_ALL;
    if (out_cfg->active_difficulty_mask == 0UL) out_cfg->active_difficulty_mask = W3D_DIFF_ALL;
}

w3d_u32 w3d_world_memory_required(const w3d_world_config *cfg)
{
    w3d_world_config c;
    w3d_u32 n;
    if (!cfg) return 0UL;
    w3d_normalize_config(&c, cfg);
    if (c.max_cells == 0U || c.max_entities == 0U || c.max_stream_sources == 0U || c.max_events == 0U) return 0UL;
    if (c.max_cell_hash_entries < c.max_cells || c.cell_size <= 0) return 0UL;
    n = 0UL;
    n += w3d_size_align((w3d_u32)c.max_cells * (w3d_u32)sizeof(w3d_cell));
    n += w3d_size_align((w3d_u32)c.max_cell_hash_entries * (w3d_u32)sizeof(w3d_cell_hash_entry));
    n += w3d_size_align((w3d_u32)c.max_entities * (w3d_u32)sizeof(w3d_entity));
    n += w3d_size_align((w3d_u32)c.max_stream_sources * (w3d_u32)sizeof(w3d_stream_source));
    n += w3d_size_align((w3d_u32)c.max_events * (w3d_u32)sizeof(w3d_event));
    n += w3d_size_align((w3d_u32)c.max_portals * (w3d_u32)sizeof(w3d_portal));
    n += 64UL;
    return n;
}

static w3d_handle w3d_make_handle(w3d_u16 slot, w3d_u16 generation)
{
    return ((w3d_u32)generation << 16) | (w3d_u32)slot;
}

static w3d_u16 w3d_handle_slot(w3d_handle h)
{
    return (w3d_u16)(h & 0xFFFFUL);
}

static w3d_u16 w3d_handle_generation(w3d_handle h)
{
    return (w3d_u16)((h >> 16) & 0xFFFFUL);
}

static void w3d_push_event(w3d_world *world, int type, int cell_index, w3d_handle entity, w3d_u32 service_mask)
{
    w3d_event *e;
    const w3d_cell *c;
    if (!world || !world->events || world->cfg.max_events == 0U) return;
    if (world->event_count >= world->cfg.max_events) {
        if (world->event_dropped_count < 65535U) world->event_dropped_count++;
        return;
    }
    e = &world->events[world->event_tail];
    e->type = (w3d_u16)type;
    e->cell_index = (cell_index >= 0) ? (w3d_u16)cell_index : W3D_U16_NONE;
    e->entity = entity;
    e->cx = 0;
    e->cy = 0;
    e->cz = 0;
    e->service_mask = service_mask;
    if (cell_index >= 0 && cell_index < (int)world->cell_count) {
        c = &world->cells[cell_index];
        e->cx = c->cx;
        e->cy = c->cy;
        e->cz = c->cz;
    }
    world->event_tail++;
    if (world->event_tail >= world->cfg.max_events) world->event_tail = 0U;
    world->event_count++;
}

int w3d_world_poll_event(w3d_world *world, w3d_event *out_event)
{
    if (!world || !out_event) return W3D_ERR_NULL;
    if (world->event_count == 0U) return W3D_ERR_NOT_FOUND;
    *out_event = world->events[world->event_head];
    world->event_head++;
    if (world->event_head >= world->cfg.max_events) world->event_head = 0U;
    world->event_count--;
    return W3D_OK;
}

static int w3d_hash_find_slot(const w3d_world *world, w3d_i16 cx, w3d_i16 cy, w3d_i16 cz, int *out_found)
{
    w3d_u32 cap;
    w3d_u32 h;
    w3d_u32 i;
    w3d_u32 pos;
    const w3d_cell_hash_entry *e;
    if (!world || !world->cell_hash || world->cfg.max_cell_hash_entries == 0U) return W3D_ERR_NULL;
    cap = (w3d_u32)world->cfg.max_cell_hash_entries;
    h = w3d_hash_coords(cx, cy, cz) % cap;
    for (i = 0UL; i < cap; ++i) {
        pos = (h + i) % cap;
        e = &world->cell_hash[pos];
        if (e->used == W3D_HASH_EMPTY) {
            if (out_found) *out_found = 0;
            return (int)pos;
        }
        if (e->cx == cx && e->cy == cy && e->cz == cz) {
            if (out_found) *out_found = 1;
            return (int)pos;
        }
    }
    return W3D_ERR_CAPACITY;
}

static int w3d_hash_insert_cell(w3d_world *world, w3d_i16 cx, w3d_i16 cy, w3d_i16 cz, w3d_u16 cell_index)
{
    int found;
    int slot;
    w3d_cell_hash_entry *e;
    slot = w3d_hash_find_slot(world, cx, cy, cz, &found);
    if (slot < 0) return slot;
    if (found) return W3D_ERR_DUPLICATE;
    e = &world->cell_hash[slot];
    e->cx = cx;
    e->cy = cy;
    e->cz = cz;
    e->cell_index = cell_index;
    e->used = W3D_HASH_FULL;
    world->hash_used++;
    return W3D_OK;
}

int w3d_world_init(w3d_world *world, const w3d_world_config *cfg, void *memory, w3d_u32 memory_size, const w3d_world_callbacks *callbacks, void *user)
{
    w3d_world_config c;
    w3d_u32 need;
    w3d_u16 i;
    if (!world || !cfg || !memory) return W3D_ERR_NULL;
    w3d_normalize_config(&c, cfg);
    if (c.max_cells == 0U || c.max_entities == 0U || c.max_stream_sources == 0U || c.max_events == 0U) return W3D_ERR_BAD_CONFIG;
    if (c.max_cell_hash_entries < c.max_cells || c.cell_size <= 0) return W3D_ERR_BAD_CONFIG;
    need = w3d_world_memory_required(&c);
    if (memory_size < need) return W3D_ERR_ARENA;
    w3d_zero(world, (w3d_u32)sizeof(*world));
    w3d_arena_init(&world->arena, memory, memory_size);
    world->cfg = c;
    if (callbacks) world->cb = *callbacks;
    world->user = user;
    world->cells = (w3d_cell*)w3d_arena_push(&world->arena, (w3d_u32)c.max_cells * (w3d_u32)sizeof(w3d_cell), 4UL);
    world->cell_hash = (w3d_cell_hash_entry*)w3d_arena_push(&world->arena, (w3d_u32)c.max_cell_hash_entries * (w3d_u32)sizeof(w3d_cell_hash_entry), 4UL);
    world->entities = (w3d_entity*)w3d_arena_push(&world->arena, (w3d_u32)c.max_entities * (w3d_u32)sizeof(w3d_entity), 4UL);
    world->sources = (w3d_stream_source*)w3d_arena_push(&world->arena, (w3d_u32)c.max_stream_sources * (w3d_u32)sizeof(w3d_stream_source), 4UL);
    world->events = (w3d_event*)w3d_arena_push(&world->arena, (w3d_u32)c.max_events * (w3d_u32)sizeof(w3d_event), 4UL);
    world->portals = (w3d_portal*)w3d_arena_push(&world->arena, (w3d_u32)c.max_portals * (w3d_u32)sizeof(w3d_portal), 4UL);
    if (!world->cells || !world->cell_hash || !world->entities || !world->sources || !world->events || (c.max_portals != 0U && !world->portals) || world->arena.failed) return W3D_ERR_ARENA;
    w3d_zero(world->cells, (w3d_u32)c.max_cells * (w3d_u32)sizeof(w3d_cell));
    w3d_zero(world->cell_hash, (w3d_u32)c.max_cell_hash_entries * (w3d_u32)sizeof(w3d_cell_hash_entry));
    w3d_zero(world->entities, (w3d_u32)c.max_entities * (w3d_u32)sizeof(w3d_entity));
    w3d_zero(world->sources, (w3d_u32)c.max_stream_sources * (w3d_u32)sizeof(w3d_stream_source));
    w3d_zero(world->events, (w3d_u32)c.max_events * (w3d_u32)sizeof(w3d_event));
    if (world->portals) w3d_zero(world->portals, (w3d_u32)c.max_portals * (w3d_u32)sizeof(w3d_portal));
    world->free_entity_head = 0U;
    for (i = 0U; i < c.max_entities; ++i) {
        world->entities[i].slot = i;
        world->entities[i].generation = 1U;
        world->entities[i].next_free = (w3d_u16)(i + 1U);
        world->entities[i].cell_index = W3D_U16_NONE;
        world->entities[i].prev_in_cell = W3D_U16_NONE;
        world->entities[i].next_in_cell = W3D_U16_NONE;
        world->entities[i].phase_mask = W3D_PHASE_DEFAULT;
        world->entities[i].difficulty_mask = W3D_DIFF_ALL;
    }
    world->entities[c.max_entities - 1U].next_free = W3D_U16_NONE;
    return W3D_OK;
}

void w3d_world_reset_runtime(w3d_world *world)
{
    w3d_u16 i;
    if (!world) return;
    world->cell_count = 0U;
    world->entity_alive_count = 0U;
    world->free_entity_head = 0U;
    world->event_head = 0U;
    world->event_tail = 0U;
    world->event_count = 0U;
    world->event_dropped_count = 0U;
    world->portal_count = 0U;
    world->hash_used = 0U;
    world->stream_cursor = 0U;
    world->load_used = 0U;
    world->unload_used = 0U;
    world->activate_used = 0U;
    world->deactivate_used = 0U;
    world->deferred_used = 0U;
    if (world->cells) w3d_zero(world->cells, (w3d_u32)world->cfg.max_cells * (w3d_u32)sizeof(w3d_cell));
    if (world->cell_hash) w3d_zero(world->cell_hash, (w3d_u32)world->cfg.max_cell_hash_entries * (w3d_u32)sizeof(w3d_cell_hash_entry));
    if (world->sources) w3d_zero(world->sources, (w3d_u32)world->cfg.max_stream_sources * (w3d_u32)sizeof(w3d_stream_source));
    if (world->events) w3d_zero(world->events, (w3d_u32)world->cfg.max_events * (w3d_u32)sizeof(w3d_event));
    if (world->portals) w3d_zero(world->portals, (w3d_u32)world->cfg.max_portals * (w3d_u32)sizeof(w3d_portal));
    if (world->entities) {
        w3d_zero(world->entities, (w3d_u32)world->cfg.max_entities * (w3d_u32)sizeof(w3d_entity));
        for (i = 0U; i < world->cfg.max_entities; ++i) {
            world->entities[i].slot = i;
            world->entities[i].generation = 1U;
            world->entities[i].next_free = (w3d_u16)(i + 1U);
            world->entities[i].cell_index = W3D_U16_NONE;
            world->entities[i].prev_in_cell = W3D_U16_NONE;
            world->entities[i].next_in_cell = W3D_U16_NONE;
            world->entities[i].phase_mask = W3D_PHASE_DEFAULT;
            world->entities[i].difficulty_mask = W3D_DIFF_ALL;
        }
        world->entities[world->cfg.max_entities - 1U].next_free = W3D_U16_NONE;
    }
}

void w3d_world_get_status(const w3d_world *world, w3d_world_status *out_status)
{
    w3d_u16 i;
    const w3d_cell *c;
    if (!out_status) return;
    w3d_zero(out_status, (w3d_u32)sizeof(*out_status));
    if (!world) return;
    out_status->cell_count = world->cell_count;
    out_status->entity_alive_count = world->entity_alive_count;
    out_status->portal_count = world->portal_count;
    out_status->event_count = world->event_count;
    out_status->event_dropped_count = world->event_dropped_count;
    out_status->hash_capacity = world->cfg.max_cell_hash_entries;
    out_status->hash_used = world->hash_used;
    out_status->stream_cursor = world->stream_cursor;
    out_status->load_requests_last_tick = world->load_used;
    out_status->unload_requests_last_tick = world->unload_used;
    out_status->activate_requests_last_tick = world->activate_used;
    out_status->deactivate_requests_last_tick = world->deactivate_used;
    out_status->deferred_requests_last_tick = world->deferred_used;
    out_status->last_query_touched_cells = world->last_query_touched_cells;
    out_status->last_query_tested_entities = world->last_query_tested_entities;
    out_status->arena_used = w3d_arena_used(&world->arena);
    out_status->arena_high_water = w3d_arena_high_water(&world->arena);
    for (i = 0U; i < world->cell_count; ++i) {
        c = &world->cells[i];
        if (c->state == W3D_CELL_ACTIVE) out_status->active_cell_count++;
        if (c->state == W3D_CELL_RESIDENT) out_status->resident_cell_count++;
        if (c->state == W3D_CELL_LOADING) out_status->loading_cell_count++;
        if (c->state == W3D_CELL_UNLOADING) out_status->unloading_cell_count++;
    }
}

void w3d_world_set_runtime_masks(w3d_world *world, w3d_u32 layer_mask, w3d_u32 phase_mask, w3d_u32 difficulty_mask)
{
    if (!world) return;
    world->cfg.active_layer_mask = layer_mask ? layer_mask : W3D_LAYER_ALL;
    world->cfg.active_phase_mask = phase_mask ? phase_mask : W3D_PHASE_ALL;
    world->cfg.active_difficulty_mask = difficulty_mask ? difficulty_mask : W3D_DIFF_ALL;
}

static w3d_aabb w3d_cell_bounds(const w3d_world *world, w3d_i16 cx, w3d_i16 cy, w3d_i16 cz)
{
    w3d_v3 minv;
    w3d_v3 maxv;
    w3d_fp cs;
    cs = world->cfg.cell_size;
    minv.x = world->cfg.origin.x + ((w3d_i32)cx * cs);
    minv.y = world->cfg.origin.y + ((w3d_i32)cy * cs);
    minv.z = world->cfg.origin.z + ((w3d_i32)cz * cs);
    maxv.x = minv.x + cs;
    maxv.y = minv.y + cs;
    maxv.z = minv.z + cs;
    return w3d_aabb_make(minv, maxv);
}

int w3d_world_find_cell(const w3d_world *world, w3d_i16 cx, w3d_i16 cy, w3d_i16 cz)
{
    int found;
    int slot;
    if (!world) return W3D_ERR_NULL;
    slot = w3d_hash_find_slot(world, cx, cy, cz, &found);
    if (slot < 0 || !found) return W3D_ERR_NOT_FOUND;
    return (int)world->cell_hash[slot].cell_index;
}

int w3d_world_find_cell_at_pos(const w3d_world *world, w3d_v3 pos)
{
    w3d_i32 rx;
    w3d_i32 ry;
    w3d_i32 rz;
    w3d_i16 cx;
    w3d_i16 cy;
    w3d_i16 cz;
    if (!world) return W3D_ERR_NULL;
    rx = pos.x - world->cfg.origin.x;
    ry = pos.y - world->cfg.origin.y;
    rz = pos.z - world->cfg.origin.z;
    cx = (w3d_i16)w3d_floor_div(rx, world->cfg.cell_size);
    cy = (w3d_i16)w3d_floor_div(ry, world->cfg.cell_size);
    cz = (w3d_i16)w3d_floor_div(rz, world->cfg.cell_size);
    return w3d_world_find_cell(world, cx, cy, cz);
}

int w3d_world_define_cell_ex(w3d_world *world, const w3d_cell_desc *desc)
{
    w3d_cell *c;
    w3d_cell_desc d;
    int old;
    int r;
    if (!world || !desc) return W3D_ERR_NULL;
    d = *desc;
    if (d.layer_mask == 0UL) d.layer_mask = W3D_LAYER_ALL;
    if (d.phase_mask == 0UL) d.phase_mask = W3D_PHASE_DEFAULT;
    if (d.difficulty_mask == 0UL) d.difficulty_mask = W3D_DIFF_ALL;
    if (d.service_mask == 0UL) d.service_mask = W3D_SERVICE_ALL;
    old = w3d_world_find_cell(world, d.cx, d.cy, d.cz);
    if (old >= 0) return W3D_ERR_DUPLICATE;
    if (world->cell_count >= world->cfg.max_cells) return W3D_ERR_CAPACITY;
    r = w3d_hash_insert_cell(world, d.cx, d.cy, d.cz, world->cell_count);
    if (r != W3D_OK) return r;
    c = &world->cells[world->cell_count];
    w3d_zero(c, (w3d_u32)sizeof(*c));
    c->cx = d.cx;
    c->cy = d.cy;
    c->cz = d.cz;
    c->generation = 1U;
    c->flags = d.flags;
    c->layer_mask = d.layer_mask;
    c->phase_mask = d.phase_mask;
    c->difficulty_mask = d.difficulty_mask;
    c->service_mask = d.service_mask;
    c->asset_ref = d.asset_ref;
    c->collision_ref = d.collision_ref;
    c->nav_ref = d.nav_ref;
    c->audio_ref = d.audio_ref;
    c->script_ref = d.script_ref;
    c->proxy_asset_ref = d.proxy_asset_ref;
    c->proxy_distance = d.proxy_distance;
    c->proxy_enabled = 0U;
    c->entity_head = W3D_U16_NONE;
    c->state = (d.flags & W3D_CELL_FLAG_ALWAYS_LOADED) ? W3D_CELL_RESIDENT : W3D_CELL_UNLOADED;
    c->desired_state = c->state;
    c->bounds = w3d_cell_bounds(world, d.cx, d.cy, d.cz);
    world->cell_count++;
    return (int)(world->cell_count - 1U);
}

int w3d_world_define_cell(w3d_world *world, w3d_i16 cx, w3d_i16 cy, w3d_i16 cz, w3d_u32 flags, w3d_u32 layer_mask, w3d_u32 asset_ref)
{
    w3d_cell_desc d;
    w3d_default_cell_desc(&d);
    d.cx = cx;
    d.cy = cy;
    d.cz = cz;
    d.flags = flags;
    d.layer_mask = layer_mask;
    d.asset_ref = asset_ref;
    return w3d_world_define_cell_ex(world, &d);
}

const w3d_cell *w3d_world_get_cell(const w3d_world *world, w3d_u16 cell_index)
{
    if (!world || cell_index >= world->cell_count) return 0;
    return &world->cells[cell_index];
}

int w3d_world_commit_cell_state(w3d_world *world, w3d_u16 cell_index, int state)
{
    if (!world) return W3D_ERR_NULL;
    if (cell_index >= world->cell_count) return W3D_ERR_NOT_FOUND;
    if (state < W3D_CELL_UNLOADED || state > W3D_CELL_UNLOADING) return W3D_ERR_BAD_CONFIG;
    world->cells[cell_index].state = (w3d_u16)state;
    return W3D_OK;
}

int w3d_world_set_cell_proxy(w3d_world *world, w3d_u16 cell_index, w3d_u32 proxy_asset_ref, w3d_fp proxy_distance)
{
    if (!world) return W3D_ERR_NULL;
    if (cell_index >= world->cell_count) return W3D_ERR_NOT_FOUND;
    world->cells[cell_index].proxy_asset_ref = proxy_asset_ref;
    world->cells[cell_index].proxy_distance = proxy_distance;
    if (proxy_asset_ref != 0UL) world->cells[cell_index].flags |= W3D_CELL_FLAG_HLOD_PROXY;
    else world->cells[cell_index].flags &= ~W3D_CELL_FLAG_HLOD_PROXY;
    return W3D_OK;
}

void w3d_world_clear_stream_sources(w3d_world *world)
{
    w3d_u16 i;
    if (!world || !world->sources) return;
    for (i = 0U; i < world->cfg.max_stream_sources; ++i) world->sources[i].enabled = 0U;
}

int w3d_world_set_stream_source_hysteresis(w3d_world *world, w3d_u16 index, w3d_v3 pos, w3d_fp active_enter, w3d_fp active_exit, w3d_fp resident_enter, w3d_fp resident_exit, w3d_u32 layer_mask, w3d_u32 phase_mask, w3d_u32 difficulty_mask, int enabled)
{
    w3d_stream_source *s;
    if (!world) return W3D_ERR_NULL;
    if (index >= world->cfg.max_stream_sources) return W3D_ERR_NOT_FOUND;
    if (active_enter < 0 || active_exit < 0 || resident_enter < 0 || resident_exit < 0) return W3D_ERR_BAD_CONFIG;
    if (active_exit < active_enter) active_exit = active_enter;
    if (resident_enter < active_enter) resident_enter = active_enter;
    if (resident_exit < resident_enter) resident_exit = resident_enter;
    s = &world->sources[index];
    s->pos = pos;
    s->active_enter_radius = active_enter;
    s->active_exit_radius = active_exit;
    s->resident_enter_radius = resident_enter;
    s->resident_exit_radius = resident_exit;
    s->layer_mask = layer_mask ? layer_mask : W3D_LAYER_ALL;
    s->phase_mask = phase_mask ? phase_mask : W3D_PHASE_ALL;
    s->difficulty_mask = difficulty_mask ? difficulty_mask : W3D_DIFF_ALL;
    s->enabled = enabled ? 1U : 0U;
    return W3D_OK;
}

int w3d_world_set_stream_source(w3d_world *world, w3d_u16 index, w3d_v3 pos, w3d_fp active_radius, w3d_fp resident_radius, w3d_u32 layer_mask, int enabled)
{
    w3d_fp pad;
    if (!world) return W3D_ERR_NULL;
    pad = world->cfg.cell_size / 2;
    return w3d_world_set_stream_source_hysteresis(world, index, pos, active_radius, active_radius + pad, resident_radius, resident_radius + pad, layer_mask, W3D_PHASE_ALL, W3D_DIFF_ALL, enabled);
}

static w3d_v3 w3d_cell_center(const w3d_cell *c)
{
    w3d_v3 p;
    p.x = c->bounds.minv.x + ((c->bounds.maxv.x - c->bounds.minv.x) / 2);
    p.y = c->bounds.minv.y + ((c->bounds.maxv.y - c->bounds.minv.y) / 2);
    p.z = c->bounds.minv.z + ((c->bounds.maxv.z - c->bounds.minv.z) / 2);
    return p;
}

static int w3d_masks_match(w3d_u32 a, w3d_u32 b)
{
    return ((a & b) != 0UL) ? 1 : 0;
}

static int w3d_cell_allowed_by_world(const w3d_world *world, const w3d_cell *c)
{
    if (!w3d_masks_match(c->layer_mask, world->cfg.active_layer_mask)) return 0;
    if (!w3d_masks_match(c->phase_mask, world->cfg.active_phase_mask)) return 0;
    if (!w3d_masks_match(c->difficulty_mask, world->cfg.active_difficulty_mask)) return 0;
    return 1;
}

static int w3d_cell_desired_for_sources(const w3d_world *world, const w3d_cell *c, w3d_fp *out_best_dist)
{
    w3d_u16 i;
    w3d_v3 center;
    int best;
    w3d_fp best_dist;
    if (!w3d_cell_allowed_by_world(world, c)) return W3D_CELL_UNLOADED;
    best = (c->flags & W3D_CELL_FLAG_ALWAYS_LOADED) ? W3D_CELL_RESIDENT : W3D_CELL_UNLOADED;
    best_dist = 2147483647L;
    center = w3d_cell_center(c);
    for (i = 0U; i < world->cfg.max_stream_sources; ++i) {
        const w3d_stream_source *s;
        w3d_fp dx;
        w3d_fp dy;
        w3d_fp dz;
        w3d_fp dist;
        w3d_fp active_limit;
        w3d_fp resident_limit;
        if (!world->sources[i].enabled) continue;
        s = &world->sources[i];
        if (!w3d_masks_match(s->layer_mask, c->layer_mask)) continue;
        if (!w3d_masks_match(s->phase_mask, c->phase_mask)) continue;
        if (!w3d_masks_match(s->difficulty_mask, c->difficulty_mask)) continue;
        dx = w3d_fp_abs(center.x - s->pos.x);
        dy = w3d_fp_abs(center.y - s->pos.y);
        dz = w3d_fp_abs(center.z - s->pos.z);
        dist = w3d_max3_fp(dx, dy, dz);
        if (dist < best_dist) best_dist = dist;
        active_limit = (c->state == W3D_CELL_ACTIVE) ? s->active_exit_radius : s->active_enter_radius;
        resident_limit = (c->state == W3D_CELL_RESIDENT || c->state == W3D_CELL_ACTIVE || c->state == W3D_CELL_LOADING) ? s->resident_exit_radius : s->resident_enter_radius;
        if (dist <= active_limit) {
            best = W3D_CELL_ACTIVE;
        } else if (best < W3D_CELL_RESIDENT && dist <= resident_limit) {
            best = W3D_CELL_RESIDENT;
        }
    }
    if (out_best_dist) *out_best_dist = best_dist;
    return best;
}

static void w3d_dispatch_cell_action(w3d_world *world, const w3d_cell *c, int action)
{
    if (!world || !c) return;
    if (world->cb.cell_event) world->cb.cell_event(world, c, action, world->user);
    if ((c->service_mask & W3D_SERVICE_ASSET) && world->cb.asset_event) world->cb.asset_event(world, c, action, world->user);
    if ((c->service_mask & W3D_SERVICE_RENDER) && world->cb.render_event) world->cb.render_event(world, c, action, world->user);
    if ((c->service_mask & W3D_SERVICE_COLLISION) && world->cb.collision_event) world->cb.collision_event(world, c, action, world->user);
    if ((c->service_mask & W3D_SERVICE_PHYSICS) && world->cb.physics_event) world->cb.physics_event(world, c, action, world->user);
    if ((c->service_mask & W3D_SERVICE_NAV) && world->cb.nav_event) world->cb.nav_event(world, c, action, world->user);
    if ((c->service_mask & W3D_SERVICE_AUDIO) && world->cb.audio_event) world->cb.audio_event(world, c, action, world->user);
    if ((c->service_mask & W3D_SERVICE_SCRIPT) && world->cb.script_event) world->cb.script_event(world, c, action, world->user);
}

static int w3d_budget_allow(w3d_world *world, int action)
{
    if (action == W3D_BRIDGE_CELL_LOAD_REQUEST) {
        if (world->load_used >= world->cfg.budget.max_load_requests_per_tick) return 0;
        world->load_used++;
        return 1;
    }
    if (action == W3D_BRIDGE_CELL_UNLOAD_REQUEST) {
        if (world->unload_used >= world->cfg.budget.max_unload_requests_per_tick) return 0;
        world->unload_used++;
        return 1;
    }
    if (action == W3D_BRIDGE_CELL_ACTIVATE_REQUEST) {
        if (world->activate_used >= world->cfg.budget.max_activate_requests_per_tick) return 0;
        world->activate_used++;
        return 1;
    }
    if (action == W3D_BRIDGE_CELL_DEACTIVATE_REQUEST) {
        if (world->deactivate_used >= world->cfg.budget.max_deactivate_requests_per_tick) return 0;
        world->deactivate_used++;
        return 1;
    }
    return 1;
}

static int w3d_request_cell_state(w3d_world *world, w3d_u16 index, int desired)
{
    w3d_cell *c;
    int action;
    int event_type;
    if (!world || index >= world->cell_count) return W3D_ERR_NOT_FOUND;
    c = &world->cells[index];
    c->desired_state = (w3d_u16)desired;
    if ((int)c->state == desired) return W3D_OK;
    if (c->state == W3D_CELL_LOADING || c->state == W3D_CELL_UNLOADING) return W3D_OK;
    action = 0;
    event_type = W3D_EVENT_NONE;
    if (desired == W3D_CELL_ACTIVE) {
        if (c->state == W3D_CELL_UNLOADED) {
            action = W3D_BRIDGE_CELL_LOAD_REQUEST;
            event_type = W3D_EVENT_CELL_LOAD;
        } else if (c->state == W3D_CELL_RESIDENT) {
            action = W3D_BRIDGE_CELL_ACTIVATE_REQUEST;
            event_type = W3D_EVENT_CELL_ACTIVATE;
        }
    } else if (desired == W3D_CELL_RESIDENT) {
        if (c->state == W3D_CELL_UNLOADED) {
            action = W3D_BRIDGE_CELL_LOAD_REQUEST;
            event_type = W3D_EVENT_CELL_LOAD;
        } else if (c->state == W3D_CELL_ACTIVE) {
            action = W3D_BRIDGE_CELL_DEACTIVATE_REQUEST;
            event_type = W3D_EVENT_CELL_DEACTIVATE;
        }
    } else {
        if (c->state == W3D_CELL_ACTIVE) {
            action = W3D_BRIDGE_CELL_DEACTIVATE_REQUEST;
            event_type = W3D_EVENT_CELL_DEACTIVATE;
        } else if (c->state == W3D_CELL_RESIDENT) {
            action = W3D_BRIDGE_CELL_UNLOAD_REQUEST;
            event_type = W3D_EVENT_CELL_UNLOAD;
        }
    }
    if (action == 0) return W3D_OK;
    if (!w3d_budget_allow(world, action)) {
        if (world->deferred_used < 65535U) world->deferred_used++;
        return W3D_ERR_CAPACITY;
    }
    w3d_dispatch_cell_action(world, c, action);
    if (event_type != W3D_EVENT_NONE) w3d_push_event(world, event_type, index, W3D_INVALID_HANDLE, c->service_mask);
    if (world->cfg.auto_commit_streaming) {
        if (action == W3D_BRIDGE_CELL_LOAD_REQUEST) {
            if (desired == W3D_CELL_ACTIVE) c->state = W3D_CELL_ACTIVE;
            else c->state = W3D_CELL_RESIDENT;
        } else if (action == W3D_BRIDGE_CELL_UNLOAD_REQUEST) {
            c->state = W3D_CELL_UNLOADED;
        } else if (action == W3D_BRIDGE_CELL_ACTIVATE_REQUEST) {
            c->state = W3D_CELL_ACTIVE;
        } else if (action == W3D_BRIDGE_CELL_DEACTIVATE_REQUEST) {
            c->state = (desired == W3D_CELL_UNLOADED) ? W3D_CELL_RESIDENT : W3D_CELL_RESIDENT;
        }
    } else {
        if (action == W3D_BRIDGE_CELL_LOAD_REQUEST) c->state = W3D_CELL_LOADING;
        else if (action == W3D_BRIDGE_CELL_UNLOAD_REQUEST) c->state = W3D_CELL_UNLOADING;
    }
    return W3D_OK;
}

static void w3d_update_cell_proxy(w3d_world *world, w3d_u16 index, int desired, w3d_fp best_dist)
{
    w3d_cell *c;
    int should_proxy;
    if (!world || index >= world->cell_count) return;
    c = &world->cells[index];
    if (c->proxy_asset_ref == 0UL) return;
    should_proxy = 0;
    if (desired != W3D_CELL_ACTIVE) {
        if (c->proxy_distance <= 0 || best_dist >= c->proxy_distance) should_proxy = 1;
    }
    if (should_proxy && !c->proxy_enabled) {
        c->proxy_enabled = 1U;
        w3d_dispatch_cell_action(world, c, W3D_BRIDGE_PROXY_ENABLE_REQUEST);
        w3d_push_event(world, W3D_EVENT_PROXY_ENABLE, index, W3D_INVALID_HANDLE, c->service_mask);
    } else if (!should_proxy && c->proxy_enabled) {
        c->proxy_enabled = 0U;
        w3d_dispatch_cell_action(world, c, W3D_BRIDGE_PROXY_DISABLE_REQUEST);
        w3d_push_event(world, W3D_EVENT_PROXY_DISABLE, index, W3D_INVALID_HANDLE, c->service_mask);
    }
}

int w3d_world_update_streaming(w3d_world *world)
{
    w3d_u16 n;
    w3d_u16 k;
    w3d_u16 idx;
    int desired;
    w3d_fp dist;
    int pass;
    if (!world) return W3D_ERR_NULL;
    world->load_used = 0U;
    world->unload_used = 0U;
    world->activate_used = 0U;
    world->deactivate_used = 0U;
    world->deferred_used = 0U;
    if (world->cell_count == 0U) return W3D_OK;
    n = world->cell_count;
    if (world->stream_cursor >= n) world->stream_cursor = 0U;

    /* Three priority passes: ACTIVE first, RESIDENT second, UNLOADED last.
       This is a no-heap substitute for distance/prioritized streaming queues. */
    for (pass = 0; pass < 3; ++pass) {
        for (k = 0U; k < n; ++k) {
            idx = (w3d_u16)((world->stream_cursor + k) % n);
            dist = 0;
            desired = w3d_cell_desired_for_sources(world, &world->cells[idx], &dist);
            if (pass == 0) w3d_update_cell_proxy(world, idx, desired, dist);
            if (pass == 0 && desired != W3D_CELL_ACTIVE) continue;
            if (pass == 1 && desired != W3D_CELL_RESIDENT) continue;
            if (pass == 2 && desired != W3D_CELL_UNLOADED) continue;
            w3d_request_cell_state(world, idx, desired);
        }
    }
    world->stream_cursor++;
    if (world->stream_cursor >= n) world->stream_cursor = 0U;
    return W3D_OK;
}

w3d_entity *w3d_entity_resolve(w3d_world *world, w3d_handle h)
{
    w3d_u16 slot;
    w3d_u16 gen;
    w3d_entity *e;
    if (!world || h == W3D_INVALID_HANDLE) return 0;
    slot = w3d_handle_slot(h);
    gen = w3d_handle_generation(h);
    if (slot >= world->cfg.max_entities) return 0;
    e = &world->entities[slot];
    if (!(e->flags & W3D_ENTITY_ALIVE)) return 0;
    if (e->generation != gen) return 0;
    return e;
}

const w3d_entity *w3d_entity_resolve_const(const w3d_world *world, w3d_handle h)
{
    w3d_u16 slot;
    w3d_u16 gen;
    const w3d_entity *e;
    if (!world || h == W3D_INVALID_HANDLE) return 0;
    slot = w3d_handle_slot(h);
    gen = w3d_handle_generation(h);
    if (slot >= world->cfg.max_entities) return 0;
    e = &world->entities[slot];
    if (!(e->flags & W3D_ENTITY_ALIVE)) return 0;
    if (e->generation != gen) return 0;
    return e;
}

static void w3d_cell_detach_entity(w3d_world *world, w3d_entity *e)
{
    w3d_cell *c;
    w3d_handle h;
    w3d_u16 old_cell;
    if (!world || !e) return;
    if (e->cell_index == W3D_U16_NONE || e->cell_index >= world->cell_count) return;
    old_cell = e->cell_index;
    c = &world->cells[old_cell];
    if (e->prev_in_cell != W3D_U16_NONE) world->entities[e->prev_in_cell].next_in_cell = e->next_in_cell;
    else c->entity_head = e->next_in_cell;
    if (e->next_in_cell != W3D_U16_NONE) world->entities[e->next_in_cell].prev_in_cell = e->prev_in_cell;
    if (c->entity_count > 0U) c->entity_count--;
    h = w3d_make_handle(e->slot, e->generation);
    if (world->cb.entity_event) world->cb.entity_event(world, e, W3D_BRIDGE_ENTITY_LEAVE_CELL, world->user);
    w3d_push_event(world, W3D_EVENT_ENTITY_LEAVE_CELL, old_cell, h, 0UL);
    e->cell_index = W3D_U16_NONE;
    e->prev_in_cell = W3D_U16_NONE;
    e->next_in_cell = W3D_U16_NONE;
}

static void w3d_cell_attach_entity(w3d_world *world, w3d_entity *e, w3d_u16 cell_index)
{
    w3d_cell *c;
    w3d_handle h;
    if (!world || !e || cell_index >= world->cell_count) return;
    c = &world->cells[cell_index];
    e->cell_index = cell_index;
    e->prev_in_cell = W3D_U16_NONE;
    e->next_in_cell = c->entity_head;
    if (c->entity_head != W3D_U16_NONE) world->entities[c->entity_head].prev_in_cell = e->slot;
    c->entity_head = e->slot;
    c->entity_count++;
    h = w3d_make_handle(e->slot, e->generation);
    if (world->cb.entity_event) world->cb.entity_event(world, e, W3D_BRIDGE_ENTITY_ENTER_CELL, world->user);
    w3d_push_event(world, W3D_EVENT_ENTITY_ENTER_CELL, cell_index, h, 0UL);
}

static void w3d_entity_recell(w3d_world *world, w3d_entity *e)
{
    int cell_idx;
    if (!world || !e) return;
    cell_idx = w3d_world_find_cell_at_pos(world, e->xf.pos);
    if (cell_idx < 0) {
        w3d_cell_detach_entity(world, e);
        return;
    }
    if (e->cell_index == (w3d_u16)cell_idx) return;
    w3d_cell_detach_entity(world, e);
    w3d_cell_attach_entity(world, e, (w3d_u16)cell_idx);
}

w3d_handle w3d_entity_spawn_ex(w3d_world *world, w3d_v3 pos, w3d_aabb local_bounds, w3d_u32 flags, w3d_u32 layer_mask, w3d_u32 phase_mask, w3d_u32 difficulty_mask, w3d_u32 group_mask, w3d_u32 kind, w3d_u32 user_index)
{
    w3d_u16 slot;
    w3d_u16 old_generation;
    w3d_u16 next_free;
    w3d_entity *e;
    w3d_handle h;
    if (!world) return W3D_INVALID_HANDLE;
    if (world->free_entity_head == W3D_U16_NONE) return W3D_INVALID_HANDLE;
    slot = world->free_entity_head;
    e = &world->entities[slot];
    old_generation = e->generation;
    next_free = e->next_free;
    world->free_entity_head = next_free;
    w3d_zero(e, (w3d_u32)sizeof(*e));
    e->slot = slot;
    e->generation = (w3d_u16)(old_generation + 1U);
    if (e->generation == 0U) e->generation = 1U;
    e->next_free = W3D_U16_NONE;
    e->cell_index = W3D_U16_NONE;
    e->prev_in_cell = W3D_U16_NONE;
    e->next_in_cell = W3D_U16_NONE;
    e->flags = flags | W3D_ENTITY_ALIVE;
    e->layer_mask = layer_mask ? layer_mask : W3D_LAYER_ALL;
    e->phase_mask = phase_mask ? phase_mask : W3D_PHASE_DEFAULT;
    e->difficulty_mask = difficulty_mask ? difficulty_mask : W3D_DIFF_ALL;
    e->group_mask = group_mask;
    e->kind = kind;
    e->user_index = user_index;
    e->xf.pos = pos;
    e->xf.rot = w3d_v3_make(0, 0, 0);
    e->xf.scale = w3d_v3_make(W3D_FP_ONE, W3D_FP_ONE, W3D_FP_ONE);
    e->local_bounds = local_bounds;
    e->world_bounds = w3d_aabb_translate(local_bounds, pos);
    world->entity_alive_count++;
    w3d_entity_recell(world, e);
    h = w3d_make_handle(slot, e->generation);
    return h;
}

w3d_handle w3d_entity_spawn(w3d_world *world, w3d_v3 pos, w3d_aabb local_bounds, w3d_u32 flags, w3d_u32 layer_mask, w3d_u32 group_mask, w3d_u32 kind, w3d_u32 user_index)
{
    return w3d_entity_spawn_ex(world, pos, local_bounds, flags, layer_mask, W3D_PHASE_DEFAULT, W3D_DIFF_ALL, group_mask, kind, user_index);
}

int w3d_entity_remove(w3d_world *world, w3d_handle h)
{
    w3d_entity *e;
    if (!world) return W3D_ERR_NULL;
    e = w3d_entity_resolve(world, h);
    if (!e) return W3D_ERR_BAD_HANDLE;
    w3d_cell_detach_entity(world, e);
    e->flags = 0UL;
    e->generation++;
    if (e->generation == 0U) e->generation = 1U;
    e->next_free = world->free_entity_head;
    world->free_entity_head = e->slot;
    if (world->entity_alive_count > 0U) world->entity_alive_count--;
    return W3D_OK;
}

int w3d_entity_set_pose(w3d_world *world, w3d_handle h, w3d_v3 pos, w3d_v3 rot)
{
    w3d_entity *e;
    if (!world) return W3D_ERR_NULL;
    e = w3d_entity_resolve(world, h);
    if (!e) return W3D_ERR_BAD_HANDLE;
    e->xf.pos = pos;
    e->xf.rot = rot;
    e->world_bounds = w3d_aabb_translate(e->local_bounds, pos);
    w3d_entity_recell(world, e);
    if (world->cb.entity_event) world->cb.entity_event(world, e, W3D_BRIDGE_ENTITY_POSE_CHANGED, world->user);
    w3d_push_event(world, W3D_EVENT_ENTITY_MOVED, e->cell_index, h, 0UL);
    return W3D_OK;
}

int w3d_entity_set_flags(w3d_world *world, w3d_handle h, w3d_u32 flags)
{
    w3d_entity *e;
    if (!world) return W3D_ERR_NULL;
    e = w3d_entity_resolve(world, h);
    if (!e) return W3D_ERR_BAD_HANDLE;
    e->flags = flags | W3D_ENTITY_ALIVE;
    return W3D_OK;
}

static void w3d_coord_range_for_aabb(const w3d_world *world, w3d_aabb area, w3d_i16 *minx, w3d_i16 *miny, w3d_i16 *minz, w3d_i16 *maxx, w3d_i16 *maxy, w3d_i16 *maxz)
{
    w3d_i32 rx0;
    w3d_i32 ry0;
    w3d_i32 rz0;
    w3d_i32 rx1;
    w3d_i32 ry1;
    w3d_i32 rz1;
    rx0 = area.minv.x - world->cfg.origin.x;
    ry0 = area.minv.y - world->cfg.origin.y;
    rz0 = area.minv.z - world->cfg.origin.z;
    rx1 = area.maxv.x - world->cfg.origin.x;
    ry1 = area.maxv.y - world->cfg.origin.y;
    rz1 = area.maxv.z - world->cfg.origin.z;
    *minx = (w3d_i16)w3d_floor_div(rx0, world->cfg.cell_size);
    *miny = (w3d_i16)w3d_floor_div(ry0, world->cfg.cell_size);
    *minz = (w3d_i16)w3d_floor_div(rz0, world->cfg.cell_size);
    *maxx = (w3d_i16)w3d_floor_div(rx1, world->cfg.cell_size);
    *maxy = (w3d_i16)w3d_floor_div(ry1, world->cfg.cell_size);
    *maxz = (w3d_i16)w3d_floor_div(rz1, world->cfg.cell_size);
    *minx = (w3d_i16)(*minx - 1);
    *miny = (w3d_i16)(*miny - 1);
    *minz = (w3d_i16)(*minz - 1);
    *maxx = (w3d_i16)(*maxx + 1);
    *maxy = (w3d_i16)(*maxy + 1);
    *maxz = (w3d_i16)(*maxz + 1);
}

int w3d_query_aabb_ex(const w3d_world *world, w3d_aabb area, w3d_u32 layer_mask, w3d_u32 phase_mask, w3d_u32 difficulty_mask, w3d_u32 required_flags, w3d_u32 group_mask, w3d_handle *out_handles, w3d_u16 max_out)
{
    w3d_i16 minx;
    w3d_i16 miny;
    w3d_i16 minz;
    w3d_i16 maxx;
    w3d_i16 maxy;
    w3d_i16 maxz;
    w3d_i32 x;
    w3d_i32 y;
    w3d_i32 z;
    w3d_u16 n;
    w3d_u16 touched;
    w3d_u16 tested;
    if (!world || !out_handles) return W3D_ERR_NULL;
    n = 0U;
    touched = 0U;
    tested = 0U;
    if (layer_mask == 0UL) layer_mask = W3D_LAYER_ALL;
    if (phase_mask == 0UL) phase_mask = W3D_PHASE_ALL;
    if (difficulty_mask == 0UL) difficulty_mask = W3D_DIFF_ALL;
    w3d_coord_range_for_aabb(world, area, &minx, &miny, &minz, &maxx, &maxy, &maxz);
    for (x = minx; x <= maxx; ++x) {
        for (y = miny; y <= maxy; ++y) {
            for (z = minz; z <= maxz; ++z) {
                int cell_index;
                w3d_u16 slot;
                const w3d_cell *c;
                const w3d_entity *e;
                cell_index = w3d_world_find_cell(world, (w3d_i16)x, (w3d_i16)y, (w3d_i16)z);
                if (cell_index < 0) continue;
                touched++;
                c = &world->cells[cell_index];
                slot = c->entity_head;
                while (slot != W3D_U16_NONE && slot < world->cfg.max_entities) {
                    e = &world->entities[slot];
                    tested++;
                    if (e->flags & W3D_ENTITY_ALIVE) {
                        if ((e->flags & W3D_ENTITY_QUERYABLE) && ((e->flags & required_flags) == required_flags) && w3d_masks_match(e->layer_mask, layer_mask) && w3d_masks_match(e->phase_mask, phase_mask) && w3d_masks_match(e->difficulty_mask, difficulty_mask) && (group_mask == 0UL || w3d_masks_match(e->group_mask, group_mask)) && w3d_aabb_overlap(e->world_bounds, area)) {
                            if (n < max_out) out_handles[n] = w3d_make_handle(e->slot, e->generation);
                            if (n < 65535U) n++;
                        }
                    }
                    slot = e->next_in_cell;
                }
            }
        }
    }
    ((w3d_world*)world)->last_query_touched_cells = touched;
    ((w3d_world*)world)->last_query_tested_entities = tested;
    return (int)((n > max_out) ? max_out : n);
}

int w3d_query_aabb(const w3d_world *world, w3d_aabb area, w3d_u32 layer_mask, w3d_handle *out_handles, w3d_u16 max_out)
{
    return w3d_query_aabb_ex(world, area, layer_mask, W3D_PHASE_ALL, W3D_DIFF_ALL, W3D_ENTITY_QUERYABLE, 0UL, out_handles, max_out);
}

int w3d_query_cell_entities(const w3d_world *world, w3d_u16 cell_index, w3d_handle *out_handles, w3d_u16 max_out)
{
    w3d_u16 n;
    w3d_u16 slot;
    const w3d_cell *c;
    const w3d_entity *e;
    if (!world || !out_handles) return W3D_ERR_NULL;
    if (cell_index >= world->cell_count) return W3D_ERR_NOT_FOUND;
    c = &world->cells[cell_index];
    n = 0U;
    slot = c->entity_head;
    while (slot != W3D_U16_NONE && slot < world->cfg.max_entities) {
        e = &world->entities[slot];
        if (e->flags & W3D_ENTITY_ALIVE) {
            if (n >= max_out) break;
            out_handles[n++] = w3d_make_handle(e->slot, e->generation);
        }
        slot = e->next_in_cell;
    }
    return (int)n;
}

int w3d_world_define_portal(w3d_world *world, w3d_u16 from_cell, w3d_u16 to_cell, w3d_aabb bounds, w3d_u32 flags, w3d_u32 layer_mask, w3d_u32 phase_mask, w3d_u32 user_index)
{
    w3d_portal *p;
    if (!world) return W3D_ERR_NULL;
    if (from_cell >= world->cell_count || to_cell >= world->cell_count) return W3D_ERR_NOT_FOUND;
    if (world->portal_count >= world->cfg.max_portals) return W3D_ERR_CAPACITY;
    p = &world->portals[world->portal_count];
    w3d_zero(p, (w3d_u32)sizeof(*p));
    p->from_cell = from_cell;
    p->to_cell = to_cell;
    p->bounds = bounds;
    p->flags = flags;
    p->layer_mask = layer_mask ? layer_mask : W3D_LAYER_ALL;
    p->phase_mask = phase_mask ? phase_mask : W3D_PHASE_ALL;
    p->user_index = user_index;
    world->portal_count++;
    return (int)(world->portal_count - 1U);
}

const w3d_portal *w3d_world_get_portal(const w3d_world *world, w3d_u16 portal_index)
{
    if (!world || portal_index >= world->portal_count) return 0;
    return &world->portals[portal_index];
}

int w3d_world_set_portal_open(w3d_world *world, w3d_u16 portal_index, int open)
{
    w3d_portal *p;
    w3d_u32 old_flags;
    if (!world) return W3D_ERR_NULL;
    if (portal_index >= world->portal_count) return W3D_ERR_NOT_FOUND;
    p = &world->portals[portal_index];
    old_flags = p->flags;
    if (open) p->flags |= W3D_PORTAL_OPEN;
    else p->flags &= ~W3D_PORTAL_OPEN;
    if (old_flags != p->flags) {
        if (world->cb.portal_event) world->cb.portal_event(world, p, W3D_BRIDGE_PORTAL_OPEN_CHANGED, world->user);
        w3d_push_event(world, W3D_EVENT_PORTAL_OPEN_CHANGED, p->from_cell, W3D_INVALID_HANDLE, 0UL);
    }
    return W3D_OK;
}

static int w3d_cell_in_list(const w3d_u16 *list, w3d_u16 count, w3d_u16 value)
{
    w3d_u16 i;
    for (i = 0U; i < count; ++i) if (list[i] == value) return 1;
    return 0;
}

int w3d_world_query_visible_cells_from(w3d_world *world, w3d_u16 start_cell, w3d_u16 max_depth, w3d_u32 layer_mask, w3d_u16 *out_cells, w3d_u16 max_out)
{
    w3d_u16 read_at;
    w3d_u16 count;
    w3d_u16 depth;
    if (!world || !out_cells) return W3D_ERR_NULL;
    if (start_cell >= world->cell_count) return W3D_ERR_NOT_FOUND;
    if (max_out == 0U) return 0;
    if (layer_mask == 0UL) layer_mask = W3D_LAYER_ALL;
    out_cells[0] = start_cell;
    count = 1U;
    read_at = 0U;
    depth = 0U;
    while (read_at < count && depth <= max_depth) {
        w3d_u16 level_end;
        level_end = count;
        while (read_at < level_end) {
            w3d_u16 current;
            w3d_u16 i;
            current = out_cells[read_at++];
            for (i = 0U; i < world->portal_count; ++i) {
                w3d_portal *p;
                w3d_u16 next;
                p = &world->portals[i];
                if (!(p->flags & W3D_PORTAL_OPEN)) continue;
                if (!w3d_masks_match(p->layer_mask, layer_mask)) continue;
                if (!w3d_masks_match(p->phase_mask, world->cfg.active_phase_mask)) continue;
                next = W3D_U16_NONE;
                if (p->from_cell == current) next = p->to_cell;
                else if (!(p->flags & W3D_PORTAL_ONE_WAY) && p->to_cell == current) next = p->from_cell;
                if (next == W3D_U16_NONE) continue;
                if (w3d_cell_in_list(out_cells, count, next)) continue;
                if (count >= max_out) return (int)count;
                out_cells[count++] = next;
            }
        }
        depth++;
    }
    return (int)count;
}

int w3d_debug_emit_cells(const w3d_world *world, w3d_debug_cell *out_cells, w3d_u16 max_out)
{
    w3d_u16 i;
    w3d_u16 n;
    const w3d_cell *c;
    if (!world || !out_cells) return W3D_ERR_NULL;
    n = 0U;
    for (i = 0U; i < world->cell_count && n < max_out; ++i) {
        c = &world->cells[i];
        out_cells[n].cell_index = i;
        out_cells[n].cx = c->cx;
        out_cells[n].cy = c->cy;
        out_cells[n].cz = c->cz;
        out_cells[n].state = c->state;
        out_cells[n].desired_state = c->desired_state;
        out_cells[n].entity_count = c->entity_count;
        out_cells[n].proxy_enabled = c->proxy_enabled;
        out_cells[n].flags = c->flags;
        out_cells[n].layer_mask = c->layer_mask;
        out_cells[n].service_mask = c->service_mask;
        out_cells[n].asset_ref = c->asset_ref;
        out_cells[n].proxy_asset_ref = c->proxy_asset_ref;
        n++;
    }
    return (int)n;
}

int w3d_debug_emit_entities(const w3d_world *world, w3d_debug_entity *out_entities, w3d_u16 max_out)
{
    w3d_u16 i;
    w3d_u16 n;
    const w3d_entity *e;
    if (!world || !out_entities) return W3D_ERR_NULL;
    n = 0U;
    for (i = 0U; i < world->cfg.max_entities && n < max_out; ++i) {
        e = &world->entities[i];
        if (!(e->flags & W3D_ENTITY_ALIVE)) continue;
        out_entities[n].handle = w3d_make_handle(e->slot, e->generation);
        out_entities[n].cell_index = e->cell_index;
        out_entities[n].flags = e->flags;
        out_entities[n].layer_mask = e->layer_mask;
        out_entities[n].kind = e->kind;
        out_entities[n].pos = e->xf.pos;
        out_entities[n].world_bounds = e->world_bounds;
        n++;
    }
    return (int)n;
}

static int w3d_parse_line(w3d_world *world, char *line)
{
    long a;
    long b;
    long c;
    long dval;
    long eval;
    unsigned long ua;
    unsigned long ub;
    unsigned long uc;
    unsigned long ud;
    unsigned long ue;
    unsigned long uf;
    int got;
    if (!line || line[0] == 0) return W3D_OK;
    if (line[0] == '#') return W3D_OK;
    got = sscanf(line, "cell %ld %ld %ld %lu %lu %lu %lu %lu %lu", &a, &b, &c, &ua, &ub, &uc, &ud, &ue, &uf);
    if (got >= 4) {
        w3d_cell_desc d;
        w3d_default_cell_desc(&d);
        d.cx = (w3d_i16)a;
        d.cy = (w3d_i16)b;
        d.cz = (w3d_i16)c;
        d.asset_ref = ua;
        if (got >= 5) d.flags = ub;
        if (got >= 6) d.layer_mask = uc;
        if (got >= 7) d.phase_mask = ud;
        if (got >= 8) d.difficulty_mask = ue;
        if (got >= 9) d.service_mask = uf;
        return w3d_world_define_cell_ex(world, &d);
    }
    got = sscanf(line, "proxy %ld %ld %ld %lu %ld", &a, &b, &c, &ua, &dval);
    if (got == 5) {
        int idx;
        idx = w3d_world_find_cell(world, (w3d_i16)a, (w3d_i16)b, (w3d_i16)c);
        if (idx < 0) return idx;
        return w3d_world_set_cell_proxy(world, (w3d_u16)idx, ua, w3d_fp_from_int((w3d_i32)dval));
    }
    got = sscanf(line, "source %ld %ld %ld %ld %ld %lu", &a, &b, &c, &dval, &eval, &ua);
    if (got == 6) {
        w3d_v3 pos;
        pos = w3d_v3_make(w3d_fp_from_int((w3d_i32)a), w3d_fp_from_int((w3d_i32)b), w3d_fp_from_int((w3d_i32)c));
        return w3d_world_set_stream_source(world, 0U, pos, w3d_fp_from_int((w3d_i32)dval), w3d_fp_from_int((w3d_i32)eval), ua, 1);
    }
    got = sscanf(line, "mask %lu %lu %lu", &ua, &ub, &uc);
    if (got == 3) {
        w3d_world_set_runtime_masks(world, ua, ub, uc);
        return W3D_OK;
    }
    got = sscanf(line, "portal %lu %lu %lu %lu", &ua, &ub, &uc, &ud);
    if (got >= 2) {
        w3d_aabb bnd;
        bnd = w3d_aabb_make(w3d_v3_make(0, 0, 0), w3d_v3_make(0, 0, 0));
        if (got < 3) uc = W3D_PORTAL_OPEN;
        if (got < 4) ud = 0UL;
        return w3d_world_define_portal(world, (w3d_u16)ua, (w3d_u16)ub, bnd, uc, W3D_LAYER_ALL, W3D_PHASE_ALL, ud);
    }
    if (line[0] == '\n' || line[0] == '\r') return W3D_OK;
    return W3D_ERR_PARSE;
}

int w3d_descriptor_apply_text(w3d_world *world, const char *text, w3d_u32 text_len)
{
    char line[192];
    w3d_u32 i;
    w3d_u32 n;
    int r;
    if (!world || !text) return W3D_ERR_NULL;
    n = 0UL;
    for (i = 0UL; i <= text_len; ++i) {
        char ch;
        ch = (i < text_len) ? text[i] : '\n';
        if (ch == '\r') continue;
        if (ch == '\n') {
            line[n] = 0;
            r = w3d_parse_line(world, line);
            if (r < 0) return r;
            n = 0UL;
        } else {
            if (n + 1UL >= (w3d_u32)sizeof(line)) return W3D_ERR_PARSE;
            line[n++] = ch;
        }
    }
    return W3D_OK;
}
