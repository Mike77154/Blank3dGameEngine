#ifndef B3D_TRANSFORM_H
#define B3D_TRANSFORM_H

#include "bolt3d/b3d_vec3.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct B3D_Basis3 {
    B3D_Vec3 right;
    B3D_Vec3 up;
    B3D_Vec3 forward;
} B3D_Basis3;

typedef struct B3D_Transform {
    B3D_Vec3 position;
    B3D_Basis3 basis;
    B3D_Vec3 scale;
} B3D_Transform;

B3D_API B3D_Basis3 b3d_basis_identity(void);
B3D_API B3D_Basis3 b3d_basis_from_forward_up(B3D_Vec3 forward, B3D_Vec3 up_hint);
B3D_API B3D_Basis3 b3d_basis_orthonormalize(B3D_Basis3 basis);
B3D_API B3D_Basis3 b3d_basis_integrate_angular(B3D_Basis3 basis, B3D_Vec3 angular_velocity, B3D_Fixed dt);
B3D_API B3D_Vec3 b3d_basis_transform_vector(B3D_Basis3 basis, B3D_Vec3 local_vector);
B3D_API B3D_Vec3 b3d_basis_untransform_vector(B3D_Basis3 basis, B3D_Vec3 world_vector);

B3D_API B3D_Transform b3d_transform_identity(void);
B3D_API B3D_Transform b3d_transform_from_position(B3D_Vec3 position);
B3D_API B3D_Vec3 b3d_transform_vector(const B3D_Transform *transform_value, B3D_Vec3 local_vector);
B3D_API B3D_Vec3 b3d_transform_point(const B3D_Transform *transform_value, B3D_Vec3 local_point);
B3D_API B3D_Vec3 b3d_transform_vector_to_local(const B3D_Transform *transform_value, B3D_Vec3 world_vector);
B3D_API B3D_Vec3 b3d_transform_point_to_local(const B3D_Transform *transform_value, B3D_Vec3 world_point);
B3D_API B3D_Transform b3d_transform_compose(const B3D_Transform *parent, const B3D_Transform *child);
B3D_API B3D_Transform b3d_transform_lerp(const B3D_Transform *a, const B3D_Transform *b, B3D_Fixed t);

#ifdef __cplusplus
}
#endif

#endif
