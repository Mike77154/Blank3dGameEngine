#include "sm3d_scene.h"
#include <string.h>
#include <stdlib.h>

static void sm3d_emit(SM3D_Context *ctx, int event_type, SM3D_Handle node, int scene_id)
{
    if (ctx == 0) return;
    if (ctx->bridge.event_fn != 0) {
        ctx->bridge.event_fn(ctx, event_type, node, scene_id, ctx->bridge.user);
    }
}

sm3d_fx sm3d_fx_from_int(int v)
{
    return ((sm3d_fx)v) << SM3D_FX_SHIFT;
}

int sm3d_fx_to_int(sm3d_fx v)
{
    return (int)(v >> SM3D_FX_SHIFT);
}

sm3d_fx sm3d_fx_mul(sm3d_fx a, sm3d_fx b)
{
    return (sm3d_fx)((a / 256L) * (b / 256L));
}

sm3d_fx sm3d_fx_div(sm3d_fx a, sm3d_fx b)
{
    if (b == 0) return 0;
    return (sm3d_fx)((a << 8) / (b >> 8));
}

SM3D_Vec3 sm3d_vec3(sm3d_fx x, sm3d_fx y, sm3d_fx z)
{
    SM3D_Vec3 v;
    v.x = x;
    v.y = y;
    v.z = z;
    return v;
}

SM3D_Transform sm3d_transform_identity(void)
{
    SM3D_Transform t;
    t.pos = sm3d_vec3(0, 0, 0);
    t.rot = sm3d_vec3(0, 0, 0);
    t.scale = sm3d_vec3(SM3D_FX_ONE, SM3D_FX_ONE, SM3D_FX_ONE);
    return t;
}

SM3D_Aabb sm3d_aabb(SM3D_Vec3 center, SM3D_Vec3 half)
{
    SM3D_Aabb b;
    b.center = center;
    b.half = half;
    return b;
}

sm3d_u32 sm3d_hash_cstr(const char *s)
{
    sm3d_u32 h;
    unsigned char c;
    h = 2166136261UL;
    if (s == 0) return 0;
    while (*s != 0) {
        c = (unsigned char)*s;
        h ^= (sm3d_u32)c;
        h *= 16777619UL;
        ++s;
    }
    if (h == 0) h = 1;
    return h;
}

static SM3D_Vec3 sm3d_vec3_add(SM3D_Vec3 a, SM3D_Vec3 b)
{
    SM3D_Vec3 r;
    r.x = a.x + b.x;
    r.y = a.y + b.y;
    r.z = a.z + b.z;
    return r;
}

static SM3D_Vec3 sm3d_vec3_mul(SM3D_Vec3 a, SM3D_Vec3 b)
{
    SM3D_Vec3 r;
    r.x = sm3d_fx_mul(a.x, b.x);
    r.y = sm3d_fx_mul(a.y, b.y);
    r.z = sm3d_fx_mul(a.z, b.z);
    return r;
}

static sm3d_fx sm3d_abs_fx(sm3d_fx v)
{
    if (v < 0) return -v;
    return v;
}

static int sm3d_aabb_overlap(const SM3D_Aabb *a, const SM3D_Aabb *b)
{
    sm3d_fx dx;
    sm3d_fx dy;
    sm3d_fx dz;
    if (a == 0 || b == 0) return 0;
    dx = sm3d_abs_fx(a->center.x - b->center.x);
    dy = sm3d_abs_fx(a->center.y - b->center.y);
    dz = sm3d_abs_fx(a->center.z - b->center.z);
    if (dx > a->half.x + b->half.x) return 0;
    if (dy > a->half.y + b->half.y) return 0;
    if (dz > a->half.z + b->half.z) return 0;
    return 1;
}

void sm3d_config_defaults(SM3D_Config *cfg)
{
    if (cfg == 0) return;
    cfg->grid.enabled = 0;
    cfg->grid.origin_x = 0;
    cfg->grid.origin_z = 0;
    cfg->grid.cell_size = sm3d_fx_from_int(256);
    cfg->default_layer_mask = SM3D_LAYER_DEFAULT;
    cfg->auto_update_grid = 0;
}

void sm3d_init(SM3D_Context *ctx, const SM3D_Config *cfg)
{
    SM3D_Config def_cfg;
    int i;

    if (ctx == 0) return;
    memset(ctx, 0, sizeof(*ctx));
    sm3d_config_defaults(&def_cfg);
    if (cfg != 0) ctx->cfg = *cfg;
    else ctx->cfg = def_cfg;

    ctx->free_top = 0;
    for (i = SM3D_MAX_NODES - 1; i >= 0; --i) {
        ctx->free_stack[ctx->free_top] = i;
        ctx->free_top++;
        ctx->nodes[i].generation = 1;
        ctx->nodes[i].grid_cell = SM3D_INVALID_INDEX;
        ctx->nodes[i].grid_next = SM3D_INVALID_INDEX;
    }

    ctx->scene_count = 0;
    ctx->next_scene_id = 1;
    ctx->next_request_id = 1;
    ctx->next_prefab_id = 1;
    ctx->dirty_count = 0;
    ctx->dirty_overflow = 0;
    sm3d_grid_clear(ctx);
}

void sm3d_set_bridge(SM3D_Context *ctx, const SM3D_Bridge *bridge)
{
    if (ctx == 0) return;
    if (bridge == 0) {
        ctx->bridge.event_fn = 0;
        ctx->bridge.user = 0;
        return;
    }
    ctx->bridge = *bridge;
}

void sm3d_reset(SM3D_Context *ctx)
{
    SM3D_Config cfg;
    SM3D_Bridge bridge;
    if (ctx == 0) return;
    cfg = ctx->cfg;
    bridge = ctx->bridge;
    sm3d_init(ctx, &cfg);
    ctx->bridge = bridge;
}

static int sm3d_scene_index_by_id(const SM3D_Context *ctx, int scene_id)
{
    int i;
    if (ctx == 0) return SM3D_INVALID_INDEX;
    for (i = 0; i < SM3D_MAX_SCENES; ++i) {
        if (ctx->scenes[i].id == (sm3d_u32)scene_id) return i;
    }
    return SM3D_INVALID_INDEX;
}

const SM3D_Scene *sm3d_scene_get(const SM3D_Context *ctx, int scene_id)
{
    int ix;
    ix = sm3d_scene_index_by_id(ctx, scene_id);
    if (ix < 0) return 0;
    return &ctx->scenes[ix];
}

SM3D_Handle sm3d_node_invalid(void)
{
    SM3D_Handle h;
    h.value = SM3D_INVALID_HANDLE_VALUE;
    return h;
}

int sm3d_handle_is_valid(SM3D_Handle h)
{
    return h.value != SM3D_INVALID_HANDLE_VALUE;
}

int sm3d_handle_index(SM3D_Handle h)
{
    if (h.value == 0UL) return SM3D_INVALID_INDEX;
    return (int)(h.value & 0xFFFFUL);
}

SM3D_Handle sm3d_handle_make(int index, sm3d_u32 generation)
{
    SM3D_Handle h;
    if (index < 0 || index > 65535) {
        h.value = 0UL;
        return h;
    }
    h.value = ((generation & 0xFFFFUL) << 16) | ((sm3d_u32)index & 0xFFFFUL);
    if (h.value == 0UL) h.value = 1UL;
    return h;
}

int sm3d_node_resolve(const SM3D_Context *ctx, SM3D_Handle h)
{
    int ix;
    sm3d_u32 gen;
    if (ctx == 0) return SM3D_INVALID_INDEX;
    if (h.value == 0UL) return SM3D_INVALID_INDEX;
    ix = (int)(h.value & 0xFFFFUL);
    gen = (h.value >> 16) & 0xFFFFUL;
    if (ix < 0 || ix >= SM3D_MAX_NODES) return SM3D_INVALID_INDEX;
    if ((ctx->nodes[ix].flags & SM3D_NODE_FLAG_USED) == 0UL) return SM3D_INVALID_INDEX;
    if ((ctx->nodes[ix].generation & 0xFFFFUL) != gen) return SM3D_INVALID_INDEX;
    return ix;
}

SM3D_Node *sm3d_node_get(SM3D_Context *ctx, SM3D_Handle h)
{
    int ix;
    ix = sm3d_node_resolve(ctx, h);
    if (ix < 0) return 0;
    return &ctx->nodes[ix];
}

const SM3D_Node *sm3d_node_get_const(const SM3D_Context *ctx, SM3D_Handle h)
{
    int ix;
    ix = sm3d_node_resolve(ctx, h);
    if (ix < 0) return 0;
    return &ctx->nodes[ix];
}

static int sm3d_alloc_node(SM3D_Context *ctx)
{
    int ix;
    if (ctx->free_top <= 0) return SM3D_INVALID_INDEX;
    ctx->free_top--;
    ix = ctx->free_stack[ctx->free_top];
    return ix;
}

static void sm3d_free_node(SM3D_Context *ctx, int ix)
{
    sm3d_u32 new_generation;
    if (ix < 0 || ix >= SM3D_MAX_NODES) return;
    new_generation = (ctx->nodes[ix].generation + 1UL) & 0xFFFFUL;
    if (new_generation == 0UL) new_generation = 1UL;
    memset(&ctx->nodes[ix], 0, sizeof(ctx->nodes[ix]));
    ctx->nodes[ix].generation = new_generation;
    ctx->nodes[ix].grid_cell = SM3D_INVALID_INDEX;
    ctx->nodes[ix].grid_next = SM3D_INVALID_INDEX;
    if (ctx->free_top < SM3D_MAX_NODES) {
        ctx->free_stack[ctx->free_top] = ix;
        ctx->free_top++;
    }
}

static void sm3d_unlink_from_parent(SM3D_Context *ctx, int child_ix)
{
    int p;
    int n;
    int q;
    if (ctx == 0) return;
    if (child_ix < 0 || child_ix >= SM3D_MAX_NODES) return;
    p = ctx->nodes[child_ix].parent;
    n = ctx->nodes[child_ix].next_sibling;
    q = ctx->nodes[child_ix].prev_sibling;
    if (p >= 0 && p < SM3D_MAX_NODES) {
        if (ctx->nodes[p].first_child == child_ix) ctx->nodes[p].first_child = n;
        if (ctx->nodes[p].last_child == child_ix) ctx->nodes[p].last_child = q;
    }
    if (q >= 0) ctx->nodes[q].next_sibling = n;
    if (n >= 0) ctx->nodes[n].prev_sibling = q;
    ctx->nodes[child_ix].parent = SM3D_INVALID_INDEX;
    ctx->nodes[child_ix].next_sibling = SM3D_INVALID_INDEX;
    ctx->nodes[child_ix].prev_sibling = SM3D_INVALID_INDEX;
}

static void sm3d_link_child(SM3D_Context *ctx, int parent_ix, int child_ix)
{
    int last;
    ctx->nodes[child_ix].parent = parent_ix;
    ctx->nodes[child_ix].prev_sibling = SM3D_INVALID_INDEX;
    ctx->nodes[child_ix].next_sibling = SM3D_INVALID_INDEX;
    if (parent_ix < 0) return;
    last = ctx->nodes[parent_ix].last_child;
    if (last >= 0) {
        ctx->nodes[last].next_sibling = child_ix;
        ctx->nodes[child_ix].prev_sibling = last;
        ctx->nodes[parent_ix].last_child = child_ix;
    } else {
        ctx->nodes[parent_ix].first_child = child_ix;
        ctx->nodes[parent_ix].last_child = child_ix;
    }
}

