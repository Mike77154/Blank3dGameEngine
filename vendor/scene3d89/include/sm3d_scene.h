#ifndef SM3D_SCENE_H
#define SM3D_SCENE_H

/*
   scene3d89 / sm3d_scene
   C89 fixed-point, no malloc/free/realloc, no float/double.
   Pure scene manager: hierarchy, scenes, layers, groups, lifecycle,
   transform propagation, query/traversal, optional grid indexing.
*/

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SM3D_MAX_NODES
#define SM3D_MAX_NODES 512
#endif

#ifndef SM3D_MAX_SCENES
#define SM3D_MAX_SCENES 16
#endif

#ifndef SM3D_MAX_GROUPS
#define SM3D_MAX_GROUPS 32
#endif

#ifndef SM3D_MAX_STACK
#define SM3D_MAX_STACK 1024
#endif

#ifndef SM3D_MAX_DEFERRED_OPS
#define SM3D_MAX_DEFERRED_OPS 128
#endif

#ifndef SM3D_MAX_SCENE_REQUESTS
#define SM3D_MAX_SCENE_REQUESTS 64
#endif

#ifndef SM3D_MAX_SCENE_DEPS
#define SM3D_MAX_SCENE_DEPS 64
#endif

#ifndef SM3D_MAX_PREFABS
#define SM3D_MAX_PREFABS 32
#endif

#ifndef SM3D_MAX_PREFAB_NODES
#define SM3D_MAX_PREFAB_NODES 256
#endif

#ifndef SM3D_MAX_PORTALS
#define SM3D_MAX_PORTALS 64
#endif

#ifndef SM3D_MAX_DAG_LINKS
#define SM3D_MAX_DAG_LINKS 256
#endif

#ifndef SM3D_MAX_DIRTY_NODES
#define SM3D_MAX_DIRTY_NODES SM3D_MAX_NODES
#endif

#ifndef SM3D_MAX_MANIFEST_TOKENS
#define SM3D_MAX_MANIFEST_TOKENS 8
#endif

#ifndef SM3D_GRID_X
#define SM3D_GRID_X 32
#endif

#ifndef SM3D_GRID_Z
#define SM3D_GRID_Z 32
#endif

#define SM3D_VERSION_MAJOR 1
#define SM3D_VERSION_MINOR 1
#define SM3D_VERSION_PATCH 0

#define SM3D_TRUE 1
#define SM3D_FALSE 0

#define SM3D_FX_SHIFT 16
#define SM3D_FX_ONE 65536L
#define SM3D_FX_HALF 32768L

#define SM3D_INVALID_INDEX (-1)
#define SM3D_INVALID_HANDLE_VALUE 0UL

#define SM3D_SCENE_FLAG_LOADED       0x0001UL
#define SM3D_SCENE_FLAG_ACTIVE       0x0002UL
#define SM3D_SCENE_FLAG_PERSISTENT   0x0004UL
#define SM3D_SCENE_FLAG_ADDITIVE     0x0008UL
#define SM3D_SCENE_FLAG_STREAMABLE   0x0010UL
#define SM3D_SCENE_FLAG_PAUSED       0x0020UL
#define SM3D_SCENE_FLAG_DIRTY        0x0040UL
#define SM3D_SCENE_FLAG_REQUESTED    0x0080UL
#define SM3D_SCENE_FLAG_DEP_BLOCKED  0x0100UL
#define SM3D_SCENE_FLAG_SLOT_LOCKED  0x0200UL

#define SM3D_SCENE_SLOT_NONE         0
#define SM3D_SCENE_SLOT_PERSISTENT   1
#define SM3D_SCENE_SLOT_LEVEL        2
#define SM3D_SCENE_SLOT_ROOM         3
#define SM3D_SCENE_SLOT_OVERLAY      4
#define SM3D_SCENE_SLOT_WORLD_CELL   5
#define SM3D_SCENE_SLOT_USER0        64

#define SM3D_SCENE_STREAM_NONE       0
#define SM3D_SCENE_STREAM_REQUESTED  1
#define SM3D_SCENE_STREAM_LOADING    2
#define SM3D_SCENE_STREAM_READY      3
#define SM3D_SCENE_STREAM_UNLOADING  4
#define SM3D_SCENE_STREAM_FAILED     5

#define SM3D_REQ_NONE                0
#define SM3D_REQ_LOAD                1
#define SM3D_REQ_UNLOAD              2
#define SM3D_REQ_DONE                3
#define SM3D_REQ_FAILED              4

