/* ============================================================
 * SICOL - Static collision world
 * ============================================================ */

#include "sicol_world.h"
#include "sicol_gjk.h"
#include "sicol_cast.h"
#include "sicol_mesh.h"

#define SICOL_WORLD_CAST_STEPS 32
#define SICOL_WORLD_CAST_BISECT_ITERS 10
#define SICOL_WORLD_DEPEN_EPS FX_FROM_RATIO(1, 1024)
#define SICOL_MANIFOLD_POINT_MERGE_TOL FX_FROM_RATIO(1, 8)
#define SICOL_MANIFOLD_POINT_MERGE_TOL_SQ FX_MUL(SICOL_MANIFOLD_POINT_MERGE_TOL, SICOL_MANIFOLD_POINT_MERGE_TOL)

static void world_touch_manifold(
    sicol_world_t* world,
    int id_a,
    int id_b,
    const sicol_contact_t* contact
);

static int world_valid_id(const sicol_world_t* world, int id)
{
    if (!world) return 0;
    if (id < 0 || id >= SICOL_WORLD_MAX) return 0;
    return world->slots[id].active;
}

static int world_masks_allow(uint32_t layer_a, uint32_t mask_a, uint32_t layer_b, uint32_t mask_b)
{
    if ((mask_a & layer_b) == 0) return 0;
    if ((mask_b & layer_a) == 0) return 0;
    return 1;
}

static int world_query_hits_layer(uint32_t query_mask, uint32_t layer)
{
    if ((query_mask & layer) == 0) return 0;
    return 1;
}

static int world_is_volume(const sicol_shape_t* s)
{
    if (!s) return 0;
    switch (s->type) {
    case SICOL_SHAPE_AABB:
    case SICOL_SHAPE_OBB:
    case SICOL_SHAPE_SPHERE:
    case SICOL_SHAPE_CAPSULE:
    case SICOL_SHAPE_CONVEX:
    case SICOL_SHAPE_TRIANGLE:
    case SICOL_SHAPE_MESH:
        return 1;
    default:
        break;
    }
    return 0;
}

static void world_zero_contact(sicol_contact_t* out)
{
    if (!out) return;
    fx_zero3(out->normal);
    fx_zero3(out->point);
    out->penetration = 0;
    out->feature_a = 0;
    out->feature_b = 0;
    out->count = 0;
}

static void world_swap_contact(sicol_contact_t* out)
{
    int tmp_feature;
    if (!out) return;
    out->normal[0] = -out->normal[0];
    out->normal[1] = -out->normal[1];
    out->normal[2] = -out->normal[2];
    tmp_feature = out->feature_a;
    out->feature_a = out->feature_b;
    out->feature_b = tmp_feature;
}

static int world_mesh_pair_contact_persistent(
    sicol_world_t* world,
    int id_a,
    int id_b,
    sicol_contact_t* out
)
{
    const sicol_shape_t* mesh_shape;
    const sicol_shape_t* other_shape;
    int mesh_is_a;
    fx other_min[3];
    fx other_max[3];
    fx query_min[3];
    fx query_max[3];
    int candidate_indices[SICOL_MESH_MAX_TRIANGLES];
    int count;
    int i;
    int best_hit;
    sicol_contact_t best_contact;

    if (!world_valid_id(world, id_a) || !world_valid_id(world, id_b)) return 0;
    if (world->slots[id_a].shape.type == SICOL_SHAPE_MESH && world->slots[id_b].shape.type != SICOL_SHAPE_MESH) {
        mesh_shape = &world->slots[id_a].shape;
        other_shape = &world->slots[id_b].shape;
        mesh_is_a = 1;
    } else if (world->slots[id_b].shape.type == SICOL_SHAPE_MESH && world->slots[id_a].shape.type != SICOL_SHAPE_MESH) {
        mesh_shape = &world->slots[id_b].shape;
        other_shape = &world->slots[id_a].shape;
        mesh_is_a = 0;
    } else {
        return 0;
    }

    sicol_shape_compute_aabb(other_shape, other_min, other_max);
    sicol_shape_mesh_world_aabb_to_local_aabb(mesh_shape, other_min, other_max, query_min, query_max);
    count = sicol_mesh_query_local_aabb(mesh_shape->u.mesh.mesh,
                                        query_min,
                                        query_max,
                                        candidate_indices,
                                        SICOL_MESH_MAX_TRIANGLES);

    best_hit = 0;
    world_zero_contact(&best_contact);

    for (i = 0; i < count; ++i) {
        sicol_shape_t tri_shape;
        sicol_contact_t contact;
        fx a[3];
        fx b[3];
        fx c[3];
        if (!sicol_shape_get_mesh_triangle_points(mesh_shape, candidate_indices[i], a, b, c)) continue;
        sicol_shape_make_triangle(&tri_shape, a, b, c);
        if (!sicol_narrowphase_test(&tri_shape, other_shape, &contact)) continue;
        contact.feature_a = candidate_indices[i] + 1;
        if (mesh_is_a) {
            world_touch_manifold(world, id_a, id_b, &contact);
        } else {
            world_swap_contact(&contact);
            world_touch_manifold(world, id_a, id_b, &contact);
        }
        if (!best_hit || contact.penetration > best_contact.penetration) {
            best_contact = contact;
            best_hit = 1;
        }
    }

    if (best_hit && out) {
        *out = best_contact;
    }
    return best_hit;
}

static void world_canonical_pair(int* a, int* b)
{
    int tmp;
    if (!a || !b) return;
    if (*a <= *b) return;
    tmp = *a;
    *a = *b;
    *b = tmp;
}

static void world_zero_manifold_point(sicol_manifold_point_t* point)
{
    if (!point) return;
    point->active = 0;
    fx_zero3(point->point);
    fx_zero3(point->normal);
    point->penetration = 0;
    point->feature_a = 0;
    point->feature_b = 0;
    point->age = 0;
    fx_zero3(point->tangent);
    point->normal_impulse = 0;
    point->tangent_impulse = 0;
}

static void world_clear_joint_entry(sicol_joint_t* joint)
{
    if (!joint) return;
    joint->active = 0;
    joint->next_free = -1;
    joint->type = SICOL_JOINT_NONE;
    joint->id_a = -1;
    joint->id_b = -1;
    fx_zero3(joint->local_anchor_a);
    fx_zero3(joint->local_anchor_b);
    fx_zero3(joint->local_axis_a);
    fx_zero3(joint->local_axis_b);
    fx_zero3(joint->local_ref_a);
    fx_zero3(joint->local_ref_b);
    joint->rest_length = 0;
    joint->stiffness = FX_ONE;
    joint->damping = 0;
    fx_zero3(joint->accumulated_impulse);
    fx_zero3(joint->angular_accumulated_impulse);
    joint->motor_accumulated_impulse = 0;
    joint->last_impulse = 0;
    joint->limit_enabled = 0;
    joint->lower_angle = 0;
    joint->upper_angle = 0;
    joint->lower_limit = 0;
    joint->upper_limit = 0;
    joint->cone_angle = 0;
    joint->motor_enabled = 0;
    joint->motor_speed = 0;
    joint->max_motor_impulse = 0;
}

