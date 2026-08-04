#ifndef GATTACH89_H
#define GATTACH89_H

#ifdef __cplusplus
extern "C" {
#endif

/*
    gattach89
    C89, fixed point, static storage attachment/socket library.

    Design notes:
    - No dynamic allocation is used by this library.
    - No floating point types are used.
    - Caller supplies entity and bone transforms through callbacks.
    - Library resolves socket transforms and child attachment transforms.
*/

#define GATTACH89_VERSION_MAJOR 1
#define GATTACH89_VERSION_MINOR 0
#define GATTACH89_VERSION_PATCH 0

#define GATTACH89_FIX_SHIFT 12
#define GATTACH89_FIX_ONE   (1 << GATTACH89_FIX_SHIFT)
#define GATTACH89_FIX_HALF  (1 << (GATTACH89_FIX_SHIFT - 1))

#ifndef GATTACH89_MAX_SOCKETS
#define GATTACH89_MAX_SOCKETS 96
#endif

#ifndef GATTACH89_MAX_ATTACHMENTS
#define GATTACH89_MAX_ATTACHMENTS 160
#endif

#ifndef GATTACH89_MAX_PARTS
#define GATTACH89_MAX_PARTS 128
#endif

#ifndef GATTACH89_MAX_EVENTS
#define GATTACH89_MAX_EVENTS 192
#endif

#ifndef GATTACH89_NAME_LEN
#define GATTACH89_NAME_LEN 32
#endif

#define GATTACH89_INVALID_ID (-1)

#define GATTACH89_TRUE  1
#define GATTACH89_FALSE 0

typedef int gatt_fix;

typedef struct GAtt89_Vec3 {
    gatt_fix x;
    gatt_fix y;
    gatt_fix z;
} GAtt89_Vec3;

typedef struct GAtt89_Quat {
    gatt_fix x;
    gatt_fix y;
    gatt_fix z;
    gatt_fix w;
} GAtt89_Quat;

typedef struct GAtt89_Xform {
    GAtt89_Vec3 pos;
    GAtt89_Quat rot;
    GAtt89_Vec3 scale;
} GAtt89_Xform;

typedef enum GAtt89_Result {
    GATTACH89_OK = 0,
    GATTACH89_ERR_NULL = -1,
    GATTACH89_ERR_FULL = -2,
    GATTACH89_ERR_NOT_FOUND = -3,
    GATTACH89_ERR_BAD_ARG = -4,
    GATTACH89_ERR_INACTIVE = -5
} GAtt89_Result;

typedef enum GAtt89_SocketKind {
    GATTACH89_SOCKET_ENTITY = 0,
    GATTACH89_SOCKET_BONE = 1,
    GATTACH89_SOCKET_SOCKET = 2,
    GATTACH89_SOCKET_VIRTUAL = 3
} GAtt89_SocketKind;

typedef enum GAtt89_AttachKind {
    GATTACH89_ATTACH_PROP = 0,
    GATTACH89_ATTACH_ENTITY = 1,
    GATTACH89_ATTACH_FX = 2,
    GATTACH89_ATTACH_HITBOX = 3
} GAtt89_AttachKind;

typedef enum GAtt89_EventKind {
    GATTACH89_EVENT_SOCKET_RESOLVED = 1,
    GATTACH89_EVENT_ATTACHMENT_RESOLVED = 2,
    GATTACH89_EVENT_ATTACHMENT_HIDDEN = 3,
    GATTACH89_EVENT_PART_VIS_CHANGED = 4,
    GATTACH89_EVENT_DETACHED = 5,
    GATTACH89_EVENT_MISSING_BASE = 6
} GAtt89_EventKind;

#define GATTACH89_AF_VISIBLE              0x0001u
#define GATTACH89_AF_ACTIVE               0x0002u
#define GATTACH89_AF_INHERIT_PARENT_VIS   0x0004u
#define GATTACH89_AF_CALL_CHILD_SETTER    0x0008u
#define GATTACH89_AF_RENDER_PACKET        0x0010u
#define GATTACH89_AF_PHYSICS_PACKET       0x0020u
#define GATTACH89_AF_KEEP_ON_PARENT_DEATH 0x0040u
#define GATTACH89_AF_SCRIPT_EVENT         0x0080u

#define GATTACH89_PF_VISIBLE              0x0001u
#define GATTACH89_PF_DIRTY                0x0002u

typedef struct GAtt89_Socket {
    int used;
    int owner_entity_id;
    int kind;
    int bone_index;
    int parent_socket_id;
    char name[GATTACH89_NAME_LEN];
    GAtt89_Xform local;
    GAtt89_Xform world;
} GAtt89_Socket;

typedef struct GAtt89_Attachment {
    int used;
    int parent_entity_id;
    int child_entity_id;
    int socket_id;
    int kind;
    unsigned int flags;
    char name[GATTACH89_NAME_LEN];
    GAtt89_Xform local;
    GAtt89_Xform world;
} GAtt89_Attachment;

typedef struct GAtt89_Part {
    int used;
    int owner_entity_id;
    unsigned int flags;
    char name[GATTACH89_NAME_LEN];
} GAtt89_Part;

typedef struct GAtt89_Event {
    int kind;
    int owner_entity_id;
    int child_entity_id;
    int socket_id;
    int attachment_id;
    int part_id;
    GAtt89_Xform xform;
} GAtt89_Event;

typedef struct GAtt89_Output {
    GAtt89_Event events[GATTACH89_MAX_EVENTS];
    int event_count;
    int dropped_event_count;
} GAtt89_Output;

struct GAtt89_Callbacks;

typedef int (*GAtt89_GetEntityXformFn)(void *user, int entity_id, GAtt89_Xform *out_xform);
typedef int (*GAtt89_GetBoneXformFn)(void *user, int entity_id, int bone_index, GAtt89_Xform *out_xform);
typedef int (*GAtt89_GetEntityVisibleFn)(void *user, int entity_id);
typedef void (*GAtt89_SetEntityXformFn)(void *user, int entity_id, const GAtt89_Xform *xform);
typedef void (*GAtt89_SetPartVisibleFn)(void *user, int entity_id, const char *part_name, int visible);
typedef void (*GAtt89_OnEventFn)(void *user, const GAtt89_Event *ev);

typedef struct GAtt89_Callbacks {
    void *user;
    GAtt89_GetEntityXformFn get_entity_xform;
    GAtt89_GetBoneXformFn get_bone_xform;
    GAtt89_GetEntityVisibleFn get_entity_visible;
    GAtt89_SetEntityXformFn set_entity_xform;
    GAtt89_SetPartVisibleFn set_part_visible;
    GAtt89_OnEventFn on_event;
} GAtt89_Callbacks;

typedef struct GAtt89_World {
    GAtt89_Socket sockets[GATTACH89_MAX_SOCKETS];
    GAtt89_Attachment attachments[GATTACH89_MAX_ATTACHMENTS];
    GAtt89_Part parts[GATTACH89_MAX_PARTS];
} GAtt89_World;

/* Fixed math helpers. */
gatt_fix gatt89_from_int(int v);
int gatt89_to_int(gatt_fix v);
gatt_fix gatt89_mul(gatt_fix a, gatt_fix b);
gatt_fix gatt89_div(gatt_fix a, gatt_fix b);
GAtt89_Vec3 gatt89_vec3(gatt_fix x, gatt_fix y, gatt_fix z);
GAtt89_Quat gatt89_quat_identity(void);
GAtt89_Xform gatt89_xform_identity(void);
GAtt89_Vec3 gatt89_vec3_add(GAtt89_Vec3 a, GAtt89_Vec3 b);
GAtt89_Vec3 gatt89_vec3_sub(GAtt89_Vec3 a, GAtt89_Vec3 b);
GAtt89_Vec3 gatt89_vec3_mul_components(GAtt89_Vec3 a, GAtt89_Vec3 b);
GAtt89_Quat gatt89_quat_mul(GAtt89_Quat a, GAtt89_Quat b);
GAtt89_Vec3 gatt89_quat_rotate_vec3(GAtt89_Quat q, GAtt89_Vec3 v);
GAtt89_Xform gatt89_xform_compose(const GAtt89_Xform *parent, const GAtt89_Xform *local);

/* String helpers: fixed-size, no external storage. */
void gatt89_name_copy(char dst[GATTACH89_NAME_LEN], const char *src);
int gatt89_name_eq(const char *a, const char *b);

/* World control. */
void gatt89_world_init(GAtt89_World *w);
void gatt89_output_init(GAtt89_Output *out);

/* Socket API. */
int gatt89_socket_add(GAtt89_World *w, int owner_entity_id, const char *name, int kind, int bone_index, int parent_socket_id, const GAtt89_Xform *local);
int gatt89_socket_find(const GAtt89_World *w, int owner_entity_id, const char *name);
int gatt89_socket_set_local(GAtt89_World *w, int socket_id, const GAtt89_Xform *local);
int gatt89_socket_get_world(const GAtt89_World *w, int socket_id, GAtt89_Xform *out_world);

/* Attachment API. */
int gatt89_attach_add(GAtt89_World *w, int parent_entity_id, int child_entity_id, const char *attachment_name, const char *socket_name, int kind, unsigned int flags, const GAtt89_Xform *local);
int gatt89_attach_add_to_socket_id(GAtt89_World *w, int parent_entity_id, int child_entity_id, const char *attachment_name, int socket_id, int kind, unsigned int flags, const GAtt89_Xform *local);
int gatt89_attach_find(const GAtt89_World *w, int parent_entity_id, int child_entity_id, const char *attachment_name);
int gatt89_attach_set_socket(GAtt89_World *w, int attachment_id, int socket_id);
int gatt89_attach_set_local(GAtt89_World *w, int attachment_id, const GAtt89_Xform *local);
int gatt89_attach_set_visible(GAtt89_World *w, int attachment_id, int visible);
int gatt89_attach_set_active(GAtt89_World *w, int attachment_id, int active);
int gatt89_attach_detach(GAtt89_World *w, int attachment_id);
int gatt89_attach_get_world(const GAtt89_World *w, int attachment_id, GAtt89_Xform *out_world);

/* Visible/hidden body-part API. */
int gatt89_part_add(GAtt89_World *w, int owner_entity_id, const char *name, int visible);
int gatt89_part_find(const GAtt89_World *w, int owner_entity_id, const char *name);
int gatt89_part_set_visible(GAtt89_World *w, int part_id, int visible);
int gatt89_part_is_visible(const GAtt89_World *w, int part_id);

/* Per-frame resolver. */
int gatt89_update(GAtt89_World *w, const GAtt89_Callbacks *cb, GAtt89_Output *out);

#ifdef __cplusplus
}
#endif

#endif
