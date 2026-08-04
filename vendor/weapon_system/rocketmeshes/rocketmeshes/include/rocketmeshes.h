/* rocketmeshes.h - tiny static projectile/ejection mesh library. CC0. */
/* C89, no heap, no malloc, no floats/doubles in runtime data. */
#ifndef ROCKETMESHES_H
#define ROCKETMESHES_H

#ifdef __cplusplus
extern "C" {
#endif

#define RMESH_Q 8
#define RMESH_ONE (1 << RMESH_Q)
#define RMESH_NO_MESH (-1)

typedef signed short rm_i16;
typedef unsigned short rm_u16;
typedef unsigned char rm_u8;

typedef struct RM_Vertex {
    rm_i16 x;
    rm_i16 y;
    rm_i16 z;
} RM_Vertex;

typedef struct RM_Tri {
    rm_u16 a;
    rm_u16 b;
    rm_u16 c;
    rm_u8 mat;
} RM_Tri;

typedef struct RM_Mesh {
    const char *name;
    const RM_Vertex *v;
    const RM_Tri *t;
    rm_u16 vcount;
    rm_u16 tcount;
    rm_i16 minx, miny, minz;
    rm_i16 maxx, maxy, maxz;
} RM_Mesh;

typedef struct RM_EjectHint {
    rm_i16 fire_fx_mesh;
    rm_i16 fire_debris_mesh;
    rm_i16 reload_eject_mesh;
    rm_i16 state_empty_mesh;
    rm_u8 behavior;
} RM_EjectHint;

/* Declarative visual/gameplay behavior.
   These are NOT real ballistic values; they are small fixed-point hints for a renderer/gameplay layer. */
typedef struct RM_BehaviorProfile {
    rm_u8 propulsion_model;
    rm_u8 eject_rule;
    rm_u8 fin_rule;
    rm_u8 trail_rule;

    rm_i16 fire_fx_mesh;
    rm_i16 fire_debris_mesh;
    rm_i16 reload_eject_mesh;
    rm_i16 state_empty_mesh;

    rm_i16 folded_fin_mesh;
    rm_i16 open_fin_mesh;

    rm_i16 ticks_to_fin_open;
    rm_i16 ticks_to_sustainer_visual;
    rm_i16 smoke_ticks;
    rm_i16 trail_ticks;

    rm_i16 visual_speed_q8;
    rm_i16 visual_arc_gravity_q8;
} RM_BehaviorProfile;

enum {
    RM_MAT_BODY = 0,
    RM_MAT_NOSE = 1,
    RM_MAT_BAND = 2,
    RM_MAT_FIN = 3,
    RM_MAT_MOTOR = 4,
    RM_MAT_TIP = 5,
    RM_MAT_DETAIL = 6,
    RM_MAT_BRASS = 7,
    RM_MAT_EMPTY = 8,
    RM_MAT_SMOKE = 9,
    RM_MAT_COUNT = 10
};

enum {
    RM_EJECT_NONE = 0,
    RM_EJECT_BREECH_CASE_ON_RELOAD = 1,
    RM_EJECT_ROCKET_SMOKE_ONLY = 2,
    RM_EJECT_M202_TUBE_COUNTER = 3,
    RM_EJECT_OPTIONAL_TAIL_CAP = 4,
    RM_EJECT_STATE_SWAP_ONLY = 5
};

enum {
    RM_PROP_NONE = 0,
    RM_PROP_CARTRIDGE_LOW_VELOCITY = 1,
    RM_PROP_ROCKET_TUBE_BURNOUT = 2,
    RM_PROP_ROCKET_CLIP_TUBE = 3,
    RM_PROP_TWO_STAGE_RPG = 4,
    RM_PROP_BOOSTED_GRENADE_NO_SUSTAINER = 5,
    RM_PROP_GENERIC_ROCKET = 6
};

enum {
    RM_FIN_NONE = 0,
    RM_FIN_SPIN_STABILIZED = 1,
    RM_FIN_FIXED_RING = 2,
    RM_FIN_FIXED_FOLDOUT = 3,
    RM_FIN_DEPLOY_AFTER_LAUNCH = 4
};

enum {
    RM_TRAIL_NONE = 0,
    RM_TRAIL_MUZZLE_PUFF_ONLY = 1,
    RM_TRAIL_BACKBLAST_ONLY = 2,
    RM_TRAIL_SHORT_ROCKET_SMOKE = 3,
    RM_TRAIL_DELAYED_SUSTAINER = 4,
    RM_TRAIL_COAST_MINIMAL = 5
};

enum {
    RMESH_GRENADE40_LV = 0,
    RMESH_M74_FLASH = 1,
    RMESH_BAZOOKA_M6A3 = 2,
    RMESH_BAZOOKA_M28 = 3,
    RMESH_RPG7_PG7V = 4,
    RMESH_RPG7_PG7VR = 5,
    RMESH_RPG7_OG7V = 6,
    RMESH_SURVIVAL_RPG7_CONE = 7,
    RMESH_GENERIC_ROCKET_STUB = 8,
    RMESH_SHELL_40MM_SPENT_CASE = 9,
    RMESH_DEBRIS_BAZOOKA_TAIL_CAP = 10,
    RMESH_DEBRIS_ROCKET_SEAL_DISC = 11,
    RMESH_M202_EMPTY_ROCKET_CLIP = 12,
    RMESH_M202_EMPTY_TUBE_CELL = 13,
    RMESH_RPG7_TAIL_FOLDED_FINS = 14,
    RMESH_RPG7_TAIL_OPEN_FINS = 15,
    RMESH_RE_RPG7_EMPTY_SOCKET = 16,
    RMESH_FX_BACKBLAST_RING = 17,
    RMESH_COUNT = 18
};

const RM_Mesh *rm_get_mesh(int mesh_id);
const char *rm_get_mesh_name(int mesh_id);
const RM_EjectHint *rm_get_eject_hint(int projectile_mesh_id);
const RM_BehaviorProfile *rm_get_behavior_profile(int mesh_id);
int rm_behavior_is_projectile(int mesh_id);
const char *rm_get_propulsion_name(int propulsion_model);
const char *rm_get_fin_rule_name(int fin_rule);
const char *rm_get_trail_rule_name(int trail_rule);

#ifdef __cplusplus
}
#endif

#endif