static int sm3d_is_ancestor(const SM3D_Context *ctx, int maybe_ancestor, int node)
{
    int p;
    p = node;
    while (p >= 0) {
        if (p == maybe_ancestor) return 1;
        p = ctx->nodes[p].parent;
    }
    return 0;
}

static void sm3d_queue_dirty_node(SM3D_Context *ctx, int ix)
{
    if (ctx == 0) return;
    if (ix < 0 || ix >= SM3D_MAX_NODES) return;
    if ((ctx->nodes[ix].flags & SM3D_NODE_FLAG_USED) == 0UL) return;
    if ((ctx->nodes[ix].flags & SM3D_NODE_FLAG_DIRTY_QUEUED) != 0UL) return;
    if (ctx->dirty_count >= SM3D_MAX_DIRTY_NODES) {
        ctx->dirty_overflow = 1;
        return;
    }
    ctx->nodes[ix].flags |= SM3D_NODE_FLAG_DIRTY_QUEUED;
    ctx->dirty_nodes[ctx->dirty_count] = ix;
    ctx->dirty_count++;
}

static void sm3d_mark_subtree_dirty(SM3D_Context *ctx, int ix)
{
    int stack[SM3D_MAX_STACK];
    int top;
    int cur;
    int child;
    if (ctx == 0) return;
    if (ix < 0) return;
    top = 0;
    stack[top++] = ix;
    while (top > 0) {
        top--;
        cur = stack[top];
        ctx->nodes[cur].flags |= SM3D_NODE_FLAG_DIRTY_WORLD;
        sm3d_queue_dirty_node(ctx, cur);
        child = ctx->nodes[cur].first_child;
        while (child >= 0) {
            if (top < SM3D_MAX_STACK) {
                stack[top++] = child;
            }
            child = ctx->nodes[child].next_sibling;
        }
    }
    ctx->grid_dirty = 1;
}

int sm3d_scene_create(SM3D_Context *ctx, sm3d_u32 name_hash, sm3d_u32 flags, int *out_scene_id)
{
    int slot;
    int i;
    SM3D_Handle root;
    int scene_id;
    int r;

    if (ctx == 0) return SM3D_ERR_BAD_ARGUMENT;
    if (ctx->scene_count >= SM3D_MAX_SCENES) return SM3D_ERR_FULL;

    slot = SM3D_INVALID_INDEX;
    for (i = 0; i < SM3D_MAX_SCENES; ++i) {
        if (ctx->scenes[i].id == 0UL) {
            slot = i;
            break;
        }
    }
    if (slot < 0) return SM3D_ERR_FULL;

    scene_id = (int)ctx->next_scene_id;
    ctx->next_scene_id++;
    if (ctx->next_scene_id == 0UL) ctx->next_scene_id = 1UL;

    memset(&ctx->scenes[slot], 0, sizeof(ctx->scenes[slot]));
    ctx->scenes[slot].id = (sm3d_u32)scene_id;
    ctx->scenes[slot].name_hash = name_hash;
    ctx->scenes[slot].flags = flags | SM3D_SCENE_FLAG_LOADED | SM3D_SCENE_FLAG_ACTIVE;
    ctx->scenes[slot].root_node = SM3D_INVALID_INDEX;
    ctx->scenes[slot].slot_type = SM3D_SCENE_SLOT_LEVEL;
    ctx->scenes[slot].slot_index = slot;
    ctx->scenes[slot].request_id = 0;
    ctx->scenes[slot].dep_mask = 0UL;
    ctx->scenes[slot].stream_state = SM3D_SCENE_STREAM_READY;
    ctx->scenes[slot].bounds = sm3d_aabb(sm3d_vec3(0, 0, 0), sm3d_vec3(sm3d_fx_from_int(32767), sm3d_fx_from_int(32767), sm3d_fx_from_int(32767)));
    ctx->scene_count++;

    r = sm3d_node_create(ctx, scene_id, sm3d_node_invalid(), SM3D_NODE_TYPE_SCENE_ROOT, name_hash, &root);
    if (r != SM3D_OK) {
        ctx->scenes[slot].id = 0UL;
        ctx->scene_count--;
        return r;
    }
    ctx->scenes[slot].root_node = sm3d_handle_index(root);

    if (out_scene_id != 0) *out_scene_id = scene_id;
    sm3d_emit(ctx, SM3D_EVENT_SCENE_LOADED, root, scene_id);
    return SM3D_OK;
}

int sm3d_scene_find_by_hash(SM3D_Context *ctx, sm3d_u32 name_hash)
{
    int i;
    if (ctx == 0) return SM3D_INVALID_INDEX;
    for (i = 0; i < SM3D_MAX_SCENES; ++i) {
        if (ctx->scenes[i].id != 0UL && ctx->scenes[i].name_hash == name_hash) return (int)ctx->scenes[i].id;
    }
    return SM3D_INVALID_INDEX;
}

int sm3d_scene_set_active(SM3D_Context *ctx, int scene_id, int active)
{
    int ix;
    ix = sm3d_scene_index_by_id(ctx, scene_id);
    if (ix < 0) return SM3D_ERR_BAD_SCENE;
    if (active) ctx->scenes[ix].flags |= SM3D_SCENE_FLAG_ACTIVE;
    else ctx->scenes[ix].flags &= ~SM3D_SCENE_FLAG_ACTIVE;
    return SM3D_OK;
}

int sm3d_scene_set_loaded(SM3D_Context *ctx, int scene_id, int loaded)
{
    int ix;
    SM3D_Handle root;
    ix = sm3d_scene_index_by_id(ctx, scene_id);
    if (ix < 0) return SM3D_ERR_BAD_SCENE;
    root = sm3d_handle_make(ctx->scenes[ix].root_node, ctx->nodes[ctx->scenes[ix].root_node].generation);
    if (loaded) {
        ctx->scenes[ix].flags |= SM3D_SCENE_FLAG_LOADED;
        sm3d_emit(ctx, SM3D_EVENT_SCENE_LOADED, root, scene_id);
    } else {
        ctx->scenes[ix].flags &= ~SM3D_SCENE_FLAG_LOADED;
        sm3d_emit(ctx, SM3D_EVENT_SCENE_UNLOADED, root, scene_id);
    }
    return SM3D_OK;
}

static void sm3d_destroy_subtree_now(SM3D_Context *ctx, int ix)
{
    int child;
    int next;
    SM3D_Handle h;
    if (ctx == 0 || ix < 0 || ix >= SM3D_MAX_NODES) return;
    child = ctx->nodes[ix].first_child;
    while (child >= 0) {
        next = ctx->nodes[child].next_sibling;
        sm3d_destroy_subtree_now(ctx, child);
        child = next;
    }
    h = sm3d_handle_make(ix, ctx->nodes[ix].generation);
    sm3d_emit(ctx, SM3D_EVENT_NODE_EXIT, h, ctx->nodes[ix].scene_id);
    sm3d_emit(ctx, SM3D_EVENT_NODE_DESTROYED, h, ctx->nodes[ix].scene_id);
    sm3d_unlink_from_parent(ctx, ix);
    sm3d_free_node(ctx, ix);
}

int sm3d_scene_destroy(SM3D_Context *ctx, int scene_id)
{
    int six;
    int root;
    if (ctx == 0) return SM3D_ERR_BAD_ARGUMENT;
    if (ctx->read_lock_count > 0) return SM3D_ERR_LOCKED;
    six = sm3d_scene_index_by_id(ctx, scene_id);
    if (six < 0) return SM3D_ERR_BAD_SCENE;
    root = ctx->scenes[six].root_node;
    if (root >= 0) sm3d_destroy_subtree_now(ctx, root);
    memset(&ctx->scenes[six], 0, sizeof(ctx->scenes[six]));
    ctx->scene_count--;
    ctx->grid_dirty = 1;
    return SM3D_OK;
}

int sm3d_node_create(SM3D_Context *ctx, int scene_id, SM3D_Handle parent, int type, sm3d_u32 name_hash, SM3D_Handle *out_handle)
{
    int scene_ix;
    int node_ix;
    int parent_ix;
    SM3D_Node *n;
    sm3d_u32 generation;

    if (ctx == 0) return SM3D_ERR_BAD_ARGUMENT;
    if (out_handle == 0) return SM3D_ERR_BAD_ARGUMENT;
    if (ctx->read_lock_count > 0) return SM3D_ERR_LOCKED;

    scene_ix = sm3d_scene_index_by_id(ctx, scene_id);
    if (scene_ix < 0 && type != SM3D_NODE_TYPE_SCENE_ROOT) return SM3D_ERR_BAD_SCENE;
    parent_ix = SM3D_INVALID_INDEX;
    if (sm3d_handle_is_valid(parent)) {
        parent_ix = sm3d_node_resolve(ctx, parent);
        if (parent_ix < 0) return SM3D_ERR_BAD_PARENT;
        if (ctx->nodes[parent_ix].scene_id != scene_id) return SM3D_ERR_BAD_PARENT;
    }

    node_ix = sm3d_alloc_node(ctx);
    if (node_ix < 0) return SM3D_ERR_FULL;
    n = &ctx->nodes[node_ix];
    generation = n->generation;
    if (generation == 0UL) generation = 1UL;
    memset(n, 0, sizeof(*n));
    n->generation = generation;
    n->flags = SM3D_NODE_FLAG_USED | SM3D_NODE_FLAG_ACTIVE | SM3D_NODE_FLAG_VISIBLE | SM3D_NODE_FLAG_DIRTY_LOCAL | SM3D_NODE_FLAG_DIRTY_WORLD;
    n->layer_mask = ctx->cfg.default_layer_mask;
    n->group_mask = 0UL;
    n->name_hash = name_hash;
    n->type = type;
    n->scene_id = scene_id;
    n->parent = SM3D_INVALID_INDEX;
    n->first_child = SM3D_INVALID_INDEX;
    n->last_child = SM3D_INVALID_INDEX;
    n->next_sibling = SM3D_INVALID_INDEX;
    n->prev_sibling = SM3D_INVALID_INDEX;
    n->grid_next = SM3D_INVALID_INDEX;
    n->grid_cell = SM3D_INVALID_INDEX;
    n->local = sm3d_transform_identity();
    n->world = sm3d_transform_identity();
    n->local_bounds = sm3d_aabb(sm3d_vec3(0, 0, 0), sm3d_vec3(0, 0, 0));
    n->world_bounds = n->local_bounds;

    if (parent_ix >= 0) sm3d_link_child(ctx, parent_ix, node_ix);
    if (scene_ix >= 0) ctx->scenes[scene_ix].node_count++;
    *out_handle = sm3d_handle_make(node_ix, n->generation);
    sm3d_mark_subtree_dirty(ctx, node_ix);
    sm3d_emit(ctx, SM3D_EVENT_NODE_ENTER, *out_handle, scene_id);
    return SM3D_OK;
}

