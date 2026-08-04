#ifndef G3DWEAPONZEROING89_H
#define G3DWEAPONZEROING89_H

#ifdef __cplusplus
extern "C" {
#endif

typedef long g3dz89_fx;

typedef struct G3DZ89_Vec3 {
    g3dz89_fx x, y, z;
} G3DZ89_Vec3;

enum {
    G3DZ89_VIEW_FPS = 0,
    G3DZ89_VIEW_TPS = 1
};

enum {
    G3DZ89_PHYSICS_LINEAR = 0,
    G3DZ89_PHYSICS_GRAVITY = 1,
    G3DZ89_PHYSICS_BOLT = 2
};

enum {
    G3DZ89_INPUT_HIT_VALID = 1u << 0
};

enum {
    G3DZ89_RESULT_TARGET_REBUILT = 1u << 0,
    G3DZ89_RESULT_HEMISPHERE_REPAIRED = 1u << 1,
    G3DZ89_RESULT_BALLISTIC_COMPENSATED = 1u << 2
};

typedef struct G3DZ89_Config {
    g3dz89_fx tps_zero_distance;
    g3dz89_fx tps_muzzle_clearance;
    g3dz89_fx min_flight_time;
    g3dz89_fx max_flight_time;
    int ballistic_iterations;
} G3DZ89_Config;

typedef struct G3DZ89_Request {
    G3DZ89_Vec3 camera_origin;
    G3DZ89_Vec3 camera_forward;
    G3DZ89_Vec3 muzzle_origin;
    G3DZ89_Vec3 target_point;
    G3DZ89_Vec3 fallback_direction;
    g3dz89_fx range;
    g3dz89_fx speed;
    g3dz89_fx gravity;
    int view_style;
    int physics_mode;
    unsigned long flags;
} G3DZ89_Request;

typedef struct G3DZ89_Result {
    G3DZ89_Vec3 target_point;
    G3DZ89_Vec3 launch_direction;
    g3dz89_fx gravity;
    unsigned long flags;
    int hit_valid;
} G3DZ89_Result;

void g3dz89_config_defaults(G3DZ89_Config *config);
int g3dz89_solve(const G3DZ89_Config *config,
                 const G3DZ89_Request *request,
                 G3DZ89_Result *result);

#ifdef __cplusplus
}
#endif
#endif
