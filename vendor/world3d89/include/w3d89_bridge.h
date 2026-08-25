#ifndef W3D89_BRIDGE_H
#define W3D89_BRIDGE_H

#include "w3d89_arena.h"

#ifdef __cplusplus
extern "C" {
#endif

struct w3d_world_s;
struct w3d_cell_s;
struct w3d_entity_s;
struct w3d_portal_s;

typedef enum w3d_bridge_action_e {
    W3D_BRIDGE_CELL_LOAD_REQUEST = 1,
    W3D_BRIDGE_CELL_UNLOAD_REQUEST = 2,
    W3D_BRIDGE_CELL_ACTIVATE_REQUEST = 3,
    W3D_BRIDGE_CELL_DEACTIVATE_REQUEST = 4,
    W3D_BRIDGE_ENTITY_ENTER_CELL = 5,
    W3D_BRIDGE_ENTITY_LEAVE_CELL = 6,
    W3D_BRIDGE_ENTITY_POSE_CHANGED = 7,
    W3D_BRIDGE_PORTAL_OPEN_CHANGED = 8,
    W3D_BRIDGE_PROXY_ENABLE_REQUEST = 9,
    W3D_BRIDGE_PROXY_DISABLE_REQUEST = 10
} w3d_bridge_action;

typedef enum w3d_bridge_result_e {
    W3D_BRIDGE_IGNORED = 0,
    W3D_BRIDGE_ACCEPTED = 1,
    W3D_BRIDGE_DENIED = 2
} w3d_bridge_result;

typedef int (*w3d_cell_bridge_fn)(struct w3d_world_s *world, const struct w3d_cell_s *cell, int action, void *user);
typedef int (*w3d_entity_bridge_fn)(struct w3d_world_s *world, const struct w3d_entity_s *entity, int action, void *user);
typedef int (*w3d_portal_bridge_fn)(struct w3d_world_s *world, const struct w3d_portal_s *portal, int action, void *user);

typedef struct w3d_world_callbacks_s {
    w3d_cell_bridge_fn cell_event;       /* legacy/general bus */
    w3d_entity_bridge_fn entity_event;   /* legacy/general bus */
    w3d_portal_bridge_fn portal_event;   /* portal bus */
    w3d_cell_bridge_fn asset_event;      /* separated services */
    w3d_cell_bridge_fn render_event;
    w3d_cell_bridge_fn collision_event;
    w3d_cell_bridge_fn physics_event;
    w3d_cell_bridge_fn nav_event;
    w3d_cell_bridge_fn audio_event;
    w3d_cell_bridge_fn script_event;
} w3d_world_callbacks;

#ifdef __cplusplus
}
#endif

#endif
