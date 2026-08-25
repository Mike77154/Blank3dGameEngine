#ifndef BLANK3D_VEHICLE_SYSTEM_H
#define BLANK3D_VEHICLE_SYSTEM_H

#include "blank3d_mount_vehicle.h"

#ifdef __cplusplus
extern "C" {
#endif

#define B3D_VEHICLE_SYSTEM_MAX 8
#define B3D_VEHICLE_NAME_CAP 32
#define B3D_VEHICLE_PATH_CAP 160

#define B3D_VEHICLE_MOVE_CAR 1
#define B3D_VEHICLE_MOVE_MOTORCYCLE 2
#define B3D_VEHICLE_MOVE_BUS 3
#define B3D_VEHICLE_MOVE_TANK 4
#define B3D_VEHICLE_MOVE_WATER 5
#define B3D_VEHICLE_MOVE_AIR 6
#define B3D_VEHICLE_MOVE_SPACE 7

typedef struct Blank3DVehicleConfigTag {
    char name[B3D_VEHICLE_NAME_CAP];
    char profile[B3D_VEHICLE_NAME_CAP];
    char driver_ddsl[B3D_VEHICLE_PATH_CAP];
    int movement;
    int playerdriving;
    int npcdriving;
    int steer_sign;
    g3d_fix drive_speed;
    g3d_fix run_speed;
    g3d_fix mount_radius;
    g3d_fix throttle_rise;
    g3d_fix throttle_fall;
    g3d_fix torque_scale;
    g3d_fix reverse_scale;
    g3d_fix mass_scale;
    g3d_fix velocity_retention;
    g3d_fix visual_scale_x;
    g3d_fix visual_scale_y;
    g3d_fix visual_scale_z;
    g3d_fix visual_y_offset;
} Blank3DVehicleConfig;

typedef struct Blank3DVehicleInstanceTag {
    int used;
    int slot;
    Blank3DVehicleConfig config;
    Blank3DMountVehicle mount;
    vehicleprovider89_movement specialized_movement;
} Blank3DVehicleInstance;

typedef struct Blank3DVehicleSystemTag {
    int initialized;
    Transform *player;
    int player_id;
    int count;
    int active_player_slot;
    Blank3DVehicleInstance vehicles[B3D_VEHICLE_SYSTEM_MAX];
} Blank3DVehicleSystem;

void blank3d_vehicle_config_defaults(Blank3DVehicleConfig *config);
int blank3d_vehicle_config_load(const char *path,
                                Blank3DVehicleConfig *config,
                                char *status,
                                unsigned int status_capacity);

int blank3d_vehicle_system_init(Blank3DVehicleSystem *system,
                                Transform *player,
                                int player_id);
void blank3d_vehicle_system_clear(Blank3DVehicleSystem *system);
int blank3d_vehicle_system_spawn_ini(Blank3DVehicleSystem *system,
                                     const char *path,
                                     g3d_fix x, g3d_fix y, g3d_fix z,
                                     char *status,
                                     unsigned int status_capacity);
int blank3d_vehicle_system_spawn_profile(Blank3DVehicleSystem *system,
                                         const char *profile_name,
                                         g3d_fix x, g3d_fix y, g3d_fix z,
                                         g3d_fix drive_speed,
                                         g3d_fix run_speed);
int blank3d_vehicle_system_update(Blank3DVehicleSystem *system, g3d_fix dt);
int blank3d_vehicle_system_toggle_player(Blank3DVehicleSystem *system);
int blank3d_vehicle_system_playerdriving(const Blank3DVehicleSystem *system);
Blank3DVehicleInstance *blank3d_vehicle_system_active(Blank3DVehicleSystem *system);
const Blank3DVehicleInstance *blank3d_vehicle_system_active_const(
    const Blank3DVehicleSystem *system);
int blank3d_vehicle_system_count(const Blank3DVehicleSystem *system);
Blank3DVehicleInstance *blank3d_vehicle_system_at(Blank3DVehicleSystem *system,
                                                  int index);
const Blank3DVehicleInstance *blank3d_vehicle_system_at_const(
    const Blank3DVehicleSystem *system, int index);

void blank3d_vehicle_system_forward(Blank3DVehicleSystem *system,
                                    g3d_fix dt, int running);
void blank3d_vehicle_system_backward(Blank3DVehicleSystem *system,
                                     g3d_fix dt, int running);
void blank3d_vehicle_system_steer(Blank3DVehicleSystem *system,
                                  g3d_fix dt, int right);

#ifdef __cplusplus
}
#endif

#endif
