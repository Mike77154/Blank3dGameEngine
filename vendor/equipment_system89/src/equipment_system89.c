#include "equipment_system89.h"
#include <string.h>

static void eq89_copy(char *dst, unsigned int cap, const char *src)
{
    unsigned int i;
    if (dst == 0 || cap == 0U) return;
    if (src == 0) src = "";
    i = 0U;
    while (i + 1U < cap && src[i] != '\0') {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
}

static void eq89_set_status(EQ89_System *system, const char *text)
{
    if (system == 0) return;
    eq89_copy(system->status, sizeof(system->status), text);
}

int eq89_find_slot(const EQ89_System *system, int actor_id)
{
    int i;
    if (system == 0) return -1;
    for (i = 0; i < EQ89_MAX_ACTORS; ++i) {
        if (system->instances[i].used &&
            system->instances[i].actor_id == actor_id) return i;
    }
    return -1;
}

static int eq89_ensure_slot(EQ89_System *system, int actor_id, int actor_kind)
{
    int i;
    i = eq89_find_slot(system, actor_id);
    if (i >= 0) return i;
    for (i = 0; i < EQ89_MAX_ACTORS; ++i) {
        if (!system->instances[i].used) {
            memset(&system->instances[i], 0, sizeof(system->instances[i]));
            system->instances[i].used = 1;
            system->instances[i].actor_id = actor_id;
            system->instances[i].actor_kind = actor_kind;
            system->instances[i].object_id = system->object_base + i;
            system->instances[i].visible = 1;
            system->count += 1;
            return i;
        }
    }
    return -1;
}

void eq89_init(EQ89_System *system, int object_base,
               const EQ89_Provider *provider)
{
    if (system == 0) return;
    memset(system, 0, sizeof(*system));
    system->object_base = object_base;
    if (provider != 0) system->provider = *provider;
    system->initialized = provider != 0 &&
        provider->resolve_item != 0 && provider->attach != 0 &&
        provider->detach != 0 && provider->set_visible != 0;
    system->last_result = system->initialized ? 1 : 0;
    eq89_set_status(system, system->initialized
        ? "equipment_system89 initialized"
        : "equipment_system89 missing provider");
}

int eq89_define_socket(EQ89_System *system, int actor_id,
                       const char *socket_name, int socket_index,
                       const void *local_offset)
{
    if (system == 0 || !system->initialized || socket_name == 0 ||
        system->provider.define_socket == 0) return 0;
    return system->provider.define_socket(system->provider.user, actor_id,
        socket_name, socket_index, local_offset) ? 1 : 0;
}

EQ89_Instance *eq89_find(EQ89_System *system, int actor_id)
{
    int slot;
    slot = eq89_find_slot(system, actor_id);
    return slot >= 0 ? &system->instances[slot] : 0;
}

const EQ89_Instance *eq89_find_const(const EQ89_System *system, int actor_id)
{
    int slot;
    slot = eq89_find_slot(system, actor_id);
    return slot >= 0 ? &system->instances[slot] : 0;
}

int eq89_equip(EQ89_System *system, int actor_id, int actor_kind, int item_id)
{
    int slot;
    EQ89_ItemDesc desc;
    EQ89_Instance *instance;
    if (system == 0 || !system->initialized || item_id <= 0) return 0;
    memset(&desc, 0, sizeof(desc));
    if (!system->provider.resolve_item(system->provider.user, item_id, &desc)) {
        system->last_result = 0;
        eq89_set_status(system, "equipment item has no descriptor");
        return 0;
    }
    slot = eq89_ensure_slot(system, actor_id, actor_kind);
    if (slot < 0) {
        system->last_result = 0;
        eq89_set_status(system, "equipment capacity exhausted");
        return 0;
    }
    instance = &system->instances[slot];
    if (instance->attached && instance->item_id == item_id) return 1;
    if (instance->attached) {
        if (!system->provider.detach(system->provider.user,
                instance->actor_id, instance->object_id,
                instance->attachment_name)) return 0;
        instance->attached = 0;
    }
    instance->actor_kind = actor_kind;
    instance->item_id = item_id;
    instance->model_id = desc.model_id;
    eq89_copy(instance->socket_name, sizeof(instance->socket_name),
              desc.socket_name);
    eq89_copy(instance->attachment_name, sizeof(instance->attachment_name),
              desc.attachment_name);
    eq89_copy(instance->model_name, sizeof(instance->model_name),
              desc.model_name);
    if (system->provider.runtime_init != 0 &&
        !system->provider.runtime_init(system->provider.user, slot, &desc)) {
        instance->last_result = 0;
        system->last_result = 0;
        eq89_set_status(system, "equipment runtime init failed");
        return 0;
    }
    if (!system->provider.attach(system->provider.user, actor_id,
            instance->object_id, instance->attachment_name,
            instance->socket_name, desc.local_offset)) {
        instance->last_result = 0;
        system->last_result = 0;
        eq89_set_status(system, "equipment attach failed");
        return 0;
    }
    instance->attached = 1;
    instance->visible = 1;
    instance->last_result = 1;
    system->last_result = 1;
    eq89_set_status(system, "equipment item mounted");
    return 1;
}

int eq89_unequip(EQ89_System *system, int actor_id)
{
    EQ89_Instance *instance;
    if (system == 0 || !system->initialized) return 0;
    instance = eq89_find(system, actor_id);
    if (instance == 0) return 1;
    if (instance->attached && !system->provider.detach(system->provider.user,
            instance->actor_id, instance->object_id,
            instance->attachment_name)) return 0;
    memset(instance, 0, sizeof(*instance));
    if (system->count > 0) system->count -= 1;
    return 1;
}

int eq89_set_visible(EQ89_System *system, int actor_id, int visible)
{
    EQ89_Instance *instance;
    if (system == 0 || !system->initialized) return 0;
    instance = eq89_find(system, actor_id);
    if (instance == 0 || !instance->attached) return 0;
    if (!system->provider.set_visible(system->provider.user, actor_id,
            instance->object_id, instance->attachment_name, visible)) return 0;
    instance->visible = visible ? 1 : 0;
    return 1;
}

int eq89_sync(EQ89_System *system, int actor_id, int actor_kind,
              int desired_item_id, unsigned short frame_ms)
{
    int slot;
    EQ89_Instance *instance;
    unsigned char xform_storage[256];
    const void *xform;
    unsigned int xform_size;
    if (system == 0 || !system->initialized || desired_item_id <= 0) return 0;
    instance = eq89_find(system, actor_id);
    if (instance == 0 || !instance->attached ||
        instance->item_id != desired_item_id) {
        if (!eq89_equip(system, actor_id, actor_kind, desired_item_id)) return 0;
        instance = eq89_find(system, actor_id);
        if (instance == 0) return 0;
    }
    slot = eq89_find_slot(system, actor_id);
    xform = 0;
    xform_size = 0U;
    if (system->provider.get_object_xform != 0 &&
        system->provider.get_object_xform(system->provider.user,
            instance->object_id, xform_storage, sizeof(xform_storage))) {
        xform = xform_storage;
        xform_size = sizeof(xform_storage);
    }
    if (system->provider.runtime_sync != 0)
        instance->last_result = system->provider.runtime_sync(
            system->provider.user, slot, actor_id, desired_item_id,
            xform, xform_size, frame_ms) ? 1 : 0;
    else
        instance->last_result = 1;
    system->last_result = instance->last_result;
    return instance->last_result;
}

void eq89_trigger(EQ89_System *system, int actor_id, int item_id, int event_id)
{
    int slot;
    const EQ89_Instance *instance;
    if (system == 0 || !system->initialized ||
        system->provider.runtime_trigger == 0) return;
    slot = eq89_find_slot(system, actor_id);
    if (slot < 0) return;
    instance = &system->instances[slot];
    if (!instance->attached) return;
    if (item_id > 0 && instance->item_id != item_id) return;
    system->provider.runtime_trigger(system->provider.user, slot, event_id);
}

int eq89_packet_count(const EQ89_System *system, int actor_id)
{
    int slot;
    const EQ89_Instance *instance;
    if (system == 0 || system->provider.runtime_packet_count == 0) return 0;
    slot = eq89_find_slot(system, actor_id);
    if (slot < 0) return 0;
    instance = &system->instances[slot];
    if (!instance->visible) return 0;
    return system->provider.runtime_packet_count(system->provider.user, slot);
}

const void *eq89_packet(const EQ89_System *system, int actor_id,
                        int packet_index)
{
    int slot;
    const EQ89_Instance *instance;
    if (system == 0 || system->provider.runtime_packet == 0) return 0;
    slot = eq89_find_slot(system, actor_id);
    if (slot < 0) return 0;
    instance = &system->instances[slot];
    if (!instance->visible) return 0;
    return system->provider.runtime_packet(system->provider.user, slot,
                                           packet_index);
}

const void *eq89_muzzle(const EQ89_System *system, int actor_id)
{
    int slot;
    const EQ89_Instance *instance;
    if (system == 0 || system->provider.runtime_muzzle == 0) return 0;
    slot = eq89_find_slot(system, actor_id);
    if (slot < 0) return 0;
    instance = &system->instances[slot];
    if (!instance->visible) return 0;
    return system->provider.runtime_muzzle(system->provider.user, slot);
}

int eq89_model_id(const EQ89_System *system, int actor_id)
{
    const EQ89_Instance *instance;
    instance = eq89_find_const(system, actor_id);
    return instance != 0 ? instance->model_id : 0;
}

const char *eq89_model_name(const EQ89_System *system, int actor_id)
{
    const EQ89_Instance *instance;
    instance = eq89_find_const(system, actor_id);
    return instance != 0 ? instance->model_name : "";
}

int eq89_object_id(const EQ89_System *system, int actor_id)
{
    const EQ89_Instance *instance;
    instance = eq89_find_const(system, actor_id);
    return instance != 0 ? instance->object_id : EQ89_INVALID_ID;
}

const char *eq89_status(const EQ89_System *system)
{
    return system != 0 ? system->status : "equipment_system89: null";
}
