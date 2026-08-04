/* ============================================================
 * SICOL - Warm-started contact solver
 * ============================================================ */

#include "sicol_solver.h"

#define SICOL_SOLVER_MAX_RESULTS SICOL_BP_MAX_PAIRS
#define SICOL_SOLVER_POS_ANGULAR_SCALE FX_FROM_RATIO(1, 4)

static sicol_persistent_manifold_t* solver_find_manifold(sicol_world_t* world, int id_a, int id_b)
{
    int i;
    int a;
    int b;
    int tmp;
    if (!world) return 0;
    a = id_a;
    b = id_b;
    if (a > b) {
        tmp = a;
        a = b;
        b = tmp;
    }
    for (i = 0; i < SICOL_WORLD_MAX_MANIFOLDS; ++i) {
        if (!world->manifolds[i].active) continue;
        if (world->manifolds[i].id_a == a && world->manifolds[i].id_b == b) {
            return &world->manifolds[i];
        }
    }
    return 0;
}

static int solver_shape_supports_rotation(const sicol_shape_t* shape)
{
    if (!shape) return 0;
    switch (shape->type) {
    case SICOL_SHAPE_OBB:
    case SICOL_SHAPE_CAPSULE:
    case SICOL_SHAPE_CONVEX:
    case SICOL_SHAPE_SEGMENT:
    case SICOL_SHAPE_RAY:
        return 1;
    default:
        break;
    }
    return 0;
}

static int solver_body_is_dynamic(const sicol_world_t* world, const sicol_solver_body_t* body, int id)
{
    const sicol_shape_t* shape;
    if (!world || !body) return 0;
    if (id < 0 || id >= SICOL_WORLD_MAX) return 0;
    if (!world->slots[id].active) return 0;
    if (body->inv_mass <= 0) return 0;
    shape = &world->slots[id].shape;
    if (shape->type == SICOL_SHAPE_PLANE) return 0;
    if (shape->type == SICOL_SHAPE_RAY) return 0;
    return 1;
}

static int solver_body_can_move(const sicol_world_t* world, const sicol_solver_body_t* body, int id)
{
    if (!solver_body_is_dynamic(world, body, id)) return 0;
    if (body->sleeping) return 0;
    return 1;
}

static void solver_zero_mat3(fx out[3][3])
{
    int i;
    int j;
    for (i = 0; i < 3; ++i) {
        for (j = 0; j < 3; ++j) {
            out[i][j] = 0;
        }
    }
}

static void solver_copy_mat3(fx out[3][3], const fx in_m[3][3])
{
    int i;
    int j;
    for (i = 0; i < 3; ++i) {
        for (j = 0; j < 3; ++j) {
            out[i][j] = in_m[i][j];
        }
    }
}

static void solver_build_diag_tensor(const fx diag[3], fx out[3][3])
{
    solver_zero_mat3(out);
    out[0][0] = diag[0];
    out[1][1] = diag[1];
    out[2][2] = diag[2];
}

static int solver_tensor_is_nonzero(const fx m[3][3])
{
    int i;
    int j;
    for (i = 0; i < 3; ++i) {
        for (j = 0; j < 3; ++j) {
            if (FX_ABS(m[i][j]) > FX_EPSILON) return 1;
        }
    }
    return 0;
}

static void solver_apply_tensor_local(const fx tensor[3][3], const fx v[3], fx out[3])
{
    out[0] = FX_MUL(tensor[0][0], v[0]) + FX_MUL(tensor[0][1], v[1]) + FX_MUL(tensor[0][2], v[2]);
    out[1] = FX_MUL(tensor[1][0], v[0]) + FX_MUL(tensor[1][1], v[1]) + FX_MUL(tensor[1][2], v[2]);
    out[2] = FX_MUL(tensor[2][0], v[0]) + FX_MUL(tensor[2][1], v[1]) + FX_MUL(tensor[2][2], v[2]);
}

static int solver_body_has_pending_input(const sicol_solver_body_t* body)
{
    if (!body) return 0;
    if (fx_len_sq3(body->force) > FX_EPSILON) return 1;
    if (fx_len_sq3(body->torque) > FX_EPSILON) return 1;
    return 0;
}

static void solver_build_box_inv_inertia(const fx half[3], fx inv_mass, fx out_inv_inertia[3])
{
    fx hy2_hz2;
    fx hx2_hz2;
    fx hx2_hy2;
    if (!out_inv_inertia) return;
    if (inv_mass <= 0) {
        fx_zero3(out_inv_inertia);
        return;
    }
    hy2_hz2 = FX_MUL(half[1], half[1]) + FX_MUL(half[2], half[2]);
    hx2_hz2 = FX_MUL(half[0], half[0]) + FX_MUL(half[2], half[2]);
    hx2_hy2 = FX_MUL(half[0], half[0]) + FX_MUL(half[1], half[1]);
    out_inv_inertia[0] = (hy2_hz2 > FX_EPSILON) ? FX_DIV(FX_MUL(FX_FROM_INT(3), inv_mass), hy2_hz2) : 0;
    out_inv_inertia[1] = (hx2_hz2 > FX_EPSILON) ? FX_DIV(FX_MUL(FX_FROM_INT(3), inv_mass), hx2_hz2) : 0;
    out_inv_inertia[2] = (hx2_hy2 > FX_EPSILON) ? FX_DIV(FX_MUL(FX_FROM_INT(3), inv_mass), hx2_hy2) : 0;
}

static void solver_compute_convex_half_extents(const sicol_shape_t* shape, fx out_half[3])
{
    int i;
    fx min_v[3];
    fx max_v[3];
    if (!shape || !out_half || shape->type != SICOL_SHAPE_CONVEX || shape->u.convex.count <= 0) {
        fx_set3(out_half, FX_ONE, FX_ONE, FX_ONE);
        return;
    }
    fx_copy3(min_v, shape->u.convex.verts[0]);
    fx_copy3(max_v, shape->u.convex.verts[0]);
    for (i = 1; i < shape->u.convex.count; ++i) {
        min_v[0] = FX_MIN(min_v[0], shape->u.convex.verts[i][0]);
        min_v[1] = FX_MIN(min_v[1], shape->u.convex.verts[i][1]);
        min_v[2] = FX_MIN(min_v[2], shape->u.convex.verts[i][2]);
        max_v[0] = FX_MAX(max_v[0], shape->u.convex.verts[i][0]);
        max_v[1] = FX_MAX(max_v[1], shape->u.convex.verts[i][1]);
        max_v[2] = FX_MAX(max_v[2], shape->u.convex.verts[i][2]);
    }
    out_half[0] = (max_v[0] - min_v[0]) / 2;
    out_half[1] = (max_v[1] - min_v[1]) / 2;
    out_half[2] = (max_v[2] - min_v[2]) / 2;
}

static void solver_auto_compute_inv_inertia(const sicol_shape_t* shape, fx inv_mass, fx out_inv_inertia[3])
{
    fx denom;
    fx half[3];
    if (!out_inv_inertia) return;
    fx_zero3(out_inv_inertia);
    if (!shape || inv_mass <= 0) return;

    switch (shape->type) {
    case SICOL_SHAPE_SPHERE:
        denom = FX_MUL(shape->u.sphere.radius, shape->u.sphere.radius);
        if (denom > FX_EPSILON) {
            denom = FX_MUL(FX_FROM_RATIO(2, 5), denom);
            if (denom > FX_EPSILON) {
                out_inv_inertia[0] = FX_DIV(inv_mass, denom);
                out_inv_inertia[1] = out_inv_inertia[0];
                out_inv_inertia[2] = out_inv_inertia[0];
            }
        }
        break;
    case SICOL_SHAPE_AABB:
        solver_build_box_inv_inertia(shape->u.aabb.half, inv_mass, out_inv_inertia);
        break;
    case SICOL_SHAPE_OBB:
        solver_build_box_inv_inertia(shape->u.obb.half, inv_mass, out_inv_inertia);
        break;
    case SICOL_SHAPE_CAPSULE:
        fx_set3(half, shape->u.capsule.radius, shape->u.capsule.half_segment + shape->u.capsule.radius, shape->u.capsule.radius);
        solver_build_box_inv_inertia(half, inv_mass, out_inv_inertia);
        break;
    case SICOL_SHAPE_CONVEX:
        solver_compute_convex_half_extents(shape, half);
        solver_build_box_inv_inertia(half, inv_mass, out_inv_inertia);
        break;
    default:
        break;
    }
}

static void solver_ensure_body_inertia(const sicol_world_t* world, sicol_solver_body_t bodies[SICOL_WORLD_MAX], int id)
{
    if (!world || !bodies) return;
    if (!solver_body_is_dynamic(world, &bodies[id], id)) return;
    if (!solver_tensor_is_nonzero((const fx (*)[3])bodies[id].inv_inertia_tensor_local)) {
        if (fx_len_sq3(bodies[id].inv_inertia_local) <= FX_EPSILON) {
            solver_auto_compute_inv_inertia(&world->slots[id].shape, bodies[id].inv_mass, bodies[id].inv_inertia_local);
        }
        solver_build_diag_tensor(bodies[id].inv_inertia_local, bodies[id].inv_inertia_tensor_local);
    }
}

static void solver_apply_inv_inertia_world(
    const sicol_shape_t* shape,
    const sicol_solver_body_t* body,
    const fx v[3],
    fx out[3]
)
{
    fx basis[3][3];
    fx local[3];
    fx local_out[3];

    if (!body || !out) return;
    if (body->inv_mass <= 0) {
        fx_zero3(out);
        return;
    }

    sicol_shape_get_basis(shape, basis);
    fx_basis_to_local3(local, (const fx (*)[3])basis, v);
    if (solver_tensor_is_nonzero((const fx (*)[3])body->inv_inertia_tensor_local)) {
        solver_apply_tensor_local((const fx (*)[3])body->inv_inertia_tensor_local, local, local_out);
    } else {
        local_out[0] = FX_MUL(local[0], body->inv_inertia_local[0]);
        local_out[1] = FX_MUL(local[1], body->inv_inertia_local[1]);
        local_out[2] = FX_MUL(local[2], body->inv_inertia_local[2]);
    }
    fx_basis_to_world3(out, (const fx (*)[3])basis, local_out);
}

static void solver_body_point_velocity(
    const sicol_world_t* world,
    const sicol_solver_body_t bodies[SICOL_WORLD_MAX],
    int id,
    const fx point[3],
    fx out[3],
    int enable_angular
)
{
    fx r[3];
    fx wxr[3];
    if (!world || !bodies || !out) return;
    fx_zero3(out);
    if (id < 0 || id >= SICOL_WORLD_MAX) return;
    if (!world->slots[id].active) return;
    fx_copy3(out, bodies[id].velocity);
    if (!enable_angular) return;
    if (bodies[id].inv_mass <= 0) return;
    fx_sub3(r, point, world->slots[id].shape.pos);
    fx_cross3(wxr, bodies[id].angular_velocity, r);
    fx_add3(out, out, wxr);
}

static fx solver_effective_mass_term(
    const sicol_world_t* world,
    const sicol_solver_body_t* body,
    int id,
    const fx point[3],
    const fx dir[3]
)
{
    fx r[3];
    fx rxn[3];
    fx i_rxn[3];
    fx cross_term[3];
    if (!world || !body) return 0;
    if (body->inv_mass <= 0) return 0;
    if (id < 0 || id >= SICOL_WORLD_MAX) return 0;
    if (!world->slots[id].active) return 0;
    fx_sub3(r, point, world->slots[id].shape.pos);
    fx_cross3(rxn, r, dir);
    solver_apply_inv_inertia_world(&world->slots[id].shape, body, rxn, i_rxn);
    fx_cross3(cross_term, i_rxn, r);
    return fx_dot3(cross_term, dir);
}

