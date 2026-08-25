#include "blank3d_runtime_spine.h"
#include <string.h>

static void b3d_rt_default_pose(Blank3DRuntimePose *pose)
{
    if (!pose) return;
    memset(pose, 0, sizeof(*pose));
    pose->half_x = 32768L;
    pose->half_y = 32768L;
    pose->half_z = 32768L;
}

static int b3d_rt_query_pose(Blank3DRuntimeSpine *spine,
                             Blank3DRuntimeInstance *instance,
                             Blank3DRuntimePose *pose)
{
    b3d_rt_default_pose(pose);
    if (!spine || !instance || !pose) return 0;
    if (!spine->provider.query_pose) return 1;
    return spine->provider.query_pose(spine->provider.user,
                                      instance->subject_id,
                                      instance->native_object, pose);
}

unsigned long blank3d_runtime_entity_key(ecs_entity entity)
{
    unsigned long raw;
    if (ecs_entity_is_null(entity)) return 0UL;
    raw = ((unsigned long)entity.generation << 16) |
          (unsigned long)entity.index;
    return raw + 1UL;
}

ecs_entity blank3d_runtime_entity_from_key(unsigned long key)
{
    ecs_entity entity;
    unsigned long raw;
    if (key == 0UL) return ecs_entity_null();
    raw = key - 1UL;
    entity.index = (ecs_u16)(raw & 0xFFFFUL);
    entity.generation = (ecs_u16)((raw >> 16) & 0xFFFFUL);
    return entity;
}

static int b3d_rt_register_component(ecs_world *world, int id,
                                     const char *name, unsigned int size,
                                     void *storage, unsigned int flags)
{
    ecs_component_desc desc;
    memset(&desc, 0, sizeof(desc));
    desc.name = name;
    desc.size = size;
    desc.stride = size;
    desc.storage = storage;
    desc.flags = flags;
    return ecs_register_component(world, id, &desc, 0);
}

static int b3d_rt_ecs_init(Blank3DRuntimeSpine *spine)
{
    if (!ecs_world_init(&spine->ecs, B3D_RT_MAX_INSTANCES)) return 0;
    if (!b3d_rt_register_component(&spine->ecs, B3D_RT_COMPONENT_IDENTITY,
        "runtime_identity", sizeof(Blank3DRuntimeIdentity), spine->identity_store, 0U)) return 0;
    if (!b3d_rt_register_component(&spine->ecs, B3D_RT_COMPONENT_THING_REF,
        "thing_ref", sizeof(Blank3DThingRef), spine->thing_store, 0U)) return 0;
    if (!b3d_rt_register_component(&spine->ecs, B3D_RT_COMPONENT_OBJECT_REF,
        "object_ref", sizeof(Blank3DObjectRef), spine->object_store, 0U)) return 0;
    if (!b3d_rt_register_component(&spine->ecs, B3D_RT_COMPONENT_WORLD_REF,
        "world_ref", sizeof(Blank3DWorldRef), spine->world_store, 0U)) return 0;
    if (!b3d_rt_register_component(&spine->ecs, B3D_RT_COMPONENT_SCENE_REF,
        "scene_ref", sizeof(Blank3DSceneRef), spine->scene_store, 0U)) return 0;
    if (!b3d_rt_register_component(&spine->ecs, B3D_RT_COMPONENT_ACTOR_REF,
        "actor_ref", sizeof(Blank3DActorRef), spine->actor_store, 0U)) return 0;
    if (!b3d_rt_register_component(&spine->ecs, B3D_RT_COMPONENT_NATIVE_PTR,
        "native_ptr", sizeof(void *), spine->native_store, ECS_COMPONENT_FLAG_POINTER)) return 0;
    return 1;
}

