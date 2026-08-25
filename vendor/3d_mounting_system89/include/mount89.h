#ifndef MOUNT89_H
#define MOUNT89_H

#include <limits.h>

#if LONG_MAX < 2147483647L
#error mount89 requires long to be at least 32 bits
#endif
#if SHRT_MAX < 32767
#error mount89 requires short to be at least 16 bits
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MOUNT89_MAX_POINTS
#define MOUNT89_MAX_POINTS 128
#endif

#ifndef MOUNT89_MAX_LINKS
#define MOUNT89_MAX_LINKS 64
#endif

#define MOUNT89_FX_SHIFT 16
#define MOUNT89_FX_ONE   65536L
#define MOUNT89_ROT_SHIFT 14
#define MOUNT89_ROT_ONE  16384

#define MOUNT89_ANY_MASK 0UL

#define MOUNT89_INHERIT_POSITION 1U
#define MOUNT89_INHERIT_ROTATION 2U
#define MOUNT89_INHERIT_ALL (MOUNT89_INHERIT_POSITION | MOUNT89_INHERIT_ROTATION)

#define MOUNT89_POINT_STATIC  0
#define MOUNT89_POINT_DYNAMIC 1

#define MOUNT89_MOUNT_SNAP       0
#define MOUNT89_MOUNT_KEEP_WORLD 1
#define MOUNT89_MOUNT_OFFSET     2

#define MOUNT89_UNMOUNT_KEEP_WORLD    0
#define MOUNT89_UNMOUNT_RESTORE_WORLD 1

#define MOUNT89_EVENT_MOUNTED   1
#define MOUNT89_EVENT_UNMOUNTED 2

#define MOUNT89_OK                    0
#define MOUNT89_ERR_ARGUMENT         -1
#define MOUNT89_ERR_NO_PROVIDER      -2
#define MOUNT89_ERR_NO_POINT         -3
#define MOUNT89_ERR_POINT_BUSY       -4
#define MOUNT89_ERR_GUEST_BUSY       -5
#define MOUNT89_ERR_INCOMPATIBLE     -6
#define MOUNT89_ERR_CAPACITY         -7
#define MOUNT89_ERR_PROVIDER         -8
#define MOUNT89_ERR_CYCLE            -9
#define MOUNT89_ERR_SAME_OBJECT     -10
#define MOUNT89_ERR_LINK_NOT_FOUND  -11
#define MOUNT89_ERR_DYNAMIC_POINT   -12

typedef long mount89_fx;
typedef short mount89_rot;

typedef struct mount89_transform {
    mount89_fx p[3];
    mount89_rot r[9];
} mount89_transform;

typedef int (*mount89_get_world_fn)(void *user, int object_id,
                                    mount89_transform *out_world);
typedef int (*mount89_set_world_fn)(void *user, int object_id,
                                    const mount89_transform *world);

typedef int (*mount89_resolve_point_fn)(void *user, int host_id,
                                        int source_id,
                                        mount89_transform *out_host_local);

typedef void (*mount89_event_fn)(void *user, int event_type,
                                 int host_id, int point_id, int guest_id);

typedef struct mount89_point {
    int used;
    int host_id;
    int point_id;
    int source_kind;
    int source_id;
    unsigned long accept_mask;
    mount89_transform local;
} mount89_point;

typedef struct mount89_link {
    int used;
    int link_id;
    int host_id;
    int guest_id;
    int point_slot;
    unsigned int inherit_flags;
    mount89_transform guest_offset;
    mount89_transform pre_mount_world;
} mount89_link;

typedef struct mount89_context {
    mount89_point points[MOUNT89_MAX_POINTS];
    mount89_link links[MOUNT89_MAX_LINKS];
    int next_link_id;

    mount89_get_world_fn get_world;
    mount89_set_world_fn set_world;
    void *transform_user;

    mount89_resolve_point_fn resolve_point;
    void *point_user;

    mount89_event_fn event_fn;
    void *event_user;
} mount89_context;

void mount89_transform_identity(mount89_transform *t);
void mount89_transform_copy(mount89_transform *dst,
                            const mount89_transform *src);
void mount89_transform_compose(const mount89_transform *parent,
                               const mount89_transform *local,
                               mount89_transform *out_world);
void mount89_transform_inverse_rigid(const mount89_transform *t,
                                     mount89_transform *out_inverse);

mount89_fx mount89_fx_from_int(long value);
long mount89_fx_to_int(mount89_fx value);

void mount89_init(mount89_context *ctx);
void mount89_set_transform_provider(mount89_context *ctx,
                                    mount89_get_world_fn get_fn,
                                    mount89_set_world_fn set_fn,
                                    void *user);
void mount89_set_point_provider(mount89_context *ctx,
                                mount89_resolve_point_fn resolve_fn,
                                void *user);
void mount89_set_event_provider(mount89_context *ctx,
                                mount89_event_fn event_fn,
                                void *user);

int mount89_add_static_point(mount89_context *ctx,
                             int host_id, int point_id,
                             unsigned long accept_mask,
                             const mount89_transform *host_local);
int mount89_add_dynamic_point(mount89_context *ctx,
                              int host_id, int point_id,
                              int source_id,
                              unsigned long accept_mask);
int mount89_remove_point(mount89_context *ctx, int host_id, int point_id);
int mount89_set_point_local(mount89_context *ctx, int host_id, int point_id,
                            const mount89_transform *host_local);

int mount89_can_mount(const mount89_context *ctx,
                      int host_id, int point_id, int guest_id,
                      unsigned long guest_mask);
int mount89_mount(mount89_context *ctx,
                  int host_id, int point_id, int guest_id,
                  unsigned long guest_mask,
                  int mount_mode,
                  unsigned int inherit_flags,
                  const mount89_transform *explicit_guest_offset,
                  int *out_link_id);
int mount89_unmount(mount89_context *ctx, int guest_id, int unmount_mode);
int mount89_unmount_link(mount89_context *ctx, int link_id, int unmount_mode);

int mount89_is_mounted(const mount89_context *ctx, int guest_id);
int mount89_get_host(const mount89_context *ctx, int guest_id,
                     int *out_host_id, int *out_point_id);
int mount89_point_is_occupied(const mount89_context *ctx,
                              int host_id, int point_id,
                              int *out_guest_id);
int mount89_get_point_world(mount89_context *ctx,
                            int host_id, int point_id,
                            mount89_transform *out_world);

int mount89_update(mount89_context *ctx);
int mount89_link_count(const mount89_context *ctx);
int mount89_point_count(const mount89_context *ctx);

#ifdef __cplusplus
}
#endif

#endif
