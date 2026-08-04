#include "ecs89.h"

#include <string.h>

static int ecs_component_id_valid(int component_id)
{
    if (component_id < 0) return 0;
    if (component_id >= ECS_MAX_COMPONENTS) return 0;
    if (component_id >= (int)(sizeof(ecs_mask) * 8u)) return 0;
    return 1;
}

static int ecs_index_valid(const ecs_world *world, int index)
{
    if (world == (const ecs_world*)0) return 0;
    if (index < 0) return 0;
    if (index >= world->max_entities) return 0;
    return 1;
}

static int ecs_component_is_tag(const ecs_world *world, int component_id)
{
    if (world == (const ecs_world*)0) return 0;
    if (!ecs_component_id_valid(component_id)) return 0;
    if (!world->registered[component_id]) return 0;
    return (world->components[component_id].flags & ECS_COMPONENT_FLAG_TAG) != 0u;
}

static int ecs_component_is_pointer(const ecs_world *world, int component_id)
{
    if (world == (const ecs_world*)0) return 0;
    if (!ecs_component_id_valid(component_id)) return 0;
    if (!world->registered[component_id]) return 0;
    return (world->components[component_id].flags & ECS_COMPONENT_FLAG_POINTER) != 0u;
}

static void *ecs_component_ptr_raw(ecs_world *world, int component_id, int entity_index)
{
    char *base;
    unsigned int stride;

    if (world == (ecs_world*)0) return (void*)0;
    if (!ecs_component_id_valid(component_id)) return (void*)0;
    if (!world->registered[component_id]) return (void*)0;
    if (ecs_component_is_tag(world, component_id)) return (void*)0;
    if (!ecs_index_valid(world, entity_index)) return (void*)0;

    base = (char*)world->components[component_id].storage;
    stride = world->components[component_id].stride;
    return (void*)(base + ((unsigned long)stride * (unsigned long)entity_index));
}

static const void *ecs_component_ptr_raw_const(const ecs_world *world, int component_id, int entity_index)
{
    const char *base;
    unsigned int stride;

    if (world == (const ecs_world*)0) return (const void*)0;
    if (!ecs_component_id_valid(component_id)) return (const void*)0;
    if (!world->registered[component_id]) return (const void*)0;
    if (ecs_component_is_tag(world, component_id)) return (const void*)0;
    if (!ecs_index_valid(world, entity_index)) return (const void*)0;

    base = (const char*)world->components[component_id].storage;
    stride = world->components[component_id].stride;
    return (const void*)(base + ((unsigned long)stride * (unsigned long)entity_index));
}

static void ecs_clear_component_bytes(ecs_world *world, int component_id, int entity_index)
{
    void *dst;
    unsigned int size;

    if (world == (ecs_world*)0) return;
    if (ecs_component_is_tag(world, component_id)) return;

    dst = ecs_component_ptr_raw(world, component_id, entity_index);
    if (dst == (void*)0) return;

    size = world->components[component_id].size;
    memset(dst, 0, (size_t)size);
}

ecs_entity ecs_entity_null(void)
{
    ecs_entity entity;

    entity.index = ECS_ENTITY_NULL_INDEX;
    entity.generation = 0;
    return entity;
}

int ecs_entity_is_null(ecs_entity entity)
{
    return entity.index == ECS_ENTITY_NULL_INDEX;
}

int ecs_entity_valid(const ecs_world *world, ecs_entity entity)
{
    if (world == (const ecs_world*)0) return 0;
    if (ecs_entity_is_null(entity)) return 0;
    if (!ecs_index_valid(world, (int)entity.index)) return 0;
    if (!world->alive[entity.index]) return 0;
    if (world->generations[entity.index] != entity.generation) return 0;
    return 1;
}

int ecs_world_init(ecs_world *world, int max_entities)
{
    int i;

    if (world == (ecs_world*)0) return 0;
    if (max_entities <= 0 || max_entities > ECS_MAX_ENTITIES) return 0;

    memset(world, 0, sizeof(*world));

    world->max_entities = max_entities;
    world->component_count = 0;
    world->component_highwater = 0;
    world->free_top = max_entities;

    for (i = 0; i < max_entities; ++i)
    {
        world->generations[i] = 1;
        world->alive[i] = 0;
        world->masks[i] = 0;
        world->free_stack[i] = (ecs_u16)(max_entities - 1 - i);
    }

    return 1;
}