#define SM3D_DEP_REQUIRED            1
#define SM3D_DEP_WEAK                2
#define SM3D_DEP_LOAD_BEFORE         3
#define SM3D_DEP_VISIBILITY          4


#define SM3D_NODE_FLAG_USED          0x00000001UL
#define SM3D_NODE_FLAG_ACTIVE        0x00000002UL
#define SM3D_NODE_FLAG_VISIBLE       0x00000004UL
#define SM3D_NODE_FLAG_DIRTY_LOCAL   0x00000008UL
#define SM3D_NODE_FLAG_DIRTY_WORLD   0x00000010UL
#define SM3D_NODE_FLAG_STATIC        0x00000020UL
#define SM3D_NODE_FLAG_PERSISTENT    0x00000040UL
#define SM3D_NODE_FLAG_STREAMABLE    0x00000080UL
#define SM3D_NODE_FLAG_RENDERABLE    0x00000100UL
#define SM3D_NODE_FLAG_COLLIDABLE    0x00000200UL
#define SM3D_NODE_FLAG_LOGIC         0x00000400UL
#define SM3D_NODE_FLAG_AUDIO         0x00000800UL
#define SM3D_NODE_FLAG_CAMERA_ANCHOR 0x00001000UL
#define SM3D_NODE_FLAG_LIGHT_ANCHOR  0x00002000UL
#define SM3D_NODE_FLAG_TRIGGER       0x00004000UL
#define SM3D_NODE_FLAG_PORTAL        0x00008000UL
#define SM3D_NODE_FLAG_SPAWN         0x00010000UL
#define SM3D_NODE_FLAG_TAGGED        0x00020000UL
#define SM3D_NODE_FLAG_NO_TRAVERSE   0x00040000UL
#define SM3D_NODE_FLAG_KILL_PENDING  0x00080000UL
#define SM3D_NODE_FLAG_DIRTY_BOUNDS  0x00100000UL
#define SM3D_NODE_FLAG_DIRTY_VISIBLE 0x00200000UL
#define SM3D_NODE_FLAG_DIRTY_QUEUED  0x00400000UL
#define SM3D_NODE_FLAG_DAG_PROXY     0x00800000UL

#define SM3D_PREFAB_FLAG_KEEP_WORLD  0x0001UL
#define SM3D_PREFAB_FLAG_ACTIVE      0x0002UL

#define SM3D_PORTAL_FLAG_OPEN        0x0001UL
#define SM3D_PORTAL_FLAG_ONE_WAY     0x0002UL
#define SM3D_PORTAL_FLAG_VISIBLE     0x0004UL
#define SM3D_PORTAL_FLAG_HINT_ONLY   0x0008UL

#define SM3D_DAG_KIND_GENERIC        0
#define SM3D_DAG_KIND_RESOURCE       1
#define SM3D_DAG_KIND_LOGIC          2
#define SM3D_DAG_KIND_RENDER         3
#define SM3D_DAG_KIND_VISIBILITY     4
#define SM3D_DAG_KIND_DEPENDENCY     5
#define SM3D_DAG_KIND_USER0          64

#define SM3D_DAG_FLAG_ACYCLIC        0x0001UL
#define SM3D_DAG_FLAG_WEAK           0x0002UL
#define SM3D_DAG_FLAG_DIRTY_PROP     0x0004UL


#define SM3D_TRAVERSE_PREORDER       0
#define SM3D_TRAVERSE_POSTORDER      1

#define SM3D_VISIT_CONTINUE          0
#define SM3D_VISIT_SKIP_CHILDREN     1
#define SM3D_VISIT_STOP              2

#define SM3D_NODE_TYPE_EMPTY         0
#define SM3D_NODE_TYPE_SCENE_ROOT    1
#define SM3D_NODE_TYPE_ACTOR         2
#define SM3D_NODE_TYPE_MESH_REF      3
#define SM3D_NODE_TYPE_SPRITE_PLANE  4
#define SM3D_NODE_TYPE_CAMERA_ANCHOR 5
#define SM3D_NODE_TYPE_LIGHT_ANCHOR  6
#define SM3D_NODE_TYPE_TRIGGER       7
#define SM3D_NODE_TYPE_PORTAL        8
#define SM3D_NODE_TYPE_SECTOR        9
#define SM3D_NODE_TYPE_SPAWN_POINT   10
#define SM3D_NODE_TYPE_ATTACH_POINT  11
#define SM3D_NODE_TYPE_MARKER        12
#define SM3D_NODE_TYPE_VOLUME        13
#define SM3D_NODE_TYPE_USER0         64

