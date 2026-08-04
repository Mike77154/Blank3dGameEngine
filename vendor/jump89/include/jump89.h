#ifndef JUMP89_H
#define JUMP89_H

#include "vmotion89_provider.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct jump89_context_tag {
    vm89_provider_bundle providers;
} jump89_context;

typedef struct jump89_state_tag {
    int active;
    vm89_scalar velocity_y;
} jump89_state;

void jump89_init(jump89_context *context,
                 const vm89_provider_bundle *providers);
void jump89_state_reset(jump89_state *state);
int jump89_start(jump89_context *context,
                 jump89_state *state,
                 void *actor,
                 vm89_scalar impulse,
                 vm89_scalar floor_y,
                 int gravity_enabled);
int jump89_tick(jump89_context *context,
                jump89_state *state,
                void *actor,
                vm89_scalar gravity_acceleration,
                vm89_scalar dt,
                vm89_scalar floor_y,
                int gravity_enabled);
int jump89_is_active(const jump89_state *state);
int jump89_is_falling(const jump89_state *state);

#ifdef __cplusplus
}
#endif

#endif