static void world_clear_manifold_entry(sicol_persistent_manifold_t* manifold)
{
    int i;
    if (!manifold) return;
    manifold->active = 0;
    manifold->id_a = -1;
    manifold->id_b = -1;
    manifold->point_count = 0;
    fx_zero3(manifold->normal);
    fx_zero3(manifold->centroid);
    manifold->max_penetration = 0;
    manifold->generation = 0;
    sicol_gjk_cache_reset(&manifold->gjk_cache);
    for (i = 0; i < SICOL_MANIFOLD_MAX_POINTS; ++i) {
        world_zero_manifold_point(&manifold->points[i]);
    }
}

void sicol_world_clear_joints(sicol_world_t* world)
{
    int i;
    if (!world) return;
    world->joint_free_head = 0;
    world->joint_active_count = 0;
    for (i = 0; i < SICOL_WORLD_MAX_JOINTS; ++i) {
        world_clear_joint_entry(&world->joints[i]);
        world->joints[i].next_free = i + 1;
    }
    world->joints[SICOL_WORLD_MAX_JOINTS - 1].next_free = -1;
}

void sicol_world_clear_manifolds(sicol_world_t* world)
{
    int i;
    if (!world) return;
    world->manifold_generation = 0;
    for (i = 0; i < SICOL_WORLD_MAX_MANIFOLDS; ++i) {
        world_clear_manifold_entry(&world->manifolds[i]);
    }
}

static void world_remove_joints_for_id(sicol_world_t* world, int id)
{
    int i;
    if (!world) return;
    for (i = 0; i < SICOL_WORLD_MAX_JOINTS; ++i) {
        if (!world->joints[i].active) continue;
        if (world->joints[i].id_a == id || world->joints[i].id_b == id) {
            sicol_world_joint_kill(world, i);
        }
    }
}

static void world_remove_manifolds_for_id(sicol_world_t* world, int id)
{
    int i;
    if (!world) return;
    for (i = 0; i < SICOL_WORLD_MAX_MANIFOLDS; ++i) {
        if (!world->manifolds[i].active) continue;
        if (world->manifolds[i].id_a == id || world->manifolds[i].id_b == id) {
            world_clear_manifold_entry(&world->manifolds[i]);
        }
    }
}

static sicol_persistent_manifold_t* world_find_manifold_slot(
    sicol_world_t* world,
    int id_a,
    int id_b,
    int create_if_missing
)
{
    int i;
    int free_index;
    int oldest_index;
    unsigned int oldest_generation;

    if (!world) return 0;
    world_canonical_pair(&id_a, &id_b);

    free_index = -1;
    oldest_index = -1;
    oldest_generation = 0;

    for (i = 0; i < SICOL_WORLD_MAX_MANIFOLDS; ++i) {
        if (world->manifolds[i].active) {
            if (world->manifolds[i].id_a == id_a && world->manifolds[i].id_b == id_b) {
                return &world->manifolds[i];
            }
            if (oldest_index < 0 || world->manifolds[i].generation < oldest_generation) {
                oldest_index = i;
                oldest_generation = world->manifolds[i].generation;
            }
        } else if (free_index < 0) {
            free_index = i;
        }
    }

    if (!create_if_missing) return 0;
    if (free_index < 0) free_index = oldest_index;
    if (free_index < 0) return 0;

    world_clear_manifold_entry(&world->manifolds[free_index]);
    world->manifolds[free_index].active = 1;
    world->manifolds[free_index].id_a = id_a;
    world->manifolds[free_index].id_b = id_b;
    world->manifolds[free_index].generation = world->manifold_generation;
    return &world->manifolds[free_index];
}

static void world_refresh_manifold_summary(sicol_persistent_manifold_t* manifold)
{
    int i;
    int count;
    fx centroid[3];
    fx normal[3];
    fx max_pen;

    if (!manifold) return;

    count = 0;
    fx_zero3(centroid);
    fx_zero3(normal);
    max_pen = 0;

    for (i = 0; i < SICOL_MANIFOLD_MAX_POINTS; ++i) {
        if (!manifold->points[i].active) continue;
        centroid[0] += manifold->points[i].point[0];
        centroid[1] += manifold->points[i].point[1];
        centroid[2] += manifold->points[i].point[2];
        normal[0] += manifold->points[i].normal[0];
        normal[1] += manifold->points[i].normal[1];
        normal[2] += manifold->points[i].normal[2];
        if (count == 0 || manifold->points[i].penetration > max_pen) {
            max_pen = manifold->points[i].penetration;
        }
        ++count;
    }

    manifold->point_count = count;
    if (count <= 0) {
        manifold->active = 0;
        return;
    }

    centroid[0] = FX_DIV(centroid[0], FX_FROM_INT(count));
    centroid[1] = FX_DIV(centroid[1], FX_FROM_INT(count));
    centroid[2] = FX_DIV(centroid[2], FX_FROM_INT(count));
    fx_copy3(manifold->centroid, centroid);

    if (!fx_normalize3(manifold->normal, normal)) {
        fx_copy3(manifold->normal, manifold->points[0].normal);
    }

    manifold->max_penetration = max_pen;
}

static int world_manifold_match_point(const sicol_manifold_point_t* point, const sicol_contact_t* contact)
{
    fx dist_sq;
    if (!point || !contact) return 0;
    if (!point->active) return 0;
    if (point->feature_a == contact->feature_a && point->feature_b == contact->feature_b) {
        return 1;
    }
    dist_sq = fx_distance_sq3(point->point, contact->point);
    if (dist_sq <= SICOL_MANIFOLD_POINT_MERGE_TOL_SQ && fx_dot3(point->normal, contact->normal) >= 0) {
        return 1;
    }
    return 0;
}

static fx world_manifold_set_score(const sicol_manifold_point_t* pts, int count)
{
    fx centroid[3];
    fx spread;
    fx max_pen;
    fx pair_sum;
    int i;
    int j;

    fx_zero3(centroid);
    spread = 0;
    max_pen = 0;
    pair_sum = 0;

    if (!pts || count <= 0) return 0;

    for (i = 0; i < count; ++i) {
        centroid[0] += pts[i].point[0];
        centroid[1] += pts[i].point[1];
        centroid[2] += pts[i].point[2];
        if (i == 0 || pts[i].penetration > max_pen) {
            max_pen = pts[i].penetration;
        }
    }
    centroid[0] = FX_DIV(centroid[0], FX_FROM_INT(count));
    centroid[1] = FX_DIV(centroid[1], FX_FROM_INT(count));
    centroid[2] = FX_DIV(centroid[2], FX_FROM_INT(count));

    for (i = 0; i < count; ++i) {
        spread += fx_distance_sq3(pts[i].point, centroid);
        for (j = i + 1; j < count; ++j) {
            pair_sum += fx_distance_sq3(pts[i].point, pts[j].point);
        }
    }

    return max_pen * 16 + spread + pair_sum;
}