int sm3d_node_destroy(SM3D_Context *ctx, SM3D_Handle h)
{
    int ix;
    int six;
    if (ctx == 0) return SM3D_ERR_BAD_ARGUMENT;
    ix = sm3d_node_resolve(ctx, h);
    if (ix < 0) return SM3D_ERR_BAD_HANDLE;
    if (ctx->read_lock_count > 0) return SM3D_ERR_LOCKED;
    six = sm3d_scene_index_by_id(ctx, ctx->nodes[ix].scene_id);
    if (six >= 0) {
        if (ctx->scenes[six].root_node == ix) return SM3D_ERR_BAD_ARGUMENT;
        if (ctx->scenes[six].node_count > 0) ctx->scenes[six].node_count--;
    }
    sm3d_destroy_subtree_now(ctx, ix);
    ctx->grid_dirty = 1;
    return SM3D_OK;
}

int sm3d_node_set_parent(SM3D_Context *ctx, SM3D_Handle child, SM3D_Handle parent)
{
    int c;
    int p;
    if (ctx == 0) return SM3D_ERR_BAD_ARGUMENT;
    if (ctx->read_lock_count > 0) return SM3D_ERR_LOCKED;
    c = sm3d_node_resolve(ctx, child);
    if (c < 0) return SM3D_ERR_BAD_HANDLE;
    p = sm3d_node_resolve(ctx, parent);
    if (p < 0) return SM3D_ERR_BAD_PARENT;
    if (ctx->nodes[c].scene_id != ctx->nodes[p].scene_id) return SM3D_ERR_BAD_PARENT;
    if (sm3d_is_ancestor(ctx, c, p)) return SM3D_ERR_CYCLE;
    sm3d_unlink_from_parent(ctx, c);
    sm3d_link_child(ctx, p, c);
    sm3d_mark_subtree_dirty(ctx, c);
    sm3d_emit(ctx, SM3D_EVENT_NODE_REPARENTED, child, ctx->nodes[c].scene_id);
    return SM3D_OK;
}

int sm3d_node_set_local_transform(SM3D_Context *ctx, SM3D_Handle h, const SM3D_Transform *t)
{
    int ix;
    if (ctx == 0 || t == 0) return SM3D_ERR_BAD_ARGUMENT;
    ix = sm3d_node_resolve(ctx, h);
    if (ix < 0) return SM3D_ERR_BAD_HANDLE;
    ctx->nodes[ix].local = *t;
    ctx->nodes[ix].flags |= SM3D_NODE_FLAG_DIRTY_LOCAL | SM3D_NODE_FLAG_DIRTY_WORLD;
    sm3d_mark_subtree_dirty(ctx, ix);
    sm3d_emit(ctx, SM3D_EVENT_NODE_MOVED, h, ctx->nodes[ix].scene_id);
    return SM3D_OK;
}

int sm3d_node_set_local_pos(SM3D_Context *ctx, SM3D_Handle h, SM3D_Vec3 p)
{
    SM3D_Node *n;
    n = sm3d_node_get(ctx, h);
    if (n == 0) return SM3D_ERR_BAD_HANDLE;
    n->local.pos = p;
    n->flags |= SM3D_NODE_FLAG_DIRTY_LOCAL | SM3D_NODE_FLAG_DIRTY_WORLD;
    sm3d_mark_subtree_dirty(ctx, sm3d_handle_index(h));
    sm3d_emit(ctx, SM3D_EVENT_NODE_MOVED, h, n->scene_id);
    return SM3D_OK;
}

int sm3d_node_set_local_rot(SM3D_Context *ctx, SM3D_Handle h, SM3D_Vec3 r)
{
    SM3D_Node *n;
    n = sm3d_node_get(ctx, h);
    if (n == 0) return SM3D_ERR_BAD_HANDLE;
    n->local.rot = r;
    n->flags |= SM3D_NODE_FLAG_DIRTY_LOCAL | SM3D_NODE_FLAG_DIRTY_WORLD;
    sm3d_mark_subtree_dirty(ctx, sm3d_handle_index(h));
    sm3d_emit(ctx, SM3D_EVENT_NODE_MOVED, h, n->scene_id);
    return SM3D_OK;
}

int sm3d_node_set_local_scale(SM3D_Context *ctx, SM3D_Handle h, SM3D_Vec3 s)
{
    SM3D_Node *n;
    n = sm3d_node_get(ctx, h);
    if (n == 0) return SM3D_ERR_BAD_HANDLE;
    n->local.scale = s;
    n->flags |= SM3D_NODE_FLAG_DIRTY_LOCAL | SM3D_NODE_FLAG_DIRTY_WORLD;
    sm3d_mark_subtree_dirty(ctx, sm3d_handle_index(h));
    sm3d_emit(ctx, SM3D_EVENT_NODE_MOVED, h, n->scene_id);
    return SM3D_OK;
}

int sm3d_node_set_bounds(SM3D_Context *ctx, SM3D_Handle h, const SM3D_Aabb *local_bounds)
{
    SM3D_Node *n;
    if (local_bounds == 0) return SM3D_ERR_BAD_ARGUMENT;
    n = sm3d_node_get(ctx, h);
    if (n == 0) return SM3D_ERR_BAD_HANDLE;
    n->local_bounds = *local_bounds;
    n->flags |= SM3D_NODE_FLAG_DIRTY_WORLD;
    sm3d_mark_subtree_dirty(ctx, sm3d_handle_index(h));
    return SM3D_OK;
}

int sm3d_node_set_flags(SM3D_Context *ctx, SM3D_Handle h, sm3d_u32 flags, int enabled)
{
    SM3D_Node *n;
    sm3d_u32 old;
    n = sm3d_node_get(ctx, h);
    if (n == 0) return SM3D_ERR_BAD_HANDLE;
    old = n->flags;
    if (enabled) n->flags |= flags;
    else n->flags &= ~flags;
    if ((old & SM3D_NODE_FLAG_VISIBLE) != (n->flags & SM3D_NODE_FLAG_VISIBLE)) {
        sm3d_emit(ctx, SM3D_EVENT_NODE_VISIBILITY, h, n->scene_id);
    }
    return SM3D_OK;
}

int sm3d_node_set_masks(SM3D_Context *ctx, SM3D_Handle h, sm3d_u32 layer_mask, sm3d_u32 group_mask)
{
    SM3D_Node *n;
    n = sm3d_node_get(ctx, h);
    if (n == 0) return SM3D_ERR_BAD_HANDLE;
    n->layer_mask = layer_mask;
    n->group_mask = group_mask;
    return SM3D_OK;
}

int sm3d_node_set_user_ids(SM3D_Context *ctx, SM3D_Handle h, sm3d_u32 resource_id, sm3d_u32 archetype_id, sm3d_u32 user_id)
{
    SM3D_Node *n;
    n = sm3d_node_get(ctx, h);
    if (n == 0) return SM3D_ERR_BAD_HANDLE;
    n->resource_id = resource_id;
    n->archetype_id = archetype_id;
    n->user_id = user_id;
    return SM3D_OK;
}

void sm3d_begin_read(SM3D_Context *ctx)
{
    if (ctx == 0) return;
    ctx->read_lock_count++;
}

int sm3d_end_read(SM3D_Context *ctx)
{
    if (ctx == 0) return SM3D_ERR_BAD_ARGUMENT;
    if (ctx->read_lock_count > 0) ctx->read_lock_count--;
    if (ctx->read_lock_count == 0) return sm3d_flush_deferred(ctx);
    return SM3D_OK;
}

int sm3d_flush_deferred(SM3D_Context *ctx)
{
    if (ctx == 0) return SM3D_ERR_BAD_ARGUMENT;
    ctx->deferred_count = 0;
    return SM3D_OK;
}

static void sm3d_compose_world(SM3D_Node *n, const SM3D_Node *p)
{
    SM3D_Vec3 scaled_local;
    SM3D_Vec3 half_scaled;
    if (p == 0) {
        n->world = n->local;
    } else {
        scaled_local = sm3d_vec3_mul(n->local.pos, p->world.scale);
        n->world.pos = sm3d_vec3_add(p->world.pos, scaled_local);
        n->world.rot = sm3d_vec3_add(p->world.rot, n->local.rot);
        n->world.scale = sm3d_vec3_mul(p->world.scale, n->local.scale);
    }
    half_scaled = sm3d_vec3_mul(n->local_bounds.half, n->world.scale);
    n->world_bounds.center = sm3d_vec3_add(n->world.pos, n->local_bounds.center);
    n->world_bounds.half.x = sm3d_abs_fx(half_scaled.x);
    n->world_bounds.half.y = sm3d_abs_fx(half_scaled.y);
    n->world_bounds.half.z = sm3d_abs_fx(half_scaled.z);
    n->flags &= ~(SM3D_NODE_FLAG_DIRTY_LOCAL | SM3D_NODE_FLAG_DIRTY_WORLD);
}

int sm3d_update_transforms(SM3D_Context *ctx)
{
    int i;
    int stack[SM3D_MAX_STACK];
    int top;
    int cur;
    int child;
    int parent_dirty;
    SM3D_Node *n;
    SM3D_Node *p;

    if (ctx == 0) return SM3D_ERR_BAD_ARGUMENT;
    top = 0;

    for (i = 0; i < SM3D_MAX_NODES; ++i) {
        if ((ctx->nodes[i].flags & SM3D_NODE_FLAG_USED) != 0UL && ctx->nodes[i].parent < 0) {
            if (top >= SM3D_MAX_STACK) return SM3D_ERR_STACK_OVERFLOW;
            stack[top++] = i;
        }
    }

    while (top > 0) {
        top--;
        cur = stack[top];
        n = &ctx->nodes[cur];
        p = 0;
        if (n->parent >= 0) p = &ctx->nodes[n->parent];
        parent_dirty = ((n->flags & (SM3D_NODE_FLAG_DIRTY_LOCAL | SM3D_NODE_FLAG_DIRTY_WORLD)) != 0UL);
        if (parent_dirty) {
            sm3d_compose_world(n, p);
        }
        child = n->last_child;
        while (child >= 0) {
            if (top >= SM3D_MAX_STACK) return SM3D_ERR_STACK_OVERFLOW;
            if (parent_dirty) ctx->nodes[child].flags |= SM3D_NODE_FLAG_DIRTY_WORLD;
            stack[top++] = child;
            child = ctx->nodes[child].prev_sibling;
        }
    }

    if (ctx->cfg.auto_update_grid && ctx->cfg.grid.enabled && ctx->grid_dirty) {
        sm3d_grid_rebuild(ctx, 0);
    }
    return SM3D_OK;
}

void sm3d_traverse_params_defaults(SM3D_TraverseParams *params)
{
    if (params == 0) return;
    params->scene_id = 0;
    params->root = sm3d_node_invalid();
    params->required_flags = 0UL;
    params->rejected_flags = 0UL;
    params->layer_mask = SM3D_LAYER_ALL;
    params->group_mask = 0UL;
    params->type_filter = -1;
    params->order = SM3D_TRAVERSE_PREORDER;
    params->include_inactive = 0;
    params->max_depth = -1;
}

