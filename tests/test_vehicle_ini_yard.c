#include "blank3d_vehicle_system.h"

#include <stdio.h>

static g3d_fix dot_xz(Vec3 a, Vec3 b)
{
    return g3d_fix_add_sat(g3d_fix_mul(a.x, b.x),
                           g3d_fix_mul(a.z, b.z));
}

static int drive_nearby_forward(Blank3DVehicleSystem *system,
                                Blank3DVehicleInstance *instance,
                                Transform *player, g3d_fix dt, int frames)
{
    const Transform *start;
    const Transform *end;
    Transform start_copy;
    Vec3 forward;
    Vec3 delta;
    int i;
    if (!system || !instance || !player) return 0;
    player->position.x = instance->mount.car.position.x;
    player->position.y = 0;
    player->position.z = instance->mount.car.position.z;
    if (blank3d_vehicle_system_toggle_player(system) != B3D_MOUNT_VEHICLE_OK)
        return 0;
    start = blank3d_mount_vehicle_car_transform_const(&instance->mount);
    if (!start) return 0;
    start_copy = *start;
    transform_get_local_axes(&start_copy, 0, 0, &forward);
    for (i = 0; i < frames; ++i) {
        blank3d_vehicle_system_forward(system, dt, 0);
        if (!blank3d_vehicle_system_update(system, dt)) return 0;
    }
    end = blank3d_mount_vehicle_car_transform_const(&instance->mount);
    if (!end || blank3d_mount_vehicle_sim_speed(&instance->mount) <= 0) return 0;
    delta.x = g3d_fix_sub_sat(end->position.x, start_copy.position.x);
    delta.y = 0;
    delta.z = g3d_fix_sub_sat(end->position.z, start_copy.position.z);
    if (dot_xz(delta, forward) <= 0) return 0;
    if (blank3d_vehicle_system_toggle_player(system) != B3D_MOUNT_VEHICLE_OK)
        return 0;
    return 1;
}