#define SM3D_EVENT_SCENE_LOADED      1
#define SM3D_EVENT_SCENE_UNLOADED    2
#define SM3D_EVENT_NODE_ENTER        3
#define SM3D_EVENT_NODE_EXIT         4
#define SM3D_EVENT_NODE_MOVED        5
#define SM3D_EVENT_NODE_VISIBILITY   6
#define SM3D_EVENT_NODE_REPARENTED   7
#define SM3D_EVENT_NODE_DESTROYED    8
#define SM3D_EVENT_SCENE_REQUESTED   9
#define SM3D_EVENT_SCENE_SLOT        10
#define SM3D_EVENT_SCENE_DEP         11
#define SM3D_EVENT_PREFAB_INSTANTIATED 12
#define SM3D_EVENT_PORTAL_CHANGED    13
#define SM3D_EVENT_DAG_LINK          14

#define SM3D_OK                      0
#define SM3D_ERR_FULL               -1
#define SM3D_ERR_BAD_HANDLE         -2
#define SM3D_ERR_BAD_SCENE          -3
#define SM3D_ERR_BAD_PARENT         -4
#define SM3D_ERR_CYCLE              -5
#define SM3D_ERR_LOCKED             -6
#define SM3D_ERR_BAD_ARGUMENT       -7
#define SM3D_ERR_NOT_FOUND          -8
#define SM3D_ERR_STACK_OVERFLOW     -9
#define SM3D_ERR_DEFERRED_FULL      -10
#define SM3D_ERR_DEP_BLOCKED        -11
#define SM3D_ERR_REQUEST_FULL       -12
#define SM3D_ERR_PREFAB_FULL        -13
#define SM3D_ERR_DAG_CYCLE          -14
#define SM3D_ERR_PARSE              -15

#define SM3D_LAYER_DEFAULT           0x00000001UL
#define SM3D_LAYER_ALL               0xFFFFFFFFUL

typedef long sm3d_fx;
typedef unsigned long sm3d_u32;
typedef long sm3d_i32;
typedef unsigned short sm3d_u16;
typedef short sm3d_i16;
typedef unsigned char sm3d_u8;

typedef struct SM3D_Handle {
    sm3d_u32 value;
} SM3D_Handle;

typedef struct SM3D_Vec3 {
    sm3d_fx x;
    sm3d_fx y;
    sm3d_fx z;
} SM3D_Vec3;

typedef struct SM3D_Transform {
    SM3D_Vec3 pos;
    SM3D_Vec3 rot;
    SM3D_Vec3 scale;
} SM3D_Transform;

typedef struct SM3D_Aabb {
    SM3D_Vec3 center;
    SM3D_Vec3 half;
} SM3D_Aabb;

typedef struct SM3D_Node {
    sm3d_u32 generation;
    sm3d_u32 flags;
    sm3d_u32 layer_mask;
    sm3d_u32 group_mask;
    sm3d_u32 name_hash;
    sm3d_u32 resource_id;
    sm3d_u32 archetype_id;
    sm3d_u32 user_id;
    sm3d_i32 user_i0;
    sm3d_i32 user_i1;
    int type;
    int scene_id;
    int parent;
    int first_child;
    int last_child;
    int next_sibling;
    int prev_sibling;
    int grid_next;
    int grid_cell;
    SM3D_Transform local;
    SM3D_Transform world;
    SM3D_Aabb local_bounds;
    SM3D_Aabb world_bounds;
} SM3D_Node;

typedef struct SM3D_Scene {
    sm3d_u32 id;
    sm3d_u32 name_hash;
    sm3d_u32 flags;
    int root_node;
    int node_count;
    int stream_state;
    int slot_type;
    int slot_index;
    int request_id;
    sm3d_u32 dep_mask;
    SM3D_Vec3 origin;
    SM3D_Aabb bounds;
} SM3D_Scene;

typedef struct SM3D_GridConfig {
    int enabled;
    sm3d_fx origin_x;
    sm3d_fx origin_z;
    sm3d_fx cell_size;
} SM3D_GridConfig;

typedef struct SM3D_Config {
    SM3D_GridConfig grid;
    sm3d_u32 default_layer_mask;
    int auto_update_grid;
} SM3D_Config;

struct SM3D_Context;

