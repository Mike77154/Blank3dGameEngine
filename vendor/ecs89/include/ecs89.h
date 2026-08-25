#ifndef ECS89_H
#define ECS89_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ECS_MAX_ENTITIES
#define ECS_MAX_ENTITIES 1024
#endif

#ifndef ECS_MAX_COMPONENTS
#define ECS_MAX_COMPONENTS 32
#endif

#ifndef ECS_MAX_SERVICES
#define ECS_MAX_SERVICES 16
#endif

typedef unsigned char ecs_u8;
typedef unsigned short ecs_u16;
typedef unsigned long ecs_mask;

typedef struct ecs_entity
{
    ecs_u16 index;
    ecs_u16 generation;
} ecs_entity;

struct ecs_world;
struct ecs_view;

typedef struct ecs_world ecs_world;
typedef struct ecs_view ecs_view;

enum
{
    ECS_COMPONENT_FLAG_NONE    = 0u,
    ECS_COMPONENT_FLAG_TAG     = 1u << 0,
    ECS_COMPONENT_FLAG_POINTER = 1u << 1
};

typedef struct ecs_component_desc
{
    const char *name;
    unsigned int size;
    unsigned int stride;
    void *storage;
    unsigned int flags;
    void *user_data;
} ecs_component_desc;

typedef struct ecs_component_hooks
{
    void (*on_add)(ecs_world *world, ecs_entity entity, int component_id, void *component_ptr, void *user_data);
    void (*on_set)(ecs_world *world, ecs_entity entity, int component_id, void *component_ptr, void *user_data);
    void (*on_remove)(ecs_world *world, ecs_entity entity, int component_id, void *component_ptr, void *user_data);
} ecs_component_hooks;

typedef struct ecs_world_hooks
{
    void (*on_entity_create)(ecs_world *world, ecs_entity entity, void *user_data);
    void (*on_entity_destroy)(ecs_world *world, ecs_entity entity, void *user_data);
} ecs_world_hooks;

struct ecs_world
{
    int max_entities;
    int component_count;
    int component_highwater;

    ecs_u16 generations[ECS_MAX_ENTITIES];
    ecs_u8 alive[ECS_MAX_ENTITIES];
    ecs_mask masks[ECS_MAX_ENTITIES];

    ecs_u16 free_stack[ECS_MAX_ENTITIES];
    int free_top;

    ecs_component_desc components[ECS_MAX_COMPONENTS];
    ecs_component_hooks hooks[ECS_MAX_COMPONENTS];
    ecs_u8 registered[ECS_MAX_COMPONENTS];

    void *user_data;
    void *services[ECS_MAX_SERVICES];

    ecs_world_hooks world_hooks;
    void *world_hooks_user_data;
};

struct ecs_view
{
    ecs_world *world;
    ecs_mask require_all;
    ecs_mask exclude_any;
    int cursor;
};

#define ECS_ENTITY_NULL_INDEX ((ecs_u16)0xFFFFu)
#define ECS_BIT(component_id) ((ecs_mask)1ul << (component_id))

#define ECS_GET_AS(type, world_ptr, entity_value, component_id) \
    ((type*)ecs_get_component((world_ptr), (entity_value), (component_id)))

#define ECS_GET_AS_CONST(type, world_ptr, entity_value, component_id) \
    ((const type*)ecs_get_component_const((world_ptr), (entity_value), (component_id)))

#define ECS_ASSIGN_AS(type, world_ptr, entity_value, component_id) \
    ((type*)ecs_assign_component((world_ptr), (entity_value), (component_id)))

ecs_entity ecs_entity_null(void);
int ecs_entity_is_null(ecs_entity entity);
int ecs_entity_valid(const ecs_world *world, ecs_entity entity);

int ecs_world_init(ecs_world *world, int max_entities);
void ecs_world_reset(ecs_world *world);
void ecs_world_set_user_data(ecs_world *world, void *user_data);
void *ecs_world_get_user_data(const ecs_world *world);
void ecs_world_set_hooks(ecs_world *world, const ecs_world_hooks *hooks, void *hooks_user_data);

int ecs_world_set_service(ecs_world *world, int slot, void *service);
void *ecs_world_get_service(const ecs_world *world, int slot);

int ecs_register_component(ecs_world *world,
                           int component_id,
                           const ecs_component_desc *desc,
                           const ecs_component_hooks *hooks);

int ecs_component_registered(const ecs_world *world, int component_id);
const char *ecs_component_name(const ecs_world *world, int component_id);
unsigned int ecs_component_size(const ecs_world *world, int component_id);
unsigned int ecs_component_stride(const ecs_world *world, int component_id);
unsigned int ecs_component_flags(const ecs_world *world, int component_id);
void *ecs_component_user_data(const ecs_world *world, int component_id);
ecs_mask ecs_component_bit(int component_id);

ecs_entity ecs_entity_create(ecs_world *world);
void ecs_entity_destroy(ecs_world *world, ecs_entity entity);
ecs_mask ecs_entity_mask(const ecs_world *world, ecs_entity entity);

int ecs_has_component(const ecs_world *world, ecs_entity entity, int component_id);
int ecs_add_component(ecs_world *world, ecs_entity entity, int component_id, const void *src);
int ecs_set_component(ecs_world *world, ecs_entity entity, int component_id, const void *src);
void *ecs_assign_component(ecs_world *world, ecs_entity entity, int component_id);
int ecs_remove_component(ecs_world *world, ecs_entity entity, int component_id);
void *ecs_get_component(ecs_world *world, ecs_entity entity, int component_id);
const void *ecs_get_component_const(const ecs_world *world, ecs_entity entity, int component_id);

int ecs_add_tag(ecs_world *world, ecs_entity entity, int component_id);
int ecs_bind_ptr(ecs_world *world, ecs_entity entity, int component_id, void *ptr);
void *ecs_get_bound_ptr(ecs_world *world, ecs_entity entity, int component_id);
const void *ecs_get_bound_ptr_const(const ecs_world *world, ecs_entity entity, int component_id);

void ecs_view_init(ecs_view *view, ecs_world *world, ecs_mask require_all, ecs_mask exclude_any);
ecs_entity ecs_view_next(ecs_view *view);

int ecs_count_alive(const ecs_world *world);
int ecs_count_matching(const ecs_world *world, ecs_mask require_all, ecs_mask exclude_any);

#ifdef __cplusplus
}
#endif

#endif
