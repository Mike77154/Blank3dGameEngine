#ifndef PDC3D_BRIDGE_H
#define PDC3D_BRIDGE_H

#include "pdc3d_damage.h"
#include "hurtbox3d.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PDC3D_WORLD_HIT_NONE     0
#define PDC3D_WORLD_HIT_SOLID    1
#define PDC3D_WORLD_HIT_GROUND   2
#define PDC3D_WORLD_HIT_WALL     3

typedef struct pdc3d_world_hit_s {
    int hit;
    int kind;
    int actor_id;
    int material_flags;
    hb3_v3 point;
    hb3_v3 normal;
} pdc3d_world_hit;

typedef int (*pdc3d_world_probe_fn)(void *user,
                                    hb3_v3 from,
                                    hb3_v3 to,
                                    hb3_fx radius,
                                    pdc3d_world_hit *out_hit);

typedef void (*pdc3d_damage_event_fn)(void *user,
                                      const pdc3d_damage_packet *packet);

typedef hb3_v3 (*pdc3d_socket_pose_fn)(void *user,
                                       int actor_id,
                                       int socket_id);

typedef struct pdc3d_bridge_s {
    void *user;
    pdc3d_world_probe_fn world_probe;
    pdc3d_damage_event_fn damage_event;
    pdc3d_socket_pose_fn socket_pose;
} pdc3d_bridge;

void pdc3d_bridge_init(pdc3d_bridge *b);

#ifdef __cplusplus
}
#endif

#endif