int main(void)
{
    Blank3DVehicleSystem system;
    Blank3DVehicleInstance *car;
    Blank3DVehicleInstance *bike;
    Blank3DVehicleInstance *tank;
    Transform player;
    const Transform *start;
    const Transform *end;
    Transform start_copy;
    Vec3 forward;
    Vec3 right_axis;
    Vec3 delta;
    Transform turn_start;
    const Transform *turn_end;
    gveh_vehicle *sim;
    gveh_fx first_reverse_speed;
    gveh_fx final_reverse_speed;
    g3d_fix dt;
    g3d_fix speed_early;
    g3d_fix speed_late;
    int i;
    char status[160];

    transform_init(&player);
    player.position = gamlib_vec3(G3D_FIX_FROM_INT(5), 0,
                                  G3D_FIX_FROM_INT(7));
    if (!blank3d_vehicle_system_init(&system, &player, 1)) return 1;
    if (!blank3d_vehicle_system_spawn_ini(&system,
            "config/vehicles/test_car.ini",
            G3D_FIX_FROM_INT(5), G3D_FIX_FROM_INT(5) / 2,
            G3D_FIX_FROM_INT(7), status, sizeof(status))) return 2;
    if (!blank3d_vehicle_system_spawn_ini(&system,
            "config/vehicles/test_motorcycle.ini",
            -G3D_FIX_FROM_INT(5), G3D_FIX_FROM_INT(23) / 10,
            G3D_FIX_FROM_INT(8), status, sizeof(status))) return 3;
    if (!blank3d_vehicle_system_spawn_ini(&system,
            "config/vehicles/test_tank.ini",
            G3D_FIX_FROM_INT(14), G3D_FIX_ONE,
            G3D_FIX_FROM_INT(9), status, sizeof(status))) return 4;
    if (blank3d_vehicle_system_count(&system) != 3) return 5;
    car = blank3d_vehicle_system_at(&system, 0);
    bike = blank3d_vehicle_system_at(&system, 1);
    tank = blank3d_vehicle_system_at(&system, 2);
    if (!car || !bike || !tank) return 6;
    if (car->config.movement != B3D_VEHICLE_MOVE_CAR) return 7;
    if (bike->config.movement != B3D_VEHICLE_MOVE_MOTORCYCLE) return 8;
    if (!bike->specialized_movement.step) return 9;
    if (tank->config.movement != B3D_VEHICLE_MOVE_TANK) return 10;
    if (!car->config.playerdriving || !car->config.npcdriving) return 11;
    if (car->config.steer_sign != -1) return 12;

    dt = G3D_FIX_ONE / 60;
    /* Regression for the spawn-time "mechanical bull" bug.  All three
       authored ground vehicles must remain effectively motionless for ten
       idle seconds before any actor mounts them.  The tolerance is tiny on
       purpose: spawn priming should establish support, not merely damp a
       large oscillation eventually. */
    {
        g3d_fix idle_min_y[3];
        g3d_fix idle_max_y[3];
        g3d_fix idle_tolerance;
        int j;
        for (j = 0; j < 3; ++j) {
            idle_min_y[j] = system.vehicles[j].mount.car.position.y;
            idle_max_y[j] = idle_min_y[j];
        }
        for (i = 0; i < 600; ++i) {
            if (!blank3d_vehicle_system_update(&system, dt)) return 13;
            for (j = 0; j < 3; ++j) {
                g3d_fix idle_y;
                idle_y = system.vehicles[j].mount.car.position.y;
                if (idle_y < idle_min_y[j]) idle_min_y[j] = idle_y;
                if (idle_y > idle_max_y[j]) idle_max_y[j] = idle_y;
            }
        }
        idle_tolerance = G3D_FIX_ONE / 64;
        for (j = 0; j < 3; ++j) {
            if (g3d_fix_sub_sat(idle_max_y[j], idle_min_y[j]) > idle_tolerance)
                return 32 + j;
        }
    }

    if (blank3d_vehicle_system_toggle_player(&system) !=
        B3D_MOUNT_VEHICLE_OK) return 14;
    if (!blank3d_vehicle_system_playerdriving(&system)) return 15;
    start = blank3d_mount_vehicle_car_transform_const(&car->mount);
    if (!start) return 16;
    start_copy = *start;
    transform_get_local_axes(&start_copy, 0, 0, &forward);

    speed_early = 0;
    speed_late = 0;
    for (i = 0; i < 180; ++i) {
        blank3d_vehicle_system_forward(&system, dt, 0);
        if (!blank3d_vehicle_system_update(&system, dt)) return 17;
        if (i == 20) speed_early = blank3d_mount_vehicle_sim_speed(&car->mount);
        if (i == 150) speed_late = blank3d_mount_vehicle_sim_speed(&car->mount);
    }
    if (speed_late <= speed_early) return 18;
    if (speed_late <= G3D_FIX_FROM_INT(7)) return 19;
    end = blank3d_mount_vehicle_car_transform_const(&car->mount);
    if (!end) return 20;
    delta.x = g3d_fix_sub_sat(end->position.x, start_copy.position.x);
    delta.y = 0;
    delta.z = g3d_fix_sub_sat(end->position.z, start_copy.position.z);
    if (dot_xz(delta, forward) <= 0) return 21;

    /* Steering bridge is deliberately inverted at the INI boundary so a
       RIGHT request maps correctly across Blank3D/gvehicle handedness.
       Verify the physical result, not only the authored request sign. */
    turn_start = *end;
    transform_get_local_axes(&turn_start, &right_axis, 0, 0);
    for (i = 0; i < 120; ++i) {
        blank3d_vehicle_system_forward(&system, dt, 0);
        blank3d_vehicle_system_steer(&system, dt, 1);
        if (!blank3d_vehicle_system_update(&system, dt)) return 22;
    }
    turn_end = blank3d_mount_vehicle_car_transform_const(&car->mount);
    if (!turn_end) return 23;
    delta.x = g3d_fix_sub_sat(turn_end->position.x, turn_start.position.x);
    delta.y = 0;
    delta.z = g3d_fix_sub_sat(turn_end->position.z, turn_start.position.z);
    if (dot_xz(delta, right_axis) <= 0) return 24;

    /* Hold reverse long enough to brake from forward motion, select gear 0,
       and prove that reverse velocity grows rather than remaining a tiny
       fixed nudge. */
    sim = gveh_vehicle_world_get(&car->mount.vehicle_world,
                                 car->mount.vehicle_id);
    if (!sim) return 25;
    first_reverse_speed = 0;
    final_reverse_speed = 0;
    for (i = 0; i < 480; ++i) {
        blank3d_vehicle_system_backward(&system, dt, 0);
        if (!blank3d_vehicle_system_update(&system, dt)) return 26;
        if (sim->speed_forward < -GVEH_FX_ONE && first_reverse_speed == 0)
            first_reverse_speed = -sim->speed_forward;
        if (i == 479 && sim->speed_forward < 0)
            final_reverse_speed = -sim->speed_forward;
    }
    if (first_reverse_speed <= 0 || final_reverse_speed <= first_reverse_speed ||
        final_reverse_speed <= gveh_fx_from_int(6))
        return 27;

    if (blank3d_vehicle_system_toggle_player(&system) !=
        B3D_MOUNT_VEHICLE_OK) return 28;
    if (blank3d_vehicle_system_playerdriving(&system)) return 29;

    if (!drive_nearby_forward(&system, bike, &player, dt, 180)) return 30;
    if (!drive_nearby_forward(&system, tank, &player, dt, 180)) return 31;

    printf("Vehicle INI yard: PASS (car + motorcycle + tank)\n");
    printf("Idle spawn stability (600 frames): PASS\n");
    printf("Motorcycle + tank independent drive paths: PASS\n");
    printf("PlayerDriving DDSL authority seam: PASS\n");
    printf("Progressive throttle + physical right-steer: PASS\n");
    printf("Progressive reverse gear growth: PASS\n");
    return 0;
}