void ecs_world_reset(ecs_world *world)
{
    int i;
    ecs_entity entity;

    if (world == (ecs_world*)0) return;

    for (i = 0; i < world->max_entities; ++i)
    {
        if (!world->alive[i]) continue;

        entity.index = (ecs_u16)i;
        entity.generation = world->generations[i];
        ecs_entity_destroy(world, entity);
    }

    for (i = 0; i < world->component_highwater; ++i)
    {
        if (!world->registered[i]) continue;
        if (ecs_component_is_tag(world, i)) continue;

        memset(world->components[i].storage,
               0,
               (size_t)((unsigned long)world->components[i].stride *
                        (unsigned long)world->max_entities));
    }
}

void ecs_world_set_user_data(ecs_world *world, void *user_data)
{
    if (world == (ecs_world*)0) return;
    world->user_data = user_data;
}

void *ecs_world_get_user_data(const ecs_world *world)
{
    if (world == (const ecs_world*)0) return (void*)0;
    return world->user_data;
}

void ecs_world_set_hooks(ecs_world *world, const ecs_world_hooks *hooks, void *hooks_user_data)
{
    if (world == (ecs_world*)0) return;

    if (hooks != (const ecs_world_hooks*)0)
    {
        world->world_hooks = *hooks;
    }
    else
    {
        memset(&world->world_hooks, 0, sizeof(world->world_hooks));
    }

    world->world_hooks_user_data = hooks_user_data;
}

int ecs_world_set_service(ecs_world *world, int slot, void *service)
{
    if (world == (ecs_world*)0) return 0;
    if (slot < 0 || slot >= ECS_MAX_SERVICES) return 0;

    world->services[slot] = service;
    return 1;
}

void *ecs_world_get_service(const ecs_world *world, int slot)
{
    if (world == (const ecs_world*)0) return (void*)0;
    if (slot < 0 || slot >= ECS_MAX_SERVICES) return (void*)0;
    return world->services[slot];
}

int ecs_register_component(ecs_world *world,
                           int component_id,
                           const ecs_component_desc *desc,
                           const ecs_component_hooks *hooks)
{
    ecs_component_desc copy;
    unsigned int stride;

    if (world == (ecs_world*)0) return 0;
    if (desc == (const ecs_component_desc*)0) return 0;
    if (!ecs_component_id_valid(component_id)) return 0;
    if (world->registered[component_id]) return 0;

    copy = *desc;

    if ((copy.flags & ECS_COMPONENT_FLAG_TAG) != 0u)
    {
        copy.size = 0u;
        copy.stride = 0u;
        copy.storage = (void*)0;
    }
    else
    {
        if (copy.size == 0u) return 0;
        if (copy.storage == (void*)0) return 0;

        stride = copy.stride;
        if (stride == 0u) stride = copy.size;
        if (stride < copy.size) return 0;

        copy.stride = stride;

        if ((copy.flags & ECS_COMPONENT_FLAG_POINTER) != 0u)
        {
            if (copy.size < sizeof(void*)) return 0;
        }

        memset(copy.storage,
               0,
               (size_t)((unsigned long)copy.stride * (unsigned long)world->max_entities));
    }

    world->components[component_id] = copy;

    if (hooks != (const ecs_component_hooks*)0)
    {
        world->hooks[component_id] = *hooks;
    }
    else
    {
        memset(&world->hooks[component_id], 0, sizeof(world->hooks[component_id]));
    }

    world->registered[component_id] = 1;
    world->component_count += 1;

    if (component_id + 1 > world->component_highwater)
    {
        world->component_highwater = component_id + 1;
    }

    return 1;
}

int ecs_component_registered(const ecs_world *world, int component_id)
{
    if (world == (const ecs_world*)0) return 0;
    if (!ecs_component_id_valid(component_id)) return 0;
    return world->registered[component_id] != 0;
}

const char *ecs_component_name(const ecs_world *world, int component_id)
{
    if (!ecs_component_registered(world, component_id)) return (const char*)0;
    return world->components[component_id].name;
}

