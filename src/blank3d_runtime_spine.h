#ifndef BLANK3D_RUNTIME_SPINE_H
#define BLANK3D_RUNTIME_SPINE_H

#include "thing_system89.h"
#include "ecs89.h"
#include "w3d89.h"
#include "sm3d_scene.h"

#ifdef __cplusplus
extern "C" {
#endif

#define B3D_RT_MAX_INSTANCES 128
#define B3D_RT_WORLD_ARENA_CAP 65536UL

#define B3D_RT_COMPONENT_IDENTITY 0
#define B3D_RT_COMPONENT_THING_REF 1
#define B3D_RT_COMPONENT_OBJECT_REF 2
#define B3D_RT_COMPONENT_WORLD_REF 3
#define B3D_RT_COMPONENT_SCENE_REF 4
#define B3D_RT_COMPONENT_ACTOR_REF 5
#define B3D_RT_COMPONENT_NATIVE_PTR 6

typedef struct Blank3DRuntimePoseTag {
    long px;
    long py;
    long pz;
    long rx;
    long ry;
    long rz;
    long half_x;
    long half_y;
    long half_z;
} Blank3DRuntimePose;

typedef struct Blank3DRuntimeProviderTag {
    void *user;
    int (*query_pose)(void *user, unsigned long subject_id,
                      void *native_object, Blank3DRuntimePose *out_pose);
} Blank3DRuntimeProvider;

typedef struct Blank3DRuntimeIdentityTag { unsigned long subject_id; } Blank3DRuntimeIdentity;
typedef struct Blank3DThingRefTag { TS89_Thing thing; } Blank3DThingRef;
typedef struct Blank3DObjectRefTag { unsigned long object_key; } Blank3DObjectRef;
typedef struct Blank3DWorldRefTag { w3d_handle proxy; } Blank3DWorldRef;
typedef struct Blank3DSceneRefTag { SM3D_Handle node; int scene_id; } Blank3DSceneRef;
typedef struct Blank3DActorRefTag { int actor_id; } Blank3DActorRef;

typedef struct Blank3DRuntimeInstanceTag {
    int used;
    unsigned long subject_id;
    unsigned long object_key;
    int actor_id;
    void *native_object;
    TS89_Thing thing;
    ecs_entity entity;
    w3d_handle world_proxy;
    SM3D_Handle scene_node;
} Blank3DRuntimeInstance;

typedef struct Blank3DRuntimeSpineTag {
    int initialized;
    TS89_System things;
    ecs_world ecs;
    w3d_world world;
    unsigned char world_arena[B3D_RT_WORLD_ARENA_CAP];
    SM3D_Context scene;
    int main_scene_id;
    Blank3DRuntimeProvider provider;
    Blank3DRuntimeInstance instances[B3D_RT_MAX_INSTANCES];

    Blank3DRuntimeIdentity identity_store[ECS_MAX_ENTITIES];
    Blank3DThingRef thing_store[ECS_MAX_ENTITIES];
    Blank3DObjectRef object_store[ECS_MAX_ENTITIES];
    Blank3DWorldRef world_store[ECS_MAX_ENTITIES];
    Blank3DSceneRef scene_store[ECS_MAX_ENTITIES];
    Blank3DActorRef actor_store[ECS_MAX_ENTITIES];
    void *native_store[ECS_MAX_ENTITIES];
} Blank3DRuntimeSpine;

int blank3d_runtime_spine_init(Blank3DRuntimeSpine *spine,
                               const Blank3DRuntimeProvider *provider);
void blank3d_runtime_spine_reset(Blank3DRuntimeSpine *spine);
int blank3d_runtime_instance_create(Blank3DRuntimeSpine *spine,
                                    unsigned long subject_id,
                                    unsigned long object_key,
                                    void *native_object,
                                    TS89_Thing *out_thing);
int blank3d_runtime_instance_destroy(Blank3DRuntimeSpine *spine,
                                     TS89_Thing thing);
int blank3d_runtime_bind_actor(Blank3DRuntimeSpine *spine,
                               TS89_Thing thing, int actor_id);
int blank3d_runtime_sync(Blank3DRuntimeSpine *spine);
Blank3DRuntimeInstance *blank3d_runtime_find_thing(Blank3DRuntimeSpine *spine,
                                                   TS89_Thing thing);
Blank3DRuntimeInstance *blank3d_runtime_find_subject(Blank3DRuntimeSpine *spine,
                                                     unsigned long subject_id);
const Blank3DRuntimeInstance *blank3d_runtime_find_subject_const(
    const Blank3DRuntimeSpine *spine, unsigned long subject_id);
unsigned long blank3d_runtime_entity_key(ecs_entity entity);
ecs_entity blank3d_runtime_entity_from_key(unsigned long key);
int blank3d_runtime_count(const Blank3DRuntimeSpine *spine);

#ifdef __cplusplus
}
#endif

#endif