static int world_manifold_pick_replacement_slot(
    const sicol_persistent_manifold_t* manifold,
    const sicol_contact_t* contact
)
{
    sicol_manifold_point_t candidates[SICOL_MANIFOLD_MAX_POINTS + 1];
    sicol_manifold_point_t subset[SICOL_MANIFOLD_MAX_POINTS];
    int index_map[SICOL_MANIFOLD_MAX_POINTS + 1];
    int count;
    int i;
    int j;
    int drop;
    int best_drop;
    fx best_score;

    if (!manifold || !contact) return -1;

    count = 0;
    for (i = 0; i < SICOL_MANIFOLD_MAX_POINTS; ++i) {
        if (!manifold->points[i].active) continue;
        candidates[count] = manifold->points[i];
        index_map[count] = i;
        ++count;
    }

    if (count < SICOL_MANIFOLD_MAX_POINTS) {
        for (i = 0; i < SICOL_MANIFOLD_MAX_POINTS; ++i) {
            if (!manifold->points[i].active) return i;
        }
    }

    world_zero_manifold_point(&candidates[count]);
    candidates[count].active = 1;
    fx_copy3(candidates[count].point, contact->point);
    fx_copy3(candidates[count].normal, contact->normal);
    candidates[count].penetration = contact->penetration;
    candidates[count].feature_a = contact->feature_a;
    candidates[count].feature_b = contact->feature_b;
    index_map[count] = -1;
    ++count;

    best_drop = count - 1;
    best_score = -1;

    for (drop = 0; drop < count; ++drop) {
        int subset_count = 0;
        fx score;
        for (j = 0; j < count; ++j) {
            if (j == drop) continue;
            subset[subset_count++] = candidates[j];
        }
        score = world_manifold_set_score(subset, subset_count);
        if (best_drop < 0 || score > best_score) {
            best_drop = drop;
            best_score = score;
        }
    }

    if (best_drop == count - 1) {
        return -1;
    }
    return index_map[best_drop];
}

static void world_manifold_store_contact(
    sicol_persistent_manifold_t* manifold,
    const sicol_contact_t* contact
)
{
    int i;
    int slot;
    int matched;

    if (!manifold || !contact) return;
    manifold->active = 1;

    for (i = 0; i < SICOL_MANIFOLD_MAX_POINTS; ++i) {
        if (manifold->points[i].active) {
            ++manifold->points[i].age;
        }
    }

    slot = -1;
    matched = 0;
    for (i = 0; i < SICOL_MANIFOLD_MAX_POINTS; ++i) {
        if (world_manifold_match_point(&manifold->points[i], contact)) {
            slot = i;
            matched = 1;
            break;
        }
    }

    if (slot < 0) {
        slot = world_manifold_pick_replacement_slot(manifold, contact);
    }
    if (slot < 0) {
        world_refresh_manifold_summary(manifold);
        return;
    }

    if (!matched) {
        fx_zero3(manifold->points[slot].tangent);
        manifold->points[slot].normal_impulse = 0;
        manifold->points[slot].tangent_impulse = 0;
    }

    manifold->points[slot].active = 1;
    fx_copy3(manifold->points[slot].point, contact->point);
    fx_copy3(manifold->points[slot].normal, contact->normal);
    manifold->points[slot].penetration = contact->penetration;
    manifold->points[slot].feature_a = contact->feature_a;
    manifold->points[slot].feature_b = contact->feature_b;
    manifold->points[slot].age = 0;

    world_refresh_manifold_summary(manifold);
}

static void world_prune_manifolds(sicol_world_t* world)
{
    int i;
    if (!world) return;
    for (i = 0; i < SICOL_WORLD_MAX_MANIFOLDS; ++i) {
        if (!world->manifolds[i].active) continue;
        if (world->manifolds[i].generation != world->manifold_generation) {
            world_clear_manifold_entry(&world->manifolds[i]);
            continue;
        }
        if (!world_valid_id(world, world->manifolds[i].id_a) || !world_valid_id(world, world->manifolds[i].id_b)) {
            world_clear_manifold_entry(&world->manifolds[i]);
        }
    }
}

int sicol_world_get_manifold(
    const sicol_world_t* world,
    int id_a,
    int id_b,
    sicol_persistent_manifold_t* out_manifold
)
{
    int i;
    if (!world || !out_manifold) return 0;
    world_canonical_pair(&id_a, &id_b);
    for (i = 0; i < SICOL_WORLD_MAX_MANIFOLDS; ++i) {
        if (!world->manifolds[i].active) continue;
        if (world->manifolds[i].id_a == id_a && world->manifolds[i].id_b == id_b) {
            *out_manifold = world->manifolds[i];
            return 1;
        }
    }
    return 0;
}

static void world_touch_manifold(
    sicol_world_t* world,
    int id_a,
    int id_b,
    const sicol_contact_t* contact
)
{
    sicol_persistent_manifold_t* manifold;
    if (!world || !contact) return;
    world_canonical_pair(&id_a, &id_b);
    manifold = world_find_manifold_slot(world, id_a, id_b, 1);
    if (!manifold) return;
    manifold->generation = world->manifold_generation;
    world_manifold_store_contact(manifold, contact);
}

static int world_support_pair_contact(
    sicol_world_t* world,
    int id_a,
    int id_b,
    sicol_contact_t* out
)
{
    sicol_persistent_manifold_t* manifold;
    sicol_epa_result_t epa;
    const sicol_shape_t* a;
    const sicol_shape_t* b;
    sicol_contact_t contact;

    if (!world_valid_id(world, id_a) || !world_valid_id(world, id_b)) return 0;

    a = &world->slots[id_a].shape;
    b = &world->slots[id_b].shape;
    manifold = world_find_manifold_slot(world, id_a, id_b, 1);
    if (!manifold) return 0;

    if (sicol_gjk_penetration_cached_query(a, b, &manifold->gjk_cache, &epa)) {
        world_zero_contact(&contact);
        fx_copy3(contact.normal, epa.normal);
        fx_copy3(contact.point, epa.point);
        contact.penetration = epa.depth;
        contact.count = 1;
        contact.feature_a = 0;
        contact.feature_b = 0;
        manifold->generation = world->manifold_generation;
        world_manifold_store_contact(manifold, &contact);
        if (out) *out = contact;
        return 1;
    }

    if (sicol_narrowphase_test(a, b, &contact)) {
        manifold->generation = world->manifold_generation;
        world_manifold_store_contact(manifold, &contact);
        if (out) *out = contact;
        return 1;
    }

    sicol_gjk_cache_reset(&manifold->gjk_cache);
    return 0;
}

static int world_find_overlap(
    const sicol_world_t* world,
    const sicol_shape_t* shape,
    uint32_t query_mask,
    int* out_id,
    sicol_contact_t* out_contact,
    int deepest_only
)
{
    int i;
    int found;
    fx best_pen;
    sicol_contact_t contact;

    if (!world || !shape) return 0;

    found = 0;
    best_pen = 0;

    for (i = 0; i < SICOL_WORLD_MAX; ++i) {
        if (!world->slots[i].active) continue;
        if (!world_query_hits_layer(query_mask, world->slots[i].layer)) continue;
        if (world->slots[i].shape.type == SICOL_SHAPE_RAY) continue;

        if (sicol_narrowphase_test(shape, &world->slots[i].shape, &contact)) {
            if (!found || !deepest_only || contact.penetration > best_pen) {
                found = 1;
                best_pen = contact.penetration;
                if (out_id) *out_id = i;
                if (out_contact) *out_contact = contact;
                if (!deepest_only) break;
            }
        }
    }

    return found;
}

static int world_joint_valid_other(const sicol_world_t* world, int id)
{
    if (id < 0) return 1;
    return world_valid_id(world, id);
}

