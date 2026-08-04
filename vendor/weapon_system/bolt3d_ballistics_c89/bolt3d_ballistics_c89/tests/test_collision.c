#include <stdio.h>
#include "bolt3d/bolt3d.h"

int main(void)
{
    B3D_Hit hit;
    B3D_Vec3 from;
    B3D_Vec3 to;
    B3D_Vec3 center;
    int ok;

    from = b3d_vec3(0, 0, 0);
    to = b3d_vec3(b3d_fixed_from_int(10), 0, 0);
    center = b3d_vec3(b3d_fixed_from_int(5), 0, 0);
    ok = b3d_segment_sphere(from, to, center, b3d_fixed_from_int(1), &hit);

    if (!ok || !hit.hit) {
        printf("FAIL segment_sphere no hit\n");
        return 1;
    }
    if (hit.t <= 0 || hit.t >= B3D_FIXED_ONE) {
        printf("FAIL segment_sphere t=%ld\n", hit.t);
        return 1;
    }

    printf("test_collision OK\n");
    return 0;
}