static int sm3d_filter_node(const SM3D_Node *n, const SM3D_TraverseParams *p)
{
    if (n == 0 || p == 0) return 0;
    if ((n->flags & SM3D_NODE_FLAG_USED) == 0UL) return 0;
    if (p->scene_id != 0 && n->scene_id != p->scene_id) return 0;
    if (!p->include_inactive && (n->flags & SM3D_NODE_FLAG_ACTIVE) == 0UL) return 0;
    if (p->required_flags != 0UL && (n->flags & p->required_flags) != p->required_flags) return 0;
    if (p->rejected_flags != 0UL && (n->flags & p->rejected_flags) != 0UL) return 0;
    if (p->layer_mask != 0UL && (n->layer_mask & p->layer_mask) == 0UL) return 0;
    if (p->group_mask != 0UL && (n->group_mask & p->group_mask) == 0UL) return 0;
    if (p->type_filter >= 0 && n->type != p->type_filter) return 0;
    return 1;
}

typedef struct SM3D_StackItem {
    int index;
    int depth;
    int post;
} SM3D_StackItem;

int sm3d_traverse(SM3D_Context *ctx, const SM3D_TraverseParams *params, SM3D_VisitFn fn, void *user)
{
    SM3D_TraverseParams pdef;
    const SM3D_TraverseParams *p;
    SM3D_StackItem stack[SM3D_MAX_STACK];
    int top;
    int root_ix;
    int i;
    int cur;
    int child;
    int visit_r;
    int cur_depth;
    SM3D_Handle h;
    SM3D_Node *n;

    if (ctx == 0 || fn == 0) return SM3D_ERR_BAD_ARGUMENT;
    if (params == 0) {
        sm3d_traverse_params_defaults(&pdef);
        p = &pdef;
    } else p = params;

    root_ix = SM3D_INVALID_INDEX;
    if (sm3d_handle_is_valid(p->root)) {
        root_ix = sm3d_node_resolve(ctx, p->root);
        if (root_ix < 0) return SM3D_ERR_BAD_HANDLE;
    }

    top = 0;
    if (root_ix >= 0) {
        stack[top].index = root_ix;
        stack[top].depth = 0;
        stack[top].post = 0;
        top++;
    } else {
        for (i = SM3D_MAX_NODES - 1; i >= 0; --i) {
            if ((ctx->nodes[i].flags & SM3D_NODE_FLAG_USED) != 0UL && ctx->nodes[i].parent < 0) {
                if (p->scene_id == 0 || ctx->nodes[i].scene_id == p->scene_id) {
                    if (top >= SM3D_MAX_STACK) return SM3D_ERR_STACK_OVERFLOW;
                    stack[top].index = i;
                    stack[top].depth = 0;
                    stack[top].post = 0;
                    top++;
                }
            }
        }
    }

    sm3d_begin_read(ctx);
    while (top > 0) {
        top--;
        cur = stack[top].index;
        cur_depth = stack[top].depth;
        n = &ctx->nodes[cur];
        if (p->max_depth >= 0 && cur_depth > p->max_depth) continue;
        h = sm3d_handle_make(cur, n->generation);

        if (p->order == SM3D_TRAVERSE_POSTORDER && stack[top].post == 0) {
            if (top >= SM3D_MAX_STACK) {
                sm3d_end_read(ctx);
                return SM3D_ERR_STACK_OVERFLOW;
            }
            stack[top].index = cur;
            stack[top].depth = cur_depth;
            stack[top].post = 1;
            top++;
            child = n->last_child;
            while (child >= 0) {
                if (top >= SM3D_MAX_STACK) {
                    sm3d_end_read(ctx);
                    return SM3D_ERR_STACK_OVERFLOW;
                }
                stack[top].index = child;
                stack[top].depth = cur_depth + 1;
                stack[top].post = 0;
                top++;
                child = ctx->nodes[child].prev_sibling;
            }
            continue;
        }

        if (sm3d_filter_node(n, p)) {
            visit_r = fn(ctx, h, n, user);
            if (visit_r == SM3D_VISIT_STOP) {
                sm3d_end_read(ctx);
                return SM3D_OK;
            }
            if (visit_r == SM3D_VISIT_SKIP_CHILDREN) continue;
        }
        if (p->order == SM3D_TRAVERSE_PREORDER && (n->flags & SM3D_NODE_FLAG_NO_TRAVERSE) == 0UL) {
            child = n->last_child;
            while (child >= 0) {
                if (top >= SM3D_MAX_STACK) {
                    sm3d_end_read(ctx);
                    return SM3D_ERR_STACK_OVERFLOW;
                }
                stack[top].index = child;
                stack[top].depth = cur_depth + 1;
                stack[top].post = 0;
                top++;
                child = ctx->nodes[child].prev_sibling;
            }
        }
    }
    return sm3d_end_read(ctx);
}

static void sm3d_qpush(SM3D_QueryResult *r, SM3D_Handle h)
{
    if (r == 0) return;
    if (r->count >= r->capacity) {
        r->overflow = 1;
        return;
    }
    r->out[r->count] = h;
    r->count++;
}

int sm3d_query_aabb(SM3D_Context *ctx, int scene_id, const SM3D_Aabb *box, sm3d_u32 layer_mask, sm3d_u32 group_mask, SM3D_QueryResult *result)
{
    int i;
    SM3D_Node *n;
    SM3D_Handle h;
    if (ctx == 0 || box == 0 || result == 0 || result->out == 0) return SM3D_ERR_BAD_ARGUMENT;
    result->count = 0;
    result->overflow = 0;
    for (i = 0; i < SM3D_MAX_NODES; ++i) {
        n = &ctx->nodes[i];
        if ((n->flags & SM3D_NODE_FLAG_USED) == 0UL) continue;
        if (scene_id != 0 && n->scene_id != scene_id) continue;
        if ((n->flags & SM3D_NODE_FLAG_ACTIVE) == 0UL) continue;
        if (layer_mask != 0UL && (n->layer_mask & layer_mask) == 0UL) continue;
        if (group_mask != 0UL && (n->group_mask & group_mask) == 0UL) continue;
        if (sm3d_aabb_overlap(&n->world_bounds, box)) {
            h = sm3d_handle_make(i, n->generation);
            sm3d_qpush(result, h);
        }
    }
    return result->overflow ? SM3D_ERR_FULL : SM3D_OK;
}

int sm3d_collect_by_flags(SM3D_Context *ctx, int scene_id, sm3d_u32 required_flags, sm3d_u32 rejected_flags, sm3d_u32 layer_mask, SM3D_QueryResult *result)
{
    int i;
    SM3D_Node *n;
    SM3D_Handle h;
    if (ctx == 0 || result == 0 || result->out == 0) return SM3D_ERR_BAD_ARGUMENT;
    result->count = 0;
    result->overflow = 0;
    for (i = 0; i < SM3D_MAX_NODES; ++i) {
        n = &ctx->nodes[i];
        if ((n->flags & SM3D_NODE_FLAG_USED) == 0UL) continue;
        if (scene_id != 0 && n->scene_id != scene_id) continue;
        if ((n->flags & required_flags) != required_flags) continue;
        if (rejected_flags != 0UL && (n->flags & rejected_flags) != 0UL) continue;
        if (layer_mask != 0UL && (n->layer_mask & layer_mask) == 0UL) continue;
        h = sm3d_handle_make(i, n->generation);
        sm3d_qpush(result, h);
    }
    return result->overflow ? SM3D_ERR_FULL : SM3D_OK;
}

void sm3d_grid_clear(SM3D_Context *ctx)
{
    int i;
    if (ctx == 0) return;
    for (i = 0; i < SM3D_GRID_X * SM3D_GRID_Z; ++i) ctx->grid_heads[i] = SM3D_INVALID_INDEX;
    for (i = 0; i < SM3D_MAX_NODES; ++i) {
        ctx->nodes[i].grid_next = SM3D_INVALID_INDEX;
        ctx->nodes[i].grid_cell = SM3D_INVALID_INDEX;
    }
    ctx->grid_dirty = 0;
}

static int sm3d_grid_coord(SM3D_Context *ctx, sm3d_fx v, sm3d_fx origin, int maxv)
{
    sm3d_fx rel;
    int c;
    if (ctx->cfg.grid.cell_size == 0) return -1;
    rel = v - origin;
    c = sm3d_fx_to_int(sm3d_fx_div(rel, ctx->cfg.grid.cell_size));
    if (c < 0) return -1;
    if (c >= maxv) return -1;
    return c;
}

static int sm3d_grid_cell(SM3D_Context *ctx, SM3D_Vec3 p)
{
    int x;
    int z;
    x = sm3d_grid_coord(ctx, p.x, ctx->cfg.grid.origin_x, SM3D_GRID_X);
    z = sm3d_grid_coord(ctx, p.z, ctx->cfg.grid.origin_z, SM3D_GRID_Z);
    if (x < 0 || z < 0) return SM3D_INVALID_INDEX;
    return z * SM3D_GRID_X + x;
}

int sm3d_grid_rebuild(SM3D_Context *ctx, int scene_id)
{
    int i;
    int c;
    SM3D_Node *n;
    if (ctx == 0) return SM3D_ERR_BAD_ARGUMENT;
    if (!ctx->cfg.grid.enabled) return SM3D_OK;
    sm3d_grid_clear(ctx);
    for (i = 0; i < SM3D_MAX_NODES; ++i) {
        n = &ctx->nodes[i];
        if ((n->flags & SM3D_NODE_FLAG_USED) == 0UL) continue;
        if (scene_id != 0 && n->scene_id != scene_id) continue;
        if ((n->flags & SM3D_NODE_FLAG_ACTIVE) == 0UL) continue;
        c = sm3d_grid_cell(ctx, n->world.pos);
        if (c >= 0) {
            n->grid_cell = c;
            n->grid_next = ctx->grid_heads[c];
            ctx->grid_heads[c] = i;
        }
    }
    ctx->grid_dirty = 0;
    return SM3D_OK;
}

int sm3d_grid_query_aabb(SM3D_Context *ctx, int scene_id, const SM3D_Aabb *box, sm3d_u32 layer_mask, sm3d_u32 group_mask, SM3D_QueryResult *result)
{
    int x0;
    int x1;
    int z0;
    int z1;
    int x;
    int z;
    int cell;
    int ix;
    SM3D_Node *n;
    SM3D_Handle h;
    SM3D_Vec3 minp;
    SM3D_Vec3 maxp;

    if (ctx == 0 || box == 0 || result == 0 || result->out == 0) return SM3D_ERR_BAD_ARGUMENT;
    if (!ctx->cfg.grid.enabled) return sm3d_query_aabb(ctx, scene_id, box, layer_mask, group_mask, result);
    if (ctx->grid_dirty) sm3d_grid_rebuild(ctx, scene_id);
    result->count = 0;
    result->overflow = 0;

    minp.x = box->center.x - box->half.x;
    minp.y = box->center.y - box->half.y;
    minp.z = box->center.z - box->half.z;
    maxp.x = box->center.x + box->half.x;
    maxp.y = box->center.y + box->half.y;
    maxp.z = box->center.z + box->half.z;

    x0 = sm3d_grid_coord(ctx, minp.x, ctx->cfg.grid.origin_x, SM3D_GRID_X);
    x1 = sm3d_grid_coord(ctx, maxp.x, ctx->cfg.grid.origin_x, SM3D_GRID_X);
    z0 = sm3d_grid_coord(ctx, minp.z, ctx->cfg.grid.origin_z, SM3D_GRID_Z);
    z1 = sm3d_grid_coord(ctx, maxp.z, ctx->cfg.grid.origin_z, SM3D_GRID_Z);
    if (x0 < 0) x0 = 0;
    if (z0 < 0) z0 = 0;
    if (x1 < 0) x1 = SM3D_GRID_X - 1;
    if (z1 < 0) z1 = SM3D_GRID_Z - 1;
    if (x1 >= SM3D_GRID_X) x1 = SM3D_GRID_X - 1;
    if (z1 >= SM3D_GRID_Z) z1 = SM3D_GRID_Z - 1;

    for (z = z0; z <= z1; ++z) {
        for (x = x0; x <= x1; ++x) {
            cell = z * SM3D_GRID_X + x;
            ix = ctx->grid_heads[cell];
            while (ix >= 0) {
                n = &ctx->nodes[ix];
                if (scene_id == 0 || n->scene_id == scene_id) {
                    if ((n->flags & SM3D_NODE_FLAG_ACTIVE) != 0UL) {
                        if ((layer_mask == 0UL || (n->layer_mask & layer_mask) != 0UL) && (group_mask == 0UL || (n->group_mask & group_mask) != 0UL)) {
                            if (sm3d_aabb_overlap(&n->world_bounds, box)) {
                                h = sm3d_handle_make(ix, n->generation);
                                sm3d_qpush(result, h);
                            }
                        }
                    }
                }
                ix = n->grid_next;
            }
        }
    }
    return result->overflow ? SM3D_ERR_FULL : SM3D_OK;
}

