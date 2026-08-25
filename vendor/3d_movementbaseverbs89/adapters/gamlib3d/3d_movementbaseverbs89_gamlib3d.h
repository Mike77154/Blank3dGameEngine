#ifndef MOVEMENTBASEVERBS89_GAMLIB3D_H
#define MOVEMENTBASEVERBS89_GAMLIB3D_H

#include "3d_movementbaseverbs89.h"
#include "gamlib3d_transform.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum mbv89_gamlib3d_move_mode {
    MBV89_GAMLIB3D_MOVE_FLAT = 0,
    MBV89_GAMLIB3D_MOVE_6DOF = 1
} mbv89_gamlib3d_move_mode;

typedef struct mbv89_gamlib3d_adapter {
    mbv89_gamlib3d_move_mode move_mode;
} mbv89_gamlib3d_adapter;

void mbv89_gamlib3d_adapter_init(mbv89_gamlib3d_adapter *adapter);
void mbv89_gamlib3d_adapter_set_move_mode(mbv89_gamlib3d_adapter *adapter,
                                           mbv89_gamlib3d_move_mode mode);

/* Bind one movement actor to a caller-owned Gamlib3D Transform. */
void mbv89_gamlib3d_bind_actor(mbv89_actor *actor, Transform *transform);
Transform *mbv89_gamlib3d_actor_transform(mbv89_actor *actor);

/* Install Gamlib3D as the base-verb provider of a context. */
void mbv89_gamlib3d_install(mbv89_context *ctx,
                             mbv89_gamlib3d_adapter *adapter);

/* Public provider entry point for custom provider chains. */
int mbv89_gamlib3d_base_provider(void *provider_user,
                                 mbv89_context *ctx,
                                 mbv89_actor *actor,
                                 mbv89_base_verb verb,
                                 mbv89_fixed amount);

#ifdef __cplusplus
}
#endif

#endif