typedef int (*SM3D_EventFn)(struct SM3D_Context *ctx, int event_type, SM3D_Handle node, int scene_id, void *user);
typedef int (*SM3D_VisitFn)(struct SM3D_Context *ctx, SM3D_Handle node, const SM3D_Node *node_ptr, void *user);

typedef struct SM3D_Bridge {
    SM3D_EventFn event_fn;
    void *user;
} SM3D_Bridge;

typedef struct SM3D_TraverseParams {
    int scene_id;
    SM3D_Handle root;
    sm3d_u32 required_flags;
    sm3d_u32 rejected_flags;
    sm3d_u32 layer_mask;
    sm3d_u32 group_mask;
    int type_filter;
    int order;
    int include_inactive;
    int max_depth;
} SM3D_TraverseParams;

typedef struct SM3D_QueryResult {
    SM3D_Handle *out;
    int capacity;
    int count;
    int overflow;
} SM3D_QueryResult;

typedef struct SM3D_DeferredOp {
    int op;
    SM3D_Handle a;
    SM3D_Handle b;
    int scene_id;
    sm3d_u32 flags;
} SM3D_DeferredOp;

typedef struct SM3D_SceneRequest {
    int used;
    int id;
    int op;
    int state;
    int scene_id;
    int slot_type;
    int slot_index;
    sm3d_u32 name_hash;
    sm3d_u32 flags;
    sm3d_u32 user_tag;
} SM3D_SceneRequest;

typedef struct SM3D_SceneDependency {
    int used;
    int from_scene_id;
    int to_scene_id;
    int kind;
    sm3d_u32 flags;
    sm3d_u32 user_tag;
} SM3D_SceneDependency;

typedef struct SM3D_PrefabNodeDesc {
    int next_index;
    int parent_index;
    int type;
    sm3d_u32 name_hash;
    sm3d_u32 flags;
    sm3d_u32 layer_mask;
    sm3d_u32 group_mask;
    sm3d_u32 resource_id;
    sm3d_u32 archetype_id;
    sm3d_u32 user_id;
    SM3D_Transform local;
    SM3D_Aabb local_bounds;
} SM3D_PrefabNodeDesc;

typedef struct SM3D_Prefab {
    int used;
    int id;
    sm3d_u32 name_hash;
    sm3d_u32 flags;
    int first_node;
    int node_count;
} SM3D_Prefab;

typedef struct SM3D_PortalHint {
    int used;
    int scene_id;
    SM3D_Handle portal_node;
    SM3D_Handle from_sector;
    SM3D_Handle to_sector;
    sm3d_u32 flags;
    sm3d_u32 layer_mask;
    sm3d_u32 user_tag;
} SM3D_PortalHint;

typedef struct SM3D_DagLink {
    int used;
    SM3D_Handle from_node;
    SM3D_Handle to_node;
    int kind;
    sm3d_u32 flags;
    sm3d_u32 user_tag;
} SM3D_DagLink;

typedef struct SM3D_ManifestConfig {
    int auto_process_requests;
    int default_slot_type;
    sm3d_u32 default_scene_flags;
} SM3D_ManifestConfig;

typedef struct SM3D_Context {
    SM3D_Node nodes[SM3D_MAX_NODES];
    SM3D_Scene scenes[SM3D_MAX_SCENES];
    int free_stack[SM3D_MAX_NODES];
    int free_top;
    int scene_count;
    sm3d_u32 next_scene_id;
    SM3D_Config cfg;
    SM3D_Bridge bridge;
    int read_lock_count;
    SM3D_DeferredOp deferred[SM3D_MAX_DEFERRED_OPS];
    int deferred_count;
    SM3D_SceneRequest requests[SM3D_MAX_SCENE_REQUESTS];
    int next_request_id;
    SM3D_SceneDependency deps[SM3D_MAX_SCENE_DEPS];
    SM3D_Prefab prefabs[SM3D_MAX_PREFABS];
    SM3D_PrefabNodeDesc prefab_nodes[SM3D_MAX_PREFAB_NODES];
    int prefab_node_used[SM3D_MAX_PREFAB_NODES];
    int next_prefab_id;
    SM3D_PortalHint portals[SM3D_MAX_PORTALS];
    SM3D_DagLink dag_links[SM3D_MAX_DAG_LINKS];
    int dirty_nodes[SM3D_MAX_DIRTY_NODES];
    int dirty_count;
    int dirty_overflow;
    int grid_heads[SM3D_GRID_X * SM3D_GRID_Z];
    int grid_dirty;
    int last_error;
} SM3D_Context;

