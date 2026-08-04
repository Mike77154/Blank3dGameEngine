#include <stdio.h>
#include "bolt3d/bolt3d.h"

static int expect_fixed(const char *name, B3D_Fixed got, B3D_Fixed expected)
{
    B3D_Fixed diff;

    diff = b3d_fixed_abs(b3d_fixed_sub_sat(got, expected));
    if (diff > b3d_fixed_div(B3D_FIXED_ONE, b3d_fixed_from_int(64))) {
        printf("FAIL %s got=%ld expected=%ld\n", name, got, expected);
        return 0;
    }
    return 1;
}

int main(void)
{
    int ok;
    B3D_Fixed two;
    B3D_Fixed three;
    B3D_Fixed six;
    B3D_Vec3 n;

    ok = 1;
    two = b3d_fixed_from_int(2);
    three = b3d_fixed_from_int(3);
    six = b3d_fixed_from_int(6);

    ok = expect_fixed("mul", b3d_fixed_mul(two, three), six) && ok;
    ok = expect_fixed("div", b3d_fixed_div(six, three), two) && ok;
    ok = expect_fixed("sqrt", b3d_fixed_sqrt(b3d_fixed_from_int(4)), two) && ok;

    n = b3d_vec3_normalize(b3d_vec3(three, 0, 0));
    ok = expect_fixed("normalize x", n.x, B3D_FIXED_ONE) && ok;

    if (!ok) {
        return 1;
    }
    printf("test_fixed OK\n");
    return 0;
}
