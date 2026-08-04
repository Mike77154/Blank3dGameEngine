#include "blank3d_vphysics.h"

#include <string.h>

#include "../vendor/gamlib3d/math_helpers/gamlib3d_math.h"

#define B3D_VPHYS_SKIN_Q16 ((vp_fx)(VP_FX_ONE / 128L))
#define B3D_VPHYS_FRICTION_Q16 ((vp_fx)(VP_FX_ONE * 7L / 10L))
#define B3D_VPHYS_RESTITUTION_Q16 ((vp_fx)(VP_FX_ONE / 5L))

static vp_fx b3d_vphys_q12_to_q16(long value)
{
    if (value > 134217727L) return (vp_fx)2147483647L;
    if (value < -134217728L) return (vp_fx)(-2147483647L - 1L);
    return (vp_fx)(value * 16L);
}

static long b3d_vphys_q16_to_q12(vp_fx value)
{
    return (long)(value / 16L);
}

static void b3d_vphys_set_status(Blank3DVPhysics *physics, const char *text)
{
    if (!physics) return;
    if (!text) text = "";
    strncpy(physics->status, text, sizeof(physics->status) - 1U);
    physics->status[sizeof(physics->status) - 1U] = '\0';
}

static Blank3DVPhysicsObject *b3d_vphys_find(
    Blank3DVPhysics *physics, vp_u32 external_id)
{
    int i;
    if (!physics || external_id == 0UL) return 0;
    for (i = 0; i < B3D_VPHYS_MAX_OBJECTS; ++i) {
        if (physics->objects[i].used &&
            physics->objects[i].external_id == external_id)
            return &physics->objects[i];
    }
    return 0;
}

static const Blank3DVPhysicsObject *b3d_vphys_find_const(
    const Blank3DVPhysics *physics, vp_u32 external_id)
{
    int i;
    if (!physics || external_id == 0UL) return 0;
    for (i = 0; i < B3D_VPHYS_MAX_OBJECTS; ++i) {
        if (physics->objects[i].used &&
            physics->objects[i].external_id == external_id)
            return &physics->objects[i];
    }
    return 0;
}

static Blank3DVPhysicsObject *b3d_vphys_alloc(
    Blank3DVPhysics *physics, vp_u32 external_id)
{
    int i;
    Blank3DVPhysicsObject *object;
    if (!physics || external_id == 0UL) return 0;
    if (b3d_vphys_find(physics, external_id)) return 0;
    for (i = 0; i < B3D_VPHYS_MAX_OBJECTS; ++i) {
        object = &physics->objects[i];
        if (!object->used) {
            memset(object, 0, sizeof(*object));
            object->used = 1;
            object->external_id = external_id;
            object->world = vpTransformIdentity();
            object->friction = B3D_VPHYS_FRICTION_Q16;
            object->restitution = B3D_VPHYS_RESTITUTION_Q16;
            object->pair_collidable = 1;
            object->ccd_enabled = 1;
            object->debug_draw = 1;
            physics->object_count += 1;
            return object;
        }
    }
    return 0;
}

static Blank3DVPhysicsObject *b3d_vphys_from_body(
    Blank3DVPhysics *physics, vpBodyId body_id)
{
    int i;
    if (!physics || body_id == 0) return 0;
    for (i = 0; i < B3D_VPHYS_MAX_OBJECTS; ++i) {
        if (physics->objects[i].used &&
            physics->objects[i].body_id == body_id)
            return &physics->objects[i];
    }
    return 0;
}

static int b3d_vphys_read_world(void *user, vp_u32 external_id,
                                vpTransform *out)
{
    Blank3DVPhysics *physics;
    Blank3DVPhysicsObject *object;
    physics = (Blank3DVPhysics *)user;
    if (!physics || !out) return 0;
    object = b3d_vphys_find(physics, external_id);
    if (!object) return 0;
    *out = object->world;
    physics->stats.transform_reads += 1UL;
    return 1;
}

static int b3d_vphys_write_world(void *user, vp_u32 external_id,
                                 const vpTransform *value)
{
    Blank3DVPhysics *physics;
    Blank3DVPhysicsObject *object;
    physics = (Blank3DVPhysics *)user;
    if (!physics || !value) return 0;
    object = b3d_vphys_find(physics, external_id);
    if (!object) return 0;
    object->world = *value;
    physics->stats.transform_writes += 1UL;
    return 1;
}

static vpVec3 b3d_vphys_vec_from_gwp(GWP89_Vec3 value)
{
    return vpVec3_make(b3d_vphys_q12_to_q16(value.x),
                       b3d_vphys_q12_to_q16(value.y),
                       b3d_vphys_q12_to_q16(value.z));
}

static GWP89_Vec3 b3d_vphys_gwp_from_vec(vpVec3 value)
{
    GWP89_Vec3 result;
    result.x = b3d_vphys_q16_to_q12(value.x);
    result.y = b3d_vphys_q16_to_q12(value.y);
    result.z = b3d_vphys_q16_to_q12(value.z);
    return result;
}

static void b3d_vphys_begin_contacts(void *user,
                                     vpTotalSolver *solver,
                                     vpWorld *world,
                                     vp_fx dt)
{
    Blank3DVPhysics *physics;
    VP_UNUSED(solver);
    VP_UNUSED(world);
    VP_UNUSED(dt);
    physics = (Blank3DVPhysics *)user;
    if (!physics) return;
    physics->stats.contacts_last_step = 0UL;
}