static void solver_apply_impulse_one(
    sicol_world_t* world,
    int id,
    sicol_solver_body_t* body,
    const fx point[3],
    const fx impulse[3],
    fx sign,
    int enable_angular,
    sicol_solver_stats_t* stats
)
{
    fx signed_impulse[3];
    fx scaled[3];
    if (!world || !body || body->inv_mass <= 0) return;
    if (id < 0 || id >= SICOL_WORLD_MAX) return;
    if (!world->slots[id].active) return;

    body->sleeping = 0;
    body->sleep_frames = 0;

    fx_scale3(signed_impulse, impulse, sign);
    fx_scale3(scaled, signed_impulse, body->inv_mass);
    fx_add3(body->velocity, body->velocity, scaled);

    if (enable_angular && solver_shape_supports_rotation(&world->slots[id].shape)) {
        fx r[3];
        fx torque[3];
        fx dw[3];
        fx_sub3(r, point, world->slots[id].shape.pos);
        fx_cross3(torque, r, signed_impulse);
        solver_apply_inv_inertia_world(&world->slots[id].shape, body, torque, dw);
        fx_add3(body->angular_velocity, body->angular_velocity, dw);
        if (stats) stats->total_angular_impulse += fx_len3(dw);
    }
}

static void solver_apply_velocity_impulse_pair(
    sicol_world_t* world,
    sicol_solver_body_t bodies[SICOL_WORLD_MAX],
    int id_a,
    int id_b,
    const fx point[3],
    const fx impulse[3],
    const sicol_solver_config_t* cfg,
    sicol_solver_stats_t* stats
)
{
    solver_apply_impulse_one(world, id_a, &bodies[id_a], point, impulse, -FX_ONE, cfg->enable_angular, stats);
    solver_apply_impulse_one(world, id_b, &bodies[id_b], point, impulse,  FX_ONE, cfg->enable_angular, stats);
}

static void solver_translate_body(sicol_world_t* world, int id, const fx delta[3])
{
    if (!world || !delta) return;
    if (id < 0 || id >= SICOL_WORLD_MAX) return;
    if (!world->slots[id].active) return;
    sicol_shape_translate(&world->slots[id].shape, delta);
}

static void solver_rotate_body(sicol_world_t* world, int id, const fx omega[3], fx dt)
{
    if (!world || !omega) return;
    if (id < 0 || id >= SICOL_WORLD_MAX) return;
    if (!world->slots[id].active) return;
    sicol_shape_apply_angular_velocity(&world->slots[id].shape, omega, dt);
}

static void solver_apply_positional_impulse_one(
    sicol_world_t* world,
    int id,
    sicol_solver_body_t* body,
    const fx point[3],
    const fx impulse[3],
    fx sign,
    const sicol_solver_config_t* cfg,
    sicol_solver_stats_t* stats
)
{
    fx linear_delta[3];
    if (!world || !body || !cfg) return;
    if (body->inv_mass <= 0) return;
    if (id < 0 || id >= SICOL_WORLD_MAX) return;
    if (!world->slots[id].active) return;

    fx_scale3(linear_delta, impulse, FX_MUL(sign, body->inv_mass));
    solver_translate_body(world, id, linear_delta);
    if (stats) stats->total_position_correction += fx_len3(linear_delta);

    if (cfg->enable_angular && solver_shape_supports_rotation(&world->slots[id].shape)) {
        fx r[3];
        fx torque[3];
        fx ang_delta[3];
        fx signed_impulse[3];
        fx_scale3(signed_impulse, impulse, sign);
        fx_sub3(r, point, world->slots[id].shape.pos);
        fx_cross3(torque, r, signed_impulse);
        solver_apply_inv_inertia_world(&world->slots[id].shape, body, torque, ang_delta);
        fx_scale3(ang_delta, ang_delta, SICOL_SOLVER_POS_ANGULAR_SCALE);
        solver_rotate_body(world, id, ang_delta, FX_ONE);
    }
}

static int solver_joint_is_active(const sicol_world_t* world, int joint_id)
{
    if (!world) return 0;
    if (joint_id < 0 || joint_id >= SICOL_WORLD_MAX_JOINTS) return 0;
    if (!world->joints[joint_id].active) return 0;
    if (world->joints[joint_id].id_a < 0 || world->joints[joint_id].id_a >= SICOL_WORLD_MAX) return 0;
    if (!world->slots[world->joints[joint_id].id_a].active) return 0;
    if (world->joints[joint_id].id_b >= 0) {
        if (world->joints[joint_id].id_b >= SICOL_WORLD_MAX) return 0;
        if (!world->slots[world->joints[joint_id].id_b].active) return 0;
    }
    return 1;
}

static void solver_joint_anchor_world(const sicol_world_t* world, const sicol_joint_t* joint, int side, fx out[3])
{
    int id;
    const fx* local_anchor;
    fx basis[3][3];
    fx offset[3];
    if (!world || !joint || !out) return;
    id = (side == 0) ? joint->id_a : joint->id_b;
    local_anchor = (side == 0) ? joint->local_anchor_a : joint->local_anchor_b;
    if (id < 0) {
        fx_copy3(out, local_anchor);
        return;
    }
    sicol_shape_get_basis(&world->slots[id].shape, basis);
    fx_basis_to_world3(offset, (const fx (*)[3])basis, local_anchor);
    fx_add3(out, world->slots[id].shape.pos, offset);
}

static void solver_joint_apply_impulse_side(
    sicol_world_t* world,
    sicol_solver_body_t bodies[SICOL_WORLD_MAX],
    int id,
    const fx point[3],
    const fx impulse[3],
    fx sign,
    const sicol_solver_config_t* cfg,
    sicol_solver_stats_t* stats
)
{
    if (id < 0) return;
    solver_apply_impulse_one(world, id, &bodies[id], point, impulse, sign, cfg->enable_angular, stats);
}

static void solver_joint_apply_positional_side(
    sicol_world_t* world,
    sicol_solver_body_t bodies[SICOL_WORLD_MAX],
    int id,
    const fx point[3],
    const fx impulse[3],
    fx sign,
    const sicol_solver_config_t* cfg,
    sicol_solver_stats_t* stats
)
{
    if (id < 0) return;
    solver_apply_positional_impulse_one(world, id, &bodies[id], point, impulse, sign, cfg, stats);
}

static void solver_joint_relative_velocity(
    const sicol_world_t* world,
    const sicol_solver_body_t bodies[SICOL_WORLD_MAX],
    int id_a,
    const fx anchor_a[3],
    int id_b,
    const fx anchor_b[3],
    int enable_angular,
    fx out[3]
)
{
    fx va[3];
    fx vb[3];
    fx_zero3(va);
    fx_zero3(vb);
    if (id_a >= 0) solver_body_point_velocity(world, bodies, id_a, anchor_a, va, enable_angular);
    if (id_b >= 0) solver_body_point_velocity(world, bodies, id_b, anchor_b, vb, enable_angular);
    fx_sub3(out, vb, va);
}

static fx solver_joint_effective_mass(
    const sicol_world_t* world,
    const sicol_solver_body_t bodies[SICOL_WORLD_MAX],
    int id_a,
    const fx anchor_a[3],
    int id_b,
    const fx anchor_b[3],
    const fx dir[3],
    int enable_angular
)
{
    fx k;
    k = 0;
    if (id_a >= 0 && solver_body_is_dynamic(world, &bodies[id_a], id_a)) {
        k += bodies[id_a].inv_mass;
        if (enable_angular) k += solver_effective_mass_term(world, &bodies[id_a], id_a, anchor_a, dir);
    }
    if (id_b >= 0 && solver_body_is_dynamic(world, &bodies[id_b], id_b)) {
        k += bodies[id_b].inv_mass;
        if (enable_angular) k += solver_effective_mass_term(world, &bodies[id_b], id_b, anchor_b, dir);
    }
    return k;
}

static void solver_pick_any_perpendicular(const fx axis[3], fx out[3])
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

static void solver_project_perp_normalized(const fx axis[3], const fx v[3], fx out[3])
{
    fx proj[3];
    fx tmp[3];
    if (!out) return;
    fx_scale3(tmp, axis, fx_dot3(v, axis));
    fx_sub3(proj, v, tmp);
    if (!fx_normalize3(out, proj)) {
        solver_pick_any_perpendicular(axis, out);
    }
}

static void solver_joint_vector_world(const sicol_world_t* world, const sicol_joint_t* joint, int side, const fx local_v[3], fx out[3])
{
    int id;
    fx basis[3][3];
    if (!world || !joint || !local_v || !out) return;
    id = (side == 0) ? joint->id_a : joint->id_b;
    if (id < 0) {
        fx_copy3(out, local_v);
        return;
    }
    sicol_shape_get_basis(&world->slots[id].shape, basis);
    fx_basis_to_world3(out, (const fx (*)[3])basis, local_v);
}

static void solver_joint_axes_world(
    const sicol_world_t* world,
    const sicol_joint_t* joint,
    fx axis_a[3],
    fx axis_b[3],
    fx ref_a[3],
    fx ref_b[3]
)
{
    if (!world || !joint) return;
    solver_joint_vector_world(world, joint, 0, joint->local_axis_a, axis_a);
    solver_joint_vector_world(world, joint, 1, joint->local_axis_b, axis_b);
    if (!fx_normalize3(axis_a, axis_a)) fx_set3(axis_a, FX_ONE, 0, 0);
    if (!fx_normalize3(axis_b, axis_b)) fx_copy3(axis_b, axis_a);
    solver_joint_vector_world(world, joint, 0, joint->local_ref_a, ref_a);
    solver_joint_vector_world(world, joint, 1, joint->local_ref_b, ref_b);
    solver_project_perp_normalized(axis_a, ref_a, ref_a);
    solver_project_perp_normalized(axis_b, ref_b, ref_b);
}

static void solver_joint_relative_angular_velocity(
    const sicol_world_t* world,
    const sicol_solver_body_t bodies[SICOL_WORLD_MAX],
    int id_a,
    int id_b,
    fx out[3]
)
{
    fx wa[3];
    fx wb[3];
    if (!out) return;
    fx_zero3(wa);
    fx_zero3(wb);
    if (id_a >= 0 && id_a < SICOL_WORLD_MAX && world && world->slots[id_a].active) {
        fx_copy3(wa, bodies[id_a].angular_velocity);
    }
    if (id_b >= 0 && id_b < SICOL_WORLD_MAX && world && world->slots[id_b].active) {
        fx_copy3(wb, bodies[id_b].angular_velocity);
    }
    fx_sub3(out, wb, wa);
}

static fx solver_joint_angular_effective_mass(
    const sicol_world_t* world,
    const sicol_solver_body_t bodies[SICOL_WORLD_MAX],
    int id_a,
    int id_b,
    const fx dir[3]
)
{
    fx k;
    fx tmp[3];
    k = 0;
    if (id_a >= 0 && solver_body_is_dynamic(world, &bodies[id_a], id_a) && solver_shape_supports_rotation(&world->slots[id_a].shape)) {
        solver_apply_inv_inertia_world(&world->slots[id_a].shape, &bodies[id_a], dir, tmp);
        k += fx_dot3(tmp, dir);
    }
    if (id_b >= 0 && solver_body_is_dynamic(world, &bodies[id_b], id_b) && solver_shape_supports_rotation(&world->slots[id_b].shape)) {
        solver_apply_inv_inertia_world(&world->slots[id_b].shape, &bodies[id_b], dir, tmp);
        k += fx_dot3(tmp, dir);
    }
    return k;
}

