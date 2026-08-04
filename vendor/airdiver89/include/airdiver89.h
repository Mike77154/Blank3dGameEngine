#ifndef AIRDIVER89_H
#define AIRDIVER89_H

#include "vmotion89_provider.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct airdiver89_context_tag {
    vm89_provider_bundle providers;
} airdiver89_context;

typedef struct airdiver89_state_tag {
    vm89_vec3 return_position;
    int return_position_valid;
} airdiver89_state;

void airdiver89_init(airdiver89_context *context,
                     const vm89_provider_bundle *providers);
void airdiver89_state_reset(airdiver89_state *state);
int airdiver89_save_position(airdiver89_context *context,
                             airdiver89_state *state,
                             void *actor);
int airdiver89_move_to_point(airdiver89_context *context,
                             void *actor,
                             const vm89_vec3 *target,
                             vm89_scalar speed,
                             vm89_scalar dt,
                             vm89_scalar floor_y,
                             int clamp_to_floor,
                             int *out_reached);
int airdiver89_descend_to_y(airdiver89_context *context,
                            void *actor,
                            vm89_scalar target_y,
                            vm89_scalar speed,
                            vm89_scalar dt,
                            vm89_scalar floor_y,
                            int *out_reached);
int airdiver89_ram_target(airdiver89_context *context,
                          void *actor,
                          const vm89_vec3 *target,
                          vm89_scalar speed,
                          vm89_scalar dt,
                          vm89_scalar floor_y,
                          int *out_reached);
int airdiver89_return_saved(airdiver89_context *context,
                            airdiver89_state *state,
                            void *actor,
                            vm89_scalar speed,
                            vm89_scalar dt,
                            vm89_scalar floor_y,
                            int *out_reached);
int airdiver89_saved_position(const airdiver89_state *state,
                              vm89_vec3 *out_position);

#ifdef __cplusplus
}
#endif

#endif