unsigned int ecs_component_size(const ecs_world *world, int component_id)
{
    if (!ecs_component_registered(world, component_id)) return 0u;
    return world->components[component_id].size;
}

unsigned int ecs_component_stride(const ecs_world *world, int component_id)
{
    if (!ecs_component_registered(world, component_id)) return 0u;
    return world->components[component_id].stride;
}

unsigned int ecs_component_flags(const ecs_world *world, int component_id)
{
    if (!ecs_component_registered(world, component_id)) return 0u;
    return world->components[component_id].flags;
}

void *ecs_component_user_data(const ecs_world *world, int component_id)
{
    if (!ecs_component_registered(world, component_id)) return (void*)0;
    return world->components[component_id].user_data;
}

ecs_mask ecs_component_bit(int component_id)
{
    if (!ecs_component_id_valid(component_id)) return (ecs_mask)0;
    return ((ecs_mask)1ul << component_id);
}

ecs_entity ecs_entity_create(ecs_world *world)
{
    ecs_entity entity;
    ecs_u16 slot;

    entity = ecs_entity_null();

    if (world == (ecs_world*)0) return entity;
    if (world->free_top <= 0) return entity;

    world->free_top -= 1;
    slot = world->free_stack[world->free_top];

    world->alive[slot] = 1;
    world->masks[slot] = 0;

    entity.index = slot;
    entity.generation = world->generations[slot];

    if (world->world_hooks.on_entity_create != 0)
    {
        world->world_hooks.on_entity_create(world, entity, world->world_hooks_user_data);
    }

    return entity;
}

void ecs_entity_destroy(ecs_world *world, ecs_entity entity)
{
    int i;

    if (!ecs_entity_valid(world, entity)) return;

    for (i = 0; i < world->component_highwater; ++i)
    {
        if (!world->registered[i]) continue;
        if ((world->masks[entity.index] & ecs_component_bit(i)) == 0) continue;
        ecs_remove_component(world, entity, i);
    }

    world->alive[entity.index] = 0;
    world->masks[entity.index] = 0;
    world->generations[entity.index] = (ecs_u16)(world->generations[entity.index] + 1u);
    if (world->generations[entity.index] == 0u) world->generations[entity.index] = 1u;

    world->free_stack[world->free_top] = entity.index;
    world->free_top += 1;

    if (world->world_hooks.on_entity_destroy != 0)
    {
        world->world_hooks.on_entity_destroy(world, entity, world->world_hooks_user_data);
    }
}

ecs_mask ecs_entity_mask(const ecs_world *world, ecs_entity entity)
{
    if (!ecs_entity_valid(world, entity)) return (ecs_mask)0;
    return world->masks[entity.index];
}

int ecs_has_component(const ecs_world *world, ecs_entity entity, int component_id)
{
    ecs_mask bit;

    if (!ecs_entity_valid(world, entity)) return 0;
    if (!ecs_component_registered(world, component_id)) return 0;

    bit = ecs_component_bit(component_id);
    return (world->masks[entity.index] & bit) != 0;
}

int ecs_add_component(ecs_world *world, ecs_entity entity, int component_id, const void *src)
{
    ecs_mask bit;
    void *dst;

    if (!ecs_entity_valid(world, entity)) return 0;
    if (!ecs_component_registered(world, component_id)) return 0;
    if (ecs_has_component(world, entity, component_id)) return 0;

    bit = ecs_component_bit(component_id);

    if (ecs_component_is_tag(world, component_id))
    {
        world->masks[entity.index] |= bit;

        if (world->hooks[component_id].on_add != 0)
        {
            world->hooks[component_id].on_add(world,
                                              entity,
                                              component_id,
                                              (void*)0,
                                              world->components[component_id].user_data);
        }

        return 1;
    }

    dst = ecs_component_ptr_raw(world, component_id, entity.index);
    if (dst == (void*)0) return 0;

    if (src != (const void*)0)
    {
        memcpy(dst, src, (size_t)world->components[component_id].size);
    }
    else
    {
        memset(dst, 0, (size_t)world->components[component_id].size);
    }

    world->masks[entity.index] |= bit;

    if (world->hooks[component_id].on_add != 0)
    {
        world->hooks[component_id].on_add(world,
                                          entity,
                                          component_id,
                                          dst,
                                          world->components[component_id].user_data);
    }

    return 1;
}