static void solver_apply_angular_impulse_one(
    sicol_world_t* world,
    int id,
    sicol_solver_body_t* body,
    const fx impulse[3],
    fx sign,
    sicol_solver_stats_t* stats
)
{
    fx signed_impulse[3];
    fx dw[3];
    if (!world || !body || !impulse) return;
    if (id < 0 || id >= SICOL_WORLD_MAX) return;
    if (!solver_body_is_dynamic(world, body, id)) return;
    if (!solver_shape_supports_rotation(&world->slots[id].shape)) return;
    body->sleeping = 0;
    body->sleep_frames = 0;
    fx_scale3(signed_impulse, impulse, sign);
    solver_apply_inv_inertia_world(&world->slots[id].shape, body, signed_impulse, dw);
    fx_add3(body->angular_velocity, body->angular_velocity, dw);
    if (stats) stats->total_angular_impulse += fx_len3(dw);
}

static void solver_joint_apply_angular_impulse_side(
    sicol_world_t* world,
    sicol_solver_body_t bodies[SICOL_WORLD_MAX],
    int id,
    const fx impulse[3],
    fx sign,
    sicol_solver_stats_t* stats
)
{
    if (id < 0) return;
    solver_apply_angular_impulse_one(world, id, &bodies[id], impulse, sign, stats);
}

static void solver_apply_angular_positional_one(
    sicol_world_t* world,
    int id,
    sicol_solver_body_t* body,
    const fx impulse[3],
    fx sign,
    sicol_solver_stats_t* stats
)
{
    fx signed_impulse[3];
    fx ang_delta[3];
    if (!world || !body || !impulse) return;
    if (id < 0 || id >= SICOL_WORLD_MAX) return;
    if (!solver_body_is_dynamic(world, body, id)) return;
    if (!solver_shape_supports_rotation(&world->slots[id].shape)) return;
    fx_scale3(signed_impulse, impulse, sign);
    solver_apply_inv_inertia_world(&world->slots[id].shape, body, signed_impulse, ang_delta);
    fx_scale3(ang_delta, ang_delta, SICOL_SOLVER_POS_ANGULAR_SCALE);
    solver_rotate_body(world, id, ang_delta, FX_ONE);
    if (stats) stats->total_position_correction += fx_len3(ang_delta);
}

static void solver_joint_apply_angular_positional_side(
    sicol_world_t* world,
    sicol_solver_body_t bodies[SICOL_WORLD_MAX],
    int id,
    const fx impulse[3],
    fx sign,
    sicol_solver_stats_t* stats
)
{
    if (id < 0) return;
    solver_apply_angular_positional_one(world, id, &bodies[id], impulse, sign, stats);
}

static fx solver_fx_atan2_approx(fx y, fx x)
{
    const fx k_pi = (fx)12868;
    const fx k_quarter_pi = (fx)3217;
    const fx k_half_pi = (fx)6434;
    const fx k_three_quarter_pi = (fx)9651;
    fx abs_y;
    fx r;
    fx angle;
    abs_y = FX_ABS(y) + FX_EPSILON;
    if (x == 0) {
        if (y > 0) return k_half_pi;
        if (y < 0) return -k_half_pi;
        return 0;
    }
    if (x > 0) {
        r = FX_DIV(x - abs_y, x + abs_y);
        angle = k_quarter_pi - FX_MUL(k_quarter_pi, r);
    } else {
        r = FX_DIV(x + abs_y, abs_y - x);
        angle = k_three_quarter_pi - FX_MUL(k_quarter_pi, r);
    }
    if (y < 0) angle = -angle;
    if (angle > k_pi) angle = k_pi;
    if (angle < -k_pi) angle = -k_pi;
    return angle;
}

static fx solver_joint_hinge_angle(const sicol_world_t* world, const sicol_joint_t* joint)
{
    fx axis_a[3];
    fx axis_b[3];
    fx ref_a[3];
    fx ref_b[3];
    fx axis[3];
    fx cross_v[3];
    fx sin_v;
    fx cos_v;
    solver_joint_axes_world(world, joint, axis_a, axis_b, ref_a, ref_b);
    fx_add3(axis, axis_a, axis_b);
    if (!fx_normalize3(axis, axis)) fx_copy3(axis, axis_a);
    solver_project_perp_normalized(axis, ref_a, ref_a);
    solver_project_perp_normalized(axis, ref_b, ref_b);
    fx_cross3(cross_v, ref_a, ref_b);
    sin_v = fx_dot3(axis, cross_v);
    cos_v = fx_dot3(ref_a, ref_b);
    return solver_fx_atan2_approx(sin_v, cos_v);
}

static fx solver_joint_angle_between(const fx a[3], const fx b[3], fx out_dir[3])
{
    fx cross_v[3];
    fx sin_v;
    fx cos_v;
    fx_cross3(cross_v, a, b);
    sin_v = fx_len3(cross_v);
    cos_v = fx_dot3(a, b);
    if (out_dir) {
        if (sin_v > FX_EPSILON) {
            fx_scale3(out_dir, cross_v, FX_DIV(FX_ONE, sin_v));
        } else {
            solver_pick_any_perpendicular(a, out_dir);
        }
    }
    return solver_fx_atan2_approx(sin_v, cos_v);
}

static void solver_joint_compute_slider_frame(
    const sicol_world_t* world,
    const sicol_joint_t* joint,
    fx axis[3],
    fx perp0[3],
    fx perp1[3]
)
{
    fx axis_a[3];
    fx axis_b[3];
    fx ref_a[3];
    fx ref_b[3];
    solver_joint_axes_world(world, joint, axis_a, axis_b, ref_a, ref_b);
    fx_add3(axis, axis_a, axis_b);
    if (!fx_normalize3(axis, axis)) fx_copy3(axis, axis_a);
    solver_project_perp_normalized(axis, ref_a, perp0);
    fx_cross3(perp1, axis, perp0);
    if (!fx_normalize3(perp1, perp1)) {
        solver_pick_any_perpendicular(axis, perp1);
    }
}

static void solver_joint_solve_linear_velocity_axis(
    sicol_world_t* world,
    sicol_solver_body_t bodies[SICOL_WORLD_MAX],
    sicol_joint_t* joint,
    const fx anchor_a[3],
    const fx anchor_b[3],
    const fx rv[3],
    const fx dir[3],
    fx error_mag,
    fx* accumulated,
    const sicol_solver_config_t* cfg,
    sicol_solver_stats_t* stats
)
{
    fx cvel;
    fx k;
    fx bias;
    fx lambda;
    fx impulse[3];
    cvel = fx_dot3(rv, dir);
    k = solver_joint_effective_mass(world, bodies, joint->id_a, anchor_a, joint->id_b, anchor_b, dir, cfg->enable_angular);
    if (k <= FX_EPSILON) return;
    if (cfg->dt > 0) {
        bias = FX_DIV(FX_MUL(joint->stiffness, error_mag), cfg->dt);
    } else {
        bias = FX_MUL(joint->stiffness, error_mag);
    }
    cvel += FX_MUL(joint->damping, cvel);
    lambda = FX_DIV(-(cvel + bias), k);
    if (accumulated) *accumulated += lambda;
    joint->last_impulse = FX_MAX(joint->last_impulse, FX_ABS(lambda));
    fx_scale3(impulse, dir, lambda);
    solver_joint_apply_impulse_side(world, bodies, joint->id_a, anchor_a, impulse,  FX_ONE, cfg, stats);
    solver_joint_apply_impulse_side(world, bodies, joint->id_b, anchor_b, impulse, -FX_ONE, cfg, stats);
    if (stats) stats->total_joint_impulse += FX_ABS(lambda);
}

static void solver_joint_solve_linear_motor_axis(
    sicol_world_t* world,
    sicol_solver_body_t bodies[SICOL_WORLD_MAX],
    sicol_joint_t* joint,
    const fx anchor_a[3],
    const fx anchor_b[3],
    const fx rv[3],
    const fx dir[3],
    fx target_speed,
    const sicol_solver_config_t* cfg,
    sicol_solver_stats_t* stats
)
{
    fx cvel;
    fx k;
    fx lambda;
    fx old_impulse;
    fx new_impulse;
    fx impulse[3];
    cvel = fx_dot3(rv, dir);
    k = solver_joint_effective_mass(world, bodies, joint->id_a, anchor_a, joint->id_b, anchor_b, dir, cfg->enable_angular);
    if (k <= FX_EPSILON) return;
    lambda = FX_DIV(cvel - target_speed, k);
    old_impulse = joint->motor_accumulated_impulse;
    new_impulse = fx_clamp(old_impulse + lambda, -joint->max_motor_impulse, joint->max_motor_impulse);
    lambda = new_impulse - old_impulse;
    joint->motor_accumulated_impulse = new_impulse;
    joint->last_impulse = FX_MAX(joint->last_impulse, FX_ABS(lambda));
    fx_scale3(impulse, dir, lambda);
    solver_joint_apply_impulse_side(world, bodies, joint->id_a, anchor_a, impulse,  FX_ONE, cfg, stats);
    solver_joint_apply_impulse_side(world, bodies, joint->id_b, anchor_b, impulse, -FX_ONE, cfg, stats);
    if (stats) stats->total_joint_impulse += FX_ABS(lambda);
}

static void solver_joint_solve_linear_position_axis(
    sicol_world_t* world,
    sicol_solver_body_t bodies[SICOL_WORLD_MAX],
    sicol_joint_t* joint,
    const fx anchor_a[3],
    const fx anchor_b[3],
    const fx dir[3],
    fx error_mag,
    const sicol_solver_config_t* cfg,
    sicol_solver_stats_t* stats
)
{
    fx k;
    fx correction;
    fx impulse[3];
    k = solver_joint_effective_mass(world, bodies, joint->id_a, anchor_a, joint->id_b, anchor_b, dir, cfg->enable_angular);
    if (k <= FX_EPSILON) return;
    correction = FX_DIV(FX_MUL(joint->stiffness, error_mag), k);
    fx_scale3(impulse, dir, correction);
    solver_joint_apply_positional_side(world, bodies, joint->id_a, anchor_a, impulse,  FX_ONE, cfg, stats);
    solver_joint_apply_positional_side(world, bodies, joint->id_b, anchor_b, impulse, -FX_ONE, cfg, stats);
}

static void solver_joint_warm_start_point(
    sicol_world_t* world,
    sicol_solver_body_t bodies[SICOL_WORLD_MAX],
    sicol_joint_t* joint,
    const fx anchor_a[3],
    const fx anchor_b[3],
    const sicol_solver_config_t* cfg,
    sicol_solver_stats_t* stats
)
{
    fx impulse[3];
    fx_copy3(impulse, joint->accumulated_impulse);
    solver_joint_apply_impulse_side(world, bodies, joint->id_a, anchor_a, impulse,  FX_ONE, cfg, stats);
    solver_joint_apply_impulse_side(world, bodies, joint->id_b, anchor_b, impulse, -FX_ONE, cfg, stats);
    if (stats) stats->total_joint_impulse += fx_len3(impulse);
}