/* ------------------------------------------------------------------------- */
/* scene3d89 v1.1 add-ons: requests, slots, deps, prefabs, portals, DAG.     */
/* ------------------------------------------------------------------------- */

static int sm3d_scene_root_handle(SM3D_Context *ctx, int scene_id, SM3D_Handle *out)
{
    int six;
    int root;
    if (ctx == 0 || out == 0) return SM3D_ERR_BAD_ARGUMENT;
    six = sm3d_scene_index_by_id(ctx, scene_id);
    if (six < 0) return SM3D_ERR_BAD_SCENE;
    root = ctx->scenes[six].root_node;
    if (root < 0 || root >= SM3D_MAX_NODES) return SM3D_ERR_BAD_HANDLE;
    *out = sm3d_handle_make(root, ctx->nodes[root].generation);
    return SM3D_OK;
}

int sm3d_scene_find_by_slot(SM3D_Context *ctx, int slot_type, int slot_index)
{
    int i;
    if (ctx == 0) return SM3D_INVALID_INDEX;
    for (i = 0; i < SM3D_MAX_SCENES; ++i) {
        if (ctx->scenes[i].id != 0UL) {
            if (ctx->scenes[i].slot_type == slot_type && ctx->scenes[i].slot_index == slot_index) {
                return (int)ctx->scenes[i].id;
            }
        }
    }
    return SM3D_INVALID_INDEX;
}

int sm3d_scene_set_slot(SM3D_Context *ctx, int scene_id, int slot_type, int slot_index)
{
    int six;
    int other;
    SM3D_Handle root;
    if (ctx == 0) return SM3D_ERR_BAD_ARGUMENT;
    six = sm3d_scene_index_by_id(ctx, scene_id);
    if (six < 0) return SM3D_ERR_BAD_SCENE;
    if (slot_type != SM3D_SCENE_SLOT_NONE) {
        other = sm3d_scene_find_by_slot(ctx, slot_type, slot_index);
        if (other != SM3D_INVALID_INDEX && other != scene_id) return SM3D_ERR_FULL;
    }
    ctx->scenes[six].slot_type = slot_type;
    ctx->scenes[six].slot_index = slot_index;
    if (slot_type == SM3D_SCENE_SLOT_PERSISTENT) ctx->scenes[six].flags |= SM3D_SCENE_FLAG_PERSISTENT;
    if (slot_type == SM3D_SCENE_SLOT_OVERLAY) ctx->scenes[six].flags |= SM3D_SCENE_FLAG_ADDITIVE;
    if (slot_type == SM3D_SCENE_SLOT_ROOM || slot_type == SM3D_SCENE_SLOT_WORLD_CELL) ctx->scenes[six].flags |= SM3D_SCENE_FLAG_STREAMABLE;
    if (sm3d_scene_root_handle(ctx, scene_id, &root) == SM3D_OK) sm3d_emit(ctx, SM3D_EVENT_SCENE_SLOT, root, scene_id);
    return SM3D_OK;
}

int sm3d_scene_create_in_slot(SM3D_Context *ctx, sm3d_u32 name_hash, sm3d_u32 flags, int slot_type, int slot_index, int *out_scene_id)
{
    int r;
    int scene_id;
    int six;
    r = sm3d_scene_create(ctx, name_hash, flags, &scene_id);
    if (r != SM3D_OK) return r;
    r = sm3d_scene_set_slot(ctx, scene_id, slot_type, slot_index);
    if (r != SM3D_OK) {
        sm3d_scene_destroy(ctx, scene_id);
        return r;
    }
    six = sm3d_scene_index_by_id(ctx, scene_id);
    if (six >= 0) ctx->scenes[six].stream_state = SM3D_SCENE_STREAM_READY;
    if (out_scene_id != 0) *out_scene_id = scene_id;
    return SM3D_OK;
}

static SM3D_SceneRequest *sm3d_request_alloc(SM3D_Context *ctx)
{
    int i;
    if (ctx == 0) return 0;
    for (i = 0; i < SM3D_MAX_SCENE_REQUESTS; ++i) {
        if (!ctx->requests[i].used) {
            memset(&ctx->requests[i], 0, sizeof(ctx->requests[i]));
            ctx->requests[i].used = 1;
            ctx->requests[i].id = ctx->next_request_id;
            ctx->next_request_id++;
            if (ctx->next_request_id <= 0) ctx->next_request_id = 1;
            ctx->requests[i].state = SM3D_SCENE_STREAM_REQUESTED;
            return &ctx->requests[i];
        }
    }
    return 0;
}

const SM3D_SceneRequest *sm3d_scene_request_get(const SM3D_Context *ctx, int request_id)
{
    int i;
    if (ctx == 0) return 0;
    for (i = 0; i < SM3D_MAX_SCENE_REQUESTS; ++i) {
        if (ctx->requests[i].used && ctx->requests[i].id == request_id) return &ctx->requests[i];
    }
    return 0;
}

int sm3d_scene_load_request(SM3D_Context *ctx, sm3d_u32 name_hash, int slot_type, int slot_index, sm3d_u32 flags, int *out_request_id)
{
    SM3D_SceneRequest *r;
    if (ctx == 0) return SM3D_ERR_BAD_ARGUMENT;
    r = sm3d_request_alloc(ctx);
    if (r == 0) return SM3D_ERR_REQUEST_FULL;
    r->op = SM3D_REQ_LOAD;
    r->name_hash = name_hash;
    r->slot_type = slot_type;
    r->slot_index = slot_index;
    r->flags = flags | SM3D_SCENE_FLAG_REQUESTED;
    if (out_request_id != 0) *out_request_id = r->id;
    sm3d_emit(ctx, SM3D_EVENT_SCENE_REQUESTED, sm3d_node_invalid(), 0);
    return SM3D_OK;
}

int sm3d_scene_unload_request(SM3D_Context *ctx, int scene_id, int *out_request_id)
{
    SM3D_SceneRequest *r;
    if (ctx == 0) return SM3D_ERR_BAD_ARGUMENT;
    if (sm3d_scene_index_by_id(ctx, scene_id) < 0) return SM3D_ERR_BAD_SCENE;
    r = sm3d_request_alloc(ctx);
    if (r == 0) return SM3D_ERR_REQUEST_FULL;
    r->op = SM3D_REQ_UNLOAD;
    r->scene_id = scene_id;
    r->state = SM3D_SCENE_STREAM_REQUESTED;
    if (out_request_id != 0) *out_request_id = r->id;
    sm3d_emit(ctx, SM3D_EVENT_SCENE_REQUESTED, sm3d_node_invalid(), scene_id);
    return SM3D_OK;
}

static int sm3d_scene_has_required_dependents(const SM3D_Context *ctx, int scene_id)
{
    int i;
    int from_ix;
    if (ctx == 0) return 0;
    for (i = 0; i < SM3D_MAX_SCENE_DEPS; ++i) {
        if (ctx->deps[i].used && ctx->deps[i].to_scene_id == scene_id) {
            if (ctx->deps[i].kind != SM3D_DEP_WEAK) {
                from_ix = sm3d_scene_index_by_id(ctx, ctx->deps[i].from_scene_id);
                if (from_ix >= 0 && (ctx->scenes[from_ix].flags & SM3D_SCENE_FLAG_LOADED) != 0UL) return 1;
            }
        }
    }
    return 0;
}

int sm3d_scene_process_requests(SM3D_Context *ctx, int max_ops)
{
    int i;
    int done;
    int rcode;
    int scene_id;
    int six;
    SM3D_SceneRequest *req;
    if (ctx == 0) return SM3D_ERR_BAD_ARGUMENT;
    done = 0;
    for (i = 0; i < SM3D_MAX_SCENE_REQUESTS; ++i) {
        if (max_ops > 0 && done >= max_ops) break;
        req = &ctx->requests[i];
        if (!req->used) continue;
        if (req->state != SM3D_SCENE_STREAM_REQUESTED) continue;
        rcode = SM3D_OK;
        if (req->op == SM3D_REQ_LOAD) {
            scene_id = sm3d_scene_find_by_hash(ctx, req->name_hash);
            if (scene_id != SM3D_INVALID_INDEX) {
                rcode = sm3d_scene_set_slot(ctx, scene_id, req->slot_type, req->slot_index);
                if (rcode == SM3D_OK) rcode = sm3d_scene_set_loaded(ctx, scene_id, 1);
            } else {
                rcode = sm3d_scene_create_in_slot(ctx, req->name_hash, req->flags & ~SM3D_SCENE_FLAG_REQUESTED, req->slot_type, req->slot_index, &scene_id);
            }
            if (rcode == SM3D_OK) {
                req->scene_id = scene_id;
                req->state = SM3D_REQ_DONE;
                six = sm3d_scene_index_by_id(ctx, scene_id);
                if (six >= 0) {
                    ctx->scenes[six].request_id = req->id;
                    ctx->scenes[six].stream_state = SM3D_SCENE_STREAM_READY;
                    ctx->scenes[six].flags &= ~SM3D_SCENE_FLAG_REQUESTED;
                }
            } else {
                req->state = SM3D_REQ_FAILED;
            }
        } else if (req->op == SM3D_REQ_UNLOAD) {
            if (sm3d_scene_has_required_dependents(ctx, req->scene_id)) {
                rcode = SM3D_ERR_DEP_BLOCKED;
                req->state = SM3D_REQ_FAILED;
            } else {
                six = sm3d_scene_index_by_id(ctx, req->scene_id);
                if (six >= 0) ctx->scenes[six].stream_state = SM3D_SCENE_STREAM_UNLOADING;
                rcode = sm3d_scene_destroy(ctx, req->scene_id);
                req->state = (rcode == SM3D_OK) ? SM3D_REQ_DONE : SM3D_REQ_FAILED;
            }
        } else {
            req->state = SM3D_REQ_FAILED;
            rcode = SM3D_ERR_BAD_ARGUMENT;
        }
        if (rcode != SM3D_OK) ctx->last_error = rcode;
        done++;
    }
    return SM3D_OK;
}

