#ifndef GRECOIL89_H
#define GRECOIL89_H

/*
   grecoil89 - C89 fixed-point recoil solver
   No malloc/free/realloc. No heap ownership. No float/double.
   Caller owns every profile, pattern, state and bridge object.
*/

#ifdef __cplusplus
extern "C" {
#endif

#define GREC_VERSION_MAJOR 1
#define GREC_VERSION_MINOR 1
#define GREC_VERSION_PATCH 0

#define GREC_FP_SHIFT 8
#define GREC_FP_ONE   256
#define GREC_FP_HALF  128

#define GREC_TRUE  1
#define GREC_FALSE 0

#define GREC_FLAG_AIM_RECOIL      1u
#define GREC_FLAG_CAMERA_RECOIL   2u
#define GREC_FLAG_WEAPON_RECOIL   4u
#define GREC_FLAG_SPREAD_BLOOM    8u
#define GREC_FLAG_PATTERN         16u
#define GREC_FLAG_RANDOM_YAW      32u
#define GREC_FLAG_RANDOM_PITCH    64u
#define GREC_FLAG_KEEP_BURST_HEAT 128u

#define GREC_MODE_HIP       0
#define GREC_MODE_ADS       1
#define GREC_MODE_MOUNTED   2
#define GREC_MODE_FIXED_CAM 3
#define GREC_MODE_TURRET    4
#define GREC_MODE_SNIPER    5

#define GREC_AXIS_PITCH 0
#define GREC_AXIS_YAW   1
#define GREC_AXIS_ROLL  2

#define GREC_POS_SIDE 0
#define GREC_POS_UP   1
#define GREC_POS_BACK 2

typedef signed char        grec_s8;
typedef unsigned char      grec_u8;
typedef signed short       grec_s16;
typedef unsigned short     grec_u16;
typedef signed int         grec_s32;
typedef unsigned int       grec_u32;
typedef grec_s32           grec_fp;

typedef struct GRecAngles_s {
    grec_fp pitch;
    grec_fp yaw;
    grec_fp roll;
} GRecAngles;

typedef struct GRecVec3_s {
    grec_fp side;
    grec_fp up;
    grec_fp back;
} GRecVec3;

typedef struct GRecPatternStep_s {
    grec_fp pitch_vel;
    grec_fp yaw_vel;
    grec_fp roll_vel;
    grec_fp side_vel;
    grec_fp up_vel;
    grec_fp back_vel;
    grec_fp spread_add;
} GRecPatternStep;

typedef struct GRecProfile_s {
    const char *name;
    grec_u32 flags;

    grec_fp kick_pitch_vel;
    grec_fp kick_yaw_vel;
    grec_fp kick_roll_vel;
    grec_fp kick_side_vel;
    grec_fp kick_up_vel;
    grec_fp kick_back_vel;

    grec_fp random_pitch_vel;
    grec_fp random_yaw_vel;

    grec_fp angle_spring;
    grec_fp angle_damping;
    grec_fp angle_fire_damping;

    grec_fp pos_spring;
    grec_fp pos_damping;
    grec_fp pos_fire_damping;

    grec_fp max_pitch;
    grec_fp max_yaw;
    grec_fp max_roll;
    grec_fp max_side;
    grec_fp max_up;
    grec_fp max_back;

    grec_fp aim_scale;
    grec_fp camera_scale;
    grec_fp weapon_angle_scale;
    grec_fp weapon_pos_scale;

    grec_fp spread_base;
    grec_fp spread_per_shot;
    grec_fp spread_max;
    grec_fp spread_recover;

    grec_u16 recovery_delay_ticks;
    grec_u16 burst_reset_ticks;
    grec_u16 max_burst_count;

    const GRecPatternStep *pattern;
    grec_u16 pattern_count;
    grec_u16 pattern_loop;
} GRecProfile;

typedef struct GRecContext_s {
    grec_u16 mode;
    grec_fp recoil_scale;
    grec_fp aim_scale;
    grec_fp camera_scale;
    grec_fp weapon_scale;
    grec_fp spread_scale;
} GRecContext;

typedef struct GRecState_s {
    GRecAngles angle_pos;
    GRecAngles angle_vel;
    GRecVec3 weapon_pos;
    GRecVec3 weapon_vel;
    grec_fp spread;
    grec_u16 recovery_ticks;
    grec_u16 no_fire_ticks;
    grec_u16 burst_count;
    grec_u16 shot_index;
    grec_u32 rng;
    grec_u8 active;
} GRecState;

typedef struct GRecOutput_s {
    GRecAngles aim_angles;
    GRecAngles camera_angles;
    GRecAngles weapon_angles;
    GRecVec3 weapon_offset;
    grec_fp spread;
    grec_u16 burst_count;
    grec_u16 shot_index;
    grec_u8 active;
} GRecOutput;

typedef void (*GRecApplyCameraFn)(void *user, const GRecAngles *angles);
typedef void (*GRecApplyAimFn)(void *user, const GRecAngles *angles, grec_fp spread);
typedef void (*GRecApplyWeaponFn)(void *user, const GRecAngles *angles, const GRecVec3 *offset);
typedef void (*GRecEventFn)(void *user, const GRecOutput *out);

typedef struct GRecCallbacks_s {
    void *user;
    GRecApplyCameraFn apply_camera;
    GRecApplyAimFn apply_aim;
    GRecApplyWeaponFn apply_weapon;
    GRecEventFn on_fire;
    GRecEventFn on_update;
} GRecCallbacks;

#define GREC_FP_FROM_INT(x) ((grec_fp)((x) << GREC_FP_SHIFT))
#define GREC_INT_FROM_FP(x) ((int)((x) >> GREC_FP_SHIFT))
#define GREC_FP_FROM_RATIO(n,d) ((grec_fp)(((n) * GREC_FP_ONE) / (d)))
#define GREC_ABS(x) (((x) < 0) ? -(x) : (x))

void grec_state_init(GRecState *st, grec_u32 seed);
void grec_context_default(GRecContext *ctx);
void grec_context_for_mode(GRecContext *ctx, grec_u16 mode);

void grec_fire(GRecState *st, const GRecProfile *profile, const GRecContext *ctx);
void grec_update(GRecState *st, const GRecProfile *profile);
void grec_sample(const GRecState *st, const GRecProfile *profile, const GRecContext *ctx, GRecOutput *out);
void grec_apply_callbacks(const GRecCallbacks *cb, const GRecOutput *out, int fire_event);
void grec_reset(GRecState *st);

int grec_is_active(const GRecState *st);
void grec_clear_burst(GRecState *st);
void grec_set_rng(GRecState *st, grec_u32 seed);

grec_fp grec_mul(grec_fp a, grec_fp b);
grec_fp grec_clamp(grec_fp v, grec_fp mn, grec_fp mx);
grec_fp grec_lerp(grec_fp a, grec_fp b, grec_fp t);
grec_fp grec_apply_scale(grec_fp v, grec_fp s);

const char *grec_version_string(void);

#ifdef __cplusplus
}
#endif

#endif