static void solver_joint_solve_point_velocity(
    sicol_world_t* world,
    sicol_solver_body_t bodies[SICOL_WORLD_MAX],
    sicol_joint_t* joint,
    const fx anchor_a[3],
    const fx anchor_b[3],
    const fx error[3],
    const fx rv[3],
    const sicol_solver_config_t* cfg,
    sicol_solver_stats_t* stats
)
{
    int axis_index;
    for (axis_index = 0; axis_index < 3; ++axis_index) {
        fx dir[3];
        fx cvel;
        fx k;
        fx bias;
        fx lambda;
        fx impulse[3];
        fx_zero3(dir);
        dir[axis_index] = FX_ONE;
        cvel = fx_dot3(rv, dir);
        k = solver_joint_effective_mass(world, bodies, joint->id_a, anchor_a, joint->id_b, anchor_b, dir, cfg->enable_angular);
        if (k <= FX_EPSILON) continue;
        if (cfg->dt > 0) {
            bias = FX_DIV(FX_MUL(joint->stiffness, error[axis_index]), cfg->dt);
        } else {
            bias = FX_MUL(joint->stiffness, error[axis_index]);
        }
        cvel += FX_MUL(joint->damping, cvel);
        lambda = FX_DIV(-(cvel + bias), k);
        joint->accumulated_impulse[axis_index] += lambda;
        joint->last_impulse = FX_MAX(joint->last_impulse, FX_ABS(lambda));
        fx_scale3(impulse, dir, lambda);
        solver_joint_apply_impulse_side(world, bodies, joint->id_a, anchor_a, impulse,  FX_ONE, cfg, stats);
        solver_joint_apply_impulse_side(world, bodies, joint->id_b, anchor_b, impulse, -FX_ONE, cfg, stats);
        if (stats) stats->total_joint_impulse += FX_ABS(lambda);
    }
}

static void solver_joint_solve_point_position(
    sicol_world_t* world,
    sicol_solver_body_t bodies[SICOL_WORLD_MAX],
    sicol_joint_t* joint,
    const fx anchor_a[3],
    const fx anchor_b[3],
    const fx error[3],
    const sicol_solver_config_t* cfg,
    sicol_solver_stats_t* stats
)
{
    int axis_index;
    for (axis_index = 0; axis_index < 3; ++axis_index) {
        fx dir[3];
        fx k;
        fx correction;
        fx impulse[3];
        fx_zero3(dir);
        dir[axis_index] = FX_ONE;
        k = solver_joint_effective_mass(world, bodies, joint->id_a, anchor_a, joint->id_b, anchor_b, dir, cfg->enable_angular);
        if (k <= FX_EPSILON) continue;
        correction = FX_DIV(FX_MUL(joint->stiffness, error[axis_index]), k);
        fx_scale3(impulse, dir, correction);
        solver_joint_apply_positional_side(world, bodies, joint->id_a, anchor_a, impulse,  FX_ONE, cfg, stats);
        solver_joint_apply_positional_side(world, bodies, joint->id_b, anchor_b, impulse, -FX_ONE, cfg, stats);
    }
}

static void solver_joint_solve_angular_velocity_axis(
    sicol_world_t* world,
    sicol_solver_body_t bodies[SICOL_WORLD_MAX],
    sicol_joint_t* joint,
    const fx dir[3],
    fx error_mag,
    int store_in_vector,
    fx target_speed,
    int clamp_motor,
    const sicol_solver_config_t* cfg,
    sicol_solver_stats_t* stats
)
{
    fx rel_w[3];
    fx k;
    fx cvel;
    fx bias;
    fx lambda;
    fx old_impulse;
    fx new_impulse;
    fx impulse[3];
    if (!cfg->enable_angular) return;
    if (fx_len_sq3(dir) <= FX_EPSILON) return;
    solver_joint_relative_angular_velocity(world, bodies, joint->id_a, joint->id_b, rel_w);
    cvel = fx_dot3(rel_w, dir);
    k = solver_joint_angular_effective_mass(world, bodies, joint->id_a, joint->id_b, dir);
    if (k <= FX_EPSILON) return;
    if (clamp_motor) {
        lambda = FX_DIV(cvel - target_speed, k);
        old_impulse = joint->motor_accumulated_impulse;
        new_impulse = fx_clamp(old_impulse + lambda, -joint->max_motor_impulse, joint->max_motor_impulse);
        lambda = new_impulse - old_impulse;
        joint->motor_accumulated_impulse = new_impulse;
    } else {
        if (cfg->dt > 0) {
            bias = FX_DIV(FX_MUL(joint->stiffness, error_mag), cfg->dt);
        } else {
            bias = FX_MUL(joint->stiffness, error_mag);
        }
        cvel += FX_MUL(joint->damping, cvel);
        lambda = FX_DIV(cvel + bias, k);
        if (store_in_vector) {
            fx_madd3(joint->angular_accumulated_impulse, joint->angular_accumulated_impulse, dir, lambda);
        } else {
            joint->motor_accumulated_impulse += lambda;
        }
    }
    joint->last_impulse = FX_MAX(joint->last_impulse, FX_ABS(lambda));
    fx_scale3(impulse, dir, lambda);
    solver_joint_apply_angular_impulse_side(world, bodies, joint->id_a, impulse,  FX_ONE, stats);
    solver_joint_apply_angular_impulse_side(world, bodies, joint->id_b, impulse, -FX_ONE, stats);
    if (stats) stats->total_joint_impulse += FX_ABS(lambda);
}

static void solver_joint_solve_angular_position_axis(
    sicol_world_t* world,
    sicol_solver_body_t bodies[SICOL_WORLD_MAX],
    sicol_joint_t* joint,
    const fx dir[3],
    fx error_mag,
    sicol_solver_stats_t* stats
)
{
    fx k;
    fx correction;
    fx impulse[3];
    if (fx_len_sq3(dir) <= FX_EPSILON) return;
    k = solver_joint_angular_effective_mass(world, bodies, joint->id_a, joint->id_b, dir);
    if (k <= FX_EPSILON) return;
    correction = FX_DIV(FX_MUL(joint->stiffness, error_mag), k);
    fx_scale3(impulse, dir, correction);
    solver_joint_apply_angular_positional_side(world, bodies, joint->id_a, impulse,  FX_ONE, stats);
    solver_joint_apply_angular_positional_side(world, bodies, joint->id_b, impulse, -FX_ONE, stats);
}

static void solver_warm_start_joint(
    sicol_world_t* world,
    sicol_solver_body_t bodies[SICOL_WORLD_MAX],
    sicol_joint_t* joint,
    const sicol_solver_config_t* cfg,
    sicol_solver_stats_t* stats
)
{
    fx anchor_a[3];
    fx anchor_b[3];
    fx impulse[3];
    fx axis_a[3];
    fx axis_b[3];
    fx ref_a[3];
    fx ref_b[3];
    fx axis_avg[3];
    fx axis[3];
    fx perp0[3];
    fx perp1[3];
    if (!world || !bodies || !joint || !cfg) return;
    if (!cfg->warm_start) return;
    if (!joint->active) return;
    solver_joint_anchor_world(world, joint, 0, anchor_a);
    solver_joint_anchor_world(world, joint, 1, anchor_b);
    if (joint->type == SICOL_JOINT_DISTANCE || joint->type == SICOL_JOINT_SPRING) {
        fx delta[3];
        fx normal[3];
        fx_sub3(delta, anchor_b, anchor_a);
        if (!fx_normalize3(normal, delta)) {
            fx_set3(normal, FX_ONE, 0, 0);
        }
        fx_scale3(impulse, normal, joint->accumulated_impulse[0]);
        solver_joint_apply_impulse_side(world, bodies, joint->id_a, anchor_a, impulse,  FX_ONE, cfg, stats);
        solver_joint_apply_impulse_side(world, bodies, joint->id_b, anchor_b, impulse, -FX_ONE, cfg, stats);
        if (stats) stats->total_joint_impulse += FX_ABS(joint->accumulated_impulse[0]);
    } else if (joint->type == SICOL_JOINT_POINT || joint->type == SICOL_JOINT_FIXED || joint->type == SICOL_JOINT_HINGE || joint->type == SICOL_JOINT_CONE_TWIST) {
        solver_joint_warm_start_point(world, bodies, joint, anchor_a, anchor_b, cfg, stats);
        if (cfg->enable_angular && fx_len_sq3(joint->angular_accumulated_impulse) > FX_EPSILON) {
            fx_copy3(impulse, joint->angular_accumulated_impulse);
            solver_joint_apply_angular_impulse_side(world, bodies, joint->id_a, impulse,  FX_ONE, stats);
            solver_joint_apply_angular_impulse_side(world, bodies, joint->id_b, impulse, -FX_ONE, stats);
            if (stats) stats->total_joint_impulse += fx_len3(impulse);
        }
        if (cfg->enable_angular && joint->type == SICOL_JOINT_HINGE && joint->motor_accumulated_impulse != 0) {
            solver_joint_axes_world(world, joint, axis_a, axis_b, ref_a, ref_b);
            fx_add3(axis_avg, axis_a, axis_b);
            if (!fx_normalize3(axis_avg, axis_avg)) fx_copy3(axis_avg, axis_a);
            fx_scale3(impulse, axis_avg, joint->motor_accumulated_impulse);
            solver_joint_apply_angular_impulse_side(world, bodies, joint->id_a, impulse,  FX_ONE, stats);
            solver_joint_apply_angular_impulse_side(world, bodies, joint->id_b, impulse, -FX_ONE, stats);
            if (stats) stats->total_joint_impulse += FX_ABS(joint->motor_accumulated_impulse);
        }
    } else if (joint->type == SICOL_JOINT_SLIDER) {
        solver_joint_compute_slider_frame(world, joint, axis, perp0, perp1);
        fx_scale3(impulse, perp0, joint->accumulated_impulse[0]);
        solver_joint_apply_impulse_side(world, bodies, joint->id_a, anchor_a, impulse,  FX_ONE, cfg, stats);
        solver_joint_apply_impulse_side(world, bodies, joint->id_b, anchor_b, impulse, -FX_ONE, cfg, stats);
        fx_scale3(impulse, perp1, joint->accumulated_impulse[1]);
        solver_joint_apply_impulse_side(world, bodies, joint->id_a, anchor_a, impulse,  FX_ONE, cfg, stats);
        solver_joint_apply_impulse_side(world, bodies, joint->id_b, anchor_b, impulse, -FX_ONE, cfg, stats);
        fx_scale3(impulse, axis, joint->accumulated_impulse[2] + joint->motor_accumulated_impulse);
        solver_joint_apply_impulse_side(world, bodies, joint->id_a, anchor_a, impulse,  FX_ONE, cfg, stats);
        solver_joint_apply_impulse_side(world, bodies, joint->id_b, anchor_b, impulse, -FX_ONE, cfg, stats);
        if (cfg->enable_angular && fx_len_sq3(joint->angular_accumulated_impulse) > FX_EPSILON) {
            fx_copy3(impulse, joint->angular_accumulated_impulse);
            solver_joint_apply_angular_impulse_side(world, bodies, joint->id_a, impulse,  FX_ONE, stats);
            solver_joint_apply_angular_impulse_side(world, bodies, joint->id_b, impulse, -FX_ONE, stats);
        }
    }
}