sm3d_fx sm3d_fx_from_int(int v);
int sm3d_fx_to_int(sm3d_fx v);
sm3d_fx sm3d_fx_mul(sm3d_fx a, sm3d_fx b);
sm3d_fx sm3d_fx_div(sm3d_fx a, sm3d_fx b);
SM3D_Vec3 sm3d_vec3(sm3d_fx x, sm3d_fx y, sm3d_fx z);
SM3D_Transform sm3d_transform_identity(void);
SM3D_Aabb sm3d_aabb(SM3D_Vec3 center, SM3D_Vec3 half);
sm3d_u32 sm3d_hash_cstr(const char *s);

void sm3d_config_defaults(SM3D_Config *cfg);
void sm3d_init(SM3D_Context *ctx, const SM3D_Config *cfg);
void sm3d_set_bridge(SM3D_Context *ctx, const SM3D_Bridge *bridge);
void sm3d_reset(SM3D_Context *ctx);

int sm3d_scene_create(SM3D_Context *ctx, sm3d_u32 name_hash, sm3d_u32 flags, int *out_scene_id);
int sm3d_scene_find_by_hash(SM3D_Context *ctx, sm3d_u32 name_hash);
int sm3d_scene_set_active(SM3D_Context *ctx, int scene_id, int active);
int sm3d_scene_set_loaded(SM3D_Context *ctx, int scene_id, int loaded);
int sm3d_scene_destroy(SM3D_Context *ctx, int scene_id);
const SM3D_Scene *sm3d_scene_get(const SM3D_Context *ctx, int scene_id);

SM3D_Handle sm3d_node_invalid(void);
int sm3d_handle_is_valid(SM3D_Handle h);
int sm3d_handle_index(SM3D_Handle h);
SM3D_Handle sm3d_handle_make(int index, sm3d_u32 generation);
int sm3d_node_resolve(const SM3D_Context *ctx, SM3D_Handle h);
SM3D_Node *sm3d_node_get(SM3D_Context *ctx, SM3D_Handle h);
const SM3D_Node *sm3d_node_get_const(const SM3D_Context *ctx, SM3D_Handle h);

int sm3d_node_create(SM3D_Context *ctx, int scene_id, SM3D_Handle parent, int type, sm3d_u32 name_hash, SM3D_Handle *out_handle);
int sm3d_node_destroy(SM3D_Context *ctx, SM3D_Handle h);
int sm3d_node_set_parent(SM3D_Context *ctx, SM3D_Handle child, SM3D_Handle parent);
int sm3d_node_set_local_transform(SM3D_Context *ctx, SM3D_Handle h, const SM3D_Transform *t);
int sm3d_node_set_local_pos(SM3D_Context *ctx, SM3D_Handle h, SM3D_Vec3 p);
int sm3d_node_set_local_rot(SM3D_Context *ctx, SM3D_Handle h, SM3D_Vec3 r);
int sm3d_node_set_local_scale(SM3D_Context *ctx, SM3D_Handle h, SM3D_Vec3 s);
int sm3d_node_set_bounds(SM3D_Context *ctx, SM3D_Handle h, const SM3D_Aabb *local_bounds);
int sm3d_node_set_flags(SM3D_Context *ctx, SM3D_Handle h, sm3d_u32 flags, int enabled);
int sm3d_node_set_masks(SM3D_Context *ctx, SM3D_Handle h, sm3d_u32 layer_mask, sm3d_u32 group_mask);
int sm3d_node_set_user_ids(SM3D_Context *ctx, SM3D_Handle h, sm3d_u32 resource_id, sm3d_u32 archetype_id, sm3d_u32 user_id);

void sm3d_begin_read(SM3D_Context *ctx);
int sm3d_end_read(SM3D_Context *ctx);
int sm3d_flush_deferred(SM3D_Context *ctx);

int sm3d_update_transforms(SM3D_Context *ctx);
int sm3d_traverse(SM3D_Context *ctx, const SM3D_TraverseParams *params, SM3D_VisitFn fn, void *user);
void sm3d_traverse_params_defaults(SM3D_TraverseParams *params);

