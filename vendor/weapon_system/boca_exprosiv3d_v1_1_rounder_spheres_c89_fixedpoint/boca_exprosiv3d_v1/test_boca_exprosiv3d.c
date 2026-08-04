#include <stdio.h>
#include <string.h>
#include "boca_exprosiv3d.h"

#define TEST_VERTEX_CAPACITY 4096UL
#define TEST_INDEX_CAPACITY  32768UL

static BEX3D_Vertex test_vertices[TEST_VERTEX_CAPACITY];
static unsigned short test_indices[TEST_INDEX_CAPACITY];

static int failures = 0;

static void test_expect(int condition, const char *name)
{
    if (condition)
    {
        printf("PASS: %s\n", name);
    }
    else
    {
        printf("FAIL: %s\n", name);
        ++failures;
    }
}

static int primitive_equal(const BEX3D_Primitive *a,
                           const BEX3D_Primitive *b)
{
    return a->type == b->type &&
           a->role == b->role &&
           a->sides == b->sides &&
           a->stacks == b->stacks &&
           a->flags == b->flags &&
           a->start_us == b->start_us &&
           a->duration_us == b->duration_us &&
           a->x0 == b->x0 && a->y0 == b->y0 && a->z0 == b->z0 &&
           a->x1 == b->x1 && a->y1 == b->y1 && a->z1 == b->z1 &&
           a->radius_a0 == b->radius_a0 &&
           a->radius_a1 == b->radius_a1 &&
           a->radius_b0 == b->radius_b0 &&
           a->radius_b1 == b->radius_b1 &&
           a->length0 == b->length0 &&
           a->length1 == b->length1 &&
           a->yaw == b->yaw &&
           a->pitch == b->pitch &&
           a->roll0 == b->roll0 &&
           a->roll1 == b->roll1 &&
           a->hot_color.r == b->hot_color.r &&
           a->hot_color.g == b->hot_color.g &&
           a->hot_color.b == b->hot_color.b &&
           a->hot_color.a == b->hot_color.a &&
           a->cool_color.r == b->cool_color.r &&
           a->cool_color.g == b->cool_color.g &&
           a->cool_color.b == b->cool_color.b &&
           a->cool_color.a == b->cool_color.a &&
           a->alpha0 == b->alpha0 &&
           a->alpha1 == b->alpha1;
}

static int state_geometry_equal(const BEX3D_State *a,
                                const BEX3D_State *b)
{
    int i;

    if (a->primitive_count != b->primitive_count)
    {
        return 0;
    }
    for (i = 0; i < a->primitive_count; ++i)
    {
        if (!primitive_equal(&a->primitives[i], &b->primitives[i]))
        {
            return 0;
        }
    }
    return 1;
}

static int count_role(const BEX3D_State *state, int role)
{
    int i;
    int count;

    count = 0;
    for (i = 0; i < state->primitive_count; ++i)
    {
        if (state->primitives[i].role == role)
        {
            ++count;
        }
    }
    return count;
}

static int indices_are_valid(const BEX3D_MeshBuffer *mesh)
{
    unsigned long i;

    for (i = 0UL; i < mesh->index_count; ++i)
    {
        if ((unsigned long)mesh->indices[i] >= mesh->vertex_count)
        {
            return 0;
        }
    }
    return 1;
}

static void prepare_mesh(BEX3D_MeshBuffer *mesh)
{
    mesh->vertices = test_vertices;
    mesh->indices = test_indices;
    mesh->vertex_capacity = TEST_VERTEX_CAPACITY;
    mesh->index_capacity = TEST_INDEX_CAPACITY;
    mesh->vertex_count = 0UL;
    mesh->index_count = 0UL;
    mesh->truncated = 0;
}

static void test_fixed_math(void)
{
    test_expect(bex3d_sin(0) == 0, "sin(0)");
    test_expect(bex3d_sin(64) == BEX3D_FP_ONE, "sin(90 degrees)");
    test_expect(bex3d_sin(128) == 0, "sin(180 degrees)");
    test_expect(bex3d_cos(0) == BEX3D_FP_ONE, "cos(0)");
    test_expect(bex3d_mul(BEX3D_INT(2), BEX3D_INT(3)) == BEX3D_INT(6),
                "fixed multiplication");
    test_expect(bex3d_div(BEX3D_INT(6), BEX3D_INT(3)) == BEX3D_INT(2),
                "fixed division");
}

static void test_determinism(void)
{
    BEX3D_State a;
    BEX3D_State b;
    BEX3D_Profile profile;

    bex3d_default_profile(&profile, BEX3D_PROFILE_SHOTGUN);
    bex3d_init(&a, 0x47494646UL);
    bex3d_init(&b, 0x47494646UL);
    bex3d_set_profile(&a, &profile);
    bex3d_set_profile(&b, &profile);
    bex3d_fire(&a);
    bex3d_fire(&b);

    test_expect(state_geometry_equal(&a, &b),
                "same seed creates identical geometry descriptors");
}

