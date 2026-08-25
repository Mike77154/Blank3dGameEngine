#ifndef EQUIPMENT_SYSTEM89_H
#define EQUIPMENT_SYSTEM89_H

#ifdef __cplusplus
extern "C" {
#endif

#define EQ89_MAX_ACTORS 64
#define EQ89_NAME_CAP 64
#define EQ89_STATUS_CAP 192
#define EQ89_INVALID_ID (-1)

typedef struct EQ89_ItemDescTag {
    int item_id;
    int model_id;
    char socket_name[EQ89_NAME_CAP];
    char attachment_name[EQ89_NAME_CAP];
    char model_name[EQ89_NAME_CAP];
    const void *host_profile;
    const void *local_offset;
} EQ89_ItemDesc;

typedef struct EQ89_InstanceTag {
    int used;
    int actor_id;
    int actor_kind;
    int object_id;
    int item_id;
    int model_id;
    int attached;
    int visible;
    int last_result;
    char socket_name[EQ89_NAME_CAP];
    char attachment_name[EQ89_NAME_CAP];
    char model_name[EQ89_NAME_CAP];
} EQ89_Instance;

typedef struct EQ89_ProviderTag {
    void *user;
    int (*resolve_item)(void *user, int item_id, EQ89_ItemDesc *out_desc);
    int (*define_socket)(void *user, int actor_id, const char *socket_name,
                         int socket_index, const void *local_offset);
    int (*attach)(void *user, int actor_id, int object_id,
                  const char *attachment_name, const char *socket_name,
                  const void *local_offset);
    int (*detach)(void *user, int actor_id, int object_id,
                  const char *attachment_name);
    int (*set_visible)(void *user, int actor_id, int object_id,
                       const char *attachment_name, int visible);
    int (*get_object_xform)(void *user, int object_id,
                            void *out_xform, unsigned int out_size);
    int (*runtime_init)(void *user, int slot, const EQ89_ItemDesc *desc);
    int (*runtime_sync)(void *user, int slot, int actor_id, int item_id,
                        const void *object_xform, unsigned int xform_size,
                        unsigned short frame_ms);
    void (*runtime_trigger)(void *user, int slot, int event_id);
    int (*runtime_packet_count)(void *user, int slot);
    const void *(*runtime_packet)(void *user, int slot, int packet_index);
    const void *(*runtime_muzzle)(void *user, int slot);
} EQ89_Provider;

typedef struct EQ89_SystemTag {
    EQ89_Instance instances[EQ89_MAX_ACTORS];
    int count;
    int initialized;
    int object_base;
    int last_result;
    EQ89_Provider provider;
    char status[EQ89_STATUS_CAP];
} EQ89_System;

void eq89_init(EQ89_System *system, int object_base,
               const EQ89_Provider *provider);
int eq89_define_socket(EQ89_System *system, int actor_id,
                       const char *socket_name, int socket_index,
                       const void *local_offset);
int eq89_equip(EQ89_System *system, int actor_id, int actor_kind, int item_id);
int eq89_unequip(EQ89_System *system, int actor_id);
int eq89_set_visible(EQ89_System *system, int actor_id, int visible);
int eq89_sync(EQ89_System *system, int actor_id, int actor_kind,
              int desired_item_id, unsigned short frame_ms);
void eq89_trigger(EQ89_System *system, int actor_id, int item_id, int event_id);
int eq89_find_slot(const EQ89_System *system, int actor_id);
EQ89_Instance *eq89_find(EQ89_System *system, int actor_id);
const EQ89_Instance *eq89_find_const(const EQ89_System *system, int actor_id);
int eq89_packet_count(const EQ89_System *system, int actor_id);
const void *eq89_packet(const EQ89_System *system, int actor_id,
                        int packet_index);
const void *eq89_muzzle(const EQ89_System *system, int actor_id);
int eq89_model_id(const EQ89_System *system, int actor_id);
const char *eq89_model_name(const EQ89_System *system, int actor_id);
int eq89_object_id(const EQ89_System *system, int actor_id);
const char *eq89_status(const EQ89_System *system);

#ifdef __cplusplus
}
#endif

#endif