int sm3d_scene_dependency_add(SM3D_Context *ctx, int from_scene_id, int to_scene_id, int kind, sm3d_u32 flags, sm3d_u32 user_tag)
{
    int i;
    if (ctx == 0) return SM3D_ERR_BAD_ARGUMENT;
    if (sm3d_scene_index_by_id(ctx, from_scene_id) < 0) return SM3D_ERR_BAD_SCENE;
    if (sm3d_scene_index_by_id(ctx, to_scene_id) < 0) return SM3D_ERR_BAD_SCENE;
    for (i = 0; i < SM3D_MAX_SCENE_DEPS; ++i) {
        if (ctx->deps[i].used && ctx->deps[i].from_scene_id == from_scene_id && ctx->deps[i].to_scene_id == to_scene_id && ctx->deps[i].kind == kind) {
            ctx->deps[i].flags = flags;
            ctx->deps[i].user_tag = user_tag;
            sm3d_emit(ctx, SM3D_EVENT_SCENE_DEP, sm3d_node_invalid(), from_scene_id);
            return SM3D_OK;
        }
    }
    for (i = 0; i < SM3D_MAX_SCENE_DEPS; ++i) {
        if (!ctx->deps[i].used) {
            ctx->deps[i].used = 1;
            ctx->deps[i].from_scene_id = from_scene_id;
            ctx->deps[i].to_scene_id = to_scene_id;
            ctx->deps[i].kind = kind;
            ctx->deps[i].flags = flags;
            ctx->deps[i].user_tag = user_tag;
            sm3d_emit(ctx, SM3D_EVENT_SCENE_DEP, sm3d_node_invalid(), from_scene_id);
            return SM3D_OK;
        }
    }
    return SM3D_ERR_FULL;
}

int sm3d_scene_dependency_remove(SM3D_Context *ctx, int from_scene_id, int to_scene_id, int kind)
{
    int i;
    if (ctx == 0) return SM3D_ERR_BAD_ARGUMENT;
    for (i = 0; i < SM3D_MAX_SCENE_DEPS; ++i) {
        if (ctx->deps[i].used && ctx->deps[i].from_scene_id == from_scene_id && ctx->deps[i].to_scene_id == to_scene_id && ctx->deps[i].kind == kind) {
            memset(&ctx->deps[i], 0, sizeof(ctx->deps[i]));
            sm3d_emit(ctx, SM3D_EVENT_SCENE_DEP, sm3d_node_invalid(), from_scene_id);
            return SM3D_OK;
        }
    }
    return SM3D_ERR_NOT_FOUND;
}

int sm3d_scene_dependencies_loaded(const SM3D_Context *ctx, int scene_id)
{
    int i;
    int six;
    if (ctx == 0) return 0;
    if (sm3d_scene_index_by_id(ctx, scene_id) < 0) return 0;
    for (i = 0; i < SM3D_MAX_SCENE_DEPS; ++i) {
        if (ctx->deps[i].used && ctx->deps[i].from_scene_id == scene_id) {
            if (ctx->deps[i].kind == SM3D_DEP_WEAK) continue;
            six = sm3d_scene_index_by_id(ctx, ctx->deps[i].to_scene_id);
            if (six < 0) return 0;
            if ((ctx->scenes[six].flags & SM3D_SCENE_FLAG_LOADED) == 0UL) return 0;
        }
    }
    return 1;
}

int sm3d_scene_dependency_count(const SM3D_Context *ctx, int scene_id)
{
    int i;
    int count;
    if (ctx == 0) return 0;
    count = 0;
    for (i = 0; i < SM3D_MAX_SCENE_DEPS; ++i) if (ctx->deps[i].used && ctx->deps[i].from_scene_id == scene_id) count++;
    return count;
}

int sm3d_prefab_create(SM3D_Context *ctx, sm3d_u32 name_hash, sm3d_u32 flags, int *out_prefab_id)
{
    int i;
    if (ctx == 0) return SM3D_ERR_BAD_ARGUMENT;
    for (i = 0; i < SM3D_MAX_PREFABS; ++i) {
        if (!ctx->prefabs[i].used) {
            memset(&ctx->prefabs[i], 0, sizeof(ctx->prefabs[i]));
            ctx->prefabs[i].used = 1;
            ctx->prefabs[i].id = ctx->next_prefab_id;
            ctx->next_prefab_id++;
            if (ctx->next_prefab_id <= 0) ctx->next_prefab_id = 1;
            ctx->prefabs[i].name_hash = name_hash;
            ctx->prefabs[i].flags = flags;
            ctx->prefabs[i].first_node = SM3D_INVALID_INDEX;
            if (out_prefab_id != 0) *out_prefab_id = ctx->prefabs[i].id;
            return SM3D_OK;
        }
    }
    return SM3D_ERR_PREFAB_FULL;
}

static SM3D_Prefab *sm3d_prefab_by_id(SM3D_Context *ctx, int prefab_id)
{
    int i;
    if (ctx == 0) return 0;
    for (i = 0; i < SM3D_MAX_PREFABS; ++i) if (ctx->prefabs[i].used && ctx->prefabs[i].id == prefab_id) return &ctx->prefabs[i];
    return 0;
}

int sm3d_prefab_find_by_hash(const SM3D_Context *ctx, sm3d_u32 name_hash)
{
    int i;
    if (ctx == 0) return SM3D_INVALID_INDEX;
    for (i = 0; i < SM3D_MAX_PREFABS; ++i) if (ctx->prefabs[i].used && ctx->prefabs[i].name_hash == name_hash) return ctx->prefabs[i].id;
    return SM3D_INVALID_INDEX;
}

int sm3d_prefab_add_node(SM3D_Context *ctx, int prefab_id, const SM3D_PrefabNodeDesc *desc, int *out_template_index)
{
    SM3D_Prefab *pf;
    int i;
    int last;
    int cur;
    SM3D_PrefabNodeDesc d;
    if (ctx == 0 || desc == 0) return SM3D_ERR_BAD_ARGUMENT;
    pf = sm3d_prefab_by_id(ctx, prefab_id);
    if (pf == 0) return SM3D_ERR_NOT_FOUND;
    if (desc->parent_index >= pf->node_count) return SM3D_ERR_BAD_PARENT;
    for (i = 0; i < SM3D_MAX_PREFAB_NODES; ++i) {
        if (!ctx->prefab_node_used[i]) {
            d = *desc;
            d.next_index = SM3D_INVALID_INDEX;
            if (d.layer_mask == 0UL) d.layer_mask = ctx->cfg.default_layer_mask;
            ctx->prefab_nodes[i] = d;
            ctx->prefab_node_used[i] = 1;
            if (pf->first_node < 0) pf->first_node = i;
            else {
                last = pf->first_node;
                cur = ctx->prefab_nodes[last].next_index;
                while (cur >= 0) {
                    last = cur;
                    cur = ctx->prefab_nodes[cur].next_index;
                }
                ctx->prefab_nodes[last].next_index = i;
            }
            if (out_template_index != 0) *out_template_index = pf->node_count;
            pf->node_count++;
            return SM3D_OK;
        }
    }
    return SM3D_ERR_PREFAB_FULL;
}

int sm3d_prefab_instantiate(SM3D_Context *ctx, int prefab_id, int scene_id, SM3D_Handle parent, const SM3D_Transform *base_transform, SM3D_Handle *out_root)
{
    SM3D_Prefab *pf;
    int storage[SM3D_MAX_PREFAB_NODES];
    SM3D_Handle created[SM3D_MAX_PREFAB_NODES];
    int count;
    int cur;
    int i;
    int r;
    SM3D_Handle p;
    SM3D_Transform t;
    SM3D_PrefabNodeDesc *d;
    if (ctx == 0) return SM3D_ERR_BAD_ARGUMENT;
    pf = sm3d_prefab_by_id(ctx, prefab_id);
    if (pf == 0) return SM3D_ERR_NOT_FOUND;
    if (pf->node_count <= 0) return SM3D_ERR_BAD_ARGUMENT;
    count = 0;
    cur = pf->first_node;
    while (cur >= 0 && count < SM3D_MAX_PREFAB_NODES) {
        storage[count] = cur;
        created[count] = sm3d_node_invalid();
        count++;
        cur = ctx->prefab_nodes[cur].next_index;
    }
    if (count != pf->node_count) return SM3D_ERR_BAD_ARGUMENT;
    for (i = 0; i < count; ++i) {
        d = &ctx->prefab_nodes[storage[i]];
        if (d->parent_index < 0) p = parent;
        else {
            if (d->parent_index >= i) return SM3D_ERR_BAD_PARENT;
            p = created[d->parent_index];
        }
        r = sm3d_node_create(ctx, scene_id, p, d->type, d->name_hash, &created[i]);
        if (r != SM3D_OK) return r;
        t = d->local;
        if (base_transform != 0 && d->parent_index < 0) {
            t.pos = sm3d_vec3_add(base_transform->pos, d->local.pos);
            t.rot = sm3d_vec3_add(base_transform->rot, d->local.rot);
            t.scale = sm3d_vec3_mul(base_transform->scale, d->local.scale);
        }
        sm3d_node_set_local_transform(ctx, created[i], &t);
        sm3d_node_set_masks(ctx, created[i], d->layer_mask, d->group_mask);
        sm3d_node_set_user_ids(ctx, created[i], d->resource_id, d->archetype_id, d->user_id);
        sm3d_node_set_bounds(ctx, created[i], &d->local_bounds);
        if (d->flags != 0UL) sm3d_node_set_flags(ctx, created[i], d->flags, 1);
    }
    if (out_root != 0) *out_root = created[0];
    sm3d_emit(ctx, SM3D_EVENT_PREFAB_INSTANTIATED, created[0], scene_id);
    return SM3D_OK;
}

int sm3d_portal_hint_add(SM3D_Context *ctx, int scene_id, SM3D_Handle portal_node, SM3D_Handle from_sector, SM3D_Handle to_sector, sm3d_u32 flags, sm3d_u32 layer_mask, sm3d_u32 user_tag)
{
    int i;
    if (ctx == 0) return SM3D_ERR_BAD_ARGUMENT;
    if (sm3d_scene_index_by_id(ctx, scene_id) < 0) return SM3D_ERR_BAD_SCENE;
    if (sm3d_node_resolve(ctx, portal_node) < 0) return SM3D_ERR_BAD_HANDLE;
    if (sm3d_node_resolve(ctx, from_sector) < 0) return SM3D_ERR_BAD_HANDLE;
    if (sm3d_node_resolve(ctx, to_sector) < 0) return SM3D_ERR_BAD_HANDLE;
    for (i = 0; i < SM3D_MAX_PORTALS; ++i) {
        if (!ctx->portals[i].used || ctx->portals[i].portal_node.value == portal_node.value) {
            ctx->portals[i].used = 1;
            ctx->portals[i].scene_id = scene_id;
            ctx->portals[i].portal_node = portal_node;
            ctx->portals[i].from_sector = from_sector;
            ctx->portals[i].to_sector = to_sector;
            ctx->portals[i].flags = flags;
            ctx->portals[i].layer_mask = layer_mask;
            ctx->portals[i].user_tag = user_tag;
            sm3d_emit(ctx, SM3D_EVENT_PORTAL_CHANGED, portal_node, scene_id);
            return SM3D_OK;
        }
    }
    return SM3D_ERR_FULL;
}