static void solver_solve_velocity_joint(
    sicol_world_t* world,
    sicol_solver_body_t bodies[SICOL_WORLD_MAX],
    sicol_joint_t* joint,
    const sicol_solver_config_t* cfg,
    sicol_solver_stats_t* stats
)
{
    fx anchor_a[3];
    fx anchor_b[3];
    fx rv[3];
    fx error[3];
    if (!world || !bodies || !joint || !cfg || !joint->active) return;
    solver_joint_anchor_world(world, joint, 0, anchor_a);
    solver_joint_anchor_world(world, joint, 1, anchor_b);
    solver_joint_relative_velocity(world, bodies, joint->id_a, anchor_a, joint->id_b, anchor_b, cfg->enable_angular, rv);
    fx_sub3(error, anchor_b, anchor_a);

    if (joint->type == SICOL_JOINT_DISTANCE || joint->type == SICOL_JOINT_SPRING) {
        fx dist;
        fx normal[3];
        fx cvel;
        fx k;
        fx bias;
        fx lambda;
        fx impulse[3];
        dist = fx_len3(error);
        if (dist > FX_EPSILON) {
            fx_scale3(normal, error, FX_DIV(FX_ONE, dist));
        } else {
            fx_set3(normal, FX_ONE, 0, 0);
        }
        cvel = fx_dot3(rv, normal);
        k = solver_joint_effective_mass(world, bodies, joint->id_a, anchor_a, joint->id_b, anchor_b, normal, cfg->enable_angular);
        if (k <= FX_EPSILON) return;
        if (cfg->dt > 0) {
            bias = FX_DIV(FX_MUL(joint->stiffness, dist - joint->rest_length), cfg->dt);
        } else {
            bias = FX_MUL(joint->stiffness, dist - joint->rest_length);
        }
        if (joint->type == SICOL_JOINT_SPRING) {
            bias = FX_MUL(bias, FX_FROM_RATIO(1, 2));
        }
        cvel += FX_MUL(joint->damping, cvel);
        lambda = FX_DIV(-(cvel + bias), k);
        joint->accumulated_impulse[0] += lambda;
        joint->last_impulse = FX_ABS(lambda);
        fx_scale3(impulse, normal, lambda);
        solver_joint_apply_impulse_side(world, bodies, joint->id_a, anchor_a, impulse,  FX_ONE, cfg, stats);
        solver_joint_apply_impulse_side(world, bodies, joint->id_b, anchor_b, impulse, -FX_ONE, cfg, stats);
        if (stats) stats->total_joint_impulse += FX_ABS(lambda);
    } else if (joint->type == SICOL_JOINT_POINT) {
        solver_joint_solve_point_velocity(world, bodies, joint, anchor_a, anchor_b, error, rv, cfg, stats);
    } else if (joint->type == SICOL_JOINT_FIXED) {
        fx axis_a[3];
        fx axis_b[3];
        fx ref_a[3];
        fx ref_b[3];
        fx err_axis[3];
        fx err_ref[3];
        fx dir[3];
        fx mag;
        solver_joint_solve_point_velocity(world, bodies, joint, anchor_a, anchor_b, error, rv, cfg, stats);
        solver_joint_axes_world(world, joint, axis_a, axis_b, ref_a, ref_b);
        fx_cross3(err_axis, axis_a, axis_b);
        mag = fx_len3(err_axis);
        if (mag > FX_EPSILON) {
            fx_scale3(dir, err_axis, FX_DIV(FX_ONE, mag));
            solver_joint_solve_angular_velocity_axis(world, bodies, joint, dir, mag, 1, 0, 0, cfg, stats);
        }
        fx_cross3(err_ref, ref_a, ref_b);
        mag = fx_len3(err_ref);
        if (mag > FX_EPSILON) {
            fx_scale3(dir, err_ref, FX_DIV(FX_ONE, mag));
            solver_joint_solve_angular_velocity_axis(world, bodies, joint, dir, mag, 1, 0, 0, cfg, stats);
        }
    } else if (joint->type == SICOL_JOINT_HINGE) {
        fx axis_a[3];
        fx axis_b[3];
        fx ref_a[3];
        fx ref_b[3];
        fx err_axis[3];
        fx dir[3];
        fx mag;
        fx angle;
        fx limit_error;
        fx axis_avg[3];
        solver_joint_solve_point_velocity(world, bodies, joint, anchor_a, anchor_b, error, rv, cfg, stats);
        solver_joint_axes_world(world, joint, axis_a, axis_b, ref_a, ref_b);
        fx_cross3(err_axis, axis_a, axis_b);
        mag = fx_len3(err_axis);
        if (mag > FX_EPSILON) {
            fx_scale3(dir, err_axis, FX_DIV(FX_ONE, mag));
            solver_joint_solve_angular_velocity_axis(world, bodies, joint, dir, mag, 1, 0, 0, cfg, stats);
        }
        fx_add3(axis_avg, axis_a, axis_b);
        if (!fx_normalize3(axis_avg, axis_avg)) fx_copy3(axis_avg, axis_a);
        if (joint->limit_enabled) {
            angle = solver_joint_hinge_angle(world, joint);
            limit_error = 0;
            if (angle < joint->lower_angle) limit_error = angle - joint->lower_angle;
            else if (angle > joint->upper_angle) limit_error = angle - joint->upper_angle;
            if (limit_error != 0) {
                solver_joint_solve_angular_velocity_axis(world, bodies, joint, axis_avg, limit_error, 0, 0, 0, cfg, stats);
            }
        }
        if (joint->motor_enabled && joint->max_motor_impulse > 0) {
            solver_joint_solve_angular_velocity_axis(world, bodies, joint, axis_avg, 0, 0, joint->motor_speed, 1, cfg, stats);
        }
    } else if (joint->type == SICOL_JOINT_SLIDER) {
        fx axis[3];
        fx perp0[3];
        fx perp1[3];
        fx axis_a[3];
        fx axis_b[3];
        fx ref_a[3];
        fx ref_b[3];
        fx err_axis[3];
        fx err_ref[3];
        fx dir[3];
        fx mag;
        fx coord;
        fx limit_error;
        solver_joint_compute_slider_frame(world, joint, axis, perp0, perp1);
        solver_joint_solve_linear_velocity_axis(world, bodies, joint, anchor_a, anchor_b, rv, perp0, fx_dot3(error, perp0), &joint->accumulated_impulse[0], cfg, stats);
        solver_joint_solve_linear_velocity_axis(world, bodies, joint, anchor_a, anchor_b, rv, perp1, fx_dot3(error, perp1), &joint->accumulated_impulse[1], cfg, stats);
        coord = fx_dot3(error, axis);
        if (joint->limit_enabled) {
            limit_error = 0;
            if (coord < joint->lower_limit) limit_error = coord - joint->lower_limit;
            else if (coord > joint->upper_limit) limit_error = coord - joint->upper_limit;
            if (limit_error != 0) {
                solver_joint_solve_linear_velocity_axis(world, bodies, joint, anchor_a, anchor_b, rv, axis, limit_error, &joint->accumulated_impulse[2], cfg, stats);
            }
        }
        if (joint->motor_enabled && joint->max_motor_impulse > 0) {
            solver_joint_solve_linear_motor_axis(world, bodies, joint, anchor_a, anchor_b, rv, axis, joint->motor_speed, cfg, stats);
        }
        solver_joint_axes_world(world, joint, axis_a, axis_b, ref_a, ref_b);
        fx_cross3(err_axis, axis_a, axis_b);
        mag = fx_len3(err_axis);
        if (mag > FX_EPSILON) {
            fx_scale3(dir, err_axis, FX_DIV(FX_ONE, mag));
            solver_joint_solve_angular_velocity_axis(world, bodies, joint, dir, mag, 1, 0, 0, cfg, stats);
        }
        fx_cross3(err_ref, ref_a, ref_b);
        mag = fx_len3(err_ref);
        if (mag > FX_EPSILON) {
            fx_scale3(dir, err_ref, FX_DIV(FX_ONE, mag));
            solver_joint_solve_angular_velocity_axis(world, bodies, joint, dir, mag, 1, 0, 0, cfg, stats);
        }
    } else if (joint->type == SICOL_JOINT_CONE_TWIST) {
        fx axis_a[3];
        fx axis_b[3];
        fx ref_a[3];
        fx ref_b[3];
        fx dir[3];
        fx swing_angle;
        fx twist_angle;
        fx limit_error;
        fx axis_avg[3];
        solver_joint_solve_point_velocity(world, bodies, joint, anchor_a, anchor_b, error, rv, cfg, stats);
        solver_joint_axes_world(world, joint, axis_a, axis_b, ref_a, ref_b);
        swing_angle = solver_joint_angle_between(axis_a, axis_b, dir);
        if (joint->cone_angle > 0 && swing_angle > joint->cone_angle) {
            solver_joint_solve_angular_velocity_axis(world, bodies, joint, dir, swing_angle - joint->cone_angle, 1, 0, 0, cfg, stats);
        }
        if (joint->limit_enabled) {
            fx_add3(axis_avg, axis_a, axis_b);
            if (!fx_normalize3(axis_avg, axis_avg)) fx_copy3(axis_avg, axis_a);
            twist_angle = solver_joint_hinge_angle(world, joint);
            limit_error = 0;
            if (twist_angle < joint->lower_angle) limit_error = twist_angle - joint->lower_angle;
            else if (twist_angle > joint->upper_angle) limit_error = twist_angle - joint->upper_angle;
            if (limit_error != 0) {
                solver_joint_solve_angular_velocity_axis(world, bodies, joint, axis_avg, limit_error, 1, 0, 0, cfg, stats);
            }
        }
    }
}

