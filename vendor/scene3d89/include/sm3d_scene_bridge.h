#ifndef SM3D_SCENE_BRIDGE_H
#define SM3D_SCENE_BRIDGE_H

#include "sm3d_scene.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
   Bridge helper contracts.
   The scene manager owns no renderer/physics/audio logic.
   External systems receive handles and pull what they need.
*/

typedef struct SM3D_RenderFeed {
    SM3D_Handle handle;
    sm3d_u32 resource_id;
    sm3d_u32 layer_mask;
    SM3D_Transform world;
    SM3D_Aabb world_bounds;
} SM3D_RenderFeed;

typedef struct SM3D_PhysicsFeed {
    SM3D_Handle handle;
    sm3d_u32 collider_id;
    sm3d_u32 layer_mask;
    SM3D_Transform world;
    SM3D_Aabb world_bounds;
} SM3D_PhysicsFeed;

typedef int (*SM3D_RenderFeedFn)(const SM3D_RenderFeed *feed, void *user);
typedef int (*SM3D_PhysicsFeedFn)(const SM3D_PhysicsFeed *feed, void *user);

int sm3d_feed_renderables(SM3D_Context *ctx, int scene_id, sm3d_u32 layer_mask, SM3D_RenderFeedFn fn, void *user);
int sm3d_feed_collidables(SM3D_Context *ctx, int scene_id, sm3d_u32 layer_mask, SM3D_PhysicsFeedFn fn, void *user);

#ifdef __cplusplus
}
#endif

#endif