int ecs_set_component(ecs_world *world, ecs_entity entity, int component_id, const void *src)
{
    void *dst;

    if (!ecs_entity_valid(world, entity)) return 0;
    if (!ecs_component_registered(world, component_id)) return 0;

    if (!ecs_has_component(world, entity, component_id))
    {
        return ecs_add_component(world, entity, component_id, src);
    }

    if (ecs_component_is_tag(world, component_id))
    {
        if (world->hooks[component_id].on_set != 0)
        {
            world->hooks[component_id].on_set(world,
                                              entity,
                                              component_id,
                                              (void*)0,
                                              world->components[component_id].user_data);
        }
        return 1;
    }

    dst = ecs_component_ptr_raw(world, component_id, entity.index);
    if (dst == (void*)0) return 0;

    if (src != (const void*)0)
    {
        memcpy(dst, src, (size_t)world->components[component_id].size);
    }
    else
    {
        memset(dst, 0, (size_t)world->components[component_id].size);
    }

    if (world->hooks[component_id].on_set != 0)
    {
        world->hooks[component_id].on_set(world,
                                          entity,
                                          component_id,
                                          dst,
                                          world->components[component_id].user_data);
    }

    return 1;
}

void *ecs_assign_component(ecs_world *world, ecs_entity entity, int component_id)
{
    ecs_mask bit;
    void *dst;

    if (!ecs_entity_valid(world, entity)) return (void*)0;
    if (!ecs_component_registered(world, component_id)) return (void*)0;
    if (ecs_component_is_tag(world, component_id)) return (void*)0;

    dst = ecs_component_ptr_raw(world, component_id, entity.index);
    if (dst == (void*)0) return (void*)0;

    bit = ecs_component_bit(component_id);

    if ((world->masks[entity.index] & bit) == 0)
    {
        memset(dst, 0, (size_t)world->components[component_id].size);
        world->masks[entity.index] |= bit;

        if (world->hooks[component_id].on_add != 0)
        {
            world->hooks[component_id].on_add(world,
                                              entity,
                                              component_id,
                                              dst,
                                              world->components[component_id].user_data);
        }
    }

    return dst;
}

int ecs_remove_component(ecs_world *world, ecs_entity entity, int component_id)
{
    ecs_mask bit;
    void *dst;

    if (!ecs_entity_valid(world, entity)) return 0;
    if (!ecs_component_registered(world, component_id)) return 0;

    bit = ecs_component_bit(component_id);
    if ((world->masks[entity.index] & bit) == 0) return 0;

    if (ecs_component_is_tag(world, component_id))
    {
        if (world->hooks[component_id].on_remove != 0)
        {
            world->hooks[component_id].on_remove(world,
                                                 entity,
                                                 component_id,
                                                 (void*)0,
                                                 world->components[component_id].user_data);
        }

        world->masks[entity.index] &= ~bit;
        return 1;
    }

    dst = ecs_component_ptr_raw(world, component_id, entity.index);
    if (dst == (void*)0) return 0;

    if (world->hooks[component_id].on_remove != 0)
    {
        world->hooks[component_id].on_remove(world,
                                             entity,
                                             component_id,
                                             dst,
                                             world->components[component_id].user_data);
    }

    ecs_clear_component_bytes(world, component_id, entity.index);
    world->masks[entity.index] &= ~bit;
    return 1;
}

void *ecs_get_component(ecs_world *world, ecs_entity entity, int component_id)
{
    if (!ecs_has_component(world, entity, component_id)) return (void*)0;
    if (ecs_component_is_tag(world, component_id)) return (void*)0;
    return ecs_component_ptr_raw(world, component_id, entity.index);
}

const void *ecs_get_component_const(const ecs_world *world, ecs_entity entity, int component_id)
{
    if (!ecs_has_component(world, entity, component_id)) return (const void*)0;
    if (ecs_component_is_tag(world, component_id)) return (const void*)0;
    return ecs_component_ptr_raw_const(world, component_id, entity.index);
}

int ecs_add_tag(ecs_world *world, ecs_entity entity, int component_id)
{
    if (!ecs_component_registered(world, component_id)) return 0;
    if (!ecs_component_is_tag(world, component_id)) return 0;
    return ecs_add_component(world, entity, component_id, (const void*)0);
}