static void world_joint_body_basis(const sicol_world_t* world, int id, fx out_basis[3][3])
{
    if (!out_basis) return;
    if (id < 0 || !world_valid_id(world, id)) {
        fx_identity_mat3(out_basis);
        return;
    }
    sicol_shape_get_basis(&world->slots[id].shape, out_basis);
}

static void world_joint_local_from_world_vector(const sicol_world_t* world, int id, const fx world_v[3], fx out_local[3])
{
    fx basis[3][3];
    if (!out_local) return;
    if (id < 0) {
        fx_copy3(out_local, world_v);
        return;
    }
    world_joint_body_basis(world, id, basis);
    fx_basis_to_local3(out_local, (const fx (*)[3])basis, world_v);
}

static void world_joint_world_from_local_vector(const sicol_world_t* world, int id, const fx local_v[3], fx out_world[3])
{
    fx basis[3][3];
    if (!out_world) return;
    if (id < 0) {
        fx_copy3(out_world, local_v);
        return;
    }
    world_joint_body_basis(world, id, basis);
    fx_basis_to_world3(out_world, (const fx (*)[3])basis, local_v);
}

static void world_pick_any_perpendicular(const fx axis[3], fx out[3])
{
    fx hint[3];
    fx proj[3];
    fx tmp[3];
    if (!out) return;
    if (FX_ABS(axis[0]) < FX_FROM_RATIO(3, 4)) {
        fx_set3(hint, FX_ONE, 0, 0);
    } else {
        fx_set3(hint, 0, FX_ONE, 0);
    }
    fx_scale3(tmp, axis, fx_dot3(hint, axis));
    fx_sub3(proj, hint, tmp);
    if (!fx_normalize3(out, proj)) {
        fx_set3(out, 0, 0, FX_ONE);
    }
}

static void world_pick_perpendicular_from_hint(const fx axis[3], const fx hint[3], fx out[3])
{
    fx proj[3];
    fx tmp[3];
    if (!out) return;
    if (!hint) {
        world_pick_any_perpendicular(axis, out);
        return;
    }
    fx_scale3(tmp, axis, fx_dot3(hint, axis));
    fx_sub3(proj, hint, tmp);
    if (!fx_normalize3(out, proj)) {
        world_pick_any_perpendicular(axis, out);
    }
}

int sicol_world_joint_is_alive(const sicol_world_t* world, int joint_id)
{
    if (!world) return 0;
    if (joint_id < 0 || joint_id >= SICOL_WORLD_MAX_JOINTS) return 0;
    if (!world->joints[joint_id].active) return 0;
    if (!world_valid_id(world, world->joints[joint_id].id_a)) return 0;
    if (!world_joint_valid_other(world, world->joints[joint_id].id_b)) return 0;
    return 1;
}

void sicol_world_joint_kill(sicol_world_t* world, int joint_id)
{
    if (!world) return;
    if (joint_id < 0 || joint_id >= SICOL_WORLD_MAX_JOINTS) return;
    if (!world->joints[joint_id].active) return;
    world_clear_joint_entry(&world->joints[joint_id]);
    world->joints[joint_id].next_free = world->joint_free_head;
    world->joint_free_head = joint_id;
    if (world->joint_active_count > 0) --world->joint_active_count;
}

int sicol_world_joint_get(const sicol_world_t* world, int joint_id, sicol_joint_t* out_joint)
{
    if (!out_joint) return 0;
    if (!sicol_world_joint_is_alive(world, joint_id)) return 0;
    *out_joint = world->joints[joint_id];
    return 1;
}

static int world_joint_alloc(sicol_world_t* world)
{
    int joint_id;
    if (!world) return -1;
    if (world->joint_free_head < 0) return -1;
    joint_id = world->joint_free_head;
    world->joint_free_head = world->joints[joint_id].next_free;
    world_clear_joint_entry(&world->joints[joint_id]);
    world->joints[joint_id].active = 1;
    world->joints[joint_id].next_free = -1;
    ++world->joint_active_count;
    return joint_id;
}

static void world_joint_setup_oriented_frame(
    const sicol_world_t* world,
    sicol_joint_t* joint,
    int id_a,
    const fx local_axis_a[3],
    int id_b,
    const fx axis_b_or_world[3]
)
{
    fx basis_a[3][3];
    fx axis_world[3];
    fx ref_world[3];
    fx hint[3];
    fx norm_axis[3];

    if (!world || !joint) return;

    if (!local_axis_a || !fx_normalize3(norm_axis, local_axis_a)) {
        fx_set3(joint->local_axis_a, FX_ONE, 0, 0);
    } else {
        fx_copy3(joint->local_axis_a, norm_axis);
    }

    world_joint_body_basis(world, id_a, basis_a);
    world_joint_world_from_local_vector(world, id_a, joint->local_axis_a, axis_world);
    if (!fx_normalize3(axis_world, axis_world)) {
        fx_set3(axis_world, FX_ONE, 0, 0);
    }

    if (id_b >= 0) {
        if (axis_b_or_world && fx_normalize3(norm_axis, axis_b_or_world)) {
            fx_copy3(joint->local_axis_b, norm_axis);
        } else {
            world_joint_local_from_world_vector(world, id_b, axis_world, joint->local_axis_b);
            if (!fx_normalize3(joint->local_axis_b, joint->local_axis_b)) {
                fx_set3(joint->local_axis_b, FX_ONE, 0, 0);
            }
        }
    } else {
        if (axis_b_or_world && fx_normalize3(norm_axis, axis_b_or_world)) {
            fx_copy3(joint->local_axis_b, norm_axis);
        } else {
            fx_copy3(joint->local_axis_b, axis_world);
        }
    }

    fx_copy3(hint, basis_a[1]);
    world_pick_perpendicular_from_hint(axis_world, hint, ref_world);
    world_joint_local_from_world_vector(world, id_a, ref_world, joint->local_ref_a);
    world_joint_local_from_world_vector(world, id_b, ref_world, joint->local_ref_b);
}

int sicol_world_joint_create_distance(
    sicol_world_t* world,
    int id_a,
    const fx local_anchor_a[3],
    int id_b,
    const fx anchor_b[3],
    fx rest_length,
    fx stiffness,
    fx damping
)
{
    int joint_id;
    sicol_joint_t* joint;
    if (!world_valid_id(world, id_a)) return -1;
    if (!world_joint_valid_other(world, id_b)) return -1;
    joint_id = world_joint_alloc(world);
    if (joint_id < 0) return -1;
    joint = &world->joints[joint_id];
    joint->type = SICOL_JOINT_DISTANCE;
    joint->id_a = id_a;
    joint->id_b = id_b;
    if (local_anchor_a) fx_copy3(joint->local_anchor_a, local_anchor_a); else fx_zero3(joint->local_anchor_a);
    if (anchor_b) fx_copy3(joint->local_anchor_b, anchor_b); else fx_zero3(joint->local_anchor_b);
    joint->rest_length = FX_ABS(rest_length);
    joint->stiffness = (stiffness > 0) ? stiffness : FX_ONE;
    joint->damping = FX_MAX(damping, 0);
    return joint_id;
}