static vp_fx b3d_vphys_box_support_on_normal(
    const Blank3DVPhysicsObject *object, vpQuat rotation, vpVec3 normal)
{
    vpMat33 matrix;
    vpVec3 axis_x;
    vpVec3 axis_y;
    vpVec3 axis_z;
    vp_fx support;
    if (!object) return 0;
    matrix = vpMat33_from_quat(rotation);
    axis_x = vpVec3_make(matrix.m00, matrix.m10, matrix.m20);
    axis_y = vpVec3_make(matrix.m01, matrix.m11, matrix.m21);
    axis_z = vpVec3_make(matrix.m02, matrix.m12, matrix.m22);
    support = vp_fx_mul(vp_fx_abs(vpVec3_dot(normal, axis_x)),
                        object->half_extents.x);
    support += vp_fx_mul(vp_fx_abs(vpVec3_dot(normal, axis_y)),
                         object->half_extents.y);
    support += vp_fx_mul(vp_fx_abs(vpVec3_dot(normal, axis_z)),
                         object->half_extents.z);
    return support;
}

static void b3d_vphys_add_ground_contact(Blank3DVPhysics *physics,
                                         Blank3DVPhysicsObject *object,
                                         vpWorld *world)
{
    vpBody *body;
    vp_fx support;
    vp_fx range;
    GWP89_Vec3 origin;
    GWP89_Vec3 direction;
    Blank3DCollisionHit hit;
    vp_fx distance;
    vp_fx penetration;
    vpVec3 point;
    vpVec3 normal;
    if (!physics || !object || !world || !physics->collision) return;
    if (!object->collidable || !object->dynamic_body) return;
    body = vpWorldGetBody(world, object->body_id);
    if (!body || !body->used || body->asleep) return;

    /* Query with the conservative radius first; after the hit normal is known
     * compute the actual oriented support for boxes. This prevents a tumbling
     * casing from being treated as an axis-aligned point on every frame. */
    support = object->collision_radius;
    range = support + vp_fx_from_int(2);
    origin = b3d_vphys_gwp_from_vec(body->pos);
    direction.x = 0;
    direction.y = -GWP89_FIX_ONE;
    direction.z = 0;
    if (!blank3d_collision_raycast(physics->collision, &origin, &direction,
            b3d_vphys_q16_to_q12(range), B3D_COLLISION_LAYER_WORLD, &hit))
        return;
    distance = b3d_vphys_q12_to_q16(hit.distance_fx);
    point = b3d_vphys_vec_from_gwp(hit.point);
    normal = b3d_vphys_vec_from_gwp(hit.normal);
    if (vpVec3_len2(normal) <= 0)
        normal = vpVec3_make(0, VP_FX_ONE, 0);
    else
        normal = vpVec3_normalize(normal);
    if (object->shape_type == B3D_VPHYS_SHAPE_BOX)
        support = b3d_vphys_box_support_on_normal(object, body->rot, normal);
    else
        support = object->collision_radius;
    if (distance > support + B3D_VPHYS_SKIN_Q16) return;
    penetration = support + B3D_VPHYS_SKIN_Q16 - distance;
    if (penetration <= 0) return;
    /* Use the support point rather than the body centre. The offset lets the
     * solver trade angular and linear momentum instead of leaving props in a
     * perpetual visual spin. */
    point = vpVec3_sub(body->pos, vpVec3_scale(normal, support));
    vpContactsAddEx(world, 0, object->body_id, point, normal,
                    penetration, object->friction,
                    object->restitution,
                    VP_CONTACT_FLAG_NONE);
    physics->stats.contacts_last_step += 1UL;
    physics->stats.contacts_total += 1UL;
}

static void b3d_vphys_add_pair_contact(Blank3DVPhysics *physics,
                                       Blank3DVPhysicsObject *a,
                                       Blank3DVPhysicsObject *b,
                                       vpWorld *world)
{
    vpBody *body_a;
    vpBody *body_b;
    vpVec3 delta;
    vpVec3 normal;
    vpVec3 point;
    vp_fx distance;
    vp_fx wanted;
    vp_fx penetration;
    vp_fx along;
    if (!physics || !a || !b || !world) return;
    if (!a->collidable || !b->collidable) return;
    if (!a->pair_collidable || !b->pair_collidable) return;
    body_a = vpWorldGetBody(world, a->body_id);
    body_b = vpWorldGetBody(world, b->body_id);
    if (!body_a || !body_b || !body_a->used || !body_b->used) return;
    if (!a->dynamic_body && !b->dynamic_body) return;

    delta = vpVec3_sub(body_b->pos, body_a->pos);
    distance = vpVec3_len(delta);
    wanted = a->collision_radius + b->collision_radius;
    if (distance >= wanted || wanted <= 0) return;
    if (distance <= (vp_fx)(VP_FX_ONE / 1024L))
        normal = vpVec3_make(VP_FX_ONE, 0, 0);
    else
        normal = vpVec3_scale(delta, vp_fx_div(VP_FX_ONE, distance));
    penetration = wanted - distance;
    along = a->collision_radius - (penetration >> 1);
    point = vpVec3_add(body_a->pos, vpVec3_scale(normal, along));
    vpContactsAddEx(world, a->body_id, b->body_id,
                    point, normal, penetration,
                    (a->friction + b->friction) >> 1,
                    (a->restitution + b->restitution) >> 1,
                    VP_CONTACT_FLAG_NONE);
    physics->stats.contacts_last_step += 1UL;
    physics->stats.contacts_total += 1UL;
}

