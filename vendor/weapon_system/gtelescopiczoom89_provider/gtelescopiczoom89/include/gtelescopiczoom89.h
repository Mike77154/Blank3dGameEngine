/*
 * gtelescopiczoom89.h - optical FOV, transition and scoped sensitivity.
 * C89, fixed-point/integer runtime, no heap ownership.
 */
#ifndef GTELESCOPICZOOM89_H
#define GTELESCOPICZOOM89_H

#include "../../include/gscope89_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef GTZ89_API
#define GTZ89_API
#endif

#define GTZ89_ABI_VERSION              2u

#define GTZ89_PROVIDER_INTERNAL        0u
#define GTZ89_PROVIDER_CAMERA         1u
#define GTZ89_PROVIDER_MATH           2u
#define GTZ89_PROVIDER_CAMERA_AND_MATH \
    (GTZ89_PROVIDER_CAMERA | GTZ89_PROVIDER_MATH)

/*
 * Provider callbacks return non-zero when they supplied a valid value.
 * All values remain integer/fixed-point. The provider and camera pointers
 * are borrowed; this module never allocates, frees or owns them.
 */
typedef short (*gtz89_provider_read_base_fov_fn)(void *user,
                                                 short *out_fov_deg_x100);
typedef void (*gtz89_provider_write_fov_fn)(void *user,
                                            short base_fov_deg_x100,
                                            short current_fov_deg_x100);
typedef void (*gtz89_provider_write_sensitivity_fn)(void *user,
                                                    short sensitivity_pct);
typedef short (*gtz89_provider_solve_fov_fn)(void *user,
                                             short base_fov_deg_x100,
                                             short zoom_x100,
                                             short *out_fov_deg_x100);
typedef short (*gtz89_provider_lerp_short_fn)(void *user,
                                              short a,
                                              short b,
                                              short t_x1000,
                                              short *out_value);

typedef struct gtz89_provider {
    unsigned short mode;
    void *user;

    /* Optional zero-copy binding to an externally owned camera. */
    g89_camera *camera;

    /* Optional camera callbacks. They may be used with or without camera. */
    gtz89_provider_read_base_fov_fn read_base_fov;
    gtz89_provider_write_fov_fn write_fov;
    gtz89_provider_write_sensitivity_fn write_sensitivity;

    /* Optional external optical math. Missing callbacks fall back internally. */
    gtz89_provider_solve_fov_fn solve_fov;
    gtz89_provider_lerp_short_fn lerp_short;
} gtz89_provider;

typedef struct gtz89_profile {
    short zoom_x100;
    short enter_frames;
    short exit_frames;
    short scoped_sens_pct;
} gtz89_profile;

typedef struct gtz89_ctx {
    gtz89_profile profile;
    short active;
    short blend_x1000;
    short base_fov_deg_x100;
    short target_fov_deg_x100;
    short current_fov_deg_x100;
    short sensitivity_pct;
    short just_entered;
    short just_exited;

    /* Appended provider state; old API calls remain source-compatible. */
    gtz89_provider provider;
    short provider_enabled;
} gtz89_ctx;

GTZ89_API short gtz89_solve_fov_deg_x100(short base_fov_deg_x100,
                                         short zoom_x100);
GTZ89_API void gtz89_provider_init(gtz89_provider *provider);
GTZ89_API void gtz89_set_provider(gtz89_ctx *ctx,
                                  const gtz89_provider *provider);
GTZ89_API void gtz89_clear_provider(gtz89_ctx *ctx);
GTZ89_API unsigned short gtz89_get_provider_mode(const gtz89_ctx *ctx);

GTZ89_API void gtz89_init(gtz89_ctx *ctx,
                          const gtz89_profile *profile,
                          short base_fov_deg_x100);
GTZ89_API void gtz89_set_profile(gtz89_ctx *ctx,
                                 const gtz89_profile *profile);
GTZ89_API void gtz89_set_base_fov(gtz89_ctx *ctx,
                                  short base_fov_deg_x100);
GTZ89_API void gtz89_begin(gtz89_ctx *ctx);
GTZ89_API void gtz89_end(gtz89_ctx *ctx);
GTZ89_API short gtz89_update(gtz89_ctx *ctx, short dt_frames);

GTZ89_API gtz89_profile gtz89_profile_red_dot(void);
GTZ89_API gtz89_profile gtz89_profile_acog4x(void);
GTZ89_API gtz89_profile gtz89_profile_sniper8x(void);
GTZ89_API gtz89_profile gtz89_profile_sniper12x(void);

#ifdef __cplusplus
}
#endif

#endif