int sicol_world_joint_create_spring(
    sicol_world_t* world,
    int id_a,
    const fx local_anchor_a[3],
    int id_b,
    const fx anchor_b[3],
    fx rest_length,
    fx stiffness,
    fx damping
)
{
    int joint_id;
    sicol_joint_t* joint;
    if (!world_valid_id(world, id_a)) return -1;
    if (!world_joint_valid_other(world, id_b)) return -1;
    joint_id = world_joint_alloc(world);
    if (joint_id < 0) return -1;
    joint = &world->joints[joint_id];
    joint->type = SICOL_JOINT_SPRING;
    joint->id_a = id_a;
    joint->id_b = id_b;
    if (local_anchor_a) fx_copy3(joint->local_anchor_a, local_anchor_a); else fx_zero3(joint->local_anchor_a);
    if (anchor_b) fx_copy3(joint->local_anchor_b, anchor_b); else fx_zero3(joint->local_anchor_b);
    joint->rest_length = FX_ABS(rest_length);
    joint->stiffness = (stiffness > 0) ? stiffness : FX_FROM_RATIO(1, 2);
    joint->damping = FX_MAX(damping, 0);
    return joint_id;
}

int sicol_world_joint_create_point(
    sicol_world_t* world,
    int id_a,
    const fx local_anchor_a[3],
    int id_b,
    const fx anchor_b[3],
    fx stiffness,
    fx damping
)
{
    int joint_id;
    sicol_joint_t* joint;
    if (!world_valid_id(world, id_a)) return -1;
    if (!world_joint_valid_other(world, id_b)) return -1;
    joint_id = world_joint_alloc(world);
    if (joint_id < 0) return -1;
    joint = &world->joints[joint_id];
    joint->type = SICOL_JOINT_POINT;
    joint->id_a = id_a;
    joint->id_b = id_b;
    if (local_anchor_a) fx_copy3(joint->local_anchor_a, local_anchor_a); else fx_zero3(joint->local_anchor_a);
    if (anchor_b) fx_copy3(joint->local_anchor_b, anchor_b); else fx_zero3(joint->local_anchor_b);
    joint->rest_length = 0;
    joint->stiffness = (stiffness > 0) ? stiffness : FX_ONE;
    joint->damping = FX_MAX(damping, 0);
    return joint_id;
}

int sicol_world_joint_create_fixed(
    sicol_world_t* world,
    int id_a,
    const fx local_anchor_a[3],
    int id_b,
    const fx anchor_b[3],
    fx stiffness,
    fx damping
)
{
    int joint_id;
    sicol_joint_t* joint;
    fx basis_a[3][3];
    fx world_axis[3];
    fx world_ref[3];
    if (!world_valid_id(world, id_a)) return -1;
    if (!world_joint_valid_other(world, id_b)) return -1;
    joint_id = world_joint_alloc(world);
    if (joint_id < 0) return -1;
    joint = &world->joints[joint_id];
    joint->type = SICOL_JOINT_FIXED;
    joint->id_a = id_a;
    joint->id_b = id_b;
    if (local_anchor_a) fx_copy3(joint->local_anchor_a, local_anchor_a); else fx_zero3(joint->local_anchor_a);
    if (anchor_b) fx_copy3(joint->local_anchor_b, anchor_b); else fx_zero3(joint->local_anchor_b);
    joint->rest_length = 0;
    joint->stiffness = (stiffness > 0) ? stiffness : FX_ONE;
    joint->damping = FX_MAX(damping, 0);
    fx_set3(joint->local_axis_a, FX_ONE, 0, 0);
    fx_set3(joint->local_ref_a, 0, FX_ONE, 0);
    world_joint_body_basis(world, id_a, basis_a);
    fx_copy3(world_axis, basis_a[0]);
    fx_copy3(world_ref, basis_a[1]);
    world_joint_local_from_world_vector(world, id_b, world_axis, joint->local_axis_b);
    world_joint_local_from_world_vector(world, id_b, world_ref, joint->local_ref_b);
    return joint_id;
}

int sicol_world_joint_create_hinge(
    sicol_world_t* world,
    int id_a,
    const fx local_anchor_a[3],
    const fx local_axis_a[3],
    int id_b,
    const fx anchor_b[3],
    const fx axis_b_or_world[3],
    fx stiffness,
    fx damping
)
{
    int joint_id;
    sicol_joint_t* joint;
    if (!world_valid_id(world, id_a)) return -1;
    if (!world_joint_valid_other(world, id_b)) return -1;
    joint_id = world_joint_alloc(world);
    if (joint_id < 0) return -1;
    joint = &world->joints[joint_id];
    joint->type = SICOL_JOINT_HINGE;
    joint->id_a = id_a;
    joint->id_b = id_b;
    if (local_anchor_a) fx_copy3(joint->local_anchor_a, local_anchor_a); else fx_zero3(joint->local_anchor_a);
    if (anchor_b) fx_copy3(joint->local_anchor_b, anchor_b); else fx_zero3(joint->local_anchor_b);
    world_joint_setup_oriented_frame(world, joint, id_a, local_axis_a, id_b, axis_b_or_world);
    joint->rest_length = 0;
    joint->stiffness = (stiffness > 0) ? stiffness : FX_ONE;
    joint->damping = FX_MAX(damping, 0);
    joint->limit_enabled = 0;
    joint->motor_enabled = 0;
    return joint_id;
}

int sicol_world_joint_create_slider(
    sicol_world_t* world,
    int id_a,
    const fx local_anchor_a[3],
    const fx local_axis_a[3],
    int id_b,
    const fx anchor_b[3],
    const fx axis_b_or_world[3],
    fx stiffness,
    fx damping
)
{
    int joint_id;
    sicol_joint_t* joint;
    if (!world_valid_id(world, id_a)) return -1;
    if (!world_joint_valid_other(world, id_b)) return -1;
    joint_id = world_joint_alloc(world);
    if (joint_id < 0) return -1;
    joint = &world->joints[joint_id];
    joint->type = SICOL_JOINT_SLIDER;
    joint->id_a = id_a;
    joint->id_b = id_b;
    if (local_anchor_a) fx_copy3(joint->local_anchor_a, local_anchor_a); else fx_zero3(joint->local_anchor_a);
    if (anchor_b) fx_copy3(joint->local_anchor_b, anchor_b); else fx_zero3(joint->local_anchor_b);
    world_joint_setup_oriented_frame(world, joint, id_a, local_axis_a, id_b, axis_b_or_world);
    joint->stiffness = (stiffness > 0) ? stiffness : FX_ONE;
    joint->damping = FX_MAX(damping, 0);
    joint->limit_enabled = 0;
    joint->motor_enabled = 0;
    joint->lower_limit = 0;
    joint->upper_limit = 0;
    return joint_id;
}

int sicol_world_joint_create_cone_twist(
    sicol_world_t* world,
    int id_a,
    const fx local_anchor_a[3],
    const fx local_axis_a[3],
    int id_b,
    const fx anchor_b[3],
    const fx axis_b_or_world[3],
    fx cone_angle,
    fx stiffness,
    fx damping
)
{
    int joint_id;
    sicol_joint_t* joint;
    if (!world_valid_id(world, id_a)) return -1;
    if (!world_joint_valid_other(world, id_b)) return -1;
    joint_id = world_joint_alloc(world);
    if (joint_id < 0) return -1;
    joint = &world->joints[joint_id];
    joint->type = SICOL_JOINT_CONE_TWIST;
    joint->id_a = id_a;
    joint->id_b = id_b;
    if (local_anchor_a) fx_copy3(joint->local_anchor_a, local_anchor_a); else fx_zero3(joint->local_anchor_a);
    if (anchor_b) fx_copy3(joint->local_anchor_b, anchor_b); else fx_zero3(joint->local_anchor_b);
    world_joint_setup_oriented_frame(world, joint, id_a, local_axis_a, id_b, axis_b_or_world);
    joint->stiffness = (stiffness > 0) ? stiffness : FX_ONE;
    joint->damping = FX_MAX(damping, 0);
    joint->cone_angle = FX_ABS(cone_angle);
    joint->limit_enabled = 0;
    joint->motor_enabled = 0;
    return joint_id;
}

