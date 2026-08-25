#ifndef VEHICLEPROVIDER89_H
#define VEHICLEPROVIDER89_H

#include "gveh_types.h"
#include "gveh_math.h"
#include "gveh_drive.h"

#ifdef __cplusplus
extern "C" {
#endif

struct gveh_runtime_s;
struct gveh_vehicle_s;

#define VEHICLEPROVIDER89_DECLINED 0
#define VEHICLEPROVIDER89_HANDLED  1

#define VEHICLEPROVIDER89_MODULE_CAR   (1UL << 0)
#define VEHICLEPROVIDER89_MODULE_TANK  (1UL << 1)
#define VEHICLEPROVIDER89_MODULE_WATER (1UL << 2)
#define VEHICLEPROVIDER89_MODULE_AIR   (1UL << 3)
#define VEHICLEPROVIDER89_MODULE_SPACE (1UL << 4)
#define VEHICLEPROVIDER89_MODULE_ALL   (0x1FUL)

#define VEHICLEPROVIDER89_PHYS_GRAVITY     (1UL << 0)
#define VEHICLEPROVIDER89_PHYS_MASSPOINTS  (1UL << 1)
#define VEHICLEPROVIDER89_PHYS_LINEAR_DRAG (1UL << 2)
#define VEHICLEPROVIDER89_PHYS_INTEGRATE   (1UL << 3)
#define VEHICLEPROVIDER89_PHYS_ALL          (0x0FUL)

typedef struct vehicleprovider89_movement_request_s {
    struct gveh_runtime_s *runtime;
    struct gveh_vehicle_s *vehicle;
    const gveh_input *input;
    gveh_basis basis;
    gveh_fx dt;
    gveh_u32 module;
} vehicleprovider89_movement_request;

typedef struct vehicleprovider89_physics_request_s {
    struct gveh_runtime_s *runtime;
    struct gveh_vehicle_s *vehicle;
    const gveh_input *input;
    gveh_basis basis;
    gveh_fx dt;
    gveh_u32 phase;
} vehicleprovider89_physics_request;

typedef gveh_i32 (*vehicleprovider89_movement_step_fn)(
    void *user, const vehicleprovider89_movement_request *request);

typedef gveh_i32 (*vehicleprovider89_physics_step_fn)(
    void *user, const vehicleprovider89_physics_request *request);

typedef struct vehicleprovider89_movement_s {
    void *user;
    gveh_u32 module_mask;
    vehicleprovider89_movement_step_fn step;
} vehicleprovider89_movement;

typedef struct vehicleprovider89_physics_s {
    void *user;
    gveh_u32 phase_mask;
    vehicleprovider89_physics_step_fn step;
} vehicleprovider89_physics;

void vehicleprovider89_movement_clear(vehicleprovider89_movement *provider);
void vehicleprovider89_physics_clear(vehicleprovider89_physics *provider);

gveh_i32 vehicleprovider89_movement_try(
    const vehicleprovider89_movement *provider,
    const vehicleprovider89_movement_request *request);

gveh_i32 vehicleprovider89_physics_try(
    const vehicleprovider89_physics *provider,
    const vehicleprovider89_physics_request *request);

#ifdef __cplusplus
}
#endif

#endif