static void b3d_vphys_generate_contacts(void *user,
                                        vpTotalSolver *solver,
                                        vpWorld *world,
                                        vp_fx dt)
{
    Blank3DVPhysics *physics;
    int i;
    int j;
    VP_UNUSED(solver);
    VP_UNUSED(dt);
    physics = (Blank3DVPhysics *)user;
    if (!physics || !world || !physics->enabled) return;
    for (i = 0; i < B3D_VPHYS_MAX_OBJECTS; ++i) {
        if (!physics->objects[i].used) continue;
        b3d_vphys_add_ground_contact(physics, &physics->objects[i], world);
    }
    for (i = 0; i < B3D_VPHYS_MAX_OBJECTS; ++i) {
        if (!physics->objects[i].used ||
            !physics->objects[i].pair_collidable) continue;
        for (j = i + 1; j < B3D_VPHYS_MAX_OBJECTS; ++j) {
            if (!physics->objects[j].used ||
                !physics->objects[j].pair_collidable) continue;
            b3d_vphys_add_pair_contact(physics,
                &physics->objects[i], &physics->objects[j], world);
        }
    }
}

static vp_fx b3d_vphys_sweep(void *user, vp_u16 body_id,
                             vpVec3 from_pos, vpQuat from_rot,
                             vpVec3 to_pos, vpQuat to_rot,
                             vp_u16 *hit_body, vpVec3 *hit_point,
                             vpVec3 *hit_normal)
{
    Blank3DVPhysics *physics;
    Blank3DVPhysicsObject *object;
    GWP89_Vec3 start;
    GWP89_Vec3 delta;
    Blank3DCollisionHit hit;
    vpVec3 displacement;
    VP_UNUSED(from_rot);
    VP_UNUSED(to_rot);
    physics = (Blank3DVPhysics *)user;
    if (!physics || !physics->collision || !physics->enabled)
        return VP_FX_ONE;
    object = b3d_vphys_from_body(physics, (vpBodyId)body_id);
    if (!object || !object->collidable || !object->dynamic_body ||
        !object->ccd_enabled)
        return VP_FX_ONE;
    displacement = vpVec3_sub(to_pos, from_pos);
    start = b3d_vphys_gwp_from_vec(from_pos);
    delta = b3d_vphys_gwp_from_vec(displacement);
    physics->stats.sweep_queries += 1UL;
    if (!blank3d_collision_sweep_bullet_mask(physics->collision,
            &start, &delta,
            b3d_vphys_q16_to_q12(object->collision_radius),
            B3D_COLLISION_LAYER_WORLD, &hit))
        return VP_FX_ONE;
    physics->stats.sweep_hits += 1UL;
    if (hit_body) *hit_body = 0;
    if (hit_point) *hit_point = b3d_vphys_vec_from_gwp(hit.point);
    if (hit_normal) *hit_normal = b3d_vphys_vec_from_gwp(hit.normal);
    return b3d_vphys_q12_to_q16(hit.fraction_fx);
}

static void b3d_vphys_set_box_inertia(Blank3DVPhysics *physics,
                                        Blank3DVPhysicsObject *object,
                                        vp_fx mass)
{
    vp_fx width;
    vp_fx height;
    vp_fx depth;
    vp_fx ixx;
    vp_fx iyy;
    vp_fx izz;
    vpVec3 inverse;
    if (!physics || !object || !physics->world || mass <= 0) return;
    width = object->half_extents.x << 1;
    height = object->half_extents.y << 1;
    depth = object->half_extents.z << 1;
    ixx = vp_fx_mul(mass,
        vp_fx_mul(height, height) + vp_fx_mul(depth, depth));
    iyy = vp_fx_mul(mass,
        vp_fx_mul(width, width) + vp_fx_mul(depth, depth));
    izz = vp_fx_mul(mass,
        vp_fx_mul(width, width) + vp_fx_mul(height, height));
    ixx = vp_fx_div(ixx, vp_fx_from_int(12));
    iyy = vp_fx_div(iyy, vp_fx_from_int(12));
    izz = vp_fx_div(izz, vp_fx_from_int(12));
    inverse.x = ixx > 0 ? vp_fx_div(VP_FX_ONE, ixx) : VP_FX_ONE;
    inverse.y = iyy > 0 ? vp_fx_div(VP_FX_ONE, iyy) : VP_FX_ONE;
    inverse.z = izz > 0 ? vp_fx_div(VP_FX_ONE, izz) : VP_FX_ONE;
    vpBodySetInertiaInvDiag(physics->world, object->body_id, inverse);
}

static void b3d_vphys_set_sphere_inertia(Blank3DVPhysics *physics,
                                         Blank3DVPhysicsObject *object,
                                         vp_fx mass)
{
    vp_fx inertia;
    vp_fx inverse;
    if (!physics || !object || !physics->world || mass <= 0) return;
    inertia = vp_fx_mul(mass,
        vp_fx_mul(object->collision_radius, object->collision_radius));
    inertia = vp_fx_mul(inertia, (vp_fx)(VP_FX_ONE * 2L / 5L));
    inverse = inertia > 0 ? vp_fx_div(VP_FX_ONE, inertia) : VP_FX_ONE;
    vpBodySetInertiaInvDiag(physics->world, object->body_id,
        vpVec3_make(inverse, inverse, inverse));
}

static int b3d_vphys_finish_object(Blank3DVPhysics *physics,
                                   Blank3DVPhysicsObject *object,
                                   vpTransformAuthority authority,
                                   vp_fx mass)
{
    vpBody *body;
    if (!physics || !object || !physics->world || !physics->total) return 0;
    if (authority == VP_TRANSFORM_AUTH_EXTERNAL) {
        object->body_id = vpBodyCreateKinematic(physics->world);
        object->dynamic_body = 0;
    } else {
        if (mass <= 0) mass = VP_FX_ONE;
        object->body_id = vpBodyCreateDynamic(physics->world, mass);
        object->dynamic_body = 1;
    }
    if (object->body_id == 0) return 0;
    vpBodySetPose(physics->world, object->body_id,
                  object->world.position, object->world.rotation);
    body = vpWorldGetBody(physics->world, object->body_id);
    if (body && object->dynamic_body) {
        vpBodySetDamping(physics->world, object->body_id,
            (vp_fx)(VP_FX_ONE / 64L), (vp_fx)(VP_FX_ONE / 32L));
    }
    vpBodySetUserData(physics->world, object->body_id, object->external_id);
    object->node_id = vpTransformNodeCreate(physics->total,
                                            object->external_id);
    if (object->node_id == 0) return 0;
    vpTransformNodeBindBody(physics->total, object->node_id,
                            object->body_id, authority);
    vpTransformNodeSetWorld(physics->total, object->node_id, &object->world);
    return 1;
}

