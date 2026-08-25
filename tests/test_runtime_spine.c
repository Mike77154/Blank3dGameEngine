#include "blank3d_runtime_spine.h"
#include <stdio.h>
#include <string.h>

typedef struct DummyTag { long x,y,z; } Dummy;
static int query_pose(void *user, unsigned long id, void *native_object,
                      Blank3DRuntimePose *out)
{
    Dummy *d;
    (void)user; (void)id;
    d=(Dummy*)native_object;
    if(!d||!out) return 0;
    memset(out,0,sizeof(*out));
    out->px=d->x; out->py=d->y; out->pz=d->z;
    out->half_x=out->half_y=out->half_z=32768L;
    return 1;
}
int main(void)
{
    static Blank3DRuntimeSpine spine;
    Blank3DRuntimeProvider provider;
    Blank3DRuntimeInstance *r;
    Dummy d;
    TS89_Thing thing;
    unsigned long key;
    memset(&provider,0,sizeof(provider)); provider.query_pose=query_pose;
    d.x=65536L; d.y=0; d.z=131072L;
    /* Per-instance records must stay handles/refs only: managers live once in spine. */
    if (sizeof(Blank3DRuntimeInstance) > 128U) return 11;
    if(!blank3d_runtime_spine_init(&spine,&provider)) return 1;
    if(!blank3d_runtime_instance_create(&spine,77UL,123UL,&d,&thing)) return 2;
    r=blank3d_runtime_find_thing(&spine,thing); if(!r) return 3;
    key=blank3d_runtime_entity_key(r->entity); if(!key || !ecs_entity_valid(&spine.ecs,r->entity)) return 4;
    if(!blank3d_runtime_bind_actor(&spine,thing,9001)) return 5;
    d.x=3L*65536L; if(blank3d_runtime_sync(&spine)!=1) return 6;
    if(!w3d_entity_resolve(&spine.world,r->world_proxy) || w3d_entity_resolve(&spine.world,r->world_proxy)->xf.pos.x!=d.x) return 7;
    if(blank3d_runtime_count(&spine)!=1 || !ts89_check_integrity(&spine.things)) return 8;
    if(!blank3d_runtime_instance_destroy(&spine,thing)) return 9;
    if(blank3d_runtime_count(&spine)!=0 || ecs_entity_valid(&spine.ecs,blank3d_runtime_entity_from_key(key))) return 10;
    puts("runtime spine Thing->ECS->World/Scene + ActorRef test: OK");
    return 0;
}