static void solver_solve_position_joint(
    sicol_world_t* world,
    sicol_solver_body_t bodies[SICOL_WORLD_MAX],
    sicol_joint_t* joint,
    const sicol_solver_config_t* cfg,
    sicol_solver_stats_t* stats
)
{
    fx anchor_a[3];
    fx anchor_b[3];
    fx error[3];
    if (!world || !bodies || !joint || !cfg || !joint->active) return;
    solver_joint_anchor_world(world, joint, 0, anchor_a);
    solver_joint_anchor_world(world, joint, 1, anchor_b);
    fx_sub3(error, anchor_b, anchor_a);

    if (joint->type == SICOL_JOINT_DISTANCE || joint->type == SICOL_JOINT_SPRING) {
        fx dist;
        fx normal[3];
        fx k;
        fx correction;
        fx impulse[3];
        dist = fx_len3(error);
        if (dist <= FX_EPSILON) return;
        fx_scale3(normal, error, FX_DIV(FX_ONE, dist));
        k = solver_joint_effective_mass(world, bodies, joint->id_a, anchor_a, joint->id_b, anchor_b, normal, cfg->enable_angular);
        if (k <= FX_EPSILON) return;
        correction = FX_DIV(FX_MUL(joint->stiffness, dist - joint->rest_length), k);
        if (joint->type == SICOL_JOINT_SPRING) {
            correction = FX_MUL(correction, FX_FROM_RATIO(1, 2));
        }
        fx_scale3(impulse, normal, correction);
        solver_joint_apply_positional_side(world, bodies, joint->id_a, anchor_a, impulse,  FX_ONE, cfg, stats);
        solver_joint_apply_positional_side(world, bodies, joint->id_b, anchor_b, impulse, -FX_ONE, cfg, stats);
    } else if (joint->type == SICOL_JOINT_POINT) {
        solver_joint_solve_point_position(world, bodies, joint, anchor_a, anchor_b, error, cfg, stats);
    } else if (joint->type == SICOL_JOINT_FIXED) {
        fx axis_a[3];
        fx axis_b[3];
        fx ref_a[3];
        fx ref_b[3];
        fx err_axis[3];
        fx err_ref[3];
        fx dir[3];
        fx mag;
        solver_joint_solve_point_position(world, bodies, joint, anchor_a, anchor_b, error, cfg, stats);
        solver_joint_axes_world(world, joint, axis_a, axis_b, ref_a, ref_b);
        fx_cross3(err_axis, axis_a, axis_b);
        mag = fx_len3(err_axis);
        if (mag > FX_EPSILON) {
            fx_scale3(dir, err_axis, FX_DIV(FX_ONE, mag));
            solver_joint_solve_angular_position_axis(world, bodies, joint, dir, mag, stats);
        }
        fx_cross3(err_ref, ref_a, ref_b);
        mag = fx_len3(err_ref);
        if (mag > FX_EPSILON) {
            fx_scale3(dir, err_ref, FX_DIV(FX_ONE, mag));
            solver_joint_solve_angular_position_axis(world, bodies, joint, dir, mag, stats);
        }
    } else if (joint->type == SICOL_JOINT_HINGE) {
        fx axis_a[3];
        fx axis_b[3];
        fx ref_a[3];
        fx ref_b[3];
        fx err_axis[3];
        fx dir[3];
        fx mag;
        fx angle;
        fx limit_error;
        fx axis_avg[3];
        solver_joint_solve_point_position(world, bodies, joint, anchor_a, anchor_b, error, cfg, stats);
        solver_joint_axes_world(world, joint, axis_a, axis_b, ref_a, ref_b);
        fx_cross3(err_axis, axis_a, axis_b);
        mag = fx_len3(err_axis);
        if (mag > FX_EPSILON) {
            fx_scale3(dir, err_axis, FX_DIV(FX_ONE, mag));
            solver_joint_solve_angular_position_axis(world, bodies, joint, dir, mag, stats);
        }
        if (joint->limit_enabled) {
            fx_add3(axis_avg, axis_a, axis_b);
            if (!fx_normalize3(axis_avg, axis_avg)) fx_copy3(axis_avg, axis_a);
            angle = solver_joint_hinge_angle(world, joint);
            limit_error = 0;
            if (angle < joint->lower_angle) limit_error = angle - joint->lower_angle;
            else if (angle > joint->upper_angle) limit_error = angle - joint->upper_angle;
            if (limit_error != 0) {
                solver_joint_solve_angular_position_axis(world, bodies, joint, axis_avg, limit_error, stats);
            }
        }
    } else if (joint->type == SICOL_JOINT_SLIDER) {
        fx axis[3];
        fx perp0[3];
        fx perp1[3];
        fx axis_a[3];
        fx axis_b[3];
        fx ref_a[3];
        fx ref_b[3];
        fx err_axis[3];
        fx err_ref[3];
        fx dir[3];
        fx mag;
        fx coord;
        fx limit_error;
        solver_joint_compute_slider_frame(world, joint, axis, perp0, perp1);
        solver_joint_solve_linear_position_axis(world, bodies, joint, anchor_a, anchor_b, perp0, fx_dot3(error, perp0), cfg, stats);
        solver_joint_solve_linear_position_axis(world, bodies, joint, anchor_a, anchor_b, perp1, fx_dot3(error, perp1), cfg, stats);
        coord = fx_dot3(error, axis);
        if (joint->limit_enabled) {
            limit_error = 0;
            if (coord < joint->lower_limit) limit_error = coord - joint->lower_limit;
            else if (coord > joint->upper_limit) limit_error = coord - joint->upper_limit;
            if (limit_error != 0) {
                solver_joint_solve_linear_position_axis(world, bodies, joint, anchor_a, anchor_b, axis, limit_error, cfg, stats);
            }
        }
        solver_joint_axes_world(world, joint, axis_a, axis_b, ref_a, ref_b);
        fx_cross3(err_axis, axis_a, axis_b);
        mag = fx_len3(err_axis);
        if (mag > FX_EPSILON) {
            fx_scale3(dir, err_axis, FX_DIV(FX_ONE, mag));
            solver_joint_solve_angular_position_axis(world, bodies, joint, dir, mag, stats);
        }
        fx_cross3(err_ref, ref_a, ref_b);
        mag = fx_len3(err_ref);
        if (mag > FX_EPSILON) {
            fx_scale3(dir, err_ref, FX_DIV(FX_ONE, mag));
            solver_joint_solve_angular_position_axis(world, bodies, joint, dir, mag, stats);
        }
    } else if (joint->type == SICOL_JOINT_CONE_TWIST) {
        fx axis_a[3];
        fx axis_b[3];
        fx ref_a[3];
        fx ref_b[3];
        fx dir[3];
        fx swing_angle;
        fx twist_angle;
        fx limit_error;
        fx axis_avg[3];
        solver_joint_solve_point_position(world, bodies, joint, anchor_a, anchor_b, error, cfg, stats);
        solver_joint_axes_world(world, joint, axis_a, axis_b, ref_a, ref_b);
        swing_angle = solver_joint_angle_between(axis_a, axis_b, dir);
        if (joint->cone_angle > 0 && swing_angle > joint->cone_angle) {
            solver_joint_solve_angular_position_axis(world, bodies, joint, dir, swing_angle - joint->cone_angle, stats);
        }
        if (joint->limit_enabled) {
            fx_add3(axis_avg, axis_a, axis_b);
            if (!fx_normalize3(axis_avg, axis_avg)) fx_copy3(axis_avg, axis_a);
            twist_angle = solver_joint_hinge_angle(world, joint);
            limit_error = 0;
            if (twist_angle < joint->lower_angle) limit_error = twist_angle - joint->lower_angle;
            else if (twist_angle > joint->upper_angle) limit_error = twist_angle - joint->upper_angle;
            if (limit_error != 0) {
                solver_joint_solve_angular_position_axis(world, bodies, joint, axis_avg, limit_error, stats);
            }
        }
    }
}

static void solver_warm_start_pair(
    sicol_world_t* world,
    sicol_solver_body_t bodies[SICOL_WORLD_MAX],
    const sicol_persistent_manifold_t* manifold,
    const sicol_solver_config_t* cfg,
    sicol_solver_stats_t* stats
)
{
    int i;
    if (!world || !bodies || !manifold || !cfg) return;
    if (!cfg->warm_start) return;

    for (i = 0; i < SICOL_MANIFOLD_MAX_POINTS; ++i) {
        fx impulse[3];
        if (!manifold->points[i].active) continue;
        if (manifold->points[i].normal_impulse != 0) {
            fx_scale3(impulse, manifold->points[i].normal, manifold->points[i].normal_impulse);
            solver_apply_velocity_impulse_pair(world, bodies, manifold->id_a, manifold->id_b, manifold->points[i].point, impulse, cfg, stats);
            if (stats) stats->total_normal_impulse += FX_ABS(manifold->points[i].normal_impulse);
        }
        if (cfg->enable_friction && manifold->points[i].tangent_impulse != 0 && fx_len_sq3(manifold->points[i].tangent) > FX_EPSILON) {
            fx_scale3(impulse, manifold->points[i].tangent, manifold->points[i].tangent_impulse);
            solver_apply_velocity_impulse_pair(world, bodies, manifold->id_a, manifold->id_b, manifold->points[i].point, impulse, cfg, stats);
            if (stats) stats->total_tangent_impulse += FX_ABS(manifold->points[i].tangent_impulse);
        }
    }
}

static void solver_solve_velocity_pair(
    sicol_world_t* world,
    sicol_solver_body_t bodies[SICOL_WORLD_MAX],
    sicol_persistent_manifold_t* manifold,
    const sicol_solver_config_t* cfg,
    sicol_solver_stats_t* stats
)
{
    int i;
    if (!world || !bodies || !manifold || !cfg) return;

    for (i = 0; i < SICOL_MANIFOLD_MAX_POINTS; ++i) {
        sicol_manifold_point_t* mp;
        fx va[3];
        fx vb[3];
        fx rv[3];
        fx vn;
        fx bias;
        fx restitution_term;
        fx k_n;
        fx lambda;
        fx new_impulse;
        fx delta_impulse;
        fx impulse[3];
        fx tangent_v[3];
        fx vt;
        fx k_t;
        fx jt;
        fx max_f;
        fx new_t;
        fx delta_t;
        fx restitution;

        mp = &manifold->points[i];
        if (!mp->active) continue;

        solver_body_point_velocity(world, bodies, manifold->id_a, mp->point, va, cfg->enable_angular);
        solver_body_point_velocity(world, bodies, manifold->id_b, mp->point, vb, cfg->enable_angular);
        fx_sub3(rv, vb, va);
        vn = fx_dot3(rv, mp->normal);

        k_n = bodies[manifold->id_a].inv_mass + bodies[manifold->id_b].inv_mass;
        if (cfg->enable_angular) {
            k_n += solver_effective_mass_term(world, &bodies[manifold->id_a], manifold->id_a, mp->point, mp->normal);
            k_n += solver_effective_mass_term(world, &bodies[manifold->id_b], manifold->id_b, mp->point, mp->normal);
        }
        if (k_n <= FX_EPSILON) continue;

        bias = FX_MUL(cfg->baumgarte, FX_MAX(mp->penetration - cfg->slop, 0));
        if (cfg->dt > 0) bias = FX_DIV(bias, cfg->dt);
        restitution_term = 0;
        if (vn < -FX_FROM_RATIO(1, 8)) {
            restitution = (bodies[manifold->id_a].restitution + bodies[manifold->id_b].restitution) / 2;
            restitution_term = FX_MUL(restitution, -vn);
        }

        lambda = FX_DIV(-(vn - restitution_term - bias), k_n);
        new_impulse = mp->normal_impulse + lambda;
        if (new_impulse < 0) new_impulse = 0;
        delta_impulse = new_impulse - mp->normal_impulse;
        mp->normal_impulse = new_impulse;
        if (delta_impulse != 0) {
            fx_scale3(impulse, mp->normal, delta_impulse);
            solver_apply_velocity_impulse_pair(world, bodies, manifold->id_a, manifold->id_b, mp->point, impulse, cfg, stats);
            if (stats) stats->total_normal_impulse += FX_ABS(delta_impulse);
        }

        if (!cfg->enable_friction) continue;

        solver_body_point_velocity(world, bodies, manifold->id_a, mp->point, va, cfg->enable_angular);
        solver_body_point_velocity(world, bodies, manifold->id_b, mp->point, vb, cfg->enable_angular);
        fx_sub3(rv, vb, va);
        fx_scale3(impulse, mp->normal, fx_dot3(rv, mp->normal));
        fx_sub3(tangent_v, rv, impulse);
        vt = fx_len3(tangent_v);
        if (vt > FX_EPSILON) {
            fx_normalize3(mp->tangent, tangent_v);
        } else if (fx_len_sq3(mp->tangent) <= FX_EPSILON) {
            continue;
        }

        k_t = bodies[manifold->id_a].inv_mass + bodies[manifold->id_b].inv_mass;
        if (cfg->enable_angular) {
            k_t += solver_effective_mass_term(world, &bodies[manifold->id_a], manifold->id_a, mp->point, mp->tangent);
            k_t += solver_effective_mass_term(world, &bodies[manifold->id_b], manifold->id_b, mp->point, mp->tangent);
        }
        if (k_t <= FX_EPSILON) continue;

        jt = FX_DIV(-fx_dot3(rv, mp->tangent), k_t);
        max_f = FX_MUL((bodies[manifold->id_a].friction + bodies[manifold->id_b].friction) / 2, mp->normal_impulse);
        new_t = mp->tangent_impulse + jt;
        new_t = fx_clamp(new_t, -max_f, max_f);
        delta_t = new_t - mp->tangent_impulse;
        mp->tangent_impulse = new_t;
        if (delta_t != 0) {
            fx_scale3(impulse, mp->tangent, delta_t);
            solver_apply_velocity_impulse_pair(world, bodies, manifold->id_a, manifold->id_b, mp->point, impulse, cfg, stats);
            if (stats) stats->total_tangent_impulse += FX_ABS(delta_t);
        }
    }
}