int sicol_world_joint_set_hinge_limits(
    sicol_world_t* world,
    int joint_id,
    fx lower_angle,
    fx upper_angle
)
{
    sicol_joint_t* joint;
    if (!world) return 0;
    if (!sicol_world_joint_is_alive(world, joint_id)) return 0;
    joint = &world->joints[joint_id];
    if (joint->type != SICOL_JOINT_HINGE) return 0;
    if (lower_angle > upper_angle) {
        fx tmp;
        tmp = lower_angle;
        lower_angle = upper_angle;
        upper_angle = tmp;
    }
    joint->limit_enabled = 1;
    joint->lower_angle = lower_angle;
    joint->upper_angle = upper_angle;
    return 1;
}

int sicol_world_joint_set_hinge_motor(
    sicol_world_t* world,
    int joint_id,
    fx motor_speed,
    fx max_motor_impulse
)
{
    sicol_joint_t* joint;
    if (!world) return 0;
    if (!sicol_world_joint_is_alive(world, joint_id)) return 0;
    joint = &world->joints[joint_id];
    if (joint->type != SICOL_JOINT_HINGE) return 0;
    joint->motor_speed = motor_speed;
    joint->max_motor_impulse = FX_ABS(max_motor_impulse);
    joint->motor_enabled = (joint->max_motor_impulse > 0) ? 1 : 0;
    if (!joint->motor_enabled) joint->motor_accumulated_impulse = 0;
    return 1;
}

int sicol_world_joint_set_slider_limits(
    sicol_world_t* world,
    int joint_id,
    fx lower_distance,
    fx upper_distance
)
{
    sicol_joint_t* joint;
    if (!world) return 0;
    if (!sicol_world_joint_is_alive(world, joint_id)) return 0;
    joint = &world->joints[joint_id];
    if (joint->type != SICOL_JOINT_SLIDER) return 0;
    if (lower_distance > upper_distance) {
        fx tmp;
        tmp = lower_distance;
        lower_distance = upper_distance;
        upper_distance = tmp;
    }
    joint->limit_enabled = 1;
    joint->lower_limit = lower_distance;
    joint->upper_limit = upper_distance;
    return 1;
}

int sicol_world_joint_set_slider_motor(
    sicol_world_t* world,
    int joint_id,
    fx motor_speed,
    fx max_motor_impulse
)
{
    sicol_joint_t* joint;
    if (!world) return 0;
    if (!sicol_world_joint_is_alive(world, joint_id)) return 0;
    joint = &world->joints[joint_id];
    if (joint->type != SICOL_JOINT_SLIDER) return 0;
    joint->motor_speed = motor_speed;
    joint->max_motor_impulse = FX_ABS(max_motor_impulse);
    joint->motor_enabled = (joint->max_motor_impulse > 0) ? 1 : 0;
    if (!joint->motor_enabled) joint->motor_accumulated_impulse = 0;
    return 1;
}

int sicol_world_joint_set_cone_twist_limits(
    sicol_world_t* world,
    int joint_id,
    fx cone_angle,
    fx twist_lower,
    fx twist_upper
)
{
    sicol_joint_t* joint;
    if (!world) return 0;
    if (!sicol_world_joint_is_alive(world, joint_id)) return 0;
    joint = &world->joints[joint_id];
    if (joint->type != SICOL_JOINT_CONE_TWIST) return 0;
    if (twist_lower > twist_upper) {
        fx tmp;
        tmp = twist_lower;
        twist_lower = twist_upper;
        twist_upper = tmp;
    }
    joint->cone_angle = FX_ABS(cone_angle);
    joint->limit_enabled = 1;
    joint->lower_angle = twist_lower;
    joint->upper_angle = twist_upper;
    return 1;
}

void sicol_world_init(sicol_world_t* world)
{
    int i;
    if (!world) return;

    world->free_head = 0;
    world->active_count = 0;

    for (i = 0; i < SICOL_WORLD_MAX; ++i) {
        world->slots[i].active = 0;
        world->slots[i].next_free = i + 1;
        world->slots[i].layer = 1u;
        world->slots[i].mask = 0xFFFFFFFFu;
        sicol_shape_make_none(&world->slots[i].shape);
    }
    world->slots[SICOL_WORLD_MAX - 1].next_free = -1;

    sicol_world_clear_manifolds(world);
    sicol_world_clear_joints(world);
}

int sicol_world_create(sicol_world_t* world, const sicol_shape_t* shape, uint32_t layer, uint32_t mask)
{
    int id;
    if (!world || !shape) return -1;
    if (!sicol_shape_validate(shape)) return -1;
    if (world->free_head < 0) return -1;

    id = world->free_head;
    world->free_head = world->slots[id].next_free;
    world->slots[id].active = 1;
    world->slots[id].next_free = -1;
    world->slots[id].shape = *shape;
    world->slots[id].layer = layer;
    world->slots[id].mask = mask;
    ++world->active_count;
    return id;
}

int sicol_world_is_alive(const sicol_world_t* world, int id)
{
    return world_valid_id(world, id);
}

void sicol_world_kill(sicol_world_t* world, int id)
{
    if (!world_valid_id(world, id)) return;
    world->slots[id].active = 0;
    world->slots[id].next_free = world->free_head;
    world->free_head = id;
    if (world->active_count > 0) --world->active_count;
    world_remove_manifolds_for_id(world, id);
    world_remove_joints_for_id(world, id);
}

int sicol_world_set_shape(sicol_world_t* world, int id, const sicol_shape_t* shape)
{
    if (!world_valid_id(world, id) || !shape) return 0;
    if (!sicol_shape_validate(shape)) return 0;
    world->slots[id].shape = *shape;
    return 1;
}

int sicol_world_get_shape(const sicol_world_t* world, int id, sicol_shape_t* out_shape)
{
    if (!world_valid_id(world, id) || !out_shape) return 0;
    *out_shape = world->slots[id].shape;
    return 1;
}

int sicol_world_set_position(sicol_world_t* world, int id, const fx pos[3])
{
    if (!world_valid_id(world, id) || !pos) return 0;

    if (world->slots[id].shape.type == SICOL_SHAPE_SEGMENT) {
        fx old_from[3];
        fx delta[3];
        fx_copy3(old_from, world->slots[id].shape.pos);
        fx_sub3(delta, pos, old_from);
        fx_add3(world->slots[id].shape.u.segment.to, world->slots[id].shape.u.segment.to, delta);
    }

    fx_copy3(world->slots[id].shape.pos, pos);
    return 1;
}

int sicol_world_set_layer_mask(sicol_world_t* world, int id, uint32_t layer, uint32_t mask)
{
    if (!world_valid_id(world, id)) return 0;
    world->slots[id].layer = layer;
    world->slots[id].mask = mask;
    return 1;
}

