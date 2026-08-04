#ifndef FLY89_H
#define FLY89_H

#include "vmotion89_provider.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FLY89_UP 1
#define FLY89_DOWN -1

typedef struct fly89_context_tag {
    vm89_provider_bundle providers;
} fly89_context;

void fly89_init(fly89_context *context,
                const vm89_provider_bundle *providers);
int fly89_move_vertical(fly89_context *context,
                        void *actor,
                        vm89_scalar speed,
                        vm89_scalar dt,
                        int direction,
                        vm89_scalar floor_y);
int fly89_move_to_y(fly89_context *context,
                    void *actor,
                    vm89_scalar target_y,
                    vm89_scalar speed,
                    vm89_scalar dt,
                    vm89_scalar floor_y,
                    int *out_reached);
int fly89_grounded(fly89_context *context,
                   void *actor,
                   vm89_scalar floor_y);
vm89_scalar fly89_height(fly89_context *context,
                         void *actor,
                         vm89_scalar floor_y);

#ifdef __cplusplus
}
#endif

#endif
