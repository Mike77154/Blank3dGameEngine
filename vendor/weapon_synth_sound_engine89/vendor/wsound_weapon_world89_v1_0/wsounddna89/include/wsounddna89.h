#ifndef WSOUNDDNA89_H
#define WSOUNDDNA89_H
#include "wsound89_common.h"
#ifdef __cplusplus
extern "C" {
#endif

#define WSOUNDDNA89_VERSION_MAJOR 2
#define WSOUNDDNA89_VERSION_MINOR 0
#define WSOUNDDNA89_VERSION_PATCH 0

typedef enum wsounddna89_class_e {
    WSOUNDDNA89_PISTOL = 0,
    WSOUNDDNA89_MACHINE = 1,
    WSOUNDDNA89_RIFLE = 2,
    WSOUNDDNA89_SHOTGUN = 3,
    WSOUNDDNA89_HEAVY = 4
} wsounddna89_class;

typedef enum wsounddna89_profile_id_e {
    WSOUNDDNA89_PROFILE_COMPACT_PISTOL = 0,
    WSOUNDDNA89_PROFILE_SERVICE_PISTOL = 1,
    WSOUNDDNA89_PROFILE_MAGNUM = 2,
    WSOUNDDNA89_PROFILE_SMG = 3,
    WSOUNDDNA89_PROFILE_CARBINE = 4,
    WSOUNDDNA89_PROFILE_RIFLE = 5,
    WSOUNDDNA89_PROFILE_SNIPER = 6,
    WSOUNDDNA89_PROFILE_SHOTGUN = 7,
    WSOUNDDNA89_PROFILE_HEAVY = 8,
    WSOUNDDNA89_PROFILE_LAUNCHER = 9,
    WSOUNDDNA89_PROFILE_COUNT = 10
} wsounddna89_profile_id;

typedef enum wsounddna89_mode_e {
    WSOUNDDNA89_MODE_REALISTIC = 0,
    WSOUNDDNA89_MODE_HYBRID = 1,
    WSOUNDDNA89_MODE_CINEMATIC = 2
} wsounddna89_mode;

typedef enum wsounddna89_action_e {
    WSOUNDDNA89_ACTION_CLOSED_BOLT = 0,
    WSOUNDDNA89_ACTION_OPEN_BOLT = 1,
    WSOUNDDNA89_ACTION_REVOLVER = 2,
    WSOUNDDNA89_ACTION_PUMP = 3,
    WSOUNDDNA89_ACTION_BOLT = 4,
    WSOUNDDNA89_ACTION_LAUNCHER = 5
} wsounddna89_action;

typedef struct wsounddna89_profile_s {
    wsounddna89_profile_id id;
    wsounddna89_class weapon_class;
    wsounddna89_action action;
    wsound89_u16 bore_mm_x100;
    wsound89_u16 barrel_length_mm;
    wsound89_u16 projectile_speed_mps;
    wsound89_u16 cyclic_rate_rpm;
    wsound89_u16 propellant_energy_q15;
    wsound89_u16 muzzle_pressure_q15;
    wsound89_u16 mechanical_mass_q15;
    wsound89_u16 suppressor_q15;
    wsound89_u16 muzzle_brake_q15;
    wsound89_u16 supersonic_q15;
} wsounddna89_profile;

typedef struct wsounddna89_shot_s {
    wsound89_u16 energy_q15;
    wsound89_u16 pressure_q15;
    wsound89_u16 gas_q15;
    wsound89_u16 receiver_q15;
    wsound89_u16 thump_q15;
    wsound89_u16 crack_q15;
    wsound89_u16 brightness_q15;
    wsound89_u16 tail_q15;
    wsound89_i16 energy_variation_q15;
    wsound89_i16 powder_variation_q15;
    wsound89_i16 mechanism_variation_q15;
    wsound89_i16 environment_variation_q15;
    wsound89_u32 pitch_q16;
    wsound89_u32 cycle_q16;
    wsound89_u32 shot_index;
    wsound89_u32 seed;
} wsounddna89_shot;

typedef struct wsounddna89_context_s {
    wsound89_u32 state;
    wsound89_u32 shot_index;
    wsounddna89_profile profile;
    wsounddna89_mode mode;
} wsounddna89_context;

void wsounddna89_profile_defaults(wsounddna89_profile_id id,
                                  wsounddna89_profile *profile);
int wsounddna89_validate_profile(const wsounddna89_profile *profile);
wsound89_result wsounddna89_init(wsounddna89_context *ctx, wsound89_u32 seed);
wsound89_result wsounddna89_set_profile(wsounddna89_context *ctx,
                                        const wsounddna89_profile *profile);
wsound89_result wsounddna89_set_mode(wsounddna89_context *ctx,
                                     wsounddna89_mode mode);
wsound89_result wsounddna89_next_profiled(wsounddna89_context *ctx,
                                          wsounddna89_shot *out_shot);
wsound89_result wsounddna89_next(wsounddna89_context *ctx,
                                 wsounddna89_class weapon_class,
                                 wsound89_u16 base_energy_q15,
                                 wsounddna89_shot *out_shot);
#ifdef __cplusplus
}
#endif
#endif