static int b3d_rt_world_init(Blank3DRuntimeSpine *spine)
{
    w3d_world_config cfg;
    w3d_u32 need;
    int x;
    int z;
    w3d_world_default_config(&cfg);
    cfg.max_cells = 32U;
    cfg.max_entities = (w3d_u16)B3D_RT_MAX_INSTANCES;
    cfg.max_stream_sources = 2U;
    cfg.max_events = 64U;
    cfg.max_cell_hash_entries = 67U;
    cfg.max_portals = 16U;
    need = w3d_world_memory_required(&cfg);
    if (need == 0UL || need > B3D_RT_WORLD_ARENA_CAP) return 0;
    if (w3d_world_init(&spine->world, &cfg, spine->world_arena,
                       (w3d_u32)sizeof(spine->world_arena), 0, spine) != W3D_OK)
        return 0;
    for (z = -2; z <= 2; ++z)
        for (x = -2; x <= 2; ++x)
            if (w3d_world_define_cell(&spine->world,
                (w3d_i16)x, 0, (w3d_i16)z,
                W3D_CELL_FLAG_EXTERIOR | W3D_CELL_FLAG_ALWAYS_LOADED,
                W3D_LAYER_ALL, 0UL) < 0) return 0;
    return 1;
}

static int b3d_rt_scene_init(Blank3DRuntimeSpine *spine)
{
    SM3D_Config cfg;
    int scene_id;
    sm3d_config_defaults(&cfg);
    sm3d_init(&spine->scene, &cfg);
    if (sm3d_scene_create(&spine->scene, sm3d_hash_cstr("blank3d_runtime"),
            SM3D_SCENE_FLAG_LOADED | SM3D_SCENE_FLAG_ACTIVE |
            SM3D_SCENE_FLAG_PERSISTENT, &scene_id) != SM3D_OK)
        return 0;
    spine->main_scene_id = scene_id;
    return 1;
}

int blank3d_runtime_spine_init(Blank3DRuntimeSpine *spine,
                               const Blank3DRuntimeProvider *provider)
{
    if (!spine) return 0;
    memset(spine, 0, sizeof(*spine));
    if (provider) spine->provider = *provider;
    ts89_init(&spine->things, B3D_RT_MAX_INSTANCES);
    if (!ts89_is_initialized(&spine->things)) return 0;
    if (!b3d_rt_ecs_init(spine)) return 0;
    if (!b3d_rt_world_init(spine)) return 0;
    if (!b3d_rt_scene_init(spine)) return 0;
    spine->initialized = 1;
    return 1;
}

void blank3d_runtime_spine_reset(Blank3DRuntimeSpine *spine)
{
    Blank3DRuntimeProvider provider;
    if (!spine) return;
    provider = spine->provider;
    (void)blank3d_runtime_spine_init(spine, &provider);
}

static Blank3DRuntimeInstance *b3d_rt_alloc_record(Blank3DRuntimeSpine *spine)
{
    int i;
    for (i = 0; i < B3D_RT_MAX_INSTANCES; ++i)
        if (!spine->instances[i].used) return &spine->instances[i];
    return 0;
}

Blank3DRuntimeInstance *blank3d_runtime_find_thing(Blank3DRuntimeSpine *spine,
                                                   TS89_Thing thing)
{
    int i;
    if (!spine) return 0;
    for (i = 0; i < B3D_RT_MAX_INSTANCES; ++i)
        if (spine->instances[i].used && spine->instances[i].thing == thing)
            return &spine->instances[i];
    return 0;
}

Blank3DRuntimeInstance *blank3d_runtime_find_subject(Blank3DRuntimeSpine *spine,
                                                     unsigned long subject_id)
{
    int i;
    if (!spine) return 0;
    for (i = 0; i < B3D_RT_MAX_INSTANCES; ++i)
        if (spine->instances[i].used && spine->instances[i].subject_id == subject_id)
            return &spine->instances[i];
    return 0;
}

const Blank3DRuntimeInstance *blank3d_runtime_find_subject_const(
    const Blank3DRuntimeSpine *spine, unsigned long subject_id)
{
    int i;
    if (!spine) return 0;
    for (i = 0; i < B3D_RT_MAX_INSTANCES; ++i)
        if (spine->instances[i].used && spine->instances[i].subject_id == subject_id)
            return &spine->instances[i];
    return 0;
}