int blank3d_vphysics_init(Blank3DVPhysics *physics,
                          Blank3DCollision *collision)
{
    vpWorldDesc world_desc;
    vpTotalSolverDesc total_desc;
    vp_u32 world_needed;
    vp_u32 total_needed;
    if (!physics) return 0;
    memset(physics, 0, sizeof(*physics));
    physics->collision = collision;

    world_desc.maxBodies = B3D_VPHYS_MAX_OBJECTS;
    world_desc.maxContacts = 256;
    world_desc.maxJoints = 32;
    world_desc.solverIters = 10;
    world_needed = vpWorldMemSize(&world_desc);
    if (world_needed > B3D_VPHYS_WORLD_ARENA_BYTES) {
        b3d_vphys_set_status(physics, "VPhysics world arena too small");
        return 0;
    }
    physics->world = vpWorldInit(physics->world_arena,
        B3D_VPHYS_WORLD_ARENA_BYTES, &world_desc);
    if (!physics->world) {
        b3d_vphys_set_status(physics, "VPhysics world init failed");
        return 0;
    }
    vpWorldSetGravity(physics->world,
        vpVec3_make(0, (vp_fx)(-10L * VP_FX_ONE), 0));
    vpWorldSetDefaultMaterial(physics->world,
        B3D_VPHYS_FRICTION_Q16, B3D_VPHYS_RESTITUTION_Q16);

    total_desc.maxTransformNodes = B3D_VPHYS_MAX_OBJECTS;
    total_desc.maxTransformConstraints = B3D_VPHYS_MAX_OBJECTS;
    total_desc.transformIterations = 2;
    total_needed = vpTotalSolverMemSize(&total_desc);
    if (total_needed > B3D_VPHYS_TOTAL_ARENA_BYTES) {
        b3d_vphys_set_status(physics, "VPhysics total arena too small");
        return 0;
    }
    physics->total = vpTotalSolverInit(physics->total_arena,
        B3D_VPHYS_TOTAL_ARENA_BYTES, physics->world, &total_desc);
    if (!physics->total) {
        b3d_vphys_set_status(physics, "VPhysics Total Solver init failed");
        return 0;
    }

    memset(&physics->transform_provider, 0,
           sizeof(physics->transform_provider));
    physics->transform_provider.user = physics;
    physics->transform_provider.readWorld = b3d_vphys_read_world;
    physics->transform_provider.writeWorld = b3d_vphys_write_world;
    vpTotalSolverSetTransformProvider(physics->total,
                                      &physics->transform_provider);

    memset(&physics->collision_provider, 0,
           sizeof(physics->collision_provider));
    physics->collision_provider.user = physics;
    physics->collision_provider.beginStep = b3d_vphys_begin_contacts;
    physics->collision_provider.generateContacts = b3d_vphys_generate_contacts;
    physics->collision_provider.bodySweepTOI = b3d_vphys_sweep;
    vpTotalSolverSetCollisionProvider(physics->total,
                                      &physics->collision_provider);

    physics->enabled = 1;
    physics->initialized = 1;
    b3d_vphys_set_status(physics,
        "VPhysics Total Solver online: transform + CCS/SICOL collision providers");
    return 1;
}

void blank3d_vphysics_set_enabled(Blank3DVPhysics *physics, int enabled)
{
    if (!physics || !physics->initialized) return;
    physics->enabled = enabled ? 1 : 0;
}

int blank3d_vphysics_is_enabled(const Blank3DVPhysics *physics)
{
    return physics && physics->initialized && physics->enabled;
}

const vpTransformProvider *blank3d_vphysics_get_transform_provider(
    const Blank3DVPhysics *physics)
{
    if (!physics || !physics->initialized) return 0;
    return &physics->transform_provider;
}

const vpCollisionProvider *blank3d_vphysics_get_collision_provider(
    const Blank3DVPhysics *physics)
{
    if (!physics || !physics->initialized) return 0;
    return &physics->collision_provider;
}

vpTotalSolver *blank3d_vphysics_get_total_solver(Blank3DVPhysics *physics)
{
    if (!physics || !physics->initialized) return 0;
    return physics->total;
}

vpWorld *blank3d_vphysics_get_world(Blank3DVPhysics *physics)
{
    if (!physics || !physics->initialized) return 0;
    return physics->world;
}

int blank3d_vphysics_create_dynamic_box_q12(
    Blank3DVPhysics *physics, vp_u32 external_id,
    long x_q12, long y_q12, long z_q12,
    long half_x_q12, long half_y_q12, long half_z_q12,
    long mass_q12)
{
    Blank3DVPhysicsObject *object;
    vp_fx hx;
    vp_fx hy;
    vp_fx hz;
    if (!physics || !physics->initialized) return 0;
    object = b3d_vphys_alloc(physics, external_id);
    if (!object) return 0;
    hx = b3d_vphys_q12_to_q16(half_x_q12);
    hy = b3d_vphys_q12_to_q16(half_y_q12);
    hz = b3d_vphys_q12_to_q16(half_z_q12);
    object->shape_type = B3D_VPHYS_SHAPE_BOX;
    object->collidable = 1;
    object->half_extents = vpVec3_make(hx, hy, hz);
    object->collision_radius = vpVec3_len(object->half_extents);
    object->world.position = vpVec3_make(
        b3d_vphys_q12_to_q16(x_q12),
        b3d_vphys_q12_to_q16(y_q12),
        b3d_vphys_q12_to_q16(z_q12));
    object->world.scale = vpVec3_make(hx << 1, hy << 1, hz << 1);
    if (!b3d_vphys_finish_object(physics, object,
            VP_TRANSFORM_AUTH_PHYSICS,
            b3d_vphys_q12_to_q16(mass_q12))) {
        object->used = 0;
        physics->object_count -= 1;
        return 0;
    }
    b3d_vphys_set_box_inertia(physics, object,
        b3d_vphys_q12_to_q16(mass_q12));
    return 1;
}