int sm3d_query_aabb(SM3D_Context *ctx, int scene_id, const SM3D_Aabb *box, sm3d_u32 layer_mask, sm3d_u32 group_mask, SM3D_QueryResult *result);
int sm3d_collect_by_flags(SM3D_Context *ctx, int scene_id, sm3d_u32 required_flags, sm3d_u32 rejected_flags, sm3d_u32 layer_mask, SM3D_QueryResult *result);

void sm3d_grid_clear(SM3D_Context *ctx);
int sm3d_grid_rebuild(SM3D_Context *ctx, int scene_id);
int sm3d_grid_query_aabb(SM3D_Context *ctx, int scene_id, const SM3D_Aabb *box, sm3d_u32 layer_mask, sm3d_u32 group_mask, SM3D_QueryResult *result);


int sm3d_scene_create_in_slot(SM3D_Context *ctx, sm3d_u32 name_hash, sm3d_u32 flags, int slot_type, int slot_index, int *out_scene_id);
int sm3d_scene_set_slot(SM3D_Context *ctx, int scene_id, int slot_type, int slot_index);
int sm3d_scene_find_by_slot(SM3D_Context *ctx, int slot_type, int slot_index);
int sm3d_scene_load_request(SM3D_Context *ctx, sm3d_u32 name_hash, int slot_type, int slot_index, sm3d_u32 flags, int *out_request_id);
int sm3d_scene_unload_request(SM3D_Context *ctx, int scene_id, int *out_request_id);
int sm3d_scene_process_requests(SM3D_Context *ctx, int max_ops);
const SM3D_SceneRequest *sm3d_scene_request_get(const SM3D_Context *ctx, int request_id);

int sm3d_scene_dependency_add(SM3D_Context *ctx, int from_scene_id, int to_scene_id, int kind, sm3d_u32 flags, sm3d_u32 user_tag);
int sm3d_scene_dependency_remove(SM3D_Context *ctx, int from_scene_id, int to_scene_id, int kind);
int sm3d_scene_dependencies_loaded(const SM3D_Context *ctx, int scene_id);
int sm3d_scene_dependency_count(const SM3D_Context *ctx, int scene_id);

int sm3d_prefab_create(SM3D_Context *ctx, sm3d_u32 name_hash, sm3d_u32 flags, int *out_prefab_id);
int sm3d_prefab_add_node(SM3D_Context *ctx, int prefab_id, const SM3D_PrefabNodeDesc *desc, int *out_template_index);
int sm3d_prefab_find_by_hash(const SM3D_Context *ctx, sm3d_u32 name_hash);
int sm3d_prefab_instantiate(SM3D_Context *ctx, int prefab_id, int scene_id, SM3D_Handle parent, const SM3D_Transform *base_transform, SM3D_Handle *out_root);

int sm3d_portal_hint_add(SM3D_Context *ctx, int scene_id, SM3D_Handle portal_node, SM3D_Handle from_sector, SM3D_Handle to_sector, sm3d_u32 flags, sm3d_u32 layer_mask, sm3d_u32 user_tag);
int sm3d_portal_hint_remove(SM3D_Context *ctx, SM3D_Handle portal_node);
int sm3d_portal_collect_visible(SM3D_Context *ctx, int scene_id, SM3D_Handle start_sector, int max_depth, SM3D_QueryResult *result);

int sm3d_update_dirty_transforms(SM3D_Context *ctx);
int sm3d_traverse_dirty(SM3D_Context *ctx, int scene_id, sm3d_u32 layer_mask, SM3D_VisitFn fn, void *user);
void sm3d_dirty_clear(SM3D_Context *ctx);

int sm3d_dag_link_add(SM3D_Context *ctx, SM3D_Handle from_node, SM3D_Handle to_node, int kind, sm3d_u32 flags, sm3d_u32 user_tag);
int sm3d_dag_link_remove(SM3D_Context *ctx, SM3D_Handle from_node, SM3D_Handle to_node, int kind);
int sm3d_dag_collect_outputs(SM3D_Context *ctx, SM3D_Handle from_node, int kind, SM3D_QueryResult *result);
int sm3d_dag_collect_inputs(SM3D_Context *ctx, SM3D_Handle to_node, int kind, SM3D_QueryResult *result);
int sm3d_dag_toposort(SM3D_Context *ctx, int scene_id, int kind, SM3D_QueryResult *result);

void sm3d_manifest_config_defaults(SM3D_ManifestConfig *cfg);
int sm3d_manifest_read_text(SM3D_Context *ctx, const char *text, const SM3D_ManifestConfig *cfg);

#ifdef __cplusplus
}
#endif

#endif