static void solver_solve_positions(
    sicol_world_t* world,
    sicol_solver_body_t bodies[SICOL_WORLD_MAX],
    const sicol_pair_t* pairs,
    int pair_count,
    const sicol_solver_config_t* cfg,
    sicol_solver_stats_t* stats
)
{
    int iter;
    int i;
    if (!world || !bodies || !pairs || !cfg) return;

    for (iter = 0; iter < cfg->position_iterations; ++iter) {
        for (i = 0; i < pair_count; ++i) {
            sicol_contact_t contact;
            fx k_n;
            fx correction;
            fx impulse[3];
            int id_a;
            int id_b;
            id_a = pairs[i].a;
            id_b = pairs[i].b;

            if (!sicol_world_overlap_pair(world, id_a, id_b, &contact)) continue;
            if (contact.penetration <= cfg->slop) continue;

            k_n = bodies[id_a].inv_mass + bodies[id_b].inv_mass;
            if (cfg->enable_angular) {
                k_n += solver_effective_mass_term(world, &bodies[id_a], id_a, contact.point, contact.normal);
                k_n += solver_effective_mass_term(world, &bodies[id_b], id_b, contact.point, contact.normal);
            }
            if (k_n <= FX_EPSILON) continue;

            correction = FX_DIV(FX_MUL(cfg->baumgarte, contact.penetration - cfg->slop), k_n);
            fx_scale3(impulse, contact.normal, correction);
            solver_apply_positional_impulse_one(world, id_a, &bodies[id_a], contact.point, impulse, -FX_ONE, cfg, stats);
            solver_apply_positional_impulse_one(world, id_b, &bodies[id_b], contact.point, impulse,  FX_ONE, cfg, stats);
        }
    }
}

static int solver_joint_belongs_to_island(
    const int island_mask[SICOL_WORLD_MAX],
    const sicol_joint_t* joint,
    const sicol_world_t* world,
    const sicol_solver_body_t bodies[SICOL_WORLD_MAX]
)
{
    int a_dynamic;
    int b_dynamic;
    if (!joint || !world || !bodies) return 0;
    a_dynamic = (joint->id_a >= 0) ? solver_body_is_dynamic(world, &bodies[joint->id_a], joint->id_a) : 0;
    b_dynamic = (joint->id_b >= 0) ? solver_body_is_dynamic(world, &bodies[joint->id_b], joint->id_b) : 0;
    if (a_dynamic && island_mask[joint->id_a]) return 1;
    if (b_dynamic && joint->id_b >= 0 && island_mask[joint->id_b]) return 1;
    return 0;
}

static int solver_pair_belongs_to_island(
    const int island_mask[SICOL_WORLD_MAX],
    const sicol_pair_t* pair,
    const sicol_world_t* world,
    const sicol_solver_body_t bodies[SICOL_WORLD_MAX]
)
{
    int a_dynamic;
    int b_dynamic;
    if (!pair || !world || !bodies) return 0;
    a_dynamic = solver_body_is_dynamic(world, &bodies[pair->a], pair->a);
    b_dynamic = solver_body_is_dynamic(world, &bodies[pair->b], pair->b);
    if (a_dynamic && island_mask[pair->a]) return 1;
    if (b_dynamic && island_mask[pair->b]) return 1;
    return 0;
}

static int solver_collect_island(
    const sicol_world_t* world,
    const sicol_solver_body_t bodies[SICOL_WORLD_MAX],
    const sicol_pair_t* pairs,
    int pair_count,
    int start_id,
    int visited[SICOL_WORLD_MAX],
    int island_mask[SICOL_WORLD_MAX],
    int out_body_ids[SICOL_WORLD_MAX],
    int* out_body_count,
    int out_pair_indices[SICOL_SOLVER_MAX_RESULTS],
    int* out_pair_count,
    int out_joint_indices[SICOL_WORLD_MAX_JOINTS],
    int* out_joint_count
)
{
    int queue[SICOL_WORLD_MAX];
    int head;
    int tail;
    int body_count;
    int pair_out_count;
    int joint_out_count;
    int i;
    if (!world || !bodies || !pairs || !visited || !island_mask || !out_body_ids || !out_body_count || !out_pair_indices || !out_pair_count || !out_joint_indices || !out_joint_count) return 0;
    if (start_id < 0 || start_id >= SICOL_WORLD_MAX) return 0;
    if (!solver_body_is_dynamic(world, &bodies[start_id], start_id)) return 0;

    for (i = 0; i < SICOL_WORLD_MAX; ++i) island_mask[i] = 0;

    head = 0;
    tail = 0;
    body_count = 0;
    queue[tail++] = start_id;
    visited[start_id] = 1;

    while (head < tail) {
        int current;
        current = queue[head++];
        island_mask[current] = 1;
        out_body_ids[body_count++] = current;

        for (i = 0; i < pair_count; ++i) {
            int other;
            if (pairs[i].a == current) {
                other = pairs[i].b;
            } else if (pairs[i].b == current) {
                other = pairs[i].a;
            } else {
                continue;
            }
            if (!solver_body_is_dynamic(world, &bodies[other], other)) continue;
            if (visited[other]) continue;
            visited[other] = 1;
            queue[tail++] = other;
        }
        for (i = 0; i < SICOL_WORLD_MAX_JOINTS; ++i) {
            int other;
            if (!solver_joint_is_active(world, i)) continue;
            if (world->joints[i].id_a == current) {
                other = world->joints[i].id_b;
            } else if (world->joints[i].id_b == current) {
                other = world->joints[i].id_a;
            } else {
                continue;
            }
            if (other < 0) continue;
            if (!solver_body_is_dynamic(world, &bodies[other], other)) continue;
            if (visited[other]) continue;
            visited[other] = 1;
            queue[tail++] = other;
        }
    }

    pair_out_count = 0;
    for (i = 0; i < pair_count; ++i) {
        if (solver_pair_belongs_to_island(island_mask, &pairs[i], world, bodies)) {
            out_pair_indices[pair_out_count++] = i;
        }
    }

    joint_out_count = 0;
    for (i = 0; i < SICOL_WORLD_MAX_JOINTS; ++i) {
        if (!solver_joint_is_active(world, i)) continue;
        if (solver_joint_belongs_to_island(island_mask, &world->joints[i], world, bodies)) {
            out_joint_indices[joint_out_count++] = i;
        }
    }

    *out_body_count = body_count;
    *out_pair_count = pair_out_count;
    *out_joint_count = joint_out_count;
    return 1;
}

static int solver_body_below_sleep_threshold(const sicol_solver_body_t* body, const sicol_solver_config_t* cfg)
{
    fx lin_sq;
    fx ang_sq;
    fx lin_th_sq;
    fx ang_th_sq;
    if (!body || !cfg) return 0;
    lin_sq = fx_len_sq3(body->velocity);
    ang_sq = fx_len_sq3(body->angular_velocity);
    lin_th_sq = FX_MUL(cfg->sleep_linear_threshold, cfg->sleep_linear_threshold);
    ang_th_sq = FX_MUL(cfg->sleep_angular_threshold, cfg->sleep_angular_threshold);
    return (lin_sq <= lin_th_sq && ang_sq <= ang_th_sq);
}

static int solver_try_sleep_island(
    sicol_solver_body_t bodies[SICOL_WORLD_MAX],
    const int* body_ids,
    int body_count,
    const sicol_solver_config_t* cfg
)
{
    int i;
    int all_low;
    int all_can_sleep;
    if (!bodies || !body_ids || !cfg || !cfg->enable_sleeping) return 0;
    if (body_count <= 0) return 0;

    all_low = 1;
    all_can_sleep = 1;
    for (i = 0; i < body_count; ++i) {
        sicol_solver_body_t* body;
        body = &bodies[body_ids[i]];
        if (!body->can_sleep) all_can_sleep = 0;
        if (!solver_body_below_sleep_threshold(body, cfg)) {
            all_low = 0;
            break;
        }
    }

    if (!all_low || !all_can_sleep) {
        for (i = 0; i < body_count; ++i) {
            bodies[body_ids[i]].sleeping = 0;
            bodies[body_ids[i]].sleep_frames = 0;
        }
        return 0;
    }

    for (i = 0; i < body_count; ++i) {
        if (bodies[body_ids[i]].sleep_frames < 0xFFFFFFFFu) {
            ++bodies[body_ids[i]].sleep_frames;
        }
    }

    for (i = 0; i < body_count; ++i) {
        if (bodies[body_ids[i]].sleep_frames < (unsigned int)cfg->sleep_frames_threshold) {
            return 0;
        }
    }

    for (i = 0; i < body_count; ++i) {
        bodies[body_ids[i]].sleeping = 1;
        fx_zero3(bodies[body_ids[i]].velocity);
        fx_zero3(bodies[body_ids[i]].angular_velocity);
    }
    return 1;
}

static void solver_integrate_forces(
    sicol_world_t* world,
    sicol_solver_body_t bodies[SICOL_WORLD_MAX],
    int id,
    const sicol_solver_config_t* cfg
)
{
    sicol_solver_body_t* body;
    fx accel[3];
    fx force_accel[3];
    fx delta_v[3];
    fx ang_accel[3];
    fx delta_w[3];
    fx damping;
    if (!world || !bodies || !cfg) return;
    body = &bodies[id];
    if (!solver_body_is_dynamic(world, body, id)) {
        fx_zero3(body->force);
        fx_zero3(body->torque);
        return;
    }
    if (cfg->dt <= 0) {
        fx_zero3(body->force);
        fx_zero3(body->torque);
        return;
    }

    fx_scale3(accel, cfg->gravity, body->gravity_scale);
    fx_scale3(force_accel, body->force, body->inv_mass);
    fx_add3(accel, accel, force_accel);
    fx_scale3(delta_v, accel, cfg->dt);
    fx_add3(body->velocity, body->velocity, delta_v);

    if (cfg->enable_angular && solver_shape_supports_rotation(&world->slots[id].shape)) {
        solver_apply_inv_inertia_world(&world->slots[id].shape, body, body->torque, ang_accel);
        fx_scale3(delta_w, ang_accel, cfg->dt);
        fx_add3(body->angular_velocity, body->angular_velocity, delta_w);
    }

    damping = FX_ONE - FX_MUL(cfg->linear_damping, cfg->dt);
    damping = fx_clamp(damping, 0, FX_ONE);
    fx_scale3(body->velocity, body->velocity, damping);

    damping = FX_ONE - FX_MUL(cfg->angular_damping, cfg->dt);
    damping = fx_clamp(damping, 0, FX_ONE);
    fx_scale3(body->angular_velocity, body->angular_velocity, damping);

    fx_zero3(body->force);
    fx_zero3(body->torque);
}

static void solver_integrate_body(
    sicol_world_t* world,
    sicol_solver_body_t bodies[SICOL_WORLD_MAX],
    int id,
    const sicol_solver_config_t* cfg,
    sicol_solver_stats_t* stats
)
{
    fx delta[3];
    sicol_solver_body_t* body;
    if (!world || !bodies || !cfg) return;
    body = &bodies[id];
    if (!solver_body_can_move(world, body, id)) return;

    fx_scale3(delta, body->velocity, cfg->dt);

    if (cfg->enable_ccd && fx_len3(body->velocity) >= cfg->ccd_min_speed && fx_len_sq3(delta) > FX_EPSILON) {
        sicol_shape_t working;
        fx remaining[3];
        int impacts;
        working = world->slots[id].shape;
        fx_copy3(remaining, delta);
        impacts = 0;
        while (impacts < cfg->ccd_max_impacts && fx_len_sq3(remaining) > FX_EPSILON) {
            sicol_world_cast_hit_t hit;
            if (!sicol_world_cast_shape_ex(
                    world,
                    &working,
                    remaining,
                    world->slots[id].mask,
                    world->slots[id].layer,
                    world->slots[id].mask,
                    id,
                    &hit)) {
                sicol_shape_translate(&working, remaining);
                fx_zero3(remaining);
                break;
            }

            {
                fx move_delta[3];
                fx leftover[3];
                fx vn;
                fx rn;
                fx clip[3];
                fx_scale3(leftover, remaining, FX_MAX(FX_ONE - hit.fraction, 0));
                fx_sub3(move_delta, hit.position, working.pos);
                sicol_shape_translate(&working, move_delta);
                ++impacts;
                if (stats) ++stats->ccd_hits;

                vn = fx_dot3(body->velocity, hit.contact.normal);
                if (vn < 0) {
                    fx_scale3(clip, hit.contact.normal, vn);
                    fx_sub3(body->velocity, body->velocity, clip);
                }
                rn = fx_dot3(leftover, hit.contact.normal);
                if (rn < 0) {
                    fx_scale3(clip, hit.contact.normal, rn);
                    fx_sub3(leftover, leftover, clip);
                }
                fx_copy3(remaining, leftover);
                if (hit.fraction <= 0) break;
            }
        }
        world->slots[id].shape = working;
    } else {
        solver_translate_body(world, id, delta);
    }

    if (cfg->enable_angular && fx_len_sq3(body->angular_velocity) > FX_EPSILON) {
        solver_rotate_body(world, id, body->angular_velocity, cfg->dt);
    }
}

