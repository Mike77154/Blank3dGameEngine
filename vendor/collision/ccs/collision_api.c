#include "collision_api.h"

#define COLLISION_MAX_ITEMS 256

typedef struct {
    ccs_vec3 position;

    union {
        ccs_shape_box box;
        ccs_shape_sphere sphere;
        ccs_shape_capsule capsule;
        ccs_shape_obb obb;
    } shape;

    ccs_shape* shape_ptr;
} collision_item;

static collision_item g_items[COLLISION_MAX_ITEMS];
static int g_item_count = 0;

void collision_reset(void)
{
    int i;

    ccs_world_clear();

    for (i = 0; i < COLLISION_MAX_ITEMS; ++i) {
        g_items[i].position = ccs_vec3_zero();
        g_items[i].shape_ptr = 0;
    }

    g_item_count = 0;
}

static int alloc_item(void)
{
    if (g_item_count >= COLLISION_MAX_ITEMS)
        return -1;
    return g_item_count++;
}

int collision_create_box_fixed(
    ccs_fixed x, ccs_fixed y, ccs_fixed z,
    ccs_fixed hx, ccs_fixed hy, ccs_fixed hz,
    unsigned int layer
) {
    int idx;
    int id;
    collision_item* it;

    idx = alloc_item();
    if (idx < 0)
        return -1;

    it = &g_items[idx];

    it->position = ccs_vec3_make(x, y, z);

    it->shape.box.header.type = CCS_SHAPE_BOX;
    it->shape.box.header.flags = 0;
    it->shape.box.center = it->position;
    it->shape.box.half_extents = ccs_vec3_make(hx, hy, hz);

    it->shape_ptr = (ccs_shape*)&it->shape.box;

    id = ccs_world_add(&it->position, it->shape_ptr);
    if (id >= 0) {
        ccs_bodies[id].layer = (layer == 0) ? 1u : (ccs_u32)layer;
        ccs_bodies[id].mask = 0xFFFFFFFFu;
    }

    return id;
}

int collision_create_sphere_fixed(
    ccs_fixed x, ccs_fixed y, ccs_fixed z,
    ccs_fixed radius,
    unsigned int layer
) {
    int idx;
    int id;
    collision_item* it;

    idx = alloc_item();
    if (idx < 0)
        return -1;

    it = &g_items[idx];

    it->position = ccs_vec3_make(x, y, z);

    it->shape.sphere.header.type = CCS_SHAPE_SPHERE;
    it->shape.sphere.header.flags = 0;
    it->shape.sphere.center = it->position;
    it->shape.sphere.radius = radius;

    it->shape_ptr = (ccs_shape*)&it->shape.sphere;

    id = ccs_world_add(&it->position, it->shape_ptr);
    if (id >= 0) {
        ccs_bodies[id].layer = (layer == 0) ? 1u : (ccs_u32)layer;
        ccs_bodies[id].mask = 0xFFFFFFFFu;
    }

    return id;
}

int collision_create_capsule_fixed(
    ccs_fixed x, ccs_fixed y, ccs_fixed z,
    ccs_fixed ax, ccs_fixed ay, ccs_fixed az,
    ccs_fixed half_height,
    ccs_fixed radius,
    unsigned int layer
) {
    int idx;
    int id;
    collision_item* it;

    idx = alloc_item();
    if (idx < 0)
        return -1;

    it = &g_items[idx];

    it->position = ccs_vec3_make(x, y, z);

    it->shape.capsule.header.type = CCS_SHAPE_CAPSULE;
    it->shape.capsule.header.flags = 0;
    it->shape.capsule.center = it->position;
    it->shape.capsule.axis = ccs_vec3_make(ax, ay, az);
    it->shape.capsule.half_height = half_height;
    it->shape.capsule.radius = radius;

    it->shape_ptr = (ccs_shape*)&it->shape.capsule;

    id = ccs_world_add(&it->position, it->shape_ptr);
    if (id >= 0) {
        ccs_bodies[id].layer = (layer == 0) ? 1u : (ccs_u32)layer;
        ccs_bodies[id].mask = 0xFFFFFFFFu;
    }

    return id;
}

int collision_create_obb_fixed(
    ccs_fixed x, ccs_fixed y, ccs_fixed z,
    ccs_fixed hx, ccs_fixed hy, ccs_fixed hz,
    ccs_fixed ax0, ccs_fixed ay0, ccs_fixed az0,
    ccs_fixed ax1, ccs_fixed ay1, ccs_fixed az1,
    ccs_fixed ax2, ccs_fixed ay2, ccs_fixed az2,
    unsigned int layer
) {
    int idx;
    int id;
    collision_item* it;

    idx = alloc_item();
    if (idx < 0)
        return -1;

    it = &g_items[idx];

    it->position = ccs_vec3_make(x, y, z);

    it->shape.obb.header.type = CCS_SHAPE_OBB;
    it->shape.obb.header.flags = 0;
    it->shape.obb.center = it->position;
    it->shape.obb.half_extents = ccs_vec3_make(hx, hy, hz);
    it->shape.obb.axis[0] = ccs_vec3_make(ax0, ay0, az0);
    it->shape.obb.axis[1] = ccs_vec3_make(ax1, ay1, az1);
    it->shape.obb.axis[2] = ccs_vec3_make(ax2, ay2, az2);

    it->shape_ptr = (ccs_shape*)&it->shape.obb;

    id = ccs_world_add(&it->position, it->shape_ptr);
    if (id >= 0) {
        ccs_bodies[id].layer = (layer == 0) ? 1u : (ccs_u32)layer;
        ccs_bodies[id].mask = 0xFFFFFFFFu;
    }

    return id;
}

void collision_set_position_fixed(int id, ccs_fixed x, ccs_fixed y, ccs_fixed z)
{
    if (id < 0 || id >= ccs_body_count)
        return;

    if (!ccs_bodies[id].position)
        return;

    *ccs_bodies[id].position = ccs_vec3_make(x, y, z);
    if (ccs_bodies[id].shape)
        ccs_shape_set_center(ccs_bodies[id].shape, *ccs_bodies[id].position);
}

void collision_step(void)
{
    ccs_world_step();
}

int collision_raycast_fixed(
    ccs_fixed ox, ccs_fixed oy, ccs_fixed oz,
    ccs_fixed dx, ccs_fixed dy, ccs_fixed dz,
    ccs_fixed tmin, ccs_fixed tmax,
    unsigned int layer_mask,
    int* out_id,
    ccs_fixed* out_t,
    ccs_fixed* out_px, ccs_fixed* out_py, ccs_fixed* out_pz,
    ccs_fixed* out_nx, ccs_fixed* out_ny, ccs_fixed* out_nz
) {
    ccs_ray ray;
    ccs_raycast_hit hit;
    int body_id;

    ray = ccs_ray_make(ccs_vec3_make(ox, oy, oz), ccs_vec3_make(dx, dy, dz), tmin, tmax);

    if (!ccs_world_raycast(&ray, (ccs_u32)layer_mask, &hit, &body_id))
        return 0;

    if (out_id) *out_id = body_id;
    if (out_t) *out_t = hit.t;

    if (out_px) *out_px = hit.point.x;
    if (out_py) *out_py = hit.point.y;
    if (out_pz) *out_pz = hit.point.z;

    if (out_nx) *out_nx = hit.normal.x;
    if (out_ny) *out_ny = hit.normal.y;
    if (out_nz) *out_nz = hit.normal.z;

    return 1;
}