int blank3d_vphysics_create_dynamic_sphere_q12(
    Blank3DVPhysics *physics, vp_u32 external_id,
    long x_q12, long y_q12, long z_q12,
    long radius_q12, long mass_q12)
{
    Blank3DVPhysicsObject *object;
    vp_fx radius;
    if (!physics || !physics->initialized) return 0;
    object = b3d_vphys_alloc(physics, external_id);
    if (!object) return 0;
    radius = b3d_vphys_q12_to_q16(radius_q12);
    object->shape_type = B3D_VPHYS_SHAPE_SPHERE;
    object->collidable = 1;
    object->half_extents = vpVec3_make(radius, radius, radius);
    object->collision_radius = radius;
    object->world.position = vpVec3_make(
        b3d_vphys_q12_to_q16(x_q12),
        b3d_vphys_q12_to_q16(y_q12),
        b3d_vphys_q12_to_q16(z_q12));
    object->world.scale = vpVec3_make(radius << 1,
                                      radius << 1,
                                      radius << 1);
    if (!b3d_vphys_finish_object(physics, object,
            VP_TRANSFORM_AUTH_PHYSICS,
            b3d_vphys_q12_to_q16(mass_q12))) {
        object->used = 0;
        physics->object_count -= 1;
        return 0;
    }
    b3d_vphys_set_sphere_inertia(physics, object,
        b3d_vphys_q12_to_q16(mass_q12));
    return 1;
}

int blank3d_vphysics_create_kinematic_box_q12(
    Blank3DVPhysics *physics, vp_u32 external_id,
    long x_q12, long y_q12, long z_q12,
    long half_x_q12, long half_y_q12, long half_z_q12,
    int collidable)
{
    Blank3DVPhysicsObject *object;
    vp_fx hx;
    vp_fx hy;
    vp_fx hz;
    if (!physics || !physics->initialized) return 0;
    object = b3d_vphys_alloc(physics, external_id);
    if (!object) return 0;
    hx = b3d_vphys_q12_to_q16(half_x_q12);
    hy = b3d_vphys_q12_to_q16(half_y_q12);
    hz = b3d_vphys_q12_to_q16(half_z_q12);
    object->shape_type = B3D_VPHYS_SHAPE_BOX;
    object->collidable = collidable ? 1 : 0;
    object->half_extents = vpVec3_make(hx, hy, hz);
    object->collision_radius = vpVec3_len(object->half_extents);
    object->world.position = vpVec3_make(
        b3d_vphys_q12_to_q16(x_q12),
        b3d_vphys_q12_to_q16(y_q12),
        b3d_vphys_q12_to_q16(z_q12));
    object->world.scale = vpVec3_make(hx << 1, hy << 1, hz << 1);
    if (!b3d_vphys_finish_object(physics, object,
            VP_TRANSFORM_AUTH_EXTERNAL, VP_FX_ONE)) {
        object->used = 0;
        physics->object_count -= 1;
        return 0;
    }
    return 1;
}

int blank3d_vphysics_destroy_object(Blank3DVPhysics *physics,
                                    vp_u32 external_id)
{
    Blank3DVPhysicsObject *object;
    vpBody *body;
    if (!physics || !physics->initialized) return 0;
    object = b3d_vphys_find(physics, external_id);
    if (!object) return 0;
    if (object->node_id)
        vpTransformNodeDestroy(physics->total, object->node_id);
    body = vpWorldGetBody(physics->world, object->body_id);
    if (body) memset(body, 0, sizeof(*body));
    memset(object, 0, sizeof(*object));
    if (physics->object_count > 0) physics->object_count -= 1;
    return 1;
}

int blank3d_vphysics_get_transform(const Blank3DVPhysics *physics,
                                   vp_u32 external_id,
                                   vpTransform *out_transform)
{
    const Blank3DVPhysicsObject *object;
    if (!out_transform) return 0;
    object = b3d_vphys_find_const(physics, external_id);
    if (!object) return 0;
    *out_transform = object->world;
    return 1;
}

int blank3d_vphysics_set_transform(Blank3DVPhysics *physics,
                                   vp_u32 external_id,
                                   const vpTransform *transform)
{
    Blank3DVPhysicsObject *object;
    if (!physics || !transform) return 0;
    object = b3d_vphys_find(physics, external_id);
    if (!object) return 0;
    object->world = *transform;
    vpTransformNodeSetWorld(physics->total, object->node_id, transform);
    vpBodySetPose(physics->world, object->body_id,
                  transform->position, transform->rotation);
    return 1;
}

int blank3d_vphysics_move_q12(Blank3DVPhysics *physics,
                              vp_u32 external_id,
                              long dx_q12, long dy_q12, long dz_q12)
{
    Blank3DVPhysicsObject *object;
    object = b3d_vphys_find(physics, external_id);
    if (!object) return 0;
    object->world.position.x += b3d_vphys_q12_to_q16(dx_q12);
    object->world.position.y += b3d_vphys_q12_to_q16(dy_q12);
    object->world.position.z += b3d_vphys_q12_to_q16(dz_q12);
    return blank3d_vphysics_set_transform(physics, external_id,
                                          &object->world);
}

