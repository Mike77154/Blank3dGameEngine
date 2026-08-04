#ifndef GRECOIL89_PROVIDER_H
#define GRECOIL89_PROVIDER_H

#include "grecoil89.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
   Generic external transform provider.

   The provider exports the current additive recoil layer to another library.
   Values are absolute for the recoil layer at the current tick; consumers must
   replace their previous recoil layer with these values, not accumulate them.
*/

#define GREC_PROVIDER_MODE_OFF  0
#define GREC_PROVIDER_MODE_PULL 1
#define GREC_PROVIDER_MODE_PUSH 2

#define GREC_PROVIDER_TARGET_AIM    0
#define GREC_PROVIDER_TARGET_CAMERA 1
#define GREC_PROVIDER_TARGET_WEAPON 2

#define GREC_PROVIDER_AIM_ROTATE    1u
#define GREC_PROVIDER_CAMERA_ROTATE 2u
#define GREC_PROVIDER_WEAPON_ROTATE 4u
#define GREC_PROVIDER_WEAPON_MOVE   8u
#define GREC_PROVIDER_ALL           15u

typedef void (*GRecProviderRotateFn)(void *user,
                                     grec_u16 target,
                                     const GRecAngles *rotate);
typedef void (*GRecProviderMoveFn)(void *user,
                                   grec_u16 target,
                                   const GRecVec3 *move);

typedef struct GRecProvider_s {
    void *user;
    GRecProviderRotateFn set_rotate;
    GRecProviderMoveFn set_move;
    grec_u32 mask;
    grec_u16 mode;
    GRecOutput last_output;
    grec_u8 has_output;
} GRecProvider;

void grec_provider_init(GRecProvider *provider,
                        void *user,
                        GRecProviderRotateFn rotate_fn,
                        GRecProviderMoveFn move_fn);

void grec_provider_set_mode(GRecProvider *provider, grec_u16 mode);
void grec_provider_set_mask(GRecProvider *provider, grec_u32 mask);

/* Cache an already sampled output and, in PUSH mode, call the external lib. */
void grec_provider_apply_output(GRecProvider *provider,
                                const GRecOutput *out);

/* Convenience wrappers: sample solver state, cache it and optionally push it. */
void grec_provider_after_fire(GRecProvider *provider,
                              const GRecState *state,
                              const GRecProfile *profile,
                              const GRecContext *ctx);
void grec_provider_after_update(GRecProvider *provider,
                                const GRecState *state,
                                const GRecProfile *profile,
                                const GRecContext *ctx);

/* PULL mode accessors. They also work in PUSH mode. */
int grec_provider_get_rotate(const GRecProvider *provider,
                             grec_u16 target,
                             GRecAngles *out_rotate);
int grec_provider_get_move(const GRecProvider *provider,
                           grec_u16 target,
                           GRecVec3 *out_move);

/* Send/cache zero transforms so an external additive layer returns to neutral. */
void grec_provider_clear(GRecProvider *provider);
int grec_provider_has_output(const GRecProvider *provider);

#ifdef __cplusplus
}
#endif

#endif
