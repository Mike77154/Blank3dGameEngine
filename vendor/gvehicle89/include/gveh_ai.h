#ifndef GVEH_AI_H
#define GVEH_AI_H

#include "gveh_math.h"

typedef struct gveh_input_s {
    gveh_fx throttle;
    gveh_fx brake;
    gveh_fx steer;
    gveh_fx handbrake;
    gveh_fx nitro;
    gveh_fx risk;
    gveh_fx aggression;
    gveh_fx crashbreak;
    gveh_fx aftertouch_x;
    gveh_fx aftertouch_z;
    gveh_fx collective;
    gveh_fx air_pitch;
    gveh_fx air_roll;
    gveh_fx air_yaw;
    gveh_fx fire;
    gveh_fx lock_on;
    gveh_fx winch;
    gveh_fx alt_hold;
    gveh_fx tank_turret;
    gveh_fx tank_gun;
    gveh_fx tank_range;
    gveh_fx tank_ammo_next;
    gveh_fx tank_station_next;
    gveh_fx tank_hatch;
    gveh_fx tank_smoke;
    gveh_fx tank_order;
    gveh_fx tank_face;
} gveh_input;

typedef struct gveh_ai_driver_s {
    gveh_vec3 target;
    gveh_fx target_speed;
    gveh_fx arrive_radius;
    gveh_fx avoid_radius;
} gveh_ai_driver;

typedef struct gveh_seat_s {
    gveh_vec3 local_pos;
    gveh_i16 occupant_id;
    gveh_i16 role;
} gveh_seat;

void gveh_input_clear(gveh_input *i);
void gveh_ai_simple_drive(const gveh_ai_driver *ai, gveh_vec3 pos, gveh_basis basis, gveh_fx speed, gveh_input *out_input);

#endif
