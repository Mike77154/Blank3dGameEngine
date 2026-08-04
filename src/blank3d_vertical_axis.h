#ifndef BLANK3D_VERTICAL_AXIS_H
#define BLANK3D_VERTICAL_AXIS_H

#include "../vendor/gamlib3d/gamlib3d_transform.h"
#include "fly89.h"
#include "jump89.h"
#include "airdiver89.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Blank3DVerticalAxisTag {
    vm89_provider_bundle providers;
    fly89_context fly;
    jump89_context jump;
    airdiver89_context airdiver;
    int initialized;
    int gravity_enabled;
    g3d_fix gravity_fall_speed;
    g3d_fix jump_gravity;
} Blank3DVerticalAxis;

typedef struct Blank3DVerticalBodyTag {
    void *actor;
    g3d_fix floor_y;
    g3d_fix spawn_y;
    int born_above_floor;
    int flying_entity;
    int gravity_immune;
    jump89_state jump_state;
    airdiver89_state airdiver_state;
} Blank3DVerticalBody;

void blank3d_vertical_axis_init_gamlib3d(Blank3DVerticalAxis *axis);
void blank3d_vertical_axis_set_providers(
    Blank3DVerticalAxis *axis,
    const vm89_provider_bundle *providers);
void blank3d_vertical_axis_set_physics_provider(
    Blank3DVerticalAxis *axis,
    const vm89_physics_provider *physics_provider);
void blank3d_vertical_axis_configure_gravity(
    Blank3DVerticalAxis *axis,
    int enabled,
    g3d_fix fall_speed,
    g3d_fix jump_gravity);

void blank3d_vertical_body_init(Blank3DVerticalBody *body,
                                void *actor,
                                g3d_fix spawn_y,
                                g3d_fix floor_y);
void blank3d_vertical_body_set_flying(Blank3DVerticalBody *body,
                                      int flying_entity);
void blank3d_vertical_body_set_gravity_immune(Blank3DVerticalBody *body,
                                               int gravity_immune);
void blank3d_vertical_body_set_floor(Blank3DVerticalBody *body,
                                     g3d_fix floor_y);

void blank3d_vertical_axis_tick(Blank3DVerticalAxis *axis,
                                Blank3DVerticalBody *body,
                                g3d_fix dt);
int blank3d_vertical_axis_fly(Blank3DVerticalAxis *axis,
                             Blank3DVerticalBody *body,
                             g3d_fix speed,
                             g3d_fix dt,
                             int direction);
int blank3d_vertical_axis_jump(Blank3DVerticalAxis *axis,
                              Blank3DVerticalBody *body,
                              g3d_fix impulse);
int blank3d_vertical_axis_grounded(Blank3DVerticalAxis *axis,
                                  Blank3DVerticalBody *body);
g3d_fix blank3d_vertical_axis_height(Blank3DVerticalAxis *axis,
                                     Blank3DVerticalBody *body);
int blank3d_vertical_axis_jumping(const Blank3DVerticalBody *body);
int blank3d_vertical_axis_falling(const Blank3DVerticalBody *body);

int blank3d_vertical_axis_save_position(Blank3DVerticalAxis *axis,
                                        Blank3DVerticalBody *body);
int blank3d_vertical_axis_descend_to_y(Blank3DVerticalAxis *axis,
                                       Blank3DVerticalBody *body,
                                       g3d_fix target_y,
                                       g3d_fix speed,
                                       g3d_fix dt,
                                       int *out_reached);
int blank3d_vertical_axis_ram_point(Blank3DVerticalAxis *axis,
                                    Blank3DVerticalBody *body,
                                    const Vec3 *target,
                                    g3d_fix speed,
                                    g3d_fix dt,
                                    int *out_reached);
int blank3d_vertical_axis_return_saved(Blank3DVerticalAxis *axis,
                                       Blank3DVerticalBody *body,
                                       g3d_fix speed,
                                       g3d_fix dt,
                                       int *out_reached);
int blank3d_vertical_axis_saved_position(
    const Blank3DVerticalBody *body,
    Vec3 *out_position);

#ifdef __cplusplus
}
#endif

#endif
