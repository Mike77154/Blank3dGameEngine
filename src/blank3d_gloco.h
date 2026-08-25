#ifndef BLANK3D_GLOCO_H
#define BLANK3D_GLOCO_H

#include "gloco89.h"
#include "3d_movementbaseverbs89.h"
#include "3d_movementbaseverbs89_gamlib3d.h"
#include "blank3d_systems.h"
#include "blank3d_variables.h"
#include "blank3d_vertical_axis.h"
#include "blank3d_collision.h"
#include "blank3d_vphysics.h"

#ifdef __cplusplus
extern "C" {
#endif

#define B3D_GLOCO_MAX_BINDINGS GLOCO_MAX_ACTORS
#define B3D_GLOCO_VPHYS_BASE_ID 72000UL

typedef struct Blank3DGlocoBindingTag {
    int used;
    int gloco_id;
    int actor_id;
    unsigned long thing_owner;
    Transform *transform;
    Blank3DVerticalBody *vertical_body;
    ns_id stamina_value;
    int suspended;
    GLOCO_Input input;
    int last_event;
    int event_value;
} Blank3DGlocoBinding;

typedef struct Blank3DGlocoTag {
    int initialized;
    GLOCO_Context context;
    Blank3DGlocoBinding bindings[B3D_GLOCO_MAX_BINDINGS];
    Blank3DSystems *systems;
    Blank3DVariables *variables;
    Blank3DVerticalAxis *vertical_axis;
    Blank3DCollision *collision;
    Blank3DVPhysics *physics;
    mbv89_gamlib3d_adapter *movement_fallback;
    int stamina_type_id;
    unsigned long event_count;
    unsigned short frame_ms;
    char status[192];
} Blank3DGloco;

void blank3d_gloco_init(Blank3DGloco *gloco,
                        Blank3DSystems *systems,
                        Blank3DVariables *variables,
                        Blank3DVerticalAxis *vertical_axis,
                        mbv89_gamlib3d_adapter *movement_fallback);
void blank3d_gloco_attach_physics(Blank3DGloco *gloco,
                                  Blank3DCollision *collision,
                                  Blank3DVPhysics *physics);
void blank3d_gloco_reset_bindings(Blank3DGloco *gloco);
int blank3d_gloco_bind(Blank3DGloco *gloco,
                       int actor_id,
                       unsigned long thing_owner,
                       Transform *transform,
                       Blank3DVerticalBody *vertical_body,
                       int preset,
                       const char *profile_ini_path);
int blank3d_gloco_unbind(Blank3DGloco *gloco, int actor_id);
void blank3d_gloco_begin_frame(Blank3DGloco *gloco, unsigned short dt_ms);
void blank3d_gloco_set_suspended(Blank3DGloco *gloco,
                                 int actor_id, int suspended);
void blank3d_gloco_tick(Blank3DGloco *gloco, unsigned short dt_ms);
int blank3d_gloco_feed_automotion_position(Blank3DGloco *gloco,
                                           int actor_id,
                                           const Vec3 *current,
                                           const Vec3 *desired,
                                           g3d_fix speed_q12,
                                           int run);
int blank3d_gloco_mbv_base_provider(void *provider_user,
                                    mbv89_context *ctx,
                                    mbv89_actor *actor,
                                    mbv89_base_verb verb,
                                    mbv89_fixed amount);
int blank3d_gloco_actor_game_verb(Blank3DGloco *gloco, int actor_id,
                                    mbv89_game_verb verb);
int blank3d_gloco_mbv_game_provider(Blank3DGloco *gloco,
                                    mbv89_game_verb verb);
void blank3d_gloco_install_movement_verbs(Blank3DGloco *gloco,
                                          mbv89_context *ctx);
const Blank3DGlocoBinding *blank3d_gloco_binding(const Blank3DGloco *gloco,
                                                 int actor_id);
const char *blank3d_gloco_status(const Blank3DGloco *gloco);

#ifdef __cplusplus
}
#endif

#endif