int blank3d_vphysics_rotate_y_q12(Blank3DVPhysics *physics,
                                  vp_u32 external_id,
                                  long yaw_degrees_q12)
{
    Blank3DVPhysicsObject *object;
    g3d_fix half_deg;
    g3d_fix half_rad;
    vpQuat delta;
    object = b3d_vphys_find(physics, external_id);
    if (!object) return 0;
    half_deg = (g3d_fix)(yaw_degrees_q12 / 2L);
    half_rad = gamlib_deg2rad(half_deg);
    delta.x = 0;
    delta.y = b3d_vphys_q12_to_q16(gamlib_sin(half_rad));
    delta.z = 0;
    delta.w = b3d_vphys_q12_to_q16(gamlib_cos(half_rad));
    object->world.rotation = vpQuat_normalize(
        vpQuat_mul(object->world.rotation, delta));
    return blank3d_vphysics_set_transform(physics, external_id,
                                          &object->world);
}

int blank3d_vphysics_scale_q12(Blank3DVPhysics *physics,
                               vp_u32 external_id,
                               long sx_q12, long sy_q12, long sz_q12)
{
    Blank3DVPhysicsObject *object;
    object = b3d_vphys_find(physics, external_id);
    if (!object) return 0;
    object->world.scale = vpVec3_make(
        b3d_vphys_q12_to_q16(sx_q12),
        b3d_vphys_q12_to_q16(sy_q12),
        b3d_vphys_q12_to_q16(sz_q12));
    vpTransformNodeSetWorld(physics->total, object->node_id, &object->world);
    return 1;
}

int blank3d_vphysics_set_position_q12(Blank3DVPhysics *physics,
                                      vp_u32 external_id,
                                      long x_q12, long y_q12, long z_q12)
{
    Blank3DVPhysicsObject *object;
    object = b3d_vphys_find(physics, external_id);
    if (!object) return 0;
    object->world.position = vpVec3_make(
        b3d_vphys_q12_to_q16(x_q12),
        b3d_vphys_q12_to_q16(y_q12),
        b3d_vphys_q12_to_q16(z_q12));
    return blank3d_vphysics_set_transform(physics, external_id,
                                          &object->world);
}

int blank3d_vphysics_set_velocity_q12(Blank3DVPhysics *physics,
                                      vp_u32 external_id,
                                      long vx_q12, long vy_q12, long vz_q12,
                                      long wx_q12, long wy_q12, long wz_q12)
{
    Blank3DVPhysicsObject *object;
    if (!physics || !physics->world) return 0;
    object = b3d_vphys_find(physics, external_id);
    if (!object || !object->dynamic_body) return 0;
    vpBodySetVel(physics->world, object->body_id,
        vpVec3_make(b3d_vphys_q12_to_q16(vx_q12),
                    b3d_vphys_q12_to_q16(vy_q12),
                    b3d_vphys_q12_to_q16(vz_q12)),
        vpVec3_make(b3d_vphys_q12_to_q16(wx_q12),
                    b3d_vphys_q12_to_q16(wy_q12),
                    b3d_vphys_q12_to_q16(wz_q12)));
    return 1;
}

int blank3d_vphysics_add_impulse_q12(Blank3DVPhysics *physics,
                                     vp_u32 external_id,
                                     long ix_q12, long iy_q12, long iz_q12)
{
    Blank3DVPhysicsObject *object;
    vpBody *body;
    vpVec3 impulse;
    if (!physics || !physics->world) return 0;
    object = b3d_vphys_find(physics, external_id);
    if (!object || !object->dynamic_body) return 0;
    body = vpWorldGetBody(physics->world, object->body_id);
    if (!body) return 0;
    impulse = vpVec3_make(b3d_vphys_q12_to_q16(ix_q12),
                          b3d_vphys_q12_to_q16(iy_q12),
                          b3d_vphys_q12_to_q16(iz_q12));
    vpBodyAddImpulse(physics->world, object->body_id, impulse, body->pos);
    return 1;
}

int blank3d_vphysics_configure_object_q12(
    Blank3DVPhysics *physics, vp_u32 external_id,
    long friction_q12, long restitution_q12,
    long linear_damping_q12, long angular_damping_q12,
    int pair_collidable, int debug_draw)
{
    Blank3DVPhysicsObject *object;
    vpBody *body;
    if (!physics || !physics->world) return 0;
    object = b3d_vphys_find(physics, external_id);
    if (!object) return 0;
    object->friction = b3d_vphys_q12_to_q16(friction_q12);
    object->restitution = b3d_vphys_q12_to_q16(restitution_q12);
    if (object->friction < 0) object->friction = 0;
    if (object->restitution < 0) object->restitution = 0;
    if (object->restitution > VP_FX_ONE)
        object->restitution = VP_FX_ONE;
    object->pair_collidable = pair_collidable ? 1 : 0;
    object->debug_draw = debug_draw ? 1 : 0;
    body = vpWorldGetBody(physics->world, object->body_id);
    if (body && object->dynamic_body)
        vpBodySetDamping(physics->world, object->body_id,
            b3d_vphys_q12_to_q16(linear_damping_q12),
            b3d_vphys_q12_to_q16(angular_damping_q12));
    return 1;
}

int blank3d_vphysics_set_ccd_enabled(
    Blank3DVPhysics *physics, vp_u32 external_id, int enabled)
{
    Blank3DVPhysicsObject *object;
    object = b3d_vphys_find(physics, external_id);
    if (!object) return 0;
    object->ccd_enabled = enabled ? 1 : 0;
    return 1;
}

