/*
 * gaimquery89.h - central aim ray query and typed hit result.
 */
#ifndef GAIMQUERY89_H
#define GAIMQUERY89_H

#include "../../include/gscope89_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef GAQ89_API
#define GAQ89_API
#endif

#define GAQ89_TARGET_UNKNOWN 0
#define GAQ89_TARGET_WORLD   1
#define GAQ89_TARGET_ACTOR   2
#define GAQ89_TARGET_PROP    3
#define GAQ89_TARGET_TRIGGER 4

typedef struct gaq89_hit {
    short valid;
    short target_id;
    short target_kind;
    short target_part;
    short material_id;
    short flags;
    g89_fx distance;
    g89_vec3 position;
    g89_vec3 normal;
} gaq89_hit;

typedef int (*gaq89_raycast_cb)(void *user,
                                const g89_vec3 *from,
                                const g89_vec3 *dir,
                                g89_fx max_dist,
                                gaq89_hit *out_hit);

typedef struct gaq89_ctx {
    gaq89_raycast_cb raycast_cb;
    void *user;
    g89_vec3 ray_from;
    g89_vec3 ray_dir;
    g89_fx max_distance;
    g89_vec3 miss_endpoint;
    gaq89_hit last_hit;
} gaq89_ctx;

GAQ89_API void gaq89_init(gaq89_ctx *ctx,
                          gaq89_raycast_cb raycast_cb,
                          void *user);
GAQ89_API void gaq89_set_callback(gaq89_ctx *ctx,
                                  gaq89_raycast_cb raycast_cb,
                                  void *user);
GAQ89_API void gaq89_clear(gaq89_ctx *ctx);
GAQ89_API int gaq89_query_ray(gaq89_ctx *ctx,
                              const g89_vec3 *from,
                              const g89_vec3 *dir,
                              g89_fx max_distance);
GAQ89_API int gaq89_query_center(gaq89_ctx *ctx,
                                 const g89_camera *camera,
                                 g89_fx max_distance);
GAQ89_API const gaq89_hit *gaq89_get_last_hit(const gaq89_ctx *ctx);

#ifdef __cplusplus
}
#endif

#endif
