#include "mount89.h"
#include <stdio.h>

#define TEST_OBJECTS 32
#define RIDER 1UL

static mount89_transform w[TEST_OBJECTS];
static int dynamic_y;

static int get_world(void *u, int id, mount89_transform *out)
{
    (void)u;
    if (id < 0 || id >= TEST_OBJECTS) return 0;
    mount89_transform_copy(out, &w[id]);
    return 1;
}

static int set_world(void *u, int id, const mount89_transform *in)
{
    (void)u;
    if (id < 0 || id >= TEST_OBJECTS) return 0;
    mount89_transform_copy(&w[id], in);
    return 1;
}

static int resolve_point(void *u, int host_id, int source_id,
                         mount89_transform *out)
{
    (void)u;
    (void)host_id;
    (void)source_id;
    mount89_transform_identity(out);
    out->p[1] = mount89_fx_from_int((long)dynamic_y);
    return 1;
}

static int eqi(mount89_fx a, long b)
{
    return mount89_fx_to_int(a) == b;
}

static int fail(const char *what, int line)
{
    printf("FAIL line %d: %s\n", line, what);
    return 1;
}

#define CHECK(x, msg) do { if (!(x)) return fail((msg), __LINE__); } while (0)

static void reset_world(void)
{
    int i;
    for (i = 0; i < TEST_OBJECTS; ++i) {
        mount89_transform_identity(&w[i]);
    }
}

static int test_snap_follow(void)
{
    mount89_context c;
    mount89_transform p;
    reset_world();
    mount89_init(&c);
    mount89_set_transform_provider(&c, get_world, set_world, 0);
    mount89_transform_identity(&p);
    w[1].p[0] = mount89_fx_from_int(10);
    p.p[1] = mount89_fx_from_int(2);
    CHECK(mount89_add_static_point(&c, 1, 10, RIDER, &p) == MOUNT89_OK, "add");
    CHECK(mount89_mount(&c, 1, 10, 2, RIDER, MOUNT89_MOUNT_SNAP,
                        MOUNT89_INHERIT_ALL, 0, 0) == MOUNT89_OK, "mount");
    CHECK(eqi(w[2].p[0], 10) && eqi(w[2].p[1], 2), "snap position");
    w[1].p[0] = mount89_fx_from_int(15);
    CHECK(mount89_update(&c) == MOUNT89_OK, "update");
    CHECK(eqi(w[2].p[0], 15) && eqi(w[2].p[1], 2), "follow");
    return 0;
}

static int test_keep_world(void)
{
    mount89_context c;
    mount89_transform p;
    reset_world();
    mount89_init(&c);
    mount89_set_transform_provider(&c, get_world, set_world, 0);
    mount89_transform_identity(&p);
    w[1].p[0] = mount89_fx_from_int(10);
    w[2].p[0] = mount89_fx_from_int(25);
    CHECK(mount89_add_static_point(&c, 1, 10, RIDER, &p) == MOUNT89_OK, "add");
    CHECK(mount89_mount(&c, 1, 10, 2, RIDER, MOUNT89_MOUNT_KEEP_WORLD,
                        MOUNT89_INHERIT_ALL, 0, 0) == MOUNT89_OK, "keep mount");
    CHECK(eqi(w[2].p[0], 25), "kept world");
    w[1].p[0] = mount89_fx_from_int(11);
    CHECK(mount89_update(&c) == MOUNT89_OK, "update");
    CHECK(eqi(w[2].p[0], 26), "offset follows");
    return 0;
}

static int test_nested_and_cycle(void)
{
    mount89_context c;
    mount89_transform p;
    reset_world();
    mount89_init(&c);
    mount89_set_transform_provider(&c, get_world, set_world, 0);
    mount89_transform_identity(&p);
    p.p[1] = mount89_fx_from_int(1);
    CHECK(mount89_add_static_point(&c, 1, 10, RIDER, &p) == MOUNT89_OK, "p1");
    CHECK(mount89_add_static_point(&c, 2, 20, RIDER, &p) == MOUNT89_OK, "p2");
    CHECK(mount89_add_static_point(&c, 3, 30, RIDER, &p) == MOUNT89_OK, "p3");
    CHECK(mount89_mount(&c, 1, 10, 2, RIDER, MOUNT89_MOUNT_SNAP,
                        MOUNT89_INHERIT_ALL, 0, 0) == MOUNT89_OK, "m1");
    CHECK(mount89_mount(&c, 2, 20, 3, RIDER, MOUNT89_MOUNT_SNAP,
                        MOUNT89_INHERIT_ALL, 0, 0) == MOUNT89_OK, "m2");
    CHECK(mount89_can_mount(&c, 3, 30, 1, RIDER) == MOUNT89_ERR_CYCLE, "cycle");
    w[1].p[0] = mount89_fx_from_int(7);
    CHECK(mount89_update(&c) == MOUNT89_OK, "nested update");
    CHECK(eqi(w[2].p[0], 7) && eqi(w[2].p[1], 1), "nested guest 1");
    CHECK(eqi(w[3].p[0], 7) && eqi(w[3].p[1], 2), "nested guest 2");
    return 0;
}