int blank3d_vphysics_body_is_asleep(
    const Blank3DVPhysics *physics, vp_u32 external_id)
{
    const Blank3DVPhysicsObject *object;
    vpBody *body;
    if (!physics || !physics->world) return 0;
    object = b3d_vphys_find_const(physics, external_id);
    if (!object) return 0;
    body = vpWorldGetBody(physics->world, object->body_id);
    return body && body->asleep ? 1 : 0;
}

int blank3d_vphysics_get_velocity_q12(
    const Blank3DVPhysics *physics, vp_u32 external_id,
    long *vx_q12, long *vy_q12, long *vz_q12,
    long *wx_q12, long *wy_q12, long *wz_q12)
{
    const Blank3DVPhysicsObject *object;
    vpBody *body;
    if (!physics || !physics->world) return 0;
    object = b3d_vphys_find_const(physics, external_id);
    if (!object) return 0;
    body = vpWorldGetBody(physics->world, object->body_id);
    if (!body) return 0;
    if (vx_q12) *vx_q12 = b3d_vphys_q16_to_q12(body->v_lin.x);
    if (vy_q12) *vy_q12 = b3d_vphys_q16_to_q12(body->v_lin.y);
    if (vz_q12) *vz_q12 = b3d_vphys_q16_to_q12(body->v_lin.z);
    if (wx_q12) *wx_q12 = b3d_vphys_q16_to_q12(body->w_ang.x);
    if (wy_q12) *wy_q12 = b3d_vphys_q16_to_q12(body->w_ang.y);
    if (wz_q12) *wz_q12 = b3d_vphys_q16_to_q12(body->w_ang.z);
    return 1;
}

void blank3d_vphysics_step_q12(Blank3DVPhysics *physics, long dt_q12)
{
    vp_fx dt;
    if (!physics || !physics->initialized || !physics->enabled) return;
    dt = b3d_vphys_q12_to_q16(dt_q12);
    if (dt <= 0) return;
    if (dt > (vp_fx)(VP_FX_ONE / 10L)) dt = (vp_fx)(VP_FX_ONE / 10L);
    vpTotalSolverStepCCD(physics->total, dt, 4);
    physics->stats.steps += 1UL;
}

void blank3d_vphysics_spawn_demo(Blank3DVPhysics *physics)
{
    Blank3DCollision *collision;
    int was_enabled;
    if (!physics || !physics->initialized) return;
    collision = physics->collision;
    was_enabled = physics->enabled;
    if (!blank3d_vphysics_init(physics, collision)) return;
    blank3d_vphysics_set_enabled(physics, was_enabled);
    (void)blank3d_vphysics_create_dynamic_box_q12(physics, 70001UL,
        -3L * GWP89_FIX_ONE, 7L * GWP89_FIX_ONE, 18L * GWP89_FIX_ONE,
        GWP89_FIX_ONE / 2L, GWP89_FIX_ONE / 2L, GWP89_FIX_ONE / 2L,
        GWP89_FIX_ONE);
    (void)blank3d_vphysics_create_dynamic_sphere_q12(physics, 70002UL,
        -1L * GWP89_FIX_ONE, 10L * GWP89_FIX_ONE, 18L * GWP89_FIX_ONE,
        GWP89_FIX_ONE / 2L, GWP89_FIX_ONE);
    (void)blank3d_vphysics_create_dynamic_box_q12(physics, 70003UL,
        1L * GWP89_FIX_ONE, 13L * GWP89_FIX_ONE, 18L * GWP89_FIX_ONE,
        3L * GWP89_FIX_ONE / 5L, 2L * GWP89_FIX_ONE / 5L,
        3L * GWP89_FIX_ONE / 5L, GWP89_FIX_ONE);
    (void)blank3d_vphysics_create_dynamic_sphere_q12(physics, 70004UL,
        3L * GWP89_FIX_ONE, 16L * GWP89_FIX_ONE, 18L * GWP89_FIX_ONE,
        3L * GWP89_FIX_ONE / 5L, GWP89_FIX_ONE);
    (void)blank3d_vphysics_create_kinematic_box_q12(physics, 70005UL,
        0, 3L * GWP89_FIX_ONE, 23L * GWP89_FIX_ONE,
        GWP89_FIX_ONE / 2L, GWP89_FIX_ONE / 2L,
        GWP89_FIX_ONE / 2L, 0);
    (void)blank3d_vphysics_set_velocity_q12(physics, 70001UL,
        GWP89_FIX_ONE / 2L, 0, 0,
        0, GWP89_FIX_ONE, GWP89_FIX_ONE / 2L);
    (void)blank3d_vphysics_set_velocity_q12(physics, 70002UL,
        -GWP89_FIX_ONE / 3L, 0, GWP89_FIX_ONE / 4L,
        GWP89_FIX_ONE / 2L, 0, GWP89_FIX_ONE);
    (void)blank3d_vphysics_set_velocity_q12(physics, 70003UL,
        0, 0, -GWP89_FIX_ONE / 3L,
        GWP89_FIX_ONE, GWP89_FIX_ONE / 2L, 0);
    (void)blank3d_vphysics_set_velocity_q12(physics, 70004UL,
        GWP89_FIX_ONE / 4L, 0, 0,
        0, GWP89_FIX_ONE, GWP89_FIX_ONE);
}