int blank3d_runtime_instance_create(Blank3DRuntimeSpine *spine,
                                    unsigned long subject_id,
                                    unsigned long object_key,
                                    void *native_object,
                                    TS89_Thing *out_thing)
{
    Blank3DRuntimeInstance *record;
    Blank3DRuntimePose pose;
    Blank3DRuntimeIdentity identity;
    Blank3DThingRef thing_ref;
    Blank3DObjectRef object_ref;
    Blank3DWorldRef world_ref;
    Blank3DSceneRef scene_ref;
    TS89_Thing thing;
    ecs_entity entity;
    w3d_v3 pos;
    w3d_aabb bounds;
    w3d_handle proxy;
    SM3D_Handle node;
    SM3D_Transform transform;
    if (!spine || !spine->initialized || subject_id == 0UL) return 0;
    if (blank3d_runtime_find_subject(spine, subject_id)) return 0;
    record = b3d_rt_alloc_record(spine);
    if (!record) return 0;
    memset(record, 0, sizeof(*record));
    thing = ts89_reserve(&spine->things);
    if (thing == TS89_THING_INVALID) return 0;
    entity = ecs_entity_create(&spine->ecs);
    if (ecs_entity_is_null(entity)) { (void)ts89_destroy(&spine->things, thing); return 0; }

    record->used = 1;
    record->subject_id = subject_id;
    record->object_key = object_key;
    record->native_object = native_object;
    record->thing = thing;
    record->entity = entity;
    record->world_proxy = W3D_INVALID_HANDLE;
    record->scene_node = sm3d_node_invalid();

    if (!b3d_rt_query_pose(spine, record, &pose)) b3d_rt_default_pose(&pose);
    pos = w3d_v3_make((w3d_fp)pose.px, (w3d_fp)pose.py, (w3d_fp)pose.pz);
    bounds = w3d_aabb_make(w3d_v3_make((w3d_fp)-pose.half_x, (w3d_fp)-pose.half_y, (w3d_fp)-pose.half_z),
                           w3d_v3_make((w3d_fp)pose.half_x, (w3d_fp)pose.half_y, (w3d_fp)pose.half_z));
    proxy = w3d_entity_spawn(&spine->world, pos, bounds,
                             W3D_ENTITY_QUERYABLE, W3D_LAYER_ALL, 0UL,
                             (w3d_u32)object_key, (w3d_u32)thing);
    if (proxy == W3D_INVALID_HANDLE) goto fail;
    record->world_proxy = proxy;

    if (sm3d_node_create(&spine->scene, spine->main_scene_id,
            sm3d_node_invalid(), SM3D_NODE_TYPE_EMPTY,
            (sm3d_u32)object_key, &node) != SM3D_OK) goto fail;
    record->scene_node = node;
    transform = sm3d_transform_identity();
    transform.pos = sm3d_vec3((sm3d_fx)pose.px, (sm3d_fx)pose.py, (sm3d_fx)pose.pz);
    transform.rot = sm3d_vec3((sm3d_fx)pose.rx, (sm3d_fx)pose.ry, (sm3d_fx)pose.rz);
    if (sm3d_node_set_local_transform(&spine->scene, node, &transform) != SM3D_OK) goto fail;
    (void)sm3d_node_set_user_ids(&spine->scene, node,
                                 (sm3d_u32)object_key, 0UL, (sm3d_u32)thing);

    identity.subject_id = subject_id;
    thing_ref.thing = thing;
    object_ref.object_key = object_key;
    world_ref.proxy = proxy;
    scene_ref.node = node;
    scene_ref.scene_id = spine->main_scene_id;
    if (!ecs_add_component(&spine->ecs, entity, B3D_RT_COMPONENT_IDENTITY, &identity)) goto fail;
    if (!ecs_add_component(&spine->ecs, entity, B3D_RT_COMPONENT_THING_REF, &thing_ref)) goto fail;
    if (!ecs_add_component(&spine->ecs, entity, B3D_RT_COMPONENT_OBJECT_REF, &object_ref)) goto fail;
    if (!ecs_add_component(&spine->ecs, entity, B3D_RT_COMPONENT_WORLD_REF, &world_ref)) goto fail;
    if (!ecs_add_component(&spine->ecs, entity, B3D_RT_COMPONENT_SCENE_REF, &scene_ref)) goto fail;
    if (!ecs_bind_ptr(&spine->ecs, entity, B3D_RT_COMPONENT_NATIVE_PTR, native_object)) goto fail;
    if (!ts89_set_user(&spine->things, thing, blank3d_runtime_entity_key(entity))) goto fail;
    if (!ts89_publish(&spine->things, thing)) goto fail;
    (void)sm3d_update_transforms(&spine->scene);
    if (out_thing) *out_thing = thing;
    return 1;

fail:
    if (sm3d_handle_is_valid(record->scene_node))
        (void)sm3d_node_destroy(&spine->scene, record->scene_node);
    if (record->world_proxy != W3D_INVALID_HANDLE)
        (void)w3d_entity_remove(&spine->world, record->world_proxy);
    if (!ecs_entity_is_null(entity) && ecs_entity_valid(&spine->ecs, entity))
        ecs_entity_destroy(&spine->ecs, entity);
    (void)ts89_destroy(&spine->things, thing);
    memset(record, 0, sizeof(*record));
    return 0;
}

