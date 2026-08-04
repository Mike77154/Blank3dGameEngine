/*
    CCS self-test (no stdio)
    -----------------------
    Build example:
        gcc -std=c89 -Wall -Wextra -pedantic -Werror -I. \
            ccs_*.c collision_api.c ccs_selftest.c -o ccs_selftest

    Returns 0 on success, non-zero on failure.
*/

#include "ccs_fixed.h"
#include "ccs_math.h"
#include "ccs_collision.h"
#include "ccs_world.h"

/* Minimal assert without <assert.h> */
#define CCS_TEST_FAIL(code) do { return (code); } while (0)

static int ccs_selftest_fixed_basic(void)
{
    ccs_fixed a;
    ccs_fixed b;
    ccs_fixed m;
    ccs_fixed d;
    ccs_fixed s;

    a = ccs_fixed_from_int(2);
    b = ccs_fixed_from_int(3);

    if (a != (ccs_fixed)(2 * CCS_FIXED_ONE)) CCS_TEST_FAIL(10);
    if (b != (ccs_fixed)(3 * CCS_FIXED_ONE)) CCS_TEST_FAIL(11);

    m = ccs_fixed_mul(a, b);
    if (m != (ccs_fixed)(6 * CCS_FIXED_ONE)) CCS_TEST_FAIL(12);

    d = ccs_fixed_div(b, a); /* 3/2 = 1.5 */
    if (d != (ccs_fixed)(CCS_FIXED_ONE + CCS_FIXED_HALF)) CCS_TEST_FAIL(13);

    s = CCS_FIXED_SQRT(ccs_fixed_from_int(4));
    if (s != ccs_fixed_from_int(2)) CCS_TEST_FAIL(14);

    return 0;
}

static int ccs_selftest_sphere_sphere_case(void)
{
    ccs_shape_sphere A;
    ccs_shape_sphere B;
    ccs_collision_result r;

    /* A at x=0, B at x=1.5, both radius 1.0 => penetration 0.5 */
    A.header.type = CCS_SHAPE_SPHERE;
    A.header.flags = 0;
    A.center = ccs_vec3_zero();
    A.radius = ccs_fixed_from_int(1);

    B.header.type = CCS_SHAPE_SPHERE;
    B.header.flags = 0;
    B.center = ccs_vec3_make(ccs_fixed_from_int(1) + CCS_FIXED_HALF, 0, 0);
    B.radius = ccs_fixed_from_int(1);

    r = ccs_collide(&A, &B, 0);
    if (!r.hit) CCS_TEST_FAIL(20);

    if (r.contact.penetration != (ccs_fixed)CCS_FIXED_HALF) CCS_TEST_FAIL(21);
    if (r.contact.normal.x <= 0) CCS_TEST_FAIL(22);

    return 0;
}

static int ccs_selftest_world_separation(void)
{
    ccs_shape_sphere A;
    ccs_shape_sphere B;
    ccs_vec3 pA;
    ccs_vec3 pB;
    int idA;
    int idB;

    A.header.type = CCS_SHAPE_SPHERE;
    A.header.flags = 0;
    A.center = ccs_vec3_zero();
    A.radius = ccs_fixed_from_int(1);

    B.header.type = CCS_SHAPE_SPHERE;
    B.header.flags = 0;
    B.center = ccs_vec3_zero();
    B.radius = ccs_fixed_from_int(1);

    pA = ccs_vec3_zero();
    pB = ccs_vec3_make(ccs_fixed_from_int(1) + CCS_FIXED_HALF, 0, 0); /* 1.5 */

    ccs_world_clear();
    ccs_world_set_broadphase(CCS_WORLDBP_SWEEP);

    idA = ccs_world_add(&pA, (ccs_shape*)&A);
    idB = ccs_world_add(&pB, (ccs_shape*)&B);

    if (idA < 0 || idB < 0) CCS_TEST_FAIL(30);

    ccs_world_step();

    /* expected: A moves left, B moves right */
    if (pA.x >= 0) CCS_TEST_FAIL(31);
    if (pB.x <= (ccs_fixed)(CCS_FIXED_ONE + CCS_FIXED_HALF)) CCS_TEST_FAIL(32);

    return 0;
}

int main(void)
{
    int rc;

    rc = ccs_selftest_fixed_basic();
    if (rc) return rc;

    rc = ccs_selftest_sphere_sphere_case();
    if (rc) return rc;

    rc = ccs_selftest_world_separation();
    if (rc) return rc;

    return 0;
}