void blank3d_vphysics_update_demo(Blank3DVPhysics *physics,
                                  long time_q12)
{
    g3d_fix phase;
    g3d_fix s;
    g3d_fix x;
    g3d_fix scale_y;
    vpTransform transform;
    if (!physics || !physics->initialized) return;
    if (!blank3d_vphysics_get_transform(physics,
            B3D_VPHYS_EXTERNAL_DEMO_BEACON, &transform)) return;
    phase = (g3d_fix)(time_q12 / 2L);
    s = gamlib_sin(phase);
    x = g3d_fix_mul(s, 2L * G3D_FIX_ONE);
    transform.position.x = b3d_vphys_q12_to_q16(x);
    transform.position.y = b3d_vphys_q12_to_q16(3L * G3D_FIX_ONE);
    transform.position.z = b3d_vphys_q12_to_q16(23L * G3D_FIX_ONE);
    (void)blank3d_vphysics_set_transform(physics,
        B3D_VPHYS_EXTERNAL_DEMO_BEACON, &transform);
    (void)blank3d_vphysics_rotate_y_q12(physics,
        B3D_VPHYS_EXTERNAL_DEMO_BEACON,
        (long)(2L * G3D_FIX_ONE));
    scale_y = G3D_FIX_ONE + g3d_fix_mul(s, G3D_FIX_ONE / 4L);
    (void)blank3d_vphysics_scale_q12(physics,
        B3D_VPHYS_EXTERNAL_DEMO_BEACON,
        G3D_FIX_ONE, scale_y, G3D_FIX_ONE);
}

int blank3d_vphysics_object_count(const Blank3DVPhysics *physics)
{
    return physics ? physics->object_count : 0;
}

const Blank3DVPhysicsObject *blank3d_vphysics_object_at(
    const Blank3DVPhysics *physics, int index)
{
    int i;
    int found;
    if (!physics || index < 0) return 0;
    found = 0;
    for (i = 0; i < B3D_VPHYS_MAX_OBJECTS; ++i) {
        if (!physics->objects[i].used) continue;
        if (found == index) return &physics->objects[i];
        found += 1;
    }
    return 0;
}

int blank3d_vphysics_get_draw_matrix_q16(
    const Blank3DVPhysics *physics, vp_u32 external_id,
    signed int out_matrix[4][4])
{
    const Blank3DVPhysicsObject *object;
    vpMat33 rotation;
    vp_fx sx;
    vp_fx sy;
    vp_fx sz;
    if (!out_matrix) return 0;
    object = b3d_vphys_find_const(physics, external_id);
    if (!object) return 0;
    rotation = vpMat33_from_quat(object->world.rotation);
    sx = object->world.scale.x;
    sy = object->world.scale.y;
    sz = object->world.scale.z;
    out_matrix[0][0] = (signed int)vp_fx_mul(rotation.m00, sx);
    out_matrix[0][1] = (signed int)vp_fx_mul(rotation.m01, sy);
    out_matrix[0][2] = (signed int)vp_fx_mul(rotation.m02, sz);
    out_matrix[0][3] = (signed int)object->world.position.x;
    out_matrix[1][0] = (signed int)vp_fx_mul(rotation.m10, sx);
    out_matrix[1][1] = (signed int)vp_fx_mul(rotation.m11, sy);
    out_matrix[1][2] = (signed int)vp_fx_mul(rotation.m12, sz);
    out_matrix[1][3] = (signed int)object->world.position.y;
    out_matrix[2][0] = (signed int)vp_fx_mul(rotation.m20, sx);
    out_matrix[2][1] = (signed int)vp_fx_mul(rotation.m21, sy);
    out_matrix[2][2] = (signed int)vp_fx_mul(rotation.m22, sz);
    out_matrix[2][3] = (signed int)object->world.position.z;
    out_matrix[3][0] = 0;
    out_matrix[3][1] = 0;
    out_matrix[3][2] = 0;
    out_matrix[3][3] = (signed int)VP_FX_ONE;
    return 1;
}

int blank3d_vphysics_get_draw_matrix_scaled_q12(
    const Blank3DVPhysics *physics, vp_u32 external_id,
    long sx_q12, long sy_q12, long sz_q12,
    signed int out_matrix[4][4])
{
    const Blank3DVPhysicsObject *object;
    vpMat33 rotation;
    vp_fx sx;
    vp_fx sy;
    vp_fx sz;
    if (!out_matrix) return 0;
    object = b3d_vphys_find_const(physics, external_id);
    if (!object) return 0;
    rotation = vpMat33_from_quat(object->world.rotation);
    sx = b3d_vphys_q12_to_q16(sx_q12);
    sy = b3d_vphys_q12_to_q16(sy_q12);
    sz = b3d_vphys_q12_to_q16(sz_q12);
    out_matrix[0][0] = (signed int)vp_fx_mul(rotation.m00, sx);
    out_matrix[0][1] = (signed int)vp_fx_mul(rotation.m01, sy);
    out_matrix[0][2] = (signed int)vp_fx_mul(rotation.m02, sz);
    out_matrix[0][3] = (signed int)object->world.position.x;
    out_matrix[1][0] = (signed int)vp_fx_mul(rotation.m10, sx);
    out_matrix[1][1] = (signed int)vp_fx_mul(rotation.m11, sy);
    out_matrix[1][2] = (signed int)vp_fx_mul(rotation.m12, sz);
    out_matrix[1][3] = (signed int)object->world.position.y;
    out_matrix[2][0] = (signed int)vp_fx_mul(rotation.m20, sx);
    out_matrix[2][1] = (signed int)vp_fx_mul(rotation.m21, sy);
    out_matrix[2][2] = (signed int)vp_fx_mul(rotation.m22, sz);
    out_matrix[2][3] = (signed int)object->world.position.z;
    out_matrix[3][0] = 0;
    out_matrix[3][1] = 0;
    out_matrix[3][2] = 0;
    out_matrix[3][3] = (signed int)VP_FX_ONE;
    return 1;
}

const Blank3DVPhysicsStats *blank3d_vphysics_stats(
    const Blank3DVPhysics *physics)
{
    return physics ? &physics->stats : 0;
}

const char *blank3d_vphysics_status(const Blank3DVPhysics *physics)
{
    return physics ? physics->status : "VPhysics unavailable";
}