int ecs_bind_ptr(ecs_world *world, ecs_entity entity, int component_id, void *ptr)
{
    void *dst;
    ecs_mask bit;
    int was_present;

    if (!ecs_entity_valid(world, entity)) return 0;
    if (!ecs_component_registered(world, component_id)) return 0;
    if (!ecs_component_is_pointer(world, component_id)) return 0;
    if (ecs_component_is_tag(world, component_id)) return 0;

    dst = ecs_component_ptr_raw(world, component_id, entity.index);
    if (dst == (void*)0) return 0;

    bit = ecs_component_bit(component_id);
    was_present = (world->masks[entity.index] & bit) != 0;

    memset(dst, 0, (size_t)world->components[component_id].size);
    memcpy(dst, &ptr, sizeof(void*));

    if (!was_present)
    {
        world->masks[entity.index] |= bit;

        if (world->hooks[component_id].on_add != 0)
        {
            world->hooks[component_id].on_add(world,
                                              entity,
                                              component_id,
                                              dst,
                                              world->components[component_id].user_data);
        }
    }
    else
    {
        if (world->hooks[component_id].on_set != 0)
        {
            world->hooks[component_id].on_set(world,
                                              entity,
                                              component_id,
                                              dst,
                                              world->components[component_id].user_data);
        }
    }

    return 1;
}

void *ecs_get_bound_ptr(ecs_world *world, ecs_entity entity, int component_id)
{
    void *ptr;
    const void *src;

    ptr = (void*)0;

    if (!ecs_has_component(world, entity, component_id)) return (void*)0;
    if (!ecs_component_is_pointer(world, component_id)) return (void*)0;

    src = ecs_component_ptr_raw_const(world, component_id, entity.index);
    if (src == (const void*)0) return (void*)0;

    memcpy(&ptr, src, sizeof(void*));
    return ptr;
}

const void *ecs_get_bound_ptr_const(const ecs_world *world, ecs_entity entity, int component_id)
{
    void *ptr;
    const void *src;

    ptr = (void*)0;

    if (!ecs_has_component(world, entity, component_id)) return (const void*)0;
    if (!ecs_component_is_pointer(world, component_id)) return (const void*)0;

    src = ecs_component_ptr_raw_const(world, component_id, entity.index);
    if (src == (const void*)0) return (const void*)0;

    memcpy(&ptr, src, sizeof(void*));
    return (const void*)ptr;
}

void ecs_view_init(ecs_view *view, ecs_world *world, ecs_mask require_all, ecs_mask exclude_any)
{
    if (view == (ecs_view*)0) return;

    view->world = world;
    view->require_all = require_all;
    view->exclude_any = exclude_any;
    view->cursor = 0;
}

ecs_entity ecs_view_next(ecs_view *view)
{
    ecs_entity entity;
    ecs_world *world;
    int i;
    ecs_mask mask;

    entity = ecs_entity_null();

    if (view == (ecs_view*)0) return entity;

    world = view->world;
    if (world == (ecs_world*)0) return entity;

    for (i = view->cursor; i < world->max_entities; ++i)
    {
        if (!world->alive[i]) continue;

        mask = world->masks[i];

        if ((mask & view->require_all) != view->require_all) continue;
        if ((mask & view->exclude_any) != 0) continue;

        view->cursor = i + 1;
        entity.index = (ecs_u16)i;
        entity.generation = world->generations[i];
        return entity;
    }

    return entity;
}

int ecs_count_alive(const ecs_world *world)
{
    int i;
    int count;

    if (world == (const ecs_world*)0) return 0;

    count = 0;
    for (i = 0; i < world->max_entities; ++i)
    {
        if (world->alive[i]) count += 1;
    }

    return count;
}

int ecs_count_matching(const ecs_world *world, ecs_mask require_all, ecs_mask exclude_any)
{
    int i;
    int count;
    ecs_mask mask;

    if (world == (const ecs_world*)0) return 0;

    count = 0;
    for (i = 0; i < world->max_entities; ++i)
    {
        if (!world->alive[i]) continue;

        mask = world->masks[i];
        if ((mask & require_all) != require_all) continue;
        if ((mask & exclude_any) != 0) continue;

        count += 1;
    }

    return count;
}
