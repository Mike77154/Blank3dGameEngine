#include "bolt3d/b3d_transform.h"

static B3D_Vec3 b3d_transform_pick_up(B3D_Vec3 forward)
{
    B3D_Vec3 up;
    B3D_Fixed dot_value;

    up = b3d_vec3(0, B3D_FIXED_ONE, 0);
    dot_value = b3d_fixed_abs(b3d_vec3_dot(forward, up));
    if (dot_value > b3d_fixed_div(b3d_fixed_from_int(9), b3d_fixed_from_int(10))) {
        up = b3d_vec3(B3D_FIXED_ONE, 0, 0);
    }
    return up;
}

B3D_Basis3 b3d_basis_identity(void)
{
    B3D_Basis3 basis;

    basis.right = b3d_vec3(B3D_FIXED_ONE, 0, 0);
    basis.up = b3d_vec3(0, B3D_FIXED_ONE, 0);
    basis.forward = b3d_vec3(0, 0, B3D_FIXED_ONE);
    return basis;
}

B3D_Basis3 b3d_basis_from_forward_up(B3D_Vec3 forward, B3D_Vec3 up_hint)
{
    B3D_Basis3 basis;
    B3D_Vec3 right;
    B3D_Vec3 up;

    forward = b3d_vec3_normalize(forward);
    if (b3d_vec3_length_sq(forward) <= B3D_FIXED_EPSILON) {
        return b3d_basis_identity();
    }

    up = b3d_vec3_normalize(up_hint);
    if (b3d_vec3_length_sq(up) <= B3D_FIXED_EPSILON) {
        up = b3d_transform_pick_up(forward);
    }

    right = b3d_vec3_cross(up, forward);
    if (b3d_vec3_length_sq(right) <= B3D_FIXED_EPSILON) {
        up = b3d_transform_pick_up(forward);
        right = b3d_vec3_cross(up, forward);
    }

    right = b3d_vec3_normalize(right);
    up = b3d_vec3_normalize(b3d_vec3_cross(forward, right));

    basis.right = right;
    basis.up = up;
    basis.forward = forward;
    return basis;
}

B3D_Basis3 b3d_basis_orthonormalize(B3D_Basis3 basis)
{
    return b3d_basis_from_forward_up(basis.forward, basis.up);
}

B3D_Basis3 b3d_basis_integrate_angular(B3D_Basis3 basis, B3D_Vec3 angular_velocity, B3D_Fixed dt)
{
    B3D_Vec3 step;

    step = b3d_vec3_scale(angular_velocity, dt);
    basis.right = b3d_vec3_add(basis.right, b3d_vec3_cross(step, basis.right));
    basis.up = b3d_vec3_add(basis.up, b3d_vec3_cross(step, basis.up));
    basis.forward = b3d_vec3_add(basis.forward, b3d_vec3_cross(step, basis.forward));
    return b3d_basis_orthonormalize(basis);
}

B3D_Vec3 b3d_basis_transform_vector(B3D_Basis3 basis, B3D_Vec3 local_vector)
{
    B3D_Vec3 result;

    result = b3d_vec3_scale(basis.right, local_vector.x);
    result = b3d_vec3_add(result, b3d_vec3_scale(basis.up, local_vector.y));
    result = b3d_vec3_add(result, b3d_vec3_scale(basis.forward, local_vector.z));
    return result;
}

B3D_Vec3 b3d_basis_untransform_vector(B3D_Basis3 basis, B3D_Vec3 world_vector)
{
    return b3d_vec3(
        b3d_vec3_dot(world_vector, basis.right),
        b3d_vec3_dot(world_vector, basis.up),
        b3d_vec3_dot(world_vector, basis.forward)
    );
}

B3D_Transform b3d_transform_identity(void)
{
    B3D_Transform transform_value;

    transform_value.position = b3d_vec3_zero();
    transform_value.basis = b3d_basis_identity();
    transform_value.scale = b3d_vec3(B3D_FIXED_ONE, B3D_FIXED_ONE, B3D_FIXED_ONE);
    return transform_value;
}

B3D_Transform b3d_transform_from_position(B3D_Vec3 position)
{
    B3D_Transform transform_value;

    transform_value = b3d_transform_identity();
    transform_value.position = position;
    return transform_value;
}

B3D_Vec3 b3d_transform_vector(const B3D_Transform *transform_value, B3D_Vec3 local_vector)
{
    B3D_Vec3 scaled;

    if (transform_value == 0) {
        return local_vector;
    }
    scaled = b3d_vec3_mul_components(local_vector, transform_value->scale);
    return b3d_basis_transform_vector(transform_value->basis, scaled);
}

B3D_Vec3 b3d_transform_point(const B3D_Transform *transform_value, B3D_Vec3 local_point)
{
    if (transform_value == 0) {
        return local_point;
    }
    return b3d_vec3_add(transform_value->position, b3d_transform_vector(transform_value, local_point));
}

B3D_Vec3 b3d_transform_vector_to_local(const B3D_Transform *transform_value, B3D_Vec3 world_vector)
{
    B3D_Vec3 unrotated;

    if (transform_value == 0) {
        return world_vector;
    }
    unrotated = b3d_basis_untransform_vector(transform_value->basis, world_vector);
    return b3d_vec3_div_components(unrotated, transform_value->scale);
}

B3D_Vec3 b3d_transform_point_to_local(const B3D_Transform *transform_value, B3D_Vec3 world_point)
{
    if (transform_value == 0) {
        return world_point;
    }
    return b3d_transform_vector_to_local(transform_value, b3d_vec3_sub(world_point, transform_value->position));
}

B3D_Transform b3d_transform_compose(const B3D_Transform *parent, const B3D_Transform *child)
{
    B3D_Transform result;

    if (parent == 0 && child == 0) {
        return b3d_transform_identity();
    }
    if (parent == 0) {
        return *child;
    }
    if (child == 0) {
        return *parent;
    }

    result.position = b3d_transform_point(parent, child->position);
    result.basis.right = b3d_vec3_normalize(b3d_basis_transform_vector(parent->basis, child->basis.right));
    result.basis.up = b3d_vec3_normalize(b3d_basis_transform_vector(parent->basis, child->basis.up));
    result.basis.forward = b3d_vec3_normalize(b3d_basis_transform_vector(parent->basis, child->basis.forward));
    result.basis = b3d_basis_orthonormalize(result.basis);
    result.scale = b3d_vec3_mul_components(parent->scale, child->scale);
    return result;
}

B3D_Transform b3d_transform_lerp(const B3D_Transform *a, const B3D_Transform *b, B3D_Fixed t)
{
    B3D_Transform result;

    if (a == 0 && b == 0) {
        return b3d_transform_identity();
    }
    if (a == 0) {
        return *b;
    }
    if (b == 0) {
        return *a;
    }

    t = b3d_fixed_clamp(t, 0, B3D_FIXED_ONE);
    result.position = b3d_vec3_lerp(a->position, b->position, t);
    result.scale = b3d_vec3_lerp(a->scale, b->scale, t);
    result.basis.right = b3d_vec3_lerp(a->basis.right, b->basis.right, t);
    result.basis.up = b3d_vec3_lerp(a->basis.up, b->basis.up, t);
    result.basis.forward = b3d_vec3_lerp(a->basis.forward, b->basis.forward, t);
    result.basis = b3d_basis_orthonormalize(result.basis);
    return result;
}
