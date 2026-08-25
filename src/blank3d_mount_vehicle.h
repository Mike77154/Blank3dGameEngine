#ifndef BLANK3D_MOUNT_VEHICLE_H
#define BLANK3D_MOUNT_VEHICLE_H

#include "../vendor/gamlib3d/gamlib3d_transform.h"
#include "../vendor/3d_mounting_system89/include/mount89.h"
#include "../vendor/gvehicle89/include/gveh.h"
#include "../vendor/gvehicle89/include/gveh_engine_adapter.h"
#include "../vendor/gvehicle89/include/gveh_profile_bank.h"
#include "../vendor/gvehpos89/include/gvpos.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Mount89 host ids stay in normal int space.  gvehicle89's entity bridge uses
   a signed 16-bit owner id, so it intentionally gets a separate adapter id. */
#define B3D_MOUNT_VEHICLE_HOST_ID 91001
#define B3D_MOUNT_VEHICLE_GVEH_OWNER_ID 9101
#define B3D_MOUNT_VEHICLE_SEAT_POINT_ID 100
#define B3D_MOUNT_VEHICLE_RIDER_MASK 1UL

#define B3D_MOUNT_VEHICLE_GVPOS_ACTORS 4
#define B3D_MOUNT_VEHICLE_GVPOS_VEHICLES 2
#define B3D_MOUNT_VEHICLE_GVPOS_SEATS GVEH_MAX_SEATS
#define B3D_MOUNT_VEHICLE_GVPOS_EVENTS 16

#define B3D_MOUNT_VEHICLE_OK 0
#define B3D_MOUNT_VEHICLE_NOT_NEAR 1
#define B3D_MOUNT_VEHICLE_ERROR -1
#define B3D_MOUNT_VEHICLE_NOT_READY -2

typedef struct Blank3DMountVehicleTag {
    int initialized;
    int player_id;
    int host_id;
    int gveh_owner_id;
    int seat_point_base;
    int mounted;
    int actor_hidden;
    int last_mount_rc;
    int last_gvpos_rc;
    int last_gvpos_event;

    /* Spatial relationship authority. */
    mount89_context mounts;

    /* Vehicle possession/seat authority. */
    GVPos_Context possession;
    GVPos_Actor possession_actors[B3D_MOUNT_VEHICLE_GVPOS_ACTORS];
    GVPos_Vehicle possession_vehicles[B3D_MOUNT_VEHICLE_GVPOS_VEHICLES];
    GVPos_Seat possession_seats[B3D_MOUNT_VEHICLE_GVPOS_SEATS];
    GVPos_Event possession_events[B3D_MOUNT_VEHICLE_GVPOS_EVENTS];
    GVPos_Input driver_input;
    int reverse_requested;
    int drive_request;
    int run_request;
    int steer_request;
    int playerdriving;
    int npcdriving;
    int steer_sign;
    g3d_fix throttle_level;
    g3d_fix throttle_rise;
    g3d_fix throttle_fall;

    /* Vehicle simulation authority. */
    gveh_vehicle_world vehicle_world;
    gveh_profile vehicle_profile;
    gveh_i16 vehicle_id;
    char profile_name[32];
    vehicleprovider89_movement movement_provider;
    vehicleprovider89_physics physics_provider;

    /* Blank3D host-facing transforms. */
    Transform *player;
    Transform car;

    /* Compatibility/tuning values used by the existing DSL prototype. */
    g3d_fix drive_speed;
    g3d_fix run_speed;
    g3d_fix strafe_speed;
    g3d_fix turn_speed;
    g3d_fix mount_radius;
    g3d_fix ground_y;
} Blank3DMountVehicle;

int blank3d_mount_vehicle_init(Blank3DMountVehicle *vehicle,
                               Transform *player,
                               int player_id);
int blank3d_mount_vehicle_init_ex(Blank3DMountVehicle *vehicle,
                                  Transform *player,
                                  int player_id,
                                  int host_id,
                                  int gveh_owner_id,
                                  int seat_point_base);
void blank3d_mount_vehicle_reset(Blank3DMountVehicle *vehicle,
                                 g3d_fix x, g3d_fix y, g3d_fix z);
int blank3d_mount_vehicle_set_profile(Blank3DMountVehicle *vehicle,
                                      const char *profile_name);
const char *blank3d_mount_vehicle_profile(const Blank3DMountVehicle *vehicle);
void blank3d_mount_vehicle_set_speeds(Blank3DMountVehicle *vehicle,
                                      g3d_fix drive_speed,
                                      g3d_fix run_speed);
void blank3d_mount_vehicle_set_driver_policy(Blank3DMountVehicle *vehicle,
                                             int playerdriving,
                                             int npcdriving);
void blank3d_mount_vehicle_set_control_tuning(Blank3DMountVehicle *vehicle,
                                              g3d_fix throttle_rise,
                                              g3d_fix throttle_fall,
                                              int steer_sign);
void blank3d_mount_vehicle_set_movement_provider(
    Blank3DMountVehicle *vehicle, const vehicleprovider89_movement *provider);
void blank3d_mount_vehicle_set_physics_provider(
    Blank3DMountVehicle *vehicle, const vehicleprovider89_physics *provider);
void blank3d_mount_vehicle_clear_providers(Blank3DMountVehicle *vehicle);
int blank3d_mount_vehicle_toggle(Blank3DMountVehicle *vehicle);
int blank3d_mount_vehicle_update(Blank3DMountVehicle *vehicle, g3d_fix dt);
int blank3d_mount_vehicle_is_mounted(const Blank3DMountVehicle *vehicle);
int blank3d_mount_vehicle_player_near(const Blank3DMountVehicle *vehicle);
int blank3d_mount_vehicle_driver(const Blank3DMountVehicle *vehicle);
int blank3d_mount_vehicle_seat_count(const Blank3DMountVehicle *vehicle);
int blank3d_mount_vehicle_last_possession_result(
    const Blank3DMountVehicle *vehicle);

void blank3d_mount_vehicle_forward(Blank3DMountVehicle *vehicle,
                                   g3d_fix dt, int running);
void blank3d_mount_vehicle_backward(Blank3DMountVehicle *vehicle,
                                    g3d_fix dt, int running);
void blank3d_mount_vehicle_strafe(Blank3DMountVehicle *vehicle,
                                  g3d_fix dt, int right);
void blank3d_mount_vehicle_turn(Blank3DMountVehicle *vehicle,
                                g3d_fix dt, int right);

Transform *blank3d_mount_vehicle_car_transform(Blank3DMountVehicle *vehicle);
const Transform *blank3d_mount_vehicle_car_transform_const(
    const Blank3DMountVehicle *vehicle);

g3d_fix blank3d_mount_vehicle_speed(const Blank3DMountVehicle *vehicle);
g3d_fix blank3d_mount_vehicle_sim_speed(const Blank3DMountVehicle *vehicle);

#ifdef __cplusplus
}
#endif

#endif
