#include "blank3d_runtime_spine.h"
#include "blank3d_objects.h"
#include <stdio.h>
#include <string.h>

typedef struct DummyTag { long x; long y; long z; } Dummy;

static int query_pose(void *user, unsigned long id, void *native_object,
                      Blank3DRuntimePose *out)
{
    Dummy *d;
    (void)user;
    (void)id;
    d = (Dummy *)native_object;
    if (!d || !out) return 0;
    memset(out, 0, sizeof(*out));
    out->px = d->x;
    out->py = d->y;
    out->pz = d->z;
    out->half_x = 32768L;
    out->half_y = 32768L;
    out->half_z = 32768L;
    return 1;
}

int main(void)
{
    static Blank3DRuntimeSpine spine;
    static Blank3DObjects objects;
    static Blank3DClassSystem classes;
    Blank3DRuntimeProvider provider;
    Blank3DObjectHost host;
    Blank3DRuntimeInstance *a;
    Blank3DRuntimeInstance *b;
    Dummy da;
    Dummy db;
    TS89_Thing ta;
    TS89_Thing tb;
    unsigned int oa;
    unsigned int ob;
    const Blank3DObjectEntity *ia;
    const Blank3DObjectEntity *ib;

    memset(&provider, 0, sizeof(provider));
    provider.query_pose = query_pose;
    memset(&host, 0, sizeof(host));
    da.x = 65536L; da.y = 0L; da.z = 0L;
    db.x = 131072L; db.y = 0L; db.z = 0L;

    if (!blank3d_classes_init(&classes, 0)) return 1;
    if (!blank3d_runtime_spine_init(&spine, &provider)) return 1;
    blank3d_objects_init(&objects, &host, &classes);

    if (!blank3d_runtime_instance_create(&spine, 11UL, 0x1111UL, &da, &ta)) return 2;
    if (!blank3d_runtime_instance_create(&spine, 12UL, 0x1111UL, &db, &tb)) return 3;
    if (ta == tb) return 4;

    a = blank3d_runtime_find_thing(&spine, ta);
    b = blank3d_runtime_find_thing(&spine, tb);
    if (!a || !b) return 5;
    if (!ecs_entity_valid(&spine.ecs, a->entity) ||
        !ecs_entity_valid(&spine.ecs, b->entity)) return 6;
    if (spine.thing_store[a->entity.index].thing != ta ||
        spine.thing_store[b->entity.index].thing != tb) return 7;

    if (!blank3d_runtime_bind_actor(&spine, ta, 77)) return 8;
    if (spine.actor_store[a->entity.index].actor_id != 77) return 9;
    if (spine.actor_store[b->entity.index].actor_id != 0) return 10;

    /* GFO self/runtime key reserves zero, while TS89 Thing may legally be zero. */
    if (!blank3d_objects_spawn_ex(&objects, "config/entities/player.ini",
                                  11UL, (unsigned long)ta + 1UL, &da, &oa)) return 11;
    if (!blank3d_objects_spawn_ex(&objects, "config/entities/player.ini",
                                  12UL, (unsigned long)tb + 1UL, &db, &ob)) return 12;
    ia = blank3d_objects_get(&objects, oa);
    ib = blank3d_objects_get(&objects, ob);
    if (!ia || !ib) return 13;
    if (ia->definition_slot != ib->definition_slot || objects.definition_count != 1U) return 14;
    if (ia->runtime_key != (unsigned long)ta + 1UL ||
        ib->runtime_key != (unsigned long)tb + 1UL) return 15;
    {
        cm89_class_h ca;
        cm89_class_h cb;
        ca = blank3d_objects_class_of_slot(&objects, oa);
        cb = blank3d_objects_class_of_slot(&objects, ob);
        if (ca == CM89_CLASS_NONE || ca != cb) return 19;
        if (!blank3d_classes_is_subclass(&classes, ca, classes.object_class)) return 20;
    }

    blank3d_objects_kill(&objects, oa);
    blank3d_objects_kill(&objects, ob);
    if (!blank3d_runtime_instance_destroy(&spine, ta)) return 16;
    if (!blank3d_runtime_instance_destroy(&spine, tb)) return 17;
    if (blank3d_runtime_count(&spine) != 0) return 18;

    puts("runtime Object->Thing->ECS->World/Scene + optional Actor test: OK");
    return 0;
}
