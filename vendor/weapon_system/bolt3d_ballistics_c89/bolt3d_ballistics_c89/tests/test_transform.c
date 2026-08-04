#include <stdio.h>
#include "bolt3d/bolt3d.h"

static int near_value(B3D_Fixed a, B3D_Fixed b)
{
    B3D_Fixed tolerance;

    tolerance = b3d_fixed_div(B3D_FIXED_ONE, b3d_fixed_from_int(128));
    return b3d_fixed_abs(b3d_fixed_sub_sat(a, b)) <= tolerance;
}

int main(void)
{
    B3D_Transform transform_value;
    B3D_Vec3 local;
    B3D_Vec3 world;
    B3D_Vec3 round_trip;

    transform_value = b3d_transform_identity();
    transform_value.position = b3d_vec3(b3d_fixed_from_int(10), b3d_fixed_from_int(2), 0);
    transform_value.scale = b3d_vec3(b3d_fixed_from_int(2), b3d_fixed_from_int(3), B3D_FIXED_ONE);
    transform_value.basis = b3d_basis_from_forward_up(
        b3d_vec3(B3D_FIXED_ONE, 0, 0),
        b3d_vec3(0, B3D_FIXED_ONE, 0)
    );

    local = b3d_vec3(0, 0, b3d_fixed_from_int(2));
    world = b3d_transform_point(&transform_value, local);
    round_trip = b3d_transform_point_to_local(&transform_value, world);

    if (!near_value(world.x, b3d_fixed_from_int(12)) || !near_value(world.y, b3d_fixed_from_int(2))) {
        printf("FAIL transform world point %ld %ld %ld\n", world.x, world.y, world.z);
        return 1;
    }
    if (!near_value(round_trip.x, local.x) || !near_value(round_trip.y, local.y) || !near_value(round_trip.z, local.z)) {
        printf("FAIL transform round trip %ld %ld %ld\n", round_trip.x, round_trip.y, round_trip.z);
        return 1;
    }

    printf("test_transform OK\n");
    return 0;
}