int sm3d_portal_hint_remove(SM3D_Context *ctx, SM3D_Handle portal_node)
{
    int i;
    if (ctx == 0) return SM3D_ERR_BAD_ARGUMENT;
    for (i = 0; i < SM3D_MAX_PORTALS; ++i) {
        if (ctx->portals[i].used && ctx->portals[i].portal_node.value == portal_node.value) {
            memset(&ctx->portals[i], 0, sizeof(ctx->portals[i]));
            sm3d_emit(ctx, SM3D_EVENT_PORTAL_CHANGED, portal_node, 0);
            return SM3D_OK;
        }
    }
    return SM3D_ERR_NOT_FOUND;
}

static int sm3d_qcontains(const SM3D_QueryResult *r, SM3D_Handle h)
{
    int i;
    if (r == 0) return 0;
    for (i = 0; i < r->count; ++i) if (r->out[i].value == h.value) return 1;
    return 0;
}

int sm3d_portal_collect_visible(SM3D_Context *ctx, int scene_id, SM3D_Handle start_sector, int max_depth, SM3D_QueryResult *result)
{
    SM3D_Handle queue[SM3D_MAX_STACK];
    int depths[SM3D_MAX_STACK];
    int head;
    int tail;
    int i;
    SM3D_Handle cur;
    SM3D_Handle next;
    if (ctx == 0 || result == 0 || result->out == 0) return SM3D_ERR_BAD_ARGUMENT;
    if (sm3d_node_resolve(ctx, start_sector) < 0) return SM3D_ERR_BAD_HANDLE;
    result->count = 0;
    result->overflow = 0;
    head = 0;
    tail = 0;
    queue[tail] = start_sector;
    depths[tail] = 0;
    tail++;
    sm3d_qpush(result, start_sector);
    while (head < tail) {
        cur = queue[head];
        for (i = 0; i < SM3D_MAX_PORTALS; ++i) {
            if (!ctx->portals[i].used) continue;
            if (scene_id != 0 && ctx->portals[i].scene_id != scene_id) continue;
            if ((ctx->portals[i].flags & SM3D_PORTAL_FLAG_OPEN) == 0UL) continue;
            next = sm3d_node_invalid();
            if (ctx->portals[i].from_sector.value == cur.value) next = ctx->portals[i].to_sector;
            else if ((ctx->portals[i].flags & SM3D_PORTAL_FLAG_ONE_WAY) == 0UL && ctx->portals[i].to_sector.value == cur.value) next = ctx->portals[i].from_sector;
            if (!sm3d_handle_is_valid(next)) continue;
            if (sm3d_qcontains(result, next)) continue;
            sm3d_qpush(result, next);
            if (result->overflow) return SM3D_ERR_FULL;
            if ((max_depth < 0 || depths[head] + 1 <= max_depth) && tail < SM3D_MAX_STACK) {
                queue[tail] = next;
                depths[tail] = depths[head] + 1;
                tail++;
            }
        }
        head++;
    }
    return result->overflow ? SM3D_ERR_FULL : SM3D_OK;
}

void sm3d_dirty_clear(SM3D_Context *ctx)
{
    int i;
    int ix;
    if (ctx == 0) return;
    for (i = 0; i < ctx->dirty_count; ++i) {
        ix = ctx->dirty_nodes[i];
        if (ix >= 0 && ix < SM3D_MAX_NODES) ctx->nodes[ix].flags &= ~SM3D_NODE_FLAG_DIRTY_QUEUED;
    }
    ctx->dirty_count = 0;
    ctx->dirty_overflow = 0;
}

static int sm3d_update_subtree_from(SM3D_Context *ctx, int root_ix)
{
    int stack[SM3D_MAX_STACK];
    int top;
    int cur;
    int child;
    int parent_dirty;
    SM3D_Node *n;
    SM3D_Node *p;
    if (ctx == 0) return SM3D_ERR_BAD_ARGUMENT;
    if (root_ix < 0 || root_ix >= SM3D_MAX_NODES) return SM3D_ERR_BAD_HANDLE;
    top = 0;
    stack[top++] = root_ix;
    while (top > 0) {
        top--;
        cur = stack[top];
        n = &ctx->nodes[cur];
        if ((n->flags & SM3D_NODE_FLAG_USED) == 0UL) continue;
        p = 0;
        if (n->parent >= 0) p = &ctx->nodes[n->parent];
        parent_dirty = ((n->flags & (SM3D_NODE_FLAG_DIRTY_LOCAL | SM3D_NODE_FLAG_DIRTY_WORLD | SM3D_NODE_FLAG_DIRTY_BOUNDS)) != 0UL);
        if (parent_dirty) sm3d_compose_world(n, p);
        n->flags &= ~SM3D_NODE_FLAG_DIRTY_QUEUED;
        child = n->last_child;
        while (child >= 0) {
            if (top >= SM3D_MAX_STACK) return SM3D_ERR_STACK_OVERFLOW;
            if (parent_dirty) ctx->nodes[child].flags |= SM3D_NODE_FLAG_DIRTY_WORLD;
            stack[top++] = child;
            child = ctx->nodes[child].prev_sibling;
        }
    }
    return SM3D_OK;
}

int sm3d_update_dirty_transforms(SM3D_Context *ctx)
{
    int i;
    int ix;
    int r;
    if (ctx == 0) return SM3D_ERR_BAD_ARGUMENT;
    if (ctx->dirty_overflow) {
        sm3d_dirty_clear(ctx);
        return sm3d_update_transforms(ctx);
    }
    for (i = 0; i < ctx->dirty_count; ++i) {
        ix = ctx->dirty_nodes[i];
        if (ix < 0 || ix >= SM3D_MAX_NODES) continue;
        if ((ctx->nodes[ix].flags & SM3D_NODE_FLAG_USED) == 0UL) continue;
        r = sm3d_update_subtree_from(ctx, ix);
        if (r != SM3D_OK) {
            sm3d_dirty_clear(ctx);
            return r;
        }
    }
    sm3d_dirty_clear(ctx);
    if (ctx->cfg.auto_update_grid && ctx->cfg.grid.enabled && ctx->grid_dirty) sm3d_grid_rebuild(ctx, 0);
    return SM3D_OK;
}

int sm3d_traverse_dirty(SM3D_Context *ctx, int scene_id, sm3d_u32 layer_mask, SM3D_VisitFn fn, void *user)
{
    int i;
    int ix;
    SM3D_Node *n;
    SM3D_Handle h;
    int vr;
    if (ctx == 0 || fn == 0) return SM3D_ERR_BAD_ARGUMENT;
    sm3d_begin_read(ctx);
    for (i = 0; i < ctx->dirty_count; ++i) {
        ix = ctx->dirty_nodes[i];
        if (ix < 0 || ix >= SM3D_MAX_NODES) continue;
        n = &ctx->nodes[ix];
        if ((n->flags & SM3D_NODE_FLAG_USED) == 0UL) continue;
        if (scene_id != 0 && n->scene_id != scene_id) continue;
        if (layer_mask != 0UL && (n->layer_mask & layer_mask) == 0UL) continue;
        h = sm3d_handle_make(ix, n->generation);
        vr = fn(ctx, h, n, user);
        if (vr == SM3D_VISIT_STOP) break;
    }
    return sm3d_end_read(ctx);
}

static int sm3d_dag_link_matches(const SM3D_DagLink *l, int kind)
{
    if (l == 0 || !l->used) return 0;
    if (kind >= 0 && l->kind != kind) return 0;
    return 1;
}

static int sm3d_dag_reaches(SM3D_Context *ctx, SM3D_Handle start, SM3D_Handle target, int kind)
{
    SM3D_Handle stack[SM3D_MAX_STACK];
    SM3D_Handle visited[SM3D_MAX_STACK];
    int top;
    int vcount;
    int i;
    int j;
    int seen;
    SM3D_Handle cur;
    top = 0;
    vcount = 0;
    stack[top++] = start;
    while (top > 0) {
        top--;
        cur = stack[top];
        if (cur.value == target.value) return 1;
        seen = 0;
        for (i = 0; i < vcount; ++i) if (visited[i].value == cur.value) seen = 1;
        if (seen) continue;
        if (vcount < SM3D_MAX_STACK) visited[vcount++] = cur;
        for (j = 0; j < SM3D_MAX_DAG_LINKS; ++j) {
            if (!sm3d_dag_link_matches(&ctx->dag_links[j], kind)) continue;
            if (ctx->dag_links[j].from_node.value == cur.value) {
                if (top >= SM3D_MAX_STACK) return 0;
                stack[top++] = ctx->dag_links[j].to_node;
            }
        }
    }
    return 0;
}

int sm3d_dag_link_add(SM3D_Context *ctx, SM3D_Handle from_node, SM3D_Handle to_node, int kind, sm3d_u32 flags, sm3d_u32 user_tag)
{
    int i;
    if (ctx == 0) return SM3D_ERR_BAD_ARGUMENT;
    if (sm3d_node_resolve(ctx, from_node) < 0 || sm3d_node_resolve(ctx, to_node) < 0) return SM3D_ERR_BAD_HANDLE;
    if ((flags & SM3D_DAG_FLAG_ACYCLIC) != 0UL) {
        if (sm3d_dag_reaches(ctx, to_node, from_node, kind)) return SM3D_ERR_DAG_CYCLE;
    }
    for (i = 0; i < SM3D_MAX_DAG_LINKS; ++i) {
        if (ctx->dag_links[i].used && ctx->dag_links[i].from_node.value == from_node.value && ctx->dag_links[i].to_node.value == to_node.value && ctx->dag_links[i].kind == kind) {
            ctx->dag_links[i].flags = flags;
            ctx->dag_links[i].user_tag = user_tag;
            sm3d_emit(ctx, SM3D_EVENT_DAG_LINK, from_node, ctx->nodes[sm3d_handle_index(from_node)].scene_id);
            return SM3D_OK;
        }
    }
    for (i = 0; i < SM3D_MAX_DAG_LINKS; ++i) {
        if (!ctx->dag_links[i].used) {
            ctx->dag_links[i].used = 1;
            ctx->dag_links[i].from_node = from_node;
            ctx->dag_links[i].to_node = to_node;
            ctx->dag_links[i].kind = kind;
            ctx->dag_links[i].flags = flags;
            ctx->dag_links[i].user_tag = user_tag;
            sm3d_emit(ctx, SM3D_EVENT_DAG_LINK, from_node, ctx->nodes[sm3d_handle_index(from_node)].scene_id);
            return SM3D_OK;
        }
    }
    return SM3D_ERR_FULL;
}

int sm3d_dag_link_remove(SM3D_Context *ctx, SM3D_Handle from_node, SM3D_Handle to_node, int kind)
{
    int i;
    if (ctx == 0) return SM3D_ERR_BAD_ARGUMENT;
    for (i = 0; i < SM3D_MAX_DAG_LINKS; ++i) {
        if (ctx->dag_links[i].used && ctx->dag_links[i].from_node.value == from_node.value && ctx->dag_links[i].to_node.value == to_node.value && ctx->dag_links[i].kind == kind) {
            memset(&ctx->dag_links[i], 0, sizeof(ctx->dag_links[i]));
            sm3d_emit(ctx, SM3D_EVENT_DAG_LINK, from_node, 0);
            return SM3D_OK;
        }
    }
    return SM3D_ERR_NOT_FOUND;
}