static void test_profiles_and_meshes(void)
{
    int profile_id;
    BEX3D_State state;
    BEX3D_Profile profile;
    BEX3D_MeshBuffer mesh;
    BEX3D_Fixed original_fuel;
    int result;

    prepare_mesh(&mesh);

    for (profile_id = 0; profile_id < BEX3D_PROFILE_COUNT; ++profile_id)
    {
        bex3d_default_profile(&profile, profile_id);
        original_fuel = profile.residual_fuel;
        bex3d_init(&state, 0xB0CA0000UL + (unsigned long)profile_id);
        bex3d_set_profile(&state, &profile);
        bex3d_fire(&state);

        test_expect(state.active != 0, "profile fire activates event");
        test_expect(state.primitive_count > 0,
                    "profile creates at least one primitive");
        test_expect(state.primitive_count <= profile.primitive_budget,
                    "profile respects primitive budget");
        test_expect(state.profile.residual_fuel == original_fuel,
                    "fire does not mutate physical profile");

        result = bex3d_build_mesh(&state, &mesh);
        test_expect(result == BEX3D_BUILD_OK,
                    "age zero mesh builds without truncation");
        test_expect(mesh.vertex_count > 0UL && mesh.index_count > 0UL,
                    "age zero mesh contains triangles");
        test_expect(indices_are_valid(&mesh),
                    "age zero indices stay in range");

        bex3d_update_us(&state, 900UL);
        result = bex3d_build_mesh(&state, &mesh);
        test_expect(result == BEX3D_BUILD_OK,
                    "peak mesh builds without truncation");
        test_expect(mesh.vertex_count > 0UL && mesh.index_count > 0UL,
                    "peak mesh contains triangles");
        test_expect(indices_are_valid(&mesh),
                    "peak indices stay in range");

        bex3d_update_us(&state, state.duration_us);
        test_expect(state.active == 0, "event deactivates after duration");
        result = bex3d_build_mesh(&state, &mesh);
        test_expect(result == BEX3D_BUILD_OK,
                    "expired event returns valid empty mesh");
        test_expect(mesh.vertex_count == 0UL && mesh.index_count == 0UL,
                    "expired event mesh is empty");
    }
}

static void test_device_signatures(void)
{
    BEX3D_State state;
    BEX3D_Profile profile;

    bex3d_default_profile(&profile, BEX3D_PROFILE_MAGNUM_REVOLVER);
    bex3d_init(&state, 0x357357UL);
    bex3d_set_profile(&state, &profile);
    bex3d_fire(&state);
    test_expect(count_role(&state, BEX3D_ROLE_CYLINDER_GAP) == 2,
                "revolver creates two cylinder-gap jets");

    bex3d_default_profile(&profile, BEX3D_PROFILE_PRECISION_BRAKE);
    bex3d_init(&state, 0x762762UL);
    bex3d_set_profile(&state, &profile);
    bex3d_fire(&state);
    test_expect(count_role(&state, BEX3D_ROLE_DEVICE_JET) >= 2,
                "muzzle brake creates lateral device jets");

    bex3d_default_profile(&profile, BEX3D_PROFILE_SUPPRESSED);
    bex3d_init(&state, 0x12345678UL);
    bex3d_set_profile(&state, &profile);
    bex3d_fire(&state);
    test_expect(state.primitive_count <= 10,
                "suppressed profile remains inside tiny budget");
}

static void test_small_buffer_truncation(void)
{
    BEX3D_State state;
    BEX3D_Profile profile;
    BEX3D_Vertex tiny_vertices[8];
    unsigned short tiny_indices[12];
    BEX3D_MeshBuffer mesh;
    int result;

    bex3d_default_profile(&profile, BEX3D_PROFILE_MAGNUM_AUTO);
    bex3d_init(&state, 1234UL);
    bex3d_set_profile(&state, &profile);
    bex3d_fire(&state);

    mesh.vertices = tiny_vertices;
    mesh.indices = tiny_indices;
    mesh.vertex_capacity = 8UL;
    mesh.index_capacity = 12UL;
    mesh.vertex_count = 0UL;
    mesh.index_count = 0UL;
    mesh.truncated = 0;

    result = bex3d_build_mesh(&state, &mesh);
    test_expect(result == BEX3D_BUILD_TRUNCATED,
                "small caller buffer reports truncation safely");
}

int main(void)
{
    printf("boca_exprosiv3d strict C89 test suite\n");
    printf("sizeof(BEX3D_Profile)   = %lu bytes\n",
           (unsigned long)sizeof(BEX3D_Profile));
    printf("sizeof(BEX3D_Primitive) = %lu bytes\n",
           (unsigned long)sizeof(BEX3D_Primitive));
    printf("sizeof(BEX3D_State)      = %lu bytes\n",
           (unsigned long)sizeof(BEX3D_State));
    printf("sizeof(BEX3D_Vertex)     = %lu bytes\n",
           (unsigned long)sizeof(BEX3D_Vertex));

    test_fixed_math();
    test_determinism();
    test_profiles_and_meshes();
    test_device_signatures();
    test_small_buffer_truncation();

    if (failures != 0)
    {
        printf("TEST RESULT: %d failure(s)\n", failures);
        return 1;
    }

    printf("TEST RESULT: all tests passed\n");
    return 0;
}