int sicol_world_overlap_pair(const sicol_world_t* world, int id_a, int id_b, sicol_contact_t* out)
{
    const sicol_world_slot_t* a;
    const sicol_world_slot_t* b;

    if (!world_valid_id(world, id_a) || !world_valid_id(world, id_b)) return 0;
    a = &world->slots[id_a];
    b = &world->slots[id_b];

    if (!world_masks_allow(a->layer, a->mask, b->layer, b->mask)) return 0;
    return sicol_narrowphase_test(&a->shape, &b->shape, out);
}

int sicol_world_overlap_shape(
    const sicol_world_t* world,
    const sicol_shape_t* shape,
    uint32_t query_mask,
    sicol_world_overlap_t* out_hits,
    int max_hits
)
{
    int i;
    int count;
    sicol_contact_t contact;

    if (!world || !shape || !out_hits || max_hits <= 0) return 0;

    count = 0;
    for (i = 0; i < SICOL_WORLD_MAX; ++i) {
        if (!world->slots[i].active) continue;
        if (!world_query_hits_layer(query_mask, world->slots[i].layer)) continue;
        if (world->slots[i].shape.type == SICOL_SHAPE_RAY) continue;

        if (sicol_narrowphase_test(shape, &world->slots[i].shape, &contact)) {
            if (count < max_hits) {
                out_hits[count].id = i;
                out_hits[count].contact = contact;
                ++count;
            } else {
                break;
            }
        }
    }

    return count;
}

int sicol_world_raycast(
    const sicol_world_t* world,
    const sicol_shape_t* ray,
    uint32_t query_mask,
    sicol_world_raycast_hit_t* out_hit
)
{
    int i;
    int found;
    sicol_hit_t hit;
    sicol_hit_t best;

    if (!world || !ray || ray->type != SICOL_SHAPE_RAY) return 0;

    found = 0;
    best.t = ray->u.ray.length;
    fx_zero3(best.point);
    fx_zero3(best.normal);
    best.shape_index = -1;

    for (i = 0; i < SICOL_WORLD_MAX; ++i) {
        if (!world->slots[i].active) continue;
        if (!world_query_hits_layer(query_mask, world->slots[i].layer)) continue;
        if (world->slots[i].shape.type == SICOL_SHAPE_RAY) continue;

        if (sicol_raycast(ray, &world->slots[i].shape, &hit)) {
            if (!found || hit.t < best.t) {
                best = hit;
                best.shape_index = i;
                found = 1;
            }
        }
    }

    if (found && out_hit) {
        out_hit->id = best.shape_index;
        out_hit->hit = best;
    }

    return found;
}

int sicol_world_raycast_all(
    const sicol_world_t* world,
    const sicol_shape_t* ray,
    uint32_t query_mask,
    sicol_world_raycast_hit_t* out_hits,
    int max_hits
)
{
    int i;
    int count;
    sicol_hit_t hit;

    if (!world || !ray || ray->type != SICOL_SHAPE_RAY || !out_hits || max_hits <= 0) return 0;

    count = 0;
    for (i = 0; i < SICOL_WORLD_MAX; ++i) {
        int j;
        if (!world->slots[i].active) continue;
        if (!world_query_hits_layer(query_mask, world->slots[i].layer)) continue;
        if (world->slots[i].shape.type == SICOL_SHAPE_RAY) continue;

        if (!sicol_raycast(ray, &world->slots[i].shape, &hit)) continue;

        if (count < max_hits) {
            out_hits[count].id = i;
            out_hits[count].hit = hit;
            ++count;
        } else {
            int farthest = 0;
            for (j = 1; j < max_hits; ++j) {
                if (out_hits[j].hit.t > out_hits[farthest].hit.t) farthest = j;
            }
            if (hit.t < out_hits[farthest].hit.t) {
                out_hits[farthest].id = i;
                out_hits[farthest].hit = hit;
            }
            count = max_hits;
        }
    }

    for (i = 1; i < count; ++i) {
        sicol_world_raycast_hit_t key = out_hits[i];
        int j = i - 1;
        while (j >= 0 && out_hits[j].hit.t > key.hit.t) {
            out_hits[j + 1] = out_hits[j];
            --j;
        }
        out_hits[j + 1] = key;
    }

    return count;
}


static void world_compute_swept_aabb(
    const sicol_shape_t* shape,
    const fx delta[3],
    fx out_min[3],
    fx out_max[3]
)
{
    sicol_shape_t moved;
    fx min_a[3];
    fx max_a[3];
    fx min_b[3];
    fx max_b[3];
    int i;

    sicol_shape_compute_aabb(shape, min_a, max_a);
    moved = *shape;
    sicol_shape_translate(&moved, delta);
    sicol_shape_compute_aabb(&moved, min_b, max_b);

    for (i = 0; i < 3; ++i) {
        out_min[i] = FX_MIN(min_a[i], min_b[i]);
        out_max[i] = FX_MAX(max_a[i], max_b[i]);
    }
}

static int world_aabb_ranges_overlap(const fx min_a[3], const fx max_a[3], const fx min_b[3], const fx max_b[3])
{
    if (max_a[0] < min_b[0] || max_b[0] < min_a[0]) return 0;
    if (max_a[1] < min_b[1] || max_b[1] < min_a[1]) return 0;
    if (max_a[2] < min_b[2] || max_b[2] < min_a[2]) return 0;
    return 1;
}

int sicol_world_cast_shape_ex(
    const sicol_world_t* world,
    const sicol_shape_t* moving_shape,
    const fx delta[3],
    uint32_t query_mask,
    uint32_t moving_layer,
    uint32_t moving_mask,
    int exclude_id,
    sicol_world_cast_hit_t* out_hit
)
{
    int i;
    int best_id;
    int found;
    fx best_fraction;
    fx best_safe_fraction;
    fx swept_min[3];
    fx swept_max[3];
    sicol_contact_t best_contact;

    if (!world || !moving_shape || !delta) return 0;
    if (!sicol_shape_validate(moving_shape)) return 0;

    world_zero_contact(&best_contact);
    best_id = -1;
    found = 0;

    world_compute_swept_aabb(moving_shape, delta, swept_min, swept_max);

    for (i = 0; i < SICOL_WORLD_MAX; ++i) {
        sicol_shape_cast_result_t cast_hit;
        sicol_gjk_cache_t cache;
        fx target_min[3];
        fx target_max[3];

        if (!world->slots[i].active) continue;
        if (i == exclude_id) continue;
        if (!world_query_hits_layer(query_mask, world->slots[i].layer)) continue;
        if (!world_masks_allow(moving_layer, moving_mask, world->slots[i].layer, world->slots[i].mask)) continue;
        if (world->slots[i].shape.type == SICOL_SHAPE_RAY) continue;

        if (sicol_narrowphase_test(moving_shape, &world->slots[i].shape, &best_contact)) {
            best_id = i;
            if (out_hit) {
                out_hit->id = best_id;
                out_hit->fraction = 0;
                fx_copy3(out_hit->position, moving_shape->pos);
                out_hit->contact = best_contact;
            }
            return 1;
        }

        if (world->slots[i].shape.type != SICOL_SHAPE_PLANE) {
            sicol_shape_compute_aabb(&world->slots[i].shape, target_min, target_max);
            if (!world_aabb_ranges_overlap(swept_min, swept_max, target_min, target_max)) {
                continue;
            }
        }

        sicol_gjk_cache_reset(&cache);
        if (!sicol_shape_cast_pair(moving_shape, delta, &world->slots[i].shape, &cache, &cast_hit)) {
            continue;
        }

        if (!found || cast_hit.fraction < best_fraction) {
            found = 1;
            best_fraction = cast_hit.fraction;
            best_safe_fraction = cast_hit.safe_fraction;
            best_id = i;
            fx_copy3(best_contact.normal, cast_hit.normal);
            fx_copy3(best_contact.point, cast_hit.point);
            best_contact.penetration = 0;
            best_contact.feature_a = 0;
            best_contact.feature_b = 0;
            best_contact.count = cast_hit.hit ? 1 : 0;
        }
    }

    if (!found) return 0;

    if (out_hit) {
        fx position[3];
        fx_madd3(position, moving_shape->pos, delta, best_safe_fraction);
        out_hit->id = best_id;
        out_hit->fraction = best_fraction;
        fx_copy3(out_hit->position, position);
        out_hit->contact = best_contact;
    }
    return 1;
}