int blank3d_runtime_bind_actor(Blank3DRuntimeSpine *spine,
                               TS89_Thing thing, int actor_id)
{
    Blank3DRuntimeInstance *record;
    Blank3DActorRef actor_ref;
    if (!spine || actor_id <= 0) return 0;
    record = blank3d_runtime_find_thing(spine, thing);
    if (!record || !ts89_is_alive(&spine->things, thing)) return 0;
    actor_ref.actor_id = actor_id;
    if (ecs_has_component(&spine->ecs, record->entity, B3D_RT_COMPONENT_ACTOR_REF)) {
        if (!ecs_set_component(&spine->ecs, record->entity, B3D_RT_COMPONENT_ACTOR_REF, &actor_ref)) return 0;
    } else if (!ecs_add_component(&spine->ecs, record->entity, B3D_RT_COMPONENT_ACTOR_REF, &actor_ref)) return 0;
    record->actor_id = actor_id;
    return 1;
}

int blank3d_runtime_instance_destroy(Blank3DRuntimeSpine *spine,
                                     TS89_Thing thing)
{
    Blank3DRuntimeInstance *record;
    if (!spine) return 0;
    record = blank3d_runtime_find_thing(spine, thing);
    if (!record) return 0;
    if (sm3d_handle_is_valid(record->scene_node))
        (void)sm3d_node_destroy(&spine->scene, record->scene_node);
    if (record->world_proxy != W3D_INVALID_HANDLE)
        (void)w3d_entity_remove(&spine->world, record->world_proxy);
    if (ecs_entity_valid(&spine->ecs, record->entity))
        ecs_entity_destroy(&spine->ecs, record->entity);
    (void)ts89_destroy(&spine->things, thing);
    memset(record, 0, sizeof(*record));
    (void)sm3d_update_transforms(&spine->scene);
    return 1;
}

int blank3d_runtime_sync(Blank3DRuntimeSpine *spine)
{
    int i;
    int count;
    Blank3DRuntimePose pose;
    w3d_v3 pos;
    w3d_v3 rot;
    SM3D_Transform transform;
    if (!spine || !spine->initialized) return 0;
    count = 0;
    for (i = 0; i < B3D_RT_MAX_INSTANCES; ++i) {
        Blank3DRuntimeInstance *record;
        record = &spine->instances[i];
        if (!record->used || !ts89_is_alive(&spine->things, record->thing)) continue;
        if (!b3d_rt_query_pose(spine, record, &pose)) continue;
        pos = w3d_v3_make((w3d_fp)pose.px, (w3d_fp)pose.py, (w3d_fp)pose.pz);
        rot = w3d_v3_make((w3d_fp)pose.rx, (w3d_fp)pose.ry, (w3d_fp)pose.rz);
        (void)w3d_entity_set_pose(&spine->world, record->world_proxy, pos, rot);
        transform = sm3d_transform_identity();
        transform.pos = sm3d_vec3((sm3d_fx)pose.px, (sm3d_fx)pose.py, (sm3d_fx)pose.pz);
        transform.rot = sm3d_vec3((sm3d_fx)pose.rx, (sm3d_fx)pose.ry, (sm3d_fx)pose.rz);
        (void)sm3d_node_set_local_transform(&spine->scene, record->scene_node, &transform);
        ++count;
    }
    (void)sm3d_update_dirty_transforms(&spine->scene);
    return count;
}

int blank3d_runtime_count(const Blank3DRuntimeSpine *spine)
{
    int i;
    int count;
    if (!spine) return 0;
    count = 0;
    for (i = 0; i < B3D_RT_MAX_INSTANCES; ++i)
        if (spine->instances[i].used) ++count;
    return count;
}