int sm3d_dag_collect_outputs(SM3D_Context *ctx, SM3D_Handle from_node, int kind, SM3D_QueryResult *result)
{
    int i;
    if (ctx == 0 || result == 0 || result->out == 0) return SM3D_ERR_BAD_ARGUMENT;
    if (sm3d_node_resolve(ctx, from_node) < 0) return SM3D_ERR_BAD_HANDLE;
    result->count = 0;
    result->overflow = 0;
    for (i = 0; i < SM3D_MAX_DAG_LINKS; ++i) {
        if (!sm3d_dag_link_matches(&ctx->dag_links[i], kind)) continue;
        if (ctx->dag_links[i].from_node.value == from_node.value) sm3d_qpush(result, ctx->dag_links[i].to_node);
    }
    return result->overflow ? SM3D_ERR_FULL : SM3D_OK;
}

int sm3d_dag_collect_inputs(SM3D_Context *ctx, SM3D_Handle to_node, int kind, SM3D_QueryResult *result)
{
    int i;
    if (ctx == 0 || result == 0 || result->out == 0) return SM3D_ERR_BAD_ARGUMENT;
    if (sm3d_node_resolve(ctx, to_node) < 0) return SM3D_ERR_BAD_HANDLE;
    result->count = 0;
    result->overflow = 0;
    for (i = 0; i < SM3D_MAX_DAG_LINKS; ++i) {
        if (!sm3d_dag_link_matches(&ctx->dag_links[i], kind)) continue;
        if (ctx->dag_links[i].to_node.value == to_node.value) sm3d_qpush(result, ctx->dag_links[i].from_node);
    }
    return result->overflow ? SM3D_ERR_FULL : SM3D_OK;
}

int sm3d_dag_toposort(SM3D_Context *ctx, int scene_id, int kind, SM3D_QueryResult *result)
{
    int present[SM3D_MAX_NODES];
    int indeg[SM3D_MAX_NODES];
    int queue[SM3D_MAX_NODES];
    int head;
    int tail;
    int i;
    int from_ix;
    int to_ix;
    int emitted;
    int present_count;
    SM3D_Handle h;
    if (ctx == 0 || result == 0 || result->out == 0) return SM3D_ERR_BAD_ARGUMENT;
    memset(present, 0, sizeof(present));
    memset(indeg, 0, sizeof(indeg));
    result->count = 0;
    result->overflow = 0;
    present_count = 0;
    for (i = 0; i < SM3D_MAX_NODES; ++i) {
        if ((ctx->nodes[i].flags & SM3D_NODE_FLAG_USED) != 0UL) {
            if (scene_id == 0 || ctx->nodes[i].scene_id == scene_id) {
                present[i] = 1;
                present_count++;
            }
        }
    }
    for (i = 0; i < SM3D_MAX_DAG_LINKS; ++i) {
        if (!sm3d_dag_link_matches(&ctx->dag_links[i], kind)) continue;
        from_ix = sm3d_node_resolve(ctx, ctx->dag_links[i].from_node);
        to_ix = sm3d_node_resolve(ctx, ctx->dag_links[i].to_node);
        if (from_ix < 0 || to_ix < 0) continue;
        if (!present[from_ix] || !present[to_ix]) continue;
        indeg[to_ix]++;
    }
    head = 0;
    tail = 0;
    for (i = 0; i < SM3D_MAX_NODES; ++i) if (present[i] && indeg[i] == 0) queue[tail++] = i;
    emitted = 0;
    while (head < tail) {
        from_ix = queue[head++];
        h = sm3d_handle_make(from_ix, ctx->nodes[from_ix].generation);
        sm3d_qpush(result, h);
        if (result->overflow) return SM3D_ERR_FULL;
        emitted++;
        for (i = 0; i < SM3D_MAX_DAG_LINKS; ++i) {
            if (!sm3d_dag_link_matches(&ctx->dag_links[i], kind)) continue;
            if (ctx->dag_links[i].from_node.value != h.value) continue;
            to_ix = sm3d_node_resolve(ctx, ctx->dag_links[i].to_node);
            if (to_ix < 0 || !present[to_ix]) continue;
            indeg[to_ix]--;
            if (indeg[to_ix] == 0 && tail < SM3D_MAX_NODES) queue[tail++] = to_ix;
        }
    }
    if (emitted != present_count) return SM3D_ERR_DAG_CYCLE;
    return result->overflow ? SM3D_ERR_FULL : SM3D_OK;
}

void sm3d_manifest_config_defaults(SM3D_ManifestConfig *cfg)
{
    if (cfg == 0) return;
    cfg->auto_process_requests = 1;
    cfg->default_slot_type = SM3D_SCENE_SLOT_LEVEL;
    cfg->default_scene_flags = SM3D_SCENE_FLAG_ADDITIVE;
}

static int sm3d_manifest_slot(const char *s)
{
    if (s == 0) return SM3D_SCENE_SLOT_LEVEL;
    if (strcmp(s, "persistent") == 0) return SM3D_SCENE_SLOT_PERSISTENT;
    if (strcmp(s, "level") == 0) return SM3D_SCENE_SLOT_LEVEL;
    if (strcmp(s, "room") == 0) return SM3D_SCENE_SLOT_ROOM;
    if (strcmp(s, "overlay") == 0) return SM3D_SCENE_SLOT_OVERLAY;
    if (strcmp(s, "world_cell") == 0) return SM3D_SCENE_SLOT_WORLD_CELL;
    if (strcmp(s, "none") == 0) return SM3D_SCENE_SLOT_NONE;
    return (int)strtol(s, 0, 0);
}

static int sm3d_manifest_dep_kind(const char *s)
{
    if (s == 0) return SM3D_DEP_REQUIRED;
    if (strcmp(s, "required") == 0) return SM3D_DEP_REQUIRED;
    if (strcmp(s, "weak") == 0) return SM3D_DEP_WEAK;
    if (strcmp(s, "load_before") == 0) return SM3D_DEP_LOAD_BEFORE;
    if (strcmp(s, "visibility") == 0) return SM3D_DEP_VISIBILITY;
    return (int)strtol(s, 0, 0);
}

static sm3d_u32 sm3d_manifest_u32(const char *s, sm3d_u32 defv)
{
    if (s == 0 || s[0] == 0) return defv;
    return (sm3d_u32)strtoul(s, 0, 0);
}

static int sm3d_manifest_tokenize(const char *line, char tok[SM3D_MAX_MANIFEST_TOKENS][64])
{
    int count;
    int pos;
    int out;
    char c;
    count = 0;
    pos = 0;
    out = 0;
    while (line[pos] != 0) {
        while (line[pos] == ' ' || line[pos] == '\t' || line[pos] == '\r' || line[pos] == '\n') pos++;
        if (line[pos] == 0 || line[pos] == '#' || line[pos] == ';') break;
        if (count >= SM3D_MAX_MANIFEST_TOKENS) break;
        out = 0;
        while (line[pos] != 0 && line[pos] != ' ' && line[pos] != '\t' && line[pos] != '\r' && line[pos] != '\n') {
            c = line[pos];
            if (c == '#' || c == ';') break;
            if (out < 63) tok[count][out++] = c;
            pos++;
        }
        tok[count][out] = 0;
        count++;
        if (line[pos] == '#' || line[pos] == ';') break;
    }
    return count;
}

int sm3d_manifest_read_text(SM3D_Context *ctx, const char *text, const SM3D_ManifestConfig *cfg_in)
{
    SM3D_ManifestConfig defcfg;
    const SM3D_ManifestConfig *cfg;
    char line[256];
    char tok[SM3D_MAX_MANIFEST_TOKENS][64];
    int lp;
    int tp;
    int n;
    int r;
    int scene_id;
    int from_id;
    int to_id;
    int req_id;
    sm3d_u32 name_hash;
    int slot_type;
    int slot_index;
    sm3d_u32 flags;
    if (ctx == 0 || text == 0) return SM3D_ERR_BAD_ARGUMENT;
    sm3d_manifest_config_defaults(&defcfg);
    cfg = (cfg_in != 0) ? cfg_in : &defcfg;
    tp = 0;
    r = SM3D_OK;
    while (1) {
        lp = 0;
        while (text[tp] != 0 && text[tp] != '\n') {
            if (lp < 255) line[lp++] = text[tp];
            tp++;
        }
        line[lp] = 0;
        if (text[tp] == '\n') tp++;
        n = sm3d_manifest_tokenize(line, tok);
        if (n > 0) {
            if (strcmp(tok[0], "scene") == 0) {
                if (n < 2) return SM3D_ERR_PARSE;
                name_hash = sm3d_hash_cstr(tok[1]);
                slot_type = (n >= 3) ? sm3d_manifest_slot(tok[2]) : cfg->default_slot_type;
                slot_index = (n >= 4) ? (int)strtol(tok[3], 0, 0) : 0;
                flags = (n >= 5) ? sm3d_manifest_u32(tok[4], cfg->default_scene_flags) : cfg->default_scene_flags;
                if (sm3d_scene_find_by_hash(ctx, name_hash) == SM3D_INVALID_INDEX) r = sm3d_scene_create_in_slot(ctx, name_hash, flags, slot_type, slot_index, &scene_id);
            } else if (strcmp(tok[0], "load") == 0) {
                if (n < 2) return SM3D_ERR_PARSE;
                name_hash = sm3d_hash_cstr(tok[1]);
                slot_type = (n >= 3) ? sm3d_manifest_slot(tok[2]) : cfg->default_slot_type;
                slot_index = (n >= 4) ? (int)strtol(tok[3], 0, 0) : 0;
                flags = (n >= 5) ? sm3d_manifest_u32(tok[4], cfg->default_scene_flags) : cfg->default_scene_flags;
                r = sm3d_scene_load_request(ctx, name_hash, slot_type, slot_index, flags, &req_id);
            } else if (strcmp(tok[0], "unload") == 0) {
                if (n < 2) return SM3D_ERR_PARSE;
                scene_id = sm3d_scene_find_by_hash(ctx, sm3d_hash_cstr(tok[1]));
                if (scene_id < 0) return SM3D_ERR_BAD_SCENE;
                r = sm3d_scene_unload_request(ctx, scene_id, &req_id);
            } else if (strcmp(tok[0], "dep") == 0) {
                if (n < 3) return SM3D_ERR_PARSE;
                from_id = sm3d_scene_find_by_hash(ctx, sm3d_hash_cstr(tok[1]));
                to_id = sm3d_scene_find_by_hash(ctx, sm3d_hash_cstr(tok[2]));
                if (from_id < 0 || to_id < 0) return SM3D_ERR_BAD_SCENE;
                r = sm3d_scene_dependency_add(ctx, from_id, to_id, (n >= 4) ? sm3d_manifest_dep_kind(tok[3]) : SM3D_DEP_REQUIRED, (n >= 5) ? sm3d_manifest_u32(tok[4], 0UL) : 0UL, 0UL);
            } else if (strcmp(tok[0], "process") == 0) {
                r = sm3d_scene_process_requests(ctx, 0);
            } else {
                return SM3D_ERR_PARSE;
            }
            if (r != SM3D_OK) return r;
        }
        if (text[tp] == 0) break;
    }
    if (cfg->auto_process_requests) return sm3d_scene_process_requests(ctx, 0);
    return SM3D_OK;
}
