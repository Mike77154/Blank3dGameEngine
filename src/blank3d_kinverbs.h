#ifndef BLANK3D_KINVERBS_H
#define BLANK3D_KINVERBS_H

#include "gameverbs89.h"
#include "gk3d.h"
#include "3d_movementbaseverbs89.h"
#include "gamlib3d_transform.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef B3D_KINVERB_MAX_BINDINGS
#define B3D_KINVERB_MAX_BINDINGS 64
#endif

#define B3D_KINVERB_TYPE_ACTOR 1
#define B3D_KINVERB_TYPE_WORLD 2
#define B3D_KINVERB_TYPE_TRIGGER 3

typedef struct Blank3DKinBindingTag {
    int used;
    unsigned long owner;
    int actor_id;
    int gk3d_id;
    Transform *transform;
    g3d_fix half_x_q12;
    g3d_fix height_q12;
    g3d_fix half_z_q12;
} Blank3DKinBinding;

typedef struct Blank3DKinVerbsTag {
    int initialized;
    gk3d_world world;
    gverb89_registry verbs;
    Blank3DKinBinding bindings[B3D_KINVERB_MAX_BINDINGS];
    int binding_count;
    g3d_fix default_probe_q12;
    unsigned long sync_serial;
    char status[192];
} Blank3DKinVerbs;

void blank3d_kinverbs_init(Blank3DKinVerbs *kin);
void blank3d_kinverbs_begin_sync(Blank3DKinVerbs *kin);
int blank3d_kinverbs_add_world_box(Blank3DKinVerbs *kin,
                                   g3d_fix min_x, g3d_fix min_y, g3d_fix min_z,
                                   g3d_fix size_x, g3d_fix size_y, g3d_fix size_z,
                                   unsigned int flags);
int blank3d_kinverbs_sync_actor(Blank3DKinVerbs *kin,
                                unsigned long owner, int actor_id,
                                Transform *transform,
                                g3d_fix half_x_q12, g3d_fix height_q12,
                                g3d_fix half_z_q12,
                                int active);
void blank3d_kinverbs_end_sync(Blank3DKinVerbs *kin);

gverb89_registry *blank3d_kinverbs_registry(Blank3DKinVerbs *kin);
const gverb89_registry *blank3d_kinverbs_registry_const(const Blank3DKinVerbs *kin);

int blank3d_kinverbs_can_game_verb(Blank3DKinVerbs *kin,
                                   unsigned long owner,
                                   mbv89_game_verb verb,
                                   mbv89_fixed amount_q16);
int blank3d_kinverbs_can_named_move(Blank3DKinVerbs *kin,
                                    unsigned long owner,
                                    const char *name,
                                    mbv89_fixed amount_q16);
int blank3d_kinverbs_query(Blank3DKinVerbs *kin,
                           unsigned long owner, void *subject,
                           const char *name,
                           long value_q16, const char *value_text,
                           int has_value, gverb89_result *out);
const char *blank3d_kinverbs_status(const Blank3DKinVerbs *kin);

#ifdef __cplusplus
}
#endif
#endif