static int test_dynamic_point(void)
{
    mount89_context c;
    reset_world();
    mount89_init(&c);
    mount89_set_transform_provider(&c, get_world, set_world, 0);
    mount89_set_point_provider(&c, resolve_point, 0);
    dynamic_y = 3;
    CHECK(mount89_add_dynamic_point(&c, 1, 99, 1234, RIDER) == MOUNT89_OK, "dynamic add");
    CHECK(mount89_mount(&c, 1, 99, 2, RIDER, MOUNT89_MOUNT_SNAP,
                        MOUNT89_INHERIT_ALL, 0, 0) == MOUNT89_OK, "dynamic mount");
    CHECK(eqi(w[2].p[1], 3), "dynamic initial");
    dynamic_y = 8;
    CHECK(mount89_update(&c) == MOUNT89_OK, "dynamic update");
    CHECK(eqi(w[2].p[1], 8), "dynamic follows source");
    return 0;
}

static int test_rotation_90z(void)
{
    mount89_context c;
    mount89_transform p;
    reset_world();
    mount89_init(&c);
    mount89_set_transform_provider(&c, get_world, set_world, 0);
    mount89_transform_identity(&p);
    p.p[0] = mount89_fx_from_int(1);

    w[1].r[0] = 0;
    w[1].r[1] = (mount89_rot)-MOUNT89_ROT_ONE;
    w[1].r[2] = 0;
    w[1].r[3] = (mount89_rot)MOUNT89_ROT_ONE;
    w[1].r[4] = 0;
    w[1].r[5] = 0;
    w[1].r[6] = 0;
    w[1].r[7] = 0;
    w[1].r[8] = (mount89_rot)MOUNT89_ROT_ONE;

    CHECK(mount89_add_static_point(&c, 1, 10, RIDER, &p) == MOUNT89_OK, "rot add");
    CHECK(mount89_mount(&c, 1, 10, 2, RIDER, MOUNT89_MOUNT_SNAP,
                        MOUNT89_INHERIT_ALL, 0, 0) == MOUNT89_OK, "rot mount");
    CHECK(eqi(w[2].p[0], 0) && eqi(w[2].p[1], 1), "rotated point");
    return 0;
}


static int test_keep_world_rotated(void)
{
    mount89_context c;
    mount89_transform p;
    reset_world();
    mount89_init(&c);
    mount89_set_transform_provider(&c, get_world, set_world, 0);
    mount89_transform_identity(&p);

    w[1].p[0] = mount89_fx_from_int(10);
    w[1].r[0] = 0;
    w[1].r[1] = (mount89_rot)-MOUNT89_ROT_ONE;
    w[1].r[2] = 0;
    w[1].r[3] = (mount89_rot)MOUNT89_ROT_ONE;
    w[1].r[4] = 0;
    w[1].r[5] = 0;
    w[1].r[6] = 0;
    w[1].r[7] = 0;
    w[1].r[8] = (mount89_rot)MOUNT89_ROT_ONE;

    w[2].p[0] = mount89_fx_from_int(13);
    w[2].p[1] = mount89_fx_from_int(4);

    CHECK(mount89_add_static_point(&c, 1, 10, RIDER, &p) == MOUNT89_OK,
          "rot keep add");
    CHECK(mount89_mount(&c, 1, 10, 2, RIDER, MOUNT89_MOUNT_KEEP_WORLD,
                        MOUNT89_INHERIT_ALL, 0, 0) == MOUNT89_OK,
          "rot keep mount");
    CHECK(eqi(w[2].p[0], 13) && eqi(w[2].p[1], 4), "rot keep initial");

    w[1].p[0] = mount89_fx_from_int(11);
    CHECK(mount89_update(&c) == MOUNT89_OK, "rot keep update");
    CHECK(eqi(w[2].p[0], 14) && eqi(w[2].p[1], 4), "rot keep follows");
    return 0;
}

static int test_unmount_restore(void)
{
    mount89_context c;
    mount89_transform p;
    reset_world();
    mount89_init(&c);
    mount89_set_transform_provider(&c, get_world, set_world, 0);
    mount89_transform_identity(&p);
    w[2].p[0] = mount89_fx_from_int(42);
    CHECK(mount89_add_static_point(&c, 1, 10, RIDER, &p) == MOUNT89_OK, "add");
    CHECK(mount89_mount(&c, 1, 10, 2, RIDER, MOUNT89_MOUNT_SNAP,
                        MOUNT89_INHERIT_ALL, 0, 0) == MOUNT89_OK, "mount");
    CHECK(eqi(w[2].p[0], 0), "snapped");
    CHECK(mount89_unmount(&c, 2, MOUNT89_UNMOUNT_RESTORE_WORLD) == MOUNT89_OK,
          "restore unmount");
    CHECK(eqi(w[2].p[0], 42), "restored");
    return 0;
}

int main(void)
{
    if (test_snap_follow()) return 1;
    if (test_keep_world()) return 1;
    if (test_nested_and_cycle()) return 1;
    if (test_dynamic_point()) return 1;
    if (test_rotation_90z()) return 1;
    if (test_keep_world_rotated()) return 1;
    if (test_unmount_restore()) return 1;
    printf("mount89: all tests passed\n");
    return 0;
}