int sicol_world_cast_shape(
    const sicol_world_t* world,
    const sicol_shape_t* moving_shape,
    const fx delta[3],
    uint32_t query_mask,
    sicol_world_cast_hit_t* out_hit
)
{
    return sicol_world_cast_shape_ex(
        world,
        moving_shape,
        delta,
        query_mask,
        0xFFFFFFFFu,
        0xFFFFFFFFu,
        -1,
        out_hit
    );
}

int sicol_world_depenetrate_shape(
    const sicol_world_t* world,
    sicol_shape_t* io_shape,
    uint32_t query_mask,
    int max_iterations,
    sicol_contact_t* out_last_contact
)
{
    int iter;
    int hit_id;
    fx push[3];
    sicol_contact_t deepest;

    if (!world || !io_shape) return 0;
    if (!sicol_shape_validate(io_shape)) return 0;
    if (max_iterations <= 0) max_iterations = 8;
    world_zero_contact(&deepest);

    for (iter = 0; iter < max_iterations; ++iter) {
        if (!world_find_overlap(world, io_shape, query_mask, &hit_id, &deepest, 1)) {
            if (out_last_contact) *out_last_contact = deepest;
            return 1;
        }

        fx_scale3(push, deepest.normal, -(deepest.penetration + SICOL_WORLD_DEPEN_EPS));
        sicol_shape_translate(io_shape, push);
    }

    if (out_last_contact) *out_last_contact = deepest;
    return !world_find_overlap(world, io_shape, query_mask, &hit_id, &deepest, 1);
}

static int world_pair_contact_persistent(
    sicol_world_t* world,
    int id_a,
    int id_b,
    sicol_contact_t* out
)
{
    const sicol_world_slot_t* a;
    const sicol_world_slot_t* b;
    sicol_contact_t contact;

    if (!world_valid_id(world, id_a) || !world_valid_id(world, id_b)) return 0;
    a = &world->slots[id_a];
    b = &world->slots[id_b];

    if (!world_masks_allow(a->layer, a->mask, b->layer, b->mask)) return 0;

    if ((a->shape.type == SICOL_SHAPE_MESH && b->shape.type != SICOL_SHAPE_MESH) ||
        (a->shape.type != SICOL_SHAPE_MESH && b->shape.type == SICOL_SHAPE_MESH)) {
        if (world_mesh_pair_contact_persistent(world, id_a, id_b, &contact)) {
            if (out) *out = contact;
            return 1;
        }
    }

    if (sicol_shape_is_support_mapped(&a->shape) && sicol_shape_is_support_mapped(&b->shape)) {
        if (world_support_pair_contact(world, id_a, id_b, &contact)) {
            if (out) *out = contact;
            return 1;
        }
    }

    if (sicol_narrowphase_test(&a->shape, &b->shape, &contact)) {
        world_touch_manifold(world, id_a, id_b, &contact);
        if (out) *out = contact;
        return 1;
    }

    return 0;
}

int sicol_world_collide_persistent(
    sicol_world_t* world,
    sicol_pair_t* out_pairs,
    sicol_contact_t* out_contacts,
    int max_results
)
{
    sicol_shape_t shapes[SICOL_WORLD_MAX];
    int ids[SICOL_WORLD_MAX];
    sicol_pair_t candidates[SICOL_BP_MAX_PAIRS];
    sicol_bp_result_t bp_result;
    int shape_count;
    int result_count;
    int i;
    int j;
    sicol_contact_t contact;

    if (!world || !out_pairs || !out_contacts || max_results <= 0) return 0;

    ++world->manifold_generation;
    if (world->manifold_generation == 0) {
        world->manifold_generation = 1;
    }

    shape_count = 0;
    result_count = 0;

    for (i = 0; i < SICOL_WORLD_MAX; ++i) {
        if (!world->slots[i].active) continue;
        if (!world_is_volume(&world->slots[i].shape)) continue;
        shapes[shape_count] = world->slots[i].shape;
        ids[shape_count] = i;
        ++shape_count;
    }

    bp_result = sicol_broadphase_sap_ex(shapes, shape_count, candidates, SICOL_BP_MAX_PAIRS);

    for (i = 0; i < bp_result.pair_count; ++i) {
        int id_a = ids[candidates[i].a];
        int id_b = ids[candidates[i].b];

        if (world_pair_contact_persistent(world, id_a, id_b, &contact)) {
            if (result_count < max_results) {
                out_pairs[result_count].a = id_a;
                out_pairs[result_count].b = id_b;
                out_contacts[result_count] = contact;
                ++result_count;
            }
        }
    }

    for (i = 0; i < SICOL_WORLD_MAX; ++i) {
        if (!world->slots[i].active) continue;
        if (world->slots[i].shape.type != SICOL_SHAPE_PLANE) continue;

        for (j = 0; j < SICOL_WORLD_MAX; ++j) {
            int pair_a;
            int pair_b;
            int duplicate;
            int k;

            if (!world->slots[j].active || i == j) continue;
            if (!world_is_volume(&world->slots[j].shape)) continue;
            if (!world_masks_allow(world->slots[i].layer, world->slots[i].mask,
                                   world->slots[j].layer, world->slots[j].mask)) {
                continue;
            }

            if (!world_pair_contact_persistent(world, i, j, &contact)) continue;

            pair_a = i;
            pair_b = j;
            world_canonical_pair(&pair_a, &pair_b);
            duplicate = 0;
            for (k = 0; k < result_count; ++k) {
                int pa = out_pairs[k].a;
                int pb = out_pairs[k].b;
                world_canonical_pair(&pa, &pb);
                if (pa == pair_a && pb == pair_b) {
                    duplicate = 1;
                    break;
                }
            }

            if (!duplicate && result_count < max_results) {
                out_pairs[result_count].a = i;
                out_pairs[result_count].b = j;
                out_contacts[result_count] = contact;
                ++result_count;
            }
        }
    }

    world_prune_manifolds(world);
    return result_count;
}

int sicol_world_collide(
    sicol_world_t* world,
    sicol_pair_t* out_pairs,
    sicol_contact_t* out_contacts,
    int max_results
)
{
    return sicol_world_collide_persistent(world, out_pairs, out_contacts, max_results);
}
