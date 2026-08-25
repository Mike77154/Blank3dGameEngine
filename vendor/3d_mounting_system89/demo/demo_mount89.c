#include "mount89.h"
#include <stdio.h>

#define DEMO_MAX_OBJECTS 16
#define MASK_RIDER 1UL
#define MASK_PROP  2UL

static mount89_transform g_world[DEMO_MAX_OBJECTS];

static int demo_get_world(void *user, int object_id, mount89_transform *out_world)
{
    (void)user;
    if (object_id < 0 || object_id >= DEMO_MAX_OBJECTS || out_world == 0) {
        return 0;
    }
    mount89_transform_copy(out_world, &g_world[object_id]);
    return 1;
}

static int demo_set_world(void *user, int object_id,
                          const mount89_transform *world)
{
    (void)user;
    if (object_id < 0 || object_id >= DEMO_MAX_OBJECTS || world == 0) {
        return 0;
    }
    mount89_transform_copy(&g_world[object_id], world);
    return 1;
}

static void demo_event(void *user, int event_type,
                       int host_id, int point_id, int guest_id)
{
    (void)user;
    printf("event=%d host=%d point=%d guest=%d\n",
           event_type, host_id, point_id, guest_id);
}

static void print_pos(const char *name, const mount89_transform *t)
{
    printf("%s = (%ld, %ld, %ld)\n", name,
           mount89_fx_to_int(t->p[0]),
           mount89_fx_to_int(t->p[1]),
           mount89_fx_to_int(t->p[2]));
}

int main(void)
{
    mount89_context mounts;
    mount89_transform saddle;
    int i;
    int rc;

    for (i = 0; i < DEMO_MAX_OBJECTS; ++i) {
        mount89_transform_identity(&g_world[i]);
    }

    /* Object 1 = mountable host. Object 2 = rider. */
    g_world[1].p[0] = mount89_fx_from_int(10L);
    mount89_transform_identity(&saddle);
    saddle.p[1] = mount89_fx_from_int(2L);

    mount89_init(&mounts);
    mount89_set_transform_provider(&mounts, demo_get_world, demo_set_world, 0);
    mount89_set_event_provider(&mounts, demo_event, 0);

    rc = mount89_add_static_point(&mounts, 1, 100, MASK_RIDER, &saddle);
    if (rc != MOUNT89_OK) {
        printf("add point failed: %d\n", rc);
        return 1;
    }

    rc = mount89_mount(&mounts, 1, 100, 2, MASK_RIDER,
                       MOUNT89_MOUNT_SNAP, MOUNT89_INHERIT_ALL, 0, 0);
    if (rc != MOUNT89_OK) {
        printf("mount failed: %d\n", rc);
        return 1;
    }

    print_pos("rider after mount", &g_world[2]);

    g_world[1].p[0] = mount89_fx_from_int(20L);
    g_world[1].p[2] = mount89_fx_from_int(5L);
    rc = mount89_update(&mounts);
    if (rc != MOUNT89_OK) {
        printf("update failed: %d\n", rc);
        return 1;
    }
    print_pos("rider after host move", &g_world[2]);

    rc = mount89_unmount(&mounts, 2, MOUNT89_UNMOUNT_KEEP_WORLD);
    if (rc != MOUNT89_OK) {
        printf("unmount failed: %d\n", rc);
        return 1;
    }
    print_pos("rider after unmount", &g_world[2]);
    return 0;
}