void sicol_solver_body_init(sicol_solver_body_t* body, fx inv_mass)
{
    if (!body) return;
    body->inv_mass = inv_mass;
    fx_zero3(body->velocity);
    fx_zero3(body->angular_velocity);
    fx_zero3(body->inv_inertia_local);
    solver_zero_mat3(body->inv_inertia_tensor_local);
    fx_zero3(body->force);
    fx_zero3(body->torque);
    body->gravity_scale = FX_ONE;
    body->restitution = 0;
    body->friction = FX_FROM_RATIO(1, 2);
    body->can_sleep = 1;
    body->sleeping = 0;
    body->sleep_frames = 0;
}

void sicol_solver_body_init_for_shape(sicol_solver_body_t* body, const sicol_shape_t* shape, fx inv_mass)
{
    sicol_solver_body_init(body, inv_mass);
    if (!body || !shape) return;
    solver_auto_compute_inv_inertia(shape, inv_mass, body->inv_inertia_local);
    solver_build_diag_tensor(body->inv_inertia_local, body->inv_inertia_tensor_local);
}

void sicol_solver_body_make_static(sicol_solver_body_t* body)
{
    sicol_solver_body_init(body, 0);
    if (!body) return;
    body->can_sleep = 0;
    body->sleeping = 1;
}

void sicol_solver_body_wake(sicol_solver_body_t* body)
{
    if (!body) return;
    body->sleeping = 0;
    body->sleep_frames = 0;
}

void sicol_solver_body_set_inv_inertia_tensor(sicol_solver_body_t* body, fx tensor[3][3])
{
    if (!body || !tensor) return;
    solver_copy_mat3(body->inv_inertia_tensor_local, (const fx (*)[3])tensor);
    body->inv_inertia_local[0] = tensor[0][0];
    body->inv_inertia_local[1] = tensor[1][1];
    body->inv_inertia_local[2] = tensor[2][2];
}

void sicol_solver_body_add_force(sicol_solver_body_t* body, const fx force[3])
{
    if (!body || !force) return;
    fx_add3(body->force, body->force, force);
}

void sicol_solver_body_add_torque(sicol_solver_body_t* body, const fx torque[3])
{
    if (!body || !torque) return;
    fx_add3(body->torque, body->torque, torque);
}

void sicol_solver_body_clear_forces(sicol_solver_body_t* body)
{
    if (!body) return;
    fx_zero3(body->force);
    fx_zero3(body->torque);
}

void sicol_solver_config_default(sicol_solver_config_t* config)
{
    if (!config) return;
    config->dt = FX_FROM_RATIO(1, 60);
    config->velocity_iterations = 8;
    config->position_iterations = 4;
    config->baumgarte = FX_FROM_RATIO(1, 5);
    config->slop = FX_FROM_RATIO(1, 100);
    config->warm_start = 1;
    config->enable_friction = 1;
    config->enable_angular = 1;
    config->enable_islands = 1;
    config->enable_sleeping = 1;
    config->sleep_linear_threshold = FX_FROM_RATIO(1, 50);
    config->sleep_angular_threshold = FX_FROM_RATIO(1, 50);
    config->sleep_frames_threshold = 20;
    config->enable_ccd = 1;
    config->ccd_min_speed = FX_FROM_RATIO(1, 4);
    config->ccd_max_impacts = 3;
    fx_zero3(config->gravity);
    config->linear_damping = 0;
    config->angular_damping = 0;
}

int sicol_world_step_solver(
    sicol_world_t* world,
    sicol_solver_body_t bodies[SICOL_WORLD_MAX],
    const sicol_solver_config_t* config,
    sicol_solver_stats_t* out_stats
)
{
    sicol_solver_config_t local_cfg;
    sicol_solver_stats_t stats;
    sicol_pair_t pairs[SICOL_SOLVER_MAX_RESULTS];
    sicol_contact_t contacts[SICOL_SOLVER_MAX_RESULTS];
    int pair_count;
    int i;
    int iter;
    int visited[SICOL_WORLD_MAX];
    int island_mask[SICOL_WORLD_MAX];
    int island_body_ids[SICOL_WORLD_MAX];
    int island_pair_indices[SICOL_SOLVER_MAX_RESULTS];
    int island_joint_indices[SICOL_WORLD_MAX_JOINTS];
    int island_body_count;
    int island_pair_count;
    int island_joint_count;
    int sleeping_island;

    if (!world || !bodies) return 0;

    if (config) {
        local_cfg = *config;
    } else {
        sicol_solver_config_default(&local_cfg);
    }

    stats.pair_count = 0;
    stats.point_count = 0;
    stats.joint_count = 0;
    stats.island_count = 0;
    stats.sleeping_count = 0;
    stats.ccd_hits = 0;
    stats.total_normal_impulse = 0;
    stats.total_tangent_impulse = 0;
    stats.total_joint_impulse = 0;
    stats.total_angular_impulse = 0;
    stats.total_position_correction = 0;

    for (i = 0; i < SICOL_WORLD_MAX; ++i) {
        visited[i] = 0;
        if (!world->slots[i].active) continue;
        solver_ensure_body_inertia(world, bodies, i);
        if (bodies[i].sleeping && (solver_body_has_pending_input(&bodies[i]) || !solver_body_below_sleep_threshold(&bodies[i], &local_cfg))) {
            sicol_solver_body_wake(&bodies[i]);
        }
        solver_integrate_forces(world, bodies, i, &local_cfg);
        solver_integrate_body(world, bodies, i, &local_cfg, &stats);
    }

    for (i = 0; i < SICOL_WORLD_MAX_JOINTS; ++i) {
        if (solver_joint_is_active(world, i)) {
            ++stats.joint_count;
        }
    }

    pair_count = sicol_world_collide_persistent(world, pairs, contacts, SICOL_SOLVER_MAX_RESULTS);
    stats.pair_count = pair_count;

    if (!local_cfg.enable_islands) {
        stats.island_count = 1;
        for (i = 0; i < pair_count; ++i) {
            sicol_persistent_manifold_t* manifold;
            manifold = solver_find_manifold(world, pairs[i].a, pairs[i].b);
            if (!manifold) continue;
            stats.point_count += manifold->point_count;
            solver_warm_start_pair(world, bodies, manifold, &local_cfg, &stats);
        }
        for (i = 0; i < SICOL_WORLD_MAX_JOINTS; ++i) {
            if (!solver_joint_is_active(world, i)) continue;
            solver_warm_start_joint(world, bodies, &world->joints[i], &local_cfg, &stats);
        }
        for (iter = 0; iter < local_cfg.velocity_iterations; ++iter) {
            for (i = 0; i < pair_count; ++i) {
                sicol_persistent_manifold_t* manifold;
                manifold = solver_find_manifold(world, pairs[i].a, pairs[i].b);
                if (!manifold) continue;
                solver_solve_velocity_pair(world, bodies, manifold, &local_cfg, &stats);
            }
            for (i = 0; i < SICOL_WORLD_MAX_JOINTS; ++i) {
                if (!solver_joint_is_active(world, i)) continue;
                solver_solve_velocity_joint(world, bodies, &world->joints[i], &local_cfg, &stats);
            }
        }
        solver_solve_positions(world, bodies, pairs, pair_count, &local_cfg, &stats);
        for (iter = 0; iter < local_cfg.position_iterations; ++iter) {
            for (i = 0; i < SICOL_WORLD_MAX_JOINTS; ++i) {
                if (!solver_joint_is_active(world, i)) continue;
                solver_solve_position_joint(world, bodies, &world->joints[i], &local_cfg, &stats);
            }
        }
    } else {
        for (i = 0; i < SICOL_WORLD_MAX; ++i) {
            if (!solver_body_is_dynamic(world, &bodies[i], i)) continue;
            if (visited[i]) continue;
            if (!solver_collect_island(world, bodies, pairs, pair_count, i, visited, island_mask,
                                       island_body_ids, &island_body_count,
                                       island_pair_indices, &island_pair_count,
                                       island_joint_indices, &island_joint_count)) {
                continue;
            }
            ++stats.island_count;
            sleeping_island = solver_try_sleep_island(bodies, island_body_ids, island_body_count, &local_cfg);
            if (sleeping_island) continue;

            for (iter = 0; iter < island_pair_count; ++iter) {
                sicol_persistent_manifold_t* manifold;
                manifold = solver_find_manifold(world, pairs[island_pair_indices[iter]].a, pairs[island_pair_indices[iter]].b);
                if (!manifold) continue;
                stats.point_count += manifold->point_count;
                solver_warm_start_pair(world, bodies, manifold, &local_cfg, &stats);
            }
            for (iter = 0; iter < island_joint_count; ++iter) {
                solver_warm_start_joint(world, bodies, &world->joints[island_joint_indices[iter]], &local_cfg, &stats);
            }

            for (iter = 0; iter < local_cfg.velocity_iterations; ++iter) {
                int p;
                for (p = 0; p < island_pair_count; ++p) {
                    sicol_persistent_manifold_t* manifold;
                    manifold = solver_find_manifold(world, pairs[island_pair_indices[p]].a, pairs[island_pair_indices[p]].b);
                    if (!manifold) continue;
                    solver_solve_velocity_pair(world, bodies, manifold, &local_cfg, &stats);
                }
                for (p = 0; p < island_joint_count; ++p) {
                    solver_solve_velocity_joint(world, bodies, &world->joints[island_joint_indices[p]], &local_cfg, &stats);
                }
            }

            {
                sicol_pair_t island_pairs[SICOL_SOLVER_MAX_RESULTS];
                int p;
                for (p = 0; p < island_pair_count; ++p) {
                    island_pairs[p] = pairs[island_pair_indices[p]];
                }
                solver_solve_positions(world, bodies, island_pairs, island_pair_count, &local_cfg, &stats);
                for (p = 0; p < local_cfg.position_iterations; ++p) {
                    int j;
                    for (j = 0; j < island_joint_count; ++j) {
                        solver_solve_position_joint(world, bodies, &world->joints[island_joint_indices[j]], &local_cfg, &stats);
                    }
                }
            }
        }
    }

    pair_count = sicol_world_collide_persistent(world, pairs, contacts, SICOL_SOLVER_MAX_RESULTS);
    stats.pair_count = pair_count;

    for (i = 0; i < SICOL_WORLD_MAX; ++i) {
        if (bodies[i].sleeping && solver_body_is_dynamic(world, &bodies[i], i)) {
            ++stats.sleeping_count;
        }
    }

    if (out_stats) {
        *out_stats = stats;
    }
    return pair_count;
}

