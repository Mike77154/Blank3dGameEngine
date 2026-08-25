#ifndef W3D89_H
#define W3D89_H

#include "w3d89_arena.h"
#include "w3d89_bridge.h"
#include "w3d89_layers.h"

#ifdef __cplusplus
extern "C" {
#endif

#define W3D89_VERSION_MAJOR 2
#define W3D89_VERSION_MINOR 0
#define W3D89_VERSION_PATCH 0

#define W3D_FP_SHIFT 16
#define W3D_FP_ONE   65536L
#define W3D_FP_HALF  32768L
#define W3D_U16_NONE 65535U
#define W3D_INVALID_HANDLE 0UL

typedef w3d_i32 w3d_fp;
typedef w3d_u32 w3d_handle;

typedef struct w3d_v3_s {
    w3d_fp x;
    w3d_fp y;
    w3d_fp z;
} w3d_v3;

typedef struct w3d_aabb_s {
    w3d_v3 minv;
    w3d_v3 maxv;
} w3d_aabb;

typedef enum w3d_result_e {
    W3D_OK = 0,
    W3D_ERR_NULL = -1,
    W3D_ERR_ARENA = -2,
    W3D_ERR_CAPACITY = -3,
    W3D_ERR_NOT_FOUND = -4,
    W3D_ERR_BAD_HANDLE = -5,
    W3D_ERR_BAD_CONFIG = -6,
    W3D_ERR_DUPLICATE = -7,
    W3D_ERR_PARSE = -8
} w3d_result;

typedef enum w3d_cell_state_e {
    W3D_CELL_UNLOADED = 0,
    W3D_CELL_LOADING = 1,
    W3D_CELL_RESIDENT = 2,
    W3D_CELL_ACTIVE = 3,
    W3D_CELL_UNLOADING = 4
} w3d_cell_state;

typedef enum w3d_event_type_e {
    W3D_EVENT_NONE = 0,
    W3D_EVENT_CELL_LOAD = 1,
    W3D_EVENT_CELL_UNLOAD = 2,
    W3D_EVENT_CELL_ACTIVATE = 3,
    W3D_EVENT_CELL_DEACTIVATE = 4,
    W3D_EVENT_ENTITY_ENTER_CELL = 5,
    W3D_EVENT_ENTITY_LEAVE_CELL = 6,
    W3D_EVENT_ENTITY_MOVED = 7,
    W3D_EVENT_PORTAL_OPEN_CHANGED = 8,
    W3D_EVENT_PROXY_ENABLE = 9,
    W3D_EVENT_PROXY_DISABLE = 10
} w3d_event_type;

#define W3D_ENTITY_ALIVE       0x00000001UL
#define W3D_ENTITY_STATIC      0x00000002UL
#define W3D_ENTITY_STREAMED    0x00000004UL
#define W3D_ENTITY_QUERYABLE   0x00000008UL
#define W3D_ENTITY_RENDERABLE  0x00000010UL
#define W3D_ENTITY_COLLIDABLE  0x00000020UL
#define W3D_ENTITY_PHYSICS     0x00000040UL
#define W3D_ENTITY_AUDIO       0x00000080UL
#define W3D_ENTITY_NAV         0x00000100UL
#define W3D_ENTITY_TRIGGER     0x00000200UL
#define W3D_ENTITY_SLEEPING    0x00000400UL

#define W3D_CELL_FLAG_LOCKED        0x00000001UL
#define W3D_CELL_FLAG_ALWAYS_LOADED 0x00000002UL
#define W3D_CELL_FLAG_INTERIOR      0x00000004UL
#define W3D_CELL_FLAG_EXTERIOR      0x00000008UL
#define W3D_CELL_FLAG_PORTAL        0x00000010UL
#define W3D_CELL_FLAG_ROOM          0x00000020UL
#define W3D_CELL_FLAG_OCCLUDER      0x00000040UL
#define W3D_CELL_FLAG_HLOD_PROXY    0x00000080UL
#define W3D_CELL_FLAG_ADDITIVE      0x00000100UL

#define W3D_PORTAL_OPEN       0x00000001UL
#define W3D_PORTAL_LOCKED     0x00000002UL
#define W3D_PORTAL_OCCLUDES   0x00000004UL
#define W3D_PORTAL_ONE_WAY    0x00000008UL

#define W3D_HASH_EMPTY 0U
#define W3D_HASH_FULL  1U

typedef struct w3d_transform_s {
    w3d_v3 pos;
    w3d_v3 rot;
    w3d_v3 scale;
} w3d_transform;

typedef struct w3d_cell_hash_entry_s {
    w3d_i16 cx;
    w3d_i16 cy;
    w3d_i16 cz;
    w3d_u16 cell_index;
    w3d_u16 used;
} w3d_cell_hash_entry;

typedef struct w3d_cell_desc_s {
    w3d_i16 cx;
    w3d_i16 cy;
    w3d_i16 cz;
    w3d_u32 flags;
    w3d_u32 layer_mask;
    w3d_u32 phase_mask;
    w3d_u32 difficulty_mask;
    w3d_u32 service_mask;
    w3d_u32 asset_ref;
    w3d_u32 collision_ref;
    w3d_u32 nav_ref;
    w3d_u32 audio_ref;
    w3d_u32 script_ref;
    w3d_u32 proxy_asset_ref;
    w3d_fp proxy_distance;
} w3d_cell_desc;

typedef struct w3d_cell_s {
    w3d_i16 cx;
    w3d_i16 cy;
    w3d_i16 cz;
    w3d_u16 generation;
    w3d_u32 flags;
    w3d_u32 layer_mask;
    w3d_u32 phase_mask;
    w3d_u32 difficulty_mask;
    w3d_u32 service_mask;
    w3d_u32 asset_ref;
    w3d_u32 collision_ref;
    w3d_u32 nav_ref;
    w3d_u32 audio_ref;
    w3d_u32 script_ref;
    w3d_u32 proxy_asset_ref;
    w3d_fp proxy_distance;
    w3d_u16 proxy_enabled;
    w3d_u16 state;
    w3d_u16 desired_state;
    w3d_u16 entity_head;
    w3d_u16 entity_count;
    w3d_aabb bounds;
} w3d_cell;

typedef struct w3d_entity_s {
    w3d_u16 generation;
    w3d_u16 slot;
    w3d_u16 next_free;
    w3d_u16 cell_index;
    w3d_u16 prev_in_cell;
    w3d_u16 next_in_cell;
    w3d_u32 flags;
    w3d_u32 layer_mask;
    w3d_u32 phase_mask;
    w3d_u32 difficulty_mask;
    w3d_u32 group_mask;
    w3d_u32 kind;
    w3d_u32 user_index;
    w3d_transform xf;
    w3d_aabb local_bounds;
    w3d_aabb world_bounds;
} w3d_entity;

typedef struct w3d_stream_source_s {
    w3d_v3 pos;
    w3d_fp active_enter_radius;
    w3d_fp active_exit_radius;
    w3d_fp resident_enter_radius;
    w3d_fp resident_exit_radius;
    w3d_u32 layer_mask;
    w3d_u32 phase_mask;
    w3d_u32 difficulty_mask;
    w3d_u16 enabled;
} w3d_stream_source;

typedef struct w3d_stream_budget_s {
    w3d_u16 max_load_requests_per_tick;
    w3d_u16 max_unload_requests_per_tick;
    w3d_u16 max_activate_requests_per_tick;
    w3d_u16 max_deactivate_requests_per_tick;
} w3d_stream_budget;

typedef struct w3d_event_s {
    w3d_u16 type;
    w3d_u16 cell_index;
    w3d_handle entity;
    w3d_i16 cx;
    w3d_i16 cy;
    w3d_i16 cz;
    w3d_u32 service_mask;
} w3d_event;

typedef struct w3d_portal_s {
    w3d_u16 from_cell;
    w3d_u16 to_cell;
    w3d_u32 flags;
    w3d_u32 layer_mask;
    w3d_u32 phase_mask;
    w3d_aabb bounds;
    w3d_u32 user_index;
} w3d_portal;

typedef struct w3d_debug_cell_s {
    w3d_u16 cell_index;
    w3d_i16 cx;
    w3d_i16 cy;
    w3d_i16 cz;
    w3d_u16 state;
    w3d_u16 desired_state;
    w3d_u16 entity_count;
    w3d_u16 proxy_enabled;
    w3d_u32 flags;
    w3d_u32 layer_mask;
    w3d_u32 service_mask;
    w3d_u32 asset_ref;
    w3d_u32 proxy_asset_ref;
} w3d_debug_cell;

typedef struct w3d_debug_entity_s {
    w3d_handle handle;
    w3d_u16 cell_index;
    w3d_u32 flags;
    w3d_u32 layer_mask;
    w3d_u32 kind;
    w3d_v3 pos;
    w3d_aabb world_bounds;
} w3d_debug_entity;

typedef struct w3d_world_config_s {
    w3d_u16 max_cells;
    w3d_u16 max_entities;
    w3d_u16 max_stream_sources;
    w3d_u16 max_events;
    w3d_u16 max_cell_hash_entries;
    w3d_u16 max_portals;
    w3d_fp cell_size;
    w3d_v3 origin;
    w3d_u16 auto_commit_streaming;
    w3d_u32 active_layer_mask;
    w3d_u32 active_phase_mask;
    w3d_u32 active_difficulty_mask;
    w3d_stream_budget budget;
} w3d_world_config;

typedef struct w3d_world_status_s {
    w3d_u16 cell_count;
    w3d_u16 entity_alive_count;
    w3d_u16 portal_count;
    w3d_u16 event_count;
    w3d_u16 event_dropped_count;
    w3d_u16 active_cell_count;
    w3d_u16 resident_cell_count;
    w3d_u16 loading_cell_count;
    w3d_u16 unloading_cell_count;
    w3d_u16 hash_capacity;
    w3d_u16 hash_used;
    w3d_u16 stream_cursor;
    w3d_u16 load_requests_last_tick;
    w3d_u16 unload_requests_last_tick;
    w3d_u16 activate_requests_last_tick;
    w3d_u16 deactivate_requests_last_tick;
    w3d_u16 deferred_requests_last_tick;
    w3d_u16 last_query_touched_cells;
    w3d_u16 last_query_tested_entities;
    w3d_u32 arena_used;
    w3d_u32 arena_high_water;
} w3d_world_status;

typedef struct w3d_world_s {
    w3d_arena arena;
    w3d_world_config cfg;
    w3d_world_callbacks cb;
    void *user;
    w3d_cell *cells;
    w3d_cell_hash_entry *cell_hash;
    w3d_entity *entities;
    w3d_stream_source *sources;
    w3d_event *events;
    w3d_portal *portals;
    w3d_u16 cell_count;
    w3d_u16 entity_alive_count;
    w3d_u16 free_entity_head;
    w3d_u16 event_head;
    w3d_u16 event_tail;
    w3d_u16 event_count;
    w3d_u16 event_dropped_count;
    w3d_u16 portal_count;
    w3d_u16 hash_used;
    w3d_u16 stream_cursor;
    w3d_u16 load_used;
    w3d_u16 unload_used;
    w3d_u16 activate_used;
    w3d_u16 deactivate_used;
    w3d_u16 deferred_used;
    w3d_u16 last_query_touched_cells;
    w3d_u16 last_query_tested_entities;
} w3d_world;

w3d_fp w3d_fp_from_int(w3d_i32 v);
w3d_i32 w3d_fp_to_int_floor(w3d_fp v);
w3d_fp w3d_fp_abs(w3d_fp v);
w3d_v3 w3d_v3_make(w3d_fp x, w3d_fp y, w3d_fp z);
w3d_aabb w3d_aabb_make(w3d_v3 minv, w3d_v3 maxv);
w3d_aabb w3d_aabb_translate(w3d_aabb b, w3d_v3 p);
int w3d_aabb_overlap(w3d_aabb a, w3d_aabb b);

void w3d_world_default_config(w3d_world_config *cfg);
w3d_u32 w3d_world_memory_required(const w3d_world_config *cfg);
int w3d_world_init(w3d_world *world, const w3d_world_config *cfg, void *memory, w3d_u32 memory_size, const w3d_world_callbacks *callbacks, void *user);
void w3d_world_reset_runtime(w3d_world *world);
void w3d_world_get_status(const w3d_world *world, w3d_world_status *out_status);
void w3d_world_set_runtime_masks(w3d_world *world, w3d_u32 layer_mask, w3d_u32 phase_mask, w3d_u32 difficulty_mask);

int w3d_world_define_cell(w3d_world *world, w3d_i16 cx, w3d_i16 cy, w3d_i16 cz, w3d_u32 flags, w3d_u32 layer_mask, w3d_u32 asset_ref);
int w3d_world_define_cell_ex(w3d_world *world, const w3d_cell_desc *desc);
int w3d_world_find_cell(const w3d_world *world, w3d_i16 cx, w3d_i16 cy, w3d_i16 cz);
int w3d_world_find_cell_at_pos(const w3d_world *world, w3d_v3 pos);
int w3d_world_commit_cell_state(w3d_world *world, w3d_u16 cell_index, int state);
int w3d_world_set_cell_proxy(w3d_world *world, w3d_u16 cell_index, w3d_u32 proxy_asset_ref, w3d_fp proxy_distance);
const w3d_cell *w3d_world_get_cell(const w3d_world *world, w3d_u16 cell_index);

void w3d_world_clear_stream_sources(w3d_world *world);
int w3d_world_set_stream_source(w3d_world *world, w3d_u16 index, w3d_v3 pos, w3d_fp active_radius, w3d_fp resident_radius, w3d_u32 layer_mask, int enabled);
int w3d_world_set_stream_source_hysteresis(w3d_world *world, w3d_u16 index, w3d_v3 pos, w3d_fp active_enter, w3d_fp active_exit, w3d_fp resident_enter, w3d_fp resident_exit, w3d_u32 layer_mask, w3d_u32 phase_mask, w3d_u32 difficulty_mask, int enabled);
int w3d_world_update_streaming(w3d_world *world);

w3d_handle w3d_entity_spawn(w3d_world *world, w3d_v3 pos, w3d_aabb local_bounds, w3d_u32 flags, w3d_u32 layer_mask, w3d_u32 group_mask, w3d_u32 kind, w3d_u32 user_index);
w3d_handle w3d_entity_spawn_ex(w3d_world *world, w3d_v3 pos, w3d_aabb local_bounds, w3d_u32 flags, w3d_u32 layer_mask, w3d_u32 phase_mask, w3d_u32 difficulty_mask, w3d_u32 group_mask, w3d_u32 kind, w3d_u32 user_index);
int w3d_entity_remove(w3d_world *world, w3d_handle h);
int w3d_entity_set_pose(w3d_world *world, w3d_handle h, w3d_v3 pos, w3d_v3 rot);
int w3d_entity_set_flags(w3d_world *world, w3d_handle h, w3d_u32 flags);
w3d_entity *w3d_entity_resolve(w3d_world *world, w3d_handle h);
const w3d_entity *w3d_entity_resolve_const(const w3d_world *world, w3d_handle h);

int w3d_query_aabb(const w3d_world *world, w3d_aabb area, w3d_u32 layer_mask, w3d_handle *out_handles, w3d_u16 max_out);
int w3d_query_aabb_ex(const w3d_world *world, w3d_aabb area, w3d_u32 layer_mask, w3d_u32 phase_mask, w3d_u32 difficulty_mask, w3d_u32 required_flags, w3d_u32 group_mask, w3d_handle *out_handles, w3d_u16 max_out);
int w3d_query_cell_entities(const w3d_world *world, w3d_u16 cell_index, w3d_handle *out_handles, w3d_u16 max_out);

int w3d_world_define_portal(w3d_world *world, w3d_u16 from_cell, w3d_u16 to_cell, w3d_aabb bounds, w3d_u32 flags, w3d_u32 layer_mask, w3d_u32 phase_mask, w3d_u32 user_index);
int w3d_world_set_portal_open(w3d_world *world, w3d_u16 portal_index, int open);
int w3d_world_query_visible_cells_from(w3d_world *world, w3d_u16 start_cell, w3d_u16 max_depth, w3d_u32 layer_mask, w3d_u16 *out_cells, w3d_u16 max_out);
const w3d_portal *w3d_world_get_portal(const w3d_world *world, w3d_u16 portal_index);

int w3d_debug_emit_cells(const w3d_world *world, w3d_debug_cell *out_cells, w3d_u16 max_out);
int w3d_debug_emit_entities(const w3d_world *world, w3d_debug_entity *out_entities, w3d_u16 max_out);

int w3d_descriptor_apply_text(w3d_world *world, const char *text, w3d_u32 text_len);

int w3d_world_poll_event(w3d_world *world, w3d_event *out_event);

#ifdef __cplusplus
}
#endif

#endif
