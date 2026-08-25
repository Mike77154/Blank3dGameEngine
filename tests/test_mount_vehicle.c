#include "blank3d_mount_vehicle.h"
#include <stdio.h>

static int near_value(g3d_fix a, g3d_fix b, g3d_fix eps)
{
    return g3d_fix_abs(g3d_fix_sub_sat(a, b)) <= eps;
}

static g3d_fix flat_dot(g3d_fix dx, g3d_fix dz, Vec3 dir)
{
    g3d_fix xpart;
    g3d_fix zpart;
    xpart = g3d_fix_mul(dx, dir.x);
    zpart = g3d_fix_mul(dz, dir.z);
    return g3d_fix_add_sat(xpart, zpart);
}

int main(void)
{
    Blank3DMountVehicle vehicle;
    Transform player;
    const Transform *car;
    Vec3 car_forward;
    g3d_fix start_x;
    g3d_fix start_z;
    g3d_fix min_y;
    g3d_fix max_y;
    g3d_fix dx;
    g3d_fix dz;
    g3d_fix dt;
    int rc;
    int i;

    transform_init(&player);
    player.position = gamlib_vec3(G3D_FIX_FROM_INT(20), 0,
                                  G3D_FIX_FROM_INT(20));
    if (!blank3d_mount_vehicle_init(&vehicle, &player, 1)) return 1;
    if (!blank3d_mount_vehicle_set_profile(&vehicle, "warthog89")) return 2;
    blank3d_mount_vehicle_reset(&vehicle,
                                G3D_FIX_FROM_INT(4),
                                (g3d_fix)(5L * G3D_FIX_ONE / 2L),
                                G3D_FIX_FROM_INT(6));
    if (blank3d_mount_vehicle_seat_count(&vehicle) != 3) return 3;
    if (blank3d_mount_vehicle_toggle(&vehicle) !=
        B3D_MOUNT_VEHICLE_NOT_NEAR) return 4;

    /* Regression: spawning exactly at the old 2.5-unit suspension threshold
       used to create a perpetual parked bounce.  The adapter must prime a
       small positive suspension compression and stay settled with no driver. */
    dt = (g3d_fix)(G3D_FIX_ONE / 60L);
    car = blank3d_mount_vehicle_car_transform_const(&vehicle);
    if (!car) return 5;
    min_y = car->position.y;
    max_y = car->position.y;
    for (i = 0; i < 240; ++i) {
        if (!blank3d_mount_vehicle_update(&vehicle, dt)) return 6;
        car = blank3d_mount_vehicle_car_transform_const(&vehicle);
        if (car->position.y < min_y) min_y = car->position.y;
        if (car->position.y > max_y) max_y = car->position.y;
    }
    if (g3d_fix_sub_sat(max_y, min_y) > (g3d_fix)(G3D_FIX_ONE / 16L))
        return 7;

    player.position = car->position;
    rc = blank3d_mount_vehicle_toggle(&vehicle);
    if (rc != B3D_MOUNT_VEHICLE_OK) return 8;
    if (!blank3d_mount_vehicle_is_mounted(&vehicle)) return 9;
    if (blank3d_mount_vehicle_driver(&vehicle) != 1) return 10;

    if (!blank3d_mount_vehicle_update(&vehicle, dt)) return 11;
    car = blank3d_mount_vehicle_car_transform_const(&vehicle);
    if (!near_value(player.position.x, car->position.x,
                    G3D_FIX_FROM_INT(2))) return 12;

    /* Regression: gvehicle89 yaw 0 is +Z while Blank3D/Gamlib yaw 0 faces -Z.
       Positive throttle must move along the Blank3D Transform's forward axis,
       never visually out of the rear of the box. */
    transform_get_local_axes(car, 0, 0, &car_forward);
    start_x = car->position.x;
    start_z = car->position.z;
    for (i = 0; i < 240; ++i) {
        blank3d_mount_vehicle_forward(&vehicle, dt, 0);
        if (!blank3d_mount_vehicle_update(&vehicle, dt)) return 13;
    }
    car = blank3d_mount_vehicle_car_transform_const(&vehicle);
    dx = g3d_fix_sub_sat(car->position.x, start_x);
    dz = g3d_fix_sub_sat(car->position.z, start_z);
    if (flat_dot(dx, dz, car_forward) <= 0) return 14;
    if (blank3d_mount_vehicle_sim_speed(&vehicle) <= 0) return 15;
    if (!near_value(player.position.x, car->position.x,
                    G3D_FIX_FROM_INT(2))) return 16;
    if (!near_value(player.position.z, car->position.z,
                    G3D_FIX_FROM_INT(3))) return 17;

    /* Existing drivetrain data contains reverse gear 0.  Backward input must
       brake the forward motion, select reverse near rest, then produce motion
       opposite the Blank3D forward axis without direct Transform writes. */
    transform_get_local_axes(car, 0, 0, &car_forward);
    start_x = car->position.x;
    start_z = car->position.z;
    for (i = 0; i < 600; ++i) {
        blank3d_mount_vehicle_backward(&vehicle, dt, 0);
        if (!blank3d_mount_vehicle_update(&vehicle, dt)) return 18;
    }
    car = blank3d_mount_vehicle_car_transform_const(&vehicle);
    dx = g3d_fix_sub_sat(car->position.x, start_x);
    dz = g3d_fix_sub_sat(car->position.z, start_z);
    if (flat_dot(dx, dz, car_forward) >= 0) return 19;

    rc = blank3d_mount_vehicle_toggle(&vehicle);
    if (rc != B3D_MOUNT_VEHICLE_OK) return 20;
    if (blank3d_mount_vehicle_is_mounted(&vehicle)) return 21;
    if (blank3d_mount_vehicle_driver(&vehicle) != GVPOS_ID_NONE) return 22;
    if (!near_value(player.position.y, 0, G3D_FIX_EPSILON)) return 23;

    printf("gvehpos89 -> mount89 -> gvehicle89 stack: PASS\n");
    printf("vehicle parked-suspension stability: PASS\n");
    printf("Blank3D forward-axis bridge: PASS\n");
    printf("gvehicle89 reverse gear 0 bridge: PASS\n");
    return 0;
}
