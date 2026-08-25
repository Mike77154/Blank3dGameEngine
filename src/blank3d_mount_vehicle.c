#include "blank3d_mount_vehicle.h"

#include <limits.h>
#include <string.h>

static mount89_fx b3d_q12_to_q16(g3d_fix value)
{
    if (value > (g3d_fix)(LONG_MAX / 16L)) return (mount89_fx)LONG_MAX;
    if (value < (g3d_fix)(LONG_MIN / 16L)) return (mount89_fx)LONG_MIN;
    return (mount89_fx)(value * 16L);
}

static g3d_fix b3d_q16_to_q12(mount89_fx value)
{
    return (g3d_fix)(value / 16L);
}

static gveh_fx b3d_q12_to_gveh_q8(g3d_fix value)
{
    return (gveh_fx)(value / 16L);
}

static g3d_fix b3d_gveh_q8_to_q12(gveh_fx value)
{
    return (g3d_fix)((long)value * 16L);
}

static GVPos_FP b3d_q12_to_gvpos_q16(g3d_fix value)
{
    return (GVPos_FP)((long)value * 16L);
}

static GVPos_FP b3d_gveh_q8_to_gvpos_q16(gveh_fx value)
{
    return (GVPos_FP)((long)value * 256L);
}

static gveh_fx b3d_gvpos_q16_to_gveh_q8(GVPos_FP value)
{
    return (gveh_fx)(value / 256);
}

static mount89_fx b3d_gveh_q8_to_mount_q16(gveh_fx value)
{
    return (mount89_fx)((long)value * 256L);
}

static mount89_rot b3d_q12_to_q14_rot(g3d_fix value)
{
    long scaled;
    scaled = (long)value * 4L;
    if (scaled > 32767L) scaled = 32767L;
    if (scaled < -32768L) scaled = -32768L;
    return (mount89_rot)scaled;
}

/* Gamlib3D/Blank3D defines local forward as -Z at yaw 0.
   gvehicle89 defines local forward as +Z at yaw 0.  Only yaw needs this
   half-turn basis conversion; pitch/roll keep their ordinary angle mapping. */
static g3d_fix b3d_angle256_to_deg_q12(gveh_i32 angle)
{
    long a;
    a = (long)(angle & 255);
    return (g3d_fix)((a * 360L * G3D_FIX_ONE) / 256L);
}

static g3d_fix b3d_gveh_yaw_to_blank_deg_q12(gveh_i32 angle)
{
    g3d_fix deg;
    deg = b3d_angle256_to_deg_q12(angle);
    deg = g3d_fix_sub_sat(deg, G3D_FIX_FROM_INT(180));
    return g3d_fix_wrap_angle_deg(deg);
}

static gveh_i32 b3d_blank_yaw_to_gveh_angle256(g3d_fix deg)
{
    long wrapped;
    long shifted;
    long turns;
    wrapped = (long)g3d_fix_wrap_angle_deg(deg);
    shifted = wrapped + (180L * G3D_FIX_ONE);
    turns = (shifted * 256L) / (360L * G3D_FIX_ONE);
    return (gveh_i32)(turns & 255L);
}

static void b3d_copy_name(char dst[32], const char *src)
{
    int i;
    i = 0;
    if (src != 0) {
        while (i < 31 && src[i] != '\0') {
            dst[i] = src[i];
            i++;
        }
    }
    dst[i] = '\0';
}

static void b3d_transform_to_mount(const Transform *src,
                                   mount89_transform *dst)
{
    g3d_fix m[16];
    if (!src || !dst) return;
    transform_to_matrix4(src, m);
    dst->p[0] = b3d_q12_to_q16(src->position.x);
    dst->p[1] = b3d_q12_to_q16(src->position.y);
    dst->p[2] = b3d_q12_to_q16(src->position.z);

    /* mount89 stores a row-major 3x3. Gamlib3D exposes column-major 4x4. */
    dst->r[0] = b3d_q12_to_q14_rot(m[0]);
    dst->r[1] = b3d_q12_to_q14_rot(m[4]);
    dst->r[2] = b3d_q12_to_q14_rot(m[8]);
    dst->r[3] = b3d_q12_to_q14_rot(m[1]);
    dst->r[4] = b3d_q12_to_q14_rot(m[5]);
    dst->r[5] = b3d_q12_to_q14_rot(m[9]);
    dst->r[6] = b3d_q12_to_q14_rot(m[2]);
    dst->r[7] = b3d_q12_to_q14_rot(m[6]);
    dst->r[8] = b3d_q12_to_q14_rot(m[10]);
}

static int b3d_mount_get_world(void *user, int object_id,
                               mount89_transform *out_world)
{
    Blank3DMountVehicle *vehicle;
    vehicle = (Blank3DMountVehicle *)user;
    if (!vehicle || !out_world) return 0;
    if (object_id == vehicle->host_id) {
        b3d_transform_to_mount(&vehicle->car, out_world);
        return 1;
    }
    if (object_id == vehicle->player_id && vehicle->player) {
        b3d_transform_to_mount(vehicle->player, out_world);
        return 1;
    }
    return 0;
}

static int b3d_mount_set_world(void *user, int object_id,
                               const mount89_transform *world)
{
    Blank3DMountVehicle *vehicle;
    vehicle = (Blank3DMountVehicle *)user;
    if (!vehicle || !world) return 0;
    if (object_id == vehicle->player_id && vehicle->player) {
        vehicle->player->position.x = b3d_q16_to_q12(world->p[0]);
        vehicle->player->position.y = b3d_q16_to_q12(world->p[1]);
        vehicle->player->position.z = b3d_q16_to_q12(world->p[2]);
        vehicle->player->rotation = vehicle->car.rotation;
        return 1;
    }
    if (object_id == vehicle->host_id) {
        vehicle->car.position.x = b3d_q16_to_q12(world->p[0]);
        vehicle->car.position.y = b3d_q16_to_q12(world->p[1]);
        vehicle->car.position.z = b3d_q16_to_q12(world->p[2]);
        return 1;
    }
    return 0;
}

static void b3d_gveh_write_state(void *user, gveh_i16 owner_id,
                                  const gveh_entity_state *state)
{
    Blank3DMountVehicle *vehicle;
    vehicle = (Blank3DMountVehicle *)user;
    if (!vehicle || !state) return;
    if (owner_id != (gveh_i16)vehicle->gveh_owner_id) return;
    vehicle->car.position.x = b3d_gveh_q8_to_q12(state->pos.x);
    vehicle->car.position.y = b3d_gveh_q8_to_q12(state->pos.y);
    vehicle->car.position.z = b3d_gveh_q8_to_q12(state->pos.z);
    vehicle->car.rotation.x = b3d_angle256_to_deg_q12(state->pitch);
    vehicle->car.rotation.y = b3d_gveh_yaw_to_blank_deg_q12(state->yaw);
    vehicle->car.rotation.z = b3d_angle256_to_deg_q12(state->roll);
}


static gveh_i32 b3d_gveh_flat_ground_probe(void *user, gveh_vec3 from,
                                            gveh_fx max_down,
                                            gveh_ground_hit *out_hit)
{
    (void)user;
    if (!out_hit) return GVEH_FALSE;
    if (from.y > max_down) return GVEH_FALSE;
    out_hit->hit = GVEH_TRUE;
    out_hit->point = gveh_v3(from.x, 0, from.z);
    out_hit->normal = gveh_v3(0, GVEH_FX_ONE, 0);
    out_hit->surface_id = GVEH_SURF_ASPHALT_DRY;
    out_hit->water_height = gveh_fx_from_int(-100);
    return GVEH_TRUE;
}

static gveh_i32 b3d_gveh_flat_sphere_probe(void *user, gveh_vec3 center,
                                            gveh_fx radius,
                                            gveh_ground_hit *out_hit)
{
    (void)user;
    if (!out_hit) return GVEH_FALSE;
    if (center.y - radius > 0) return GVEH_FALSE;
    out_hit->hit = GVEH_TRUE;
    out_hit->point = gveh_v3(center.x, 0, center.z);
    out_hit->normal = gveh_v3(0, GVEH_FX_ONE, 0);
    out_hit->surface_id = GVEH_SURF_ASPHALT_DRY;
    out_hit->water_height = gveh_fx_from_int(-100);
    return GVEH_TRUE;
}

static gveh_fx b3d_gveh_no_water(void *user, gveh_fx x, gveh_fx z,
                                  gveh_i32 tick)
{
    (void)user;
    (void)x;
    (void)z;
    (void)tick;
    return gveh_fx_from_int(-100);
}

static GVPos_Bool b3d_gvpos_get_actor_pos(GVPos_Context *ctx,
                                           GVPos_Id object_id,
                                           GVPos_Vec3 *out_pos,
                                           void *user)
{
    Blank3DMountVehicle *vehicle;
    (void)ctx;
    vehicle = (Blank3DMountVehicle *)user;
    if (!vehicle || !out_pos || !vehicle->player) return GVPOS_FALSE;
    if (object_id != vehicle->player_id) return GVPOS_FALSE;
    out_pos->x = b3d_q12_to_gvpos_q16(vehicle->player->position.x);
    out_pos->y = b3d_q12_to_gvpos_q16(vehicle->player->position.y);
    out_pos->z = b3d_q12_to_gvpos_q16(vehicle->player->position.z);
    return GVPOS_TRUE;
}

static GVPos_Bool b3d_gvpos_get_vehicle_pos(GVPos_Context *ctx,
                                             GVPos_Id object_id,
                                             GVPos_Vec3 *out_pos,
                                             void *user)
{
    Blank3DMountVehicle *vehicle;
    (void)ctx;
    vehicle = (Blank3DMountVehicle *)user;
    if (!vehicle || !out_pos) return GVPOS_FALSE;
    if (object_id != vehicle->host_id) return GVPOS_FALSE;
    out_pos->x = b3d_q12_to_gvpos_q16(vehicle->car.position.x);
    out_pos->y = b3d_q12_to_gvpos_q16(vehicle->car.position.y);
    out_pos->z = b3d_q12_to_gvpos_q16(vehicle->car.position.z);
    return GVPOS_TRUE;
}

static void b3d_gvpos_hidden(GVPos_Context *ctx, GVPos_Id actor_id,
                             int hidden, void *user)
{
    Blank3DMountVehicle *vehicle;
    (void)ctx;
    vehicle = (Blank3DMountVehicle *)user;
    if (!vehicle || actor_id != vehicle->player_id) return;
    vehicle->actor_hidden = hidden ? 1 : 0;
}

static void b3d_gvpos_attach(GVPos_Context *ctx, GVPos_Id actor_id,
                             GVPos_Id vehicle_id, int seat_slot,
                             int attached, void *user)
{
    Blank3DMountVehicle *vehicle;
    int rc;
    Vec3 right;
    (void)ctx;
    vehicle = (Blank3DMountVehicle *)user;
    if (!vehicle || actor_id != vehicle->player_id ||
        vehicle_id != vehicle->host_id) return;

    if (attached) {
        rc = mount89_mount(&vehicle->mounts,
                           vehicle->host_id,
                           vehicle->seat_point_base + seat_slot,
                           vehicle->player_id,
                           B3D_MOUNT_VEHICLE_RIDER_MASK,
                           MOUNT89_MOUNT_SNAP,
                           MOUNT89_INHERIT_ALL,
                           0, 0);
        vehicle->last_mount_rc = rc;
        if (rc == MOUNT89_OK) {
            vehicle->mounted = 1;
            (void)mount89_update(&vehicle->mounts);
        }
        return;
    }

    rc = mount89_unmount(&vehicle->mounts, vehicle->player_id,
                         MOUNT89_UNMOUNT_KEEP_WORLD);
    vehicle->last_mount_rc = rc;
    if (rc == MOUNT89_OK || rc == MOUNT89_ERR_LINK_NOT_FOUND) {
        vehicle->mounted = 0;
        if (vehicle->player) {
            transform_get_local_axes(&vehicle->car, &right, 0, 0);
            vehicle->player->position.x = g3d_fix_add_sat(
                vehicle->car.position.x,
                g3d_fix_mul(right.x, G3D_FIX_FROM_INT(3)));
            vehicle->player->position.z = g3d_fix_add_sat(
                vehicle->car.position.z,
                g3d_fix_mul(right.z, G3D_FIX_FROM_INT(3)));
            vehicle->player->position.y = vehicle->ground_y;
            vehicle->player->rotation = vehicle->car.rotation;
        }
    }
}

static void b3d_gvpos_vehicle_input(GVPos_Context *ctx,
                                    GVPos_Id vehicle_id,
                                    GVPos_Id driver_actor_id,
                                    const GVPos_Input *input,
                                    void *user)
{
    Blank3DMountVehicle *vehicle;
    gveh_input *dst;
    gveh_vehicle *sim;
    gveh_fx stop_threshold;
    (void)ctx;
    vehicle = (Blank3DMountVehicle *)user;
    if (!vehicle || !input) return;
    if (vehicle_id != vehicle->host_id ||
        driver_actor_id != vehicle->player_id || vehicle->vehicle_id < 0)
        return;
    dst = gveh_vehicle_world_input(&vehicle->vehicle_world,
                                   vehicle->vehicle_id);
    sim = gveh_vehicle_world_get(&vehicle->vehicle_world,
                                 vehicle->vehicle_id);
    if (!dst || !sim) return;

    dst->throttle = b3d_gvpos_q16_to_gveh_q8(input->throttle);
    dst->brake = b3d_gvpos_q16_to_gveh_q8(input->brake);
    dst->steer = b3d_gvpos_q16_to_gveh_q8(input->steer);
    dst->handbrake = b3d_gvpos_q16_to_gveh_q8(input->handbrake);

    /* gvehicle89 already authors gear_ratio[0] as reverse.  Select it only
       near a stop; opposite-direction input first acts as a brake, avoiding
       an instantaneous forward/reverse torque flip. */
    stop_threshold = GVEH_FX_ONE;
    if (vehicle->reverse_requested) {
        if (sim->speed_forward > stop_threshold) {
            dst->throttle = 0;
            dst->brake = GVEH_FX_ONE;
        } else {
            sim->profile.drive.gearbox.current_gear = 0;
        }
    } else {
        if (sim->speed_forward < -stop_threshold) {
            dst->throttle = 0;
            dst->brake = GVEH_FX_ONE;
        } else if (sim->profile.drive.gearbox.current_gear == 0) {
            sim->profile.drive.gearbox.current_gear = 1;
        }
    }
}

static void b3d_gvpos_event(GVPos_Context *ctx, const GVPos_Event *event,
                            void *user)
{
    Blank3DMountVehicle *vehicle;
    (void)ctx;
    vehicle = (Blank3DMountVehicle *)user;
    if (!vehicle || !event) return;
    vehicle->last_gvpos_event = event->type;
    vehicle->last_gvpos_rc = event->result;
}

static void b3d_mount_rebuild(Blank3DMountVehicle *vehicle)
{
    int i;
    int count;
    mount89_transform seat;
    if (!vehicle) return;
    mount89_init(&vehicle->mounts);
    mount89_set_transform_provider(&vehicle->mounts,
                                   b3d_mount_get_world,
                                   b3d_mount_set_world,
                                   vehicle);
    count = (int)vehicle->vehicle_profile.seat_count;
    if (count <= 0) count = 1;
    if (count > GVEH_MAX_SEATS) count = GVEH_MAX_SEATS;
    for (i = 0; i < count; ++i) {
        mount89_transform_identity(&seat);
        if (vehicle->vehicle_profile.seat_count > 0) {
            seat.p[0] = b3d_gveh_q8_to_mount_q16(
                vehicle->vehicle_profile.seats[i].local_pos.x);
            seat.p[1] = b3d_gveh_q8_to_mount_q16(
                vehicle->vehicle_profile.seats[i].local_pos.y);
            seat.p[2] = b3d_gveh_q8_to_mount_q16(
                vehicle->vehicle_profile.seats[i].local_pos.z);
        } else {
            seat.p[1] = (mount89_fx)(105L * MOUNT89_FX_ONE / 100L);
            seat.p[2] = (mount89_fx)(-20L * MOUNT89_FX_ONE / 100L);
        }
        vehicle->last_mount_rc = mount89_add_static_point(
            &vehicle->mounts,
            vehicle->host_id,
            vehicle->seat_point_base + i,
            B3D_MOUNT_VEHICLE_RIDER_MASK,
            &seat);
        if (vehicle->last_mount_rc != MOUNT89_OK) return;
    }
}

static int b3d_possession_rebuild(Blank3DMountVehicle *vehicle)
{
    GVPos_Config cfg;
    GVPos_Callbacks cb;
    GVPos_Vec3 actor_pos;
    GVPos_Vec3 vehicle_pos;
    GVPos_Vec3 seat_pos;
    GVPos_Vec3 exit_pos;
    int i;
    int count;
    int flags;
    int rc;
    if (!vehicle || !vehicle->player) return 0;

    gvpos_default_config(&cfg);
    /* Mount89 supplies the actual spatial snap.  No duplicate enter/exit
       animation timer is needed in this first Blank3D bridge. */
    cfg.default_enter_ticks = 0;
    cfg.default_exit_ticks = 0;
    cfg.default_cooldown_ticks = 0;
    cfg.default_entry_radius = (GVPos_FP)(3L * GVPOS_FP_ONE);
    gvpos_init(&vehicle->possession, &cfg);
    rc = gvpos_bind_storage(&vehicle->possession,
        vehicle->possession_actors, B3D_MOUNT_VEHICLE_GVPOS_ACTORS,
        vehicle->possession_vehicles, B3D_MOUNT_VEHICLE_GVPOS_VEHICLES,
        vehicle->possession_seats, B3D_MOUNT_VEHICLE_GVPOS_SEATS,
        vehicle->possession_events, B3D_MOUNT_VEHICLE_GVPOS_EVENTS);
    if (rc != GVPOS_OK) return 0;

    memset(&cb, 0, sizeof(cb));
    cb.get_actor_pos = b3d_gvpos_get_actor_pos;
    cb.get_vehicle_pos = b3d_gvpos_get_vehicle_pos;
    cb.on_event = b3d_gvpos_event;
    cb.set_actor_hidden = b3d_gvpos_hidden;
    /* Despite the upstream callback name, Blank3D maps this to Mount89,
       never to GAttach. */
    cb.set_actor_attached = b3d_gvpos_attach;
    cb.apply_vehicle_input = b3d_gvpos_vehicle_input;
    gvpos_set_callbacks(&vehicle->possession, &cb, vehicle);

    actor_pos.x = b3d_q12_to_gvpos_q16(vehicle->player->position.x);
    actor_pos.y = b3d_q12_to_gvpos_q16(vehicle->player->position.y);
    actor_pos.z = b3d_q12_to_gvpos_q16(vehicle->player->position.z);
    rc = gvpos_add_actor(&vehicle->possession, vehicle->player_id,
        GVPOS_ACTOR_FLAG_PLAYER | GVPOS_ACTOR_FLAG_ALLOW_DRIVE |
        GVPOS_ACTOR_FLAG_ALLOW_RIDE, actor_pos);
    if (rc != GVPOS_OK) return 0;

    vehicle_pos.x = b3d_q12_to_gvpos_q16(vehicle->car.position.x);
    vehicle_pos.y = b3d_q12_to_gvpos_q16(vehicle->car.position.y);
    vehicle_pos.z = b3d_q12_to_gvpos_q16(vehicle->car.position.z);
    rc = gvpos_add_vehicle(&vehicle->possession,
        vehicle->host_id,
        GVPOS_VEHICLE_FLAG_USABLE | GVPOS_VEHICLE_FLAG_AI_ALLOWED,
        0, vehicle_pos);
    if (rc != GVPOS_OK) return 0;

    count = (int)vehicle->vehicle_profile.seat_count;
    if (count <= 0) count = 1;
    if (count > GVEH_MAX_SEATS) count = GVEH_MAX_SEATS;
    for (i = 0; i < count; ++i) {
        if (vehicle->vehicle_profile.seat_count > 0) {
            seat_pos.x = b3d_gveh_q8_to_gvpos_q16(
                vehicle->vehicle_profile.seats[i].local_pos.x);
            seat_pos.y = b3d_gveh_q8_to_gvpos_q16(
                vehicle->vehicle_profile.seats[i].local_pos.y);
            seat_pos.z = b3d_gveh_q8_to_gvpos_q16(
                vehicle->vehicle_profile.seats[i].local_pos.z);
        } else {
            seat_pos.x = 0;
            seat_pos.y = GVPOS_FP_ONE;
            seat_pos.z = 0;
        }
        exit_pos = seat_pos;
        if (i == 0) exit_pos.x += (GVPos_FP)(3L * GVPOS_FP_ONE);
        else if ((i & 1) != 0)
            exit_pos.x -= (GVPos_FP)(3L * GVPOS_FP_ONE);
        else
            exit_pos.x += (GVPos_FP)(3L * GVPOS_FP_ONE);

        flags = GVPOS_SEAT_FLAG_NO_ANIM;
        if (vehicle->playerdriving) flags |= GVPOS_SEAT_FLAG_ALLOW_PLAYER;
        if (vehicle->npcdriving) flags |= GVPOS_SEAT_FLAG_ALLOW_NPC;
        if (i == 0) flags |= GVPOS_SEAT_FLAG_DRIVER;
        else flags |= GVPOS_SEAT_FLAG_PASSENGER;
        rc = gvpos_add_seat(&vehicle->possession,
            vehicle->host_id, i, flags,
            b3d_q12_to_gvpos_q16(vehicle->mount_radius), seat_pos, exit_pos);
        if (rc != GVPOS_OK) return 0;
    }
    gvpos_zero_input(&vehicle->driver_input);
    vehicle->reverse_requested = 0;
    return 1;
}

/* Ground vehicles should enter the simulation already resting instead of
   receiving a launch impulse on frame zero.

   Wheeled gvehicle89 profiles consider a wheel grounded only while
   suspension compression is strictly positive.  The old Blank3D bridge
   forced 0.10 units of compression.  That happened to be tolerable for a
   heavy Warthog, but it stores far too much spring energy in light/scaled
   profiles such as the motorcycle recipe and produces the "mechanical bull"
   oscillation while the vehicle is otherwise idle.

   Prime only ONE Q8 quantum (1/256 unit): enough to make contact positive,
   but too small to preload a visible bounce.  Authored airborne spawns stay
   untouched.

   TankSim already clamps its chassis to y >= 2 after integration.  Apply the
   same floor before the first published frame so a tank authored below that
   floor does not visibly pop upward on its first idle tick. */
static gveh_fx b3d_gveh_prime_ground_spawn_y(const gveh_profile *profile,
                                              gveh_fx requested_y)
{
    gveh_fx contact_y;
    gveh_fx delta;
    gveh_i32 i;

    if (!profile) return requested_y;

    if ((profile->class_flags & GVEH_CLASS_TANKSIM) != 0u &&
        requested_y < gveh_fx_from_int(2))
        requested_y = gveh_fx_from_int(2);

    if (profile->wheel_count <= 0 ||
        (profile->module_flags & GVEH_MODULE_CAR) == 0u)
        return requested_y;

    contact_y = profile->susp.rest_len + profile->wheels[0].radius -
                profile->wheels[0].local_pos.y;
    i = 1;
    while (i < profile->wheel_count) {
        gveh_fx wheel_contact;
        wheel_contact = profile->susp.rest_len + profile->wheels[i].radius -
                        profile->wheels[i].local_pos.y;
        if (wheel_contact < contact_y) contact_y = wheel_contact;
        i++;
    }

    delta = requested_y - contact_y;
    if (delta < 0) delta = -delta;
    if (delta > GVEH_FX_ONE / 4) return requested_y;

    return contact_y - 1;
}

static int b3d_vehicle_sim_rebuild(Blank3DMountVehicle *vehicle,
                                   g3d_fix x, g3d_fix y, g3d_fix z)
{
    gveh_world_i world_i;
    gveh_entity_bridge_i entity_i;
    gveh_vec3 spawn;
    gveh_vehicle *sim;
    if (!vehicle) return 0;

    gveh_vehicle_world_init(&vehicle->vehicle_world);
    if (vehicle->movement_provider.step)
        gveh_vehicle_world_set_movement_provider(&vehicle->vehicle_world,
            &vehicle->movement_provider);
    if (vehicle->physics_provider.step)
        gveh_vehicle_world_set_physics_provider(&vehicle->vehicle_world,
            &vehicle->physics_provider);
    world_i.user = vehicle;
    world_i.ground_probe = b3d_gveh_flat_ground_probe;
    world_i.sphere_probe = b3d_gveh_flat_sphere_probe;
    world_i.water_sample = b3d_gveh_no_water;
    gveh_vehicle_world_set_world(&vehicle->vehicle_world, world_i);

    entity_i.user = vehicle;
    entity_i.read_state = 0; /* gvehicle89 is simulation authority. */
    entity_i.write_state = b3d_gveh_write_state;
    gveh_vehicle_world_set_entity_bridge(&vehicle->vehicle_world, entity_i);

    spawn = gveh_v3(b3d_q12_to_gveh_q8(x),
                    b3d_q12_to_gveh_q8(y),
                    b3d_q12_to_gveh_q8(z));
    spawn.y = b3d_gveh_prime_ground_spawn_y(&vehicle->vehicle_profile,
                                             spawn.y);
    vehicle->vehicle_id = gveh_vehicle_world_spawn(
        &vehicle->vehicle_world, &vehicle->vehicle_profile, spawn,
        (gveh_i16)vehicle->gveh_owner_id);
    if (vehicle->vehicle_id < 0) return 0;

    sim = gveh_vehicle_world_get(&vehicle->vehicle_world,
                                 vehicle->vehicle_id);
    if (sim && vehicle->player) {
        sim->body.yaw = b3d_blank_yaw_to_gveh_angle256(vehicle->player->rotation.y);
        if (vehicle->run_speed > 0)
            sim->profile.max_speed = b3d_q12_to_gveh_q8(vehicle->run_speed);
    }
    /* Publish the initial state immediately. */
    if (sim) {
        gveh_entity_bridge_push(&vehicle->vehicle_world.entity_bridge,
            (gveh_i16)vehicle->gveh_owner_id, sim);
    }
    return 1;
}

int blank3d_mount_vehicle_init_ex(Blank3DMountVehicle *vehicle,
                                  Transform *player,
                                  int player_id,
                                  int host_id,
                                  int gveh_owner_id,
                                  int seat_point_base)
{
    if (!vehicle || !player || player_id <= 0 || host_id <= 0 ||
        gveh_owner_id <= 0 || seat_point_base <= 0) return 0;
    memset(vehicle, 0, sizeof(*vehicle));
    vehicle->initialized = 1;
    vehicle->player_id = player_id;
    vehicle->host_id = host_id;
    vehicle->gveh_owner_id = gveh_owner_id;
    vehicle->seat_point_base = seat_point_base;
    vehicle->player = player;
    vehicle->vehicle_id = -1;
    vehicle->drive_speed = G3D_FIX_FROM_INT(16);
    vehicle->run_speed = G3D_FIX_FROM_INT(19);
    vehicle->strafe_speed = G3D_FIX_FROM_INT(10);
    vehicle->turn_speed = G3D_FIX_FROM_INT(105);
    vehicle->mount_radius = (g3d_fix)(3L * G3D_FIX_ONE);
    vehicle->ground_y = 0;
    vehicle->playerdriving = 1;
    vehicle->npcdriving = 1;
    vehicle->steer_sign = -1;
    vehicle->throttle_rise = (g3d_fix)(3L * G3D_FIX_ONE);
    vehicle->throttle_fall = (g3d_fix)(5L * G3D_FIX_ONE);
    vehicle->throttle_level = 0;
    vehicleprovider89_movement_clear(&vehicle->movement_provider);
    vehicleprovider89_physics_clear(&vehicle->physics_provider);
    transform_init(&vehicle->car);
    if (!blank3d_mount_vehicle_set_profile(vehicle, "warthog89")) return 0;
    b3d_mount_rebuild(vehicle);
    return vehicle->last_mount_rc == MOUNT89_OK ? 1 : 0;
}

int blank3d_mount_vehicle_init(Blank3DMountVehicle *vehicle,
                               Transform *player,
                               int player_id)
{
    return blank3d_mount_vehicle_init_ex(vehicle, player, player_id,
        B3D_MOUNT_VEHICLE_HOST_ID, B3D_MOUNT_VEHICLE_GVEH_OWNER_ID,
        B3D_MOUNT_VEHICLE_SEAT_POINT_ID);
}

int blank3d_mount_vehicle_set_profile(Blank3DMountVehicle *vehicle,
                                      const char *profile_name)
{
    if (!vehicle || !vehicle->initialized || !profile_name ||
        profile_name[0] == '\0') return 0;
    if (!gveh_profile_bank_make_by_name(profile_name,
                                        &vehicle->vehicle_profile))
        return 0;
    b3d_copy_name(vehicle->profile_name, profile_name);
    return 1;
}

const char *blank3d_mount_vehicle_profile(const Blank3DMountVehicle *vehicle)
{
    if (!vehicle || !vehicle->initialized) return "";
    return vehicle->profile_name;
}

void blank3d_mount_vehicle_reset(Blank3DMountVehicle *vehicle,
                                 g3d_fix x, g3d_fix y, g3d_fix z)
{
    if (!vehicle || !vehicle->initialized) return;
    vehicle->mounted = 0;
    vehicle->actor_hidden = 0;
    vehicle->last_mount_rc = MOUNT89_OK;
    vehicle->last_gvpos_rc = GVPOS_OK;
    vehicle->last_gvpos_event = GVPOS_EVENT_NONE;
    vehicle->reverse_requested = 0;
    vehicle->drive_request = 0;
    vehicle->run_request = 0;
    vehicle->steer_request = 0;
    vehicle->throttle_level = 0;
    transform_init(&vehicle->car);
    vehicle->car.position = gamlib_vec3(x, y, z);
    if (vehicle->player) vehicle->car.rotation = vehicle->player->rotation;
    /* y supplied to vehicle profiles is the simulation body center.  Player
       exits return to the gameplay ground plane. */
    vehicle->ground_y = 0;
    b3d_mount_rebuild(vehicle);
    if (!b3d_vehicle_sim_rebuild(vehicle, x, y, z)) {
        vehicle->last_gvpos_rc = GVPOS_ERR_BAD_STATE;
        return;
    }
    if (!b3d_possession_rebuild(vehicle)) {
        vehicle->last_gvpos_rc = GVPOS_ERR_BAD_STORAGE;
        return;
    }
}

void blank3d_mount_vehicle_set_speeds(Blank3DMountVehicle *vehicle,
                                      g3d_fix drive_speed,
                                      g3d_fix run_speed)
{
    gveh_vehicle *sim;
    if (!vehicle || !vehicle->initialized) return;
    if (drive_speed > 0) vehicle->drive_speed = drive_speed;
    if (run_speed > 0) vehicle->run_speed = run_speed;
    if (vehicle->vehicle_id < 0) return;
    sim = gveh_vehicle_world_get(&vehicle->vehicle_world,
                                 vehicle->vehicle_id);
    if (sim && vehicle->run_speed > 0)
        sim->profile.max_speed = b3d_q12_to_gveh_q8(vehicle->run_speed);
}

void blank3d_mount_vehicle_set_driver_policy(Blank3DMountVehicle *vehicle,
                                             int playerdriving,
                                             int npcdriving)
{
    if (!vehicle || !vehicle->initialized) return;
    vehicle->playerdriving = playerdriving ? 1 : 0;
    vehicle->npcdriving = npcdriving ? 1 : 0;
}

void blank3d_mount_vehicle_set_control_tuning(Blank3DMountVehicle *vehicle,
                                              g3d_fix throttle_rise,
                                              g3d_fix throttle_fall,
                                              int steer_sign)
{
    if (!vehicle || !vehicle->initialized) return;
    if (throttle_rise > 0) vehicle->throttle_rise = throttle_rise;
    if (throttle_fall > 0) vehicle->throttle_fall = throttle_fall;
    vehicle->steer_sign = steer_sign < 0 ? -1 : 1;
}

void blank3d_mount_vehicle_set_movement_provider(
    Blank3DMountVehicle *vehicle, const vehicleprovider89_movement *provider)
{
    if (!vehicle || !vehicle->initialized) return;
    if (provider) vehicle->movement_provider = *provider;
    else vehicleprovider89_movement_clear(&vehicle->movement_provider);
    gveh_vehicle_world_set_movement_provider(&vehicle->vehicle_world, provider);
}

void blank3d_mount_vehicle_set_physics_provider(
    Blank3DMountVehicle *vehicle, const vehicleprovider89_physics *provider)
{
    if (!vehicle || !vehicle->initialized) return;
    if (provider) vehicle->physics_provider = *provider;
    else vehicleprovider89_physics_clear(&vehicle->physics_provider);
    gveh_vehicle_world_set_physics_provider(&vehicle->vehicle_world, provider);
}

void blank3d_mount_vehicle_clear_providers(Blank3DMountVehicle *vehicle)
{
    if (!vehicle || !vehicle->initialized) return;
    vehicleprovider89_movement_clear(&vehicle->movement_provider);
    vehicleprovider89_physics_clear(&vehicle->physics_provider);
    gveh_vehicle_world_clear_providers(&vehicle->vehicle_world);
}

int blank3d_mount_vehicle_player_near(const Blank3DMountVehicle *vehicle)
{
    g3d_fix dx;
    g3d_fix dy;
    g3d_fix dz;
    g3d_fix d;
    if (!vehicle || !vehicle->initialized || !vehicle->player ||
        vehicle->vehicle_id < 0) return 0;
    dx = g3d_fix_abs(g3d_fix_sub_sat(vehicle->player->position.x,
                                     vehicle->car.position.x));
    dy = g3d_fix_abs(g3d_fix_sub_sat(vehicle->player->position.y,
                                     vehicle->car.position.y));
    dz = g3d_fix_abs(g3d_fix_sub_sat(vehicle->player->position.z,
                                     vehicle->car.position.z));
    d = g3d_fix_max(dx, dz);
    d = g3d_fix_max(d, dy);
    return d <= vehicle->mount_radius ? 1 : 0;
}

int blank3d_mount_vehicle_toggle(Blank3DMountVehicle *vehicle)
{
    int rc;
    if (!vehicle || !vehicle->initialized || !vehicle->player ||
        vehicle->vehicle_id < 0) return B3D_MOUNT_VEHICLE_NOT_READY;

    if (!vehicle->playerdriving &&
        !gvpos_actor_is_mounted(&vehicle->possession, vehicle->player_id))
        return B3D_MOUNT_VEHICLE_NOT_READY;

    if (gvpos_actor_is_mounted(&vehicle->possession,
                               vehicle->player_id)) {
        rc = gvpos_request_exit(&vehicle->possession, vehicle->player_id,
                                GVPOS_REQ_CHECK_CALLBACK);
        vehicle->last_gvpos_rc = rc;
        return rc == GVPOS_OK ? B3D_MOUNT_VEHICLE_OK
                              : B3D_MOUNT_VEHICLE_ERROR;
    }

    rc = gvpos_request_mount(&vehicle->possession,
        vehicle->player_id, vehicle->host_id, 0,
        GVPOS_REQ_CHECK_DISTANCE | GVPOS_REQ_CHECK_CALLBACK);
    vehicle->last_gvpos_rc = rc;
    if (rc == GVPOS_ERR_TOO_FAR) return B3D_MOUNT_VEHICLE_NOT_NEAR;
    if (rc != GVPOS_OK) return B3D_MOUNT_VEHICLE_ERROR;
    return B3D_MOUNT_VEHICLE_OK;
}

int blank3d_mount_vehicle_update(Blank3DMountVehicle *vehicle, g3d_fix dt)
{
    GVPos_Vec3 p;
    gveh_fx sim_dt;
    int rc;
    if (!vehicle || !vehicle->initialized || vehicle->vehicle_id < 0)
        return 0;

    p.x = b3d_q12_to_gvpos_q16(vehicle->car.position.x);
    p.y = b3d_q12_to_gvpos_q16(vehicle->car.position.y);
    p.z = b3d_q12_to_gvpos_q16(vehicle->car.position.z);
    (void)gvpos_set_vehicle_pos(&vehicle->possession,
                                vehicle->host_id, p);
    if (vehicle->player) {
        p.x = b3d_q12_to_gvpos_q16(vehicle->player->position.x);
        p.y = b3d_q12_to_gvpos_q16(vehicle->player->position.y);
        p.z = b3d_q12_to_gvpos_q16(vehicle->player->position.z);
        (void)gvpos_set_actor_pos(&vehicle->possession,
                                  vehicle->player_id, p);
    }
    {
        g3d_fix target;
        g3d_fix step;
        target = vehicle->drive_request != 0 ? G3D_FIX_ONE : 0;
        if (target > 0 && !vehicle->run_request && vehicle->drive_speed > 0 &&
            vehicle->run_speed > vehicle->drive_speed)
            target = g3d_fix_div(vehicle->drive_speed, vehicle->run_speed);
        if (vehicle->throttle_level < target) {
            step = g3d_fix_mul(vehicle->throttle_rise, dt);
            vehicle->throttle_level = g3d_fix_add_sat(vehicle->throttle_level, step);
            if (vehicle->throttle_level > target) vehicle->throttle_level = target;
        } else if (vehicle->throttle_level > target) {
            step = g3d_fix_mul(vehicle->throttle_fall, dt);
            vehicle->throttle_level = g3d_fix_sub_sat(vehicle->throttle_level, step);
            if (vehicle->throttle_level < target) vehicle->throttle_level = target;
        }
        gvpos_zero_input(&vehicle->driver_input);
        if (vehicle->drive_request != 0) {
            vehicle->reverse_requested = vehicle->drive_request < 0 ? 1 : 0;
            vehicle->driver_input.throttle =
                (GVPos_FP)((long)vehicle->throttle_level * 16L);
        }
        if (vehicle->steer_request != 0) {
            long steer;
            steer = (long)vehicle->steer_request * (long)vehicle->steer_sign;
            vehicle->driver_input.steer = steer > 0 ? GVPOS_FP_ONE : -GVPOS_FP_ONE;
        }
    }
    (void)gvpos_set_actor_input(&vehicle->possession,
                                vehicle->player_id,
                                &vehicle->driver_input);
    gvpos_update(&vehicle->possession);

    sim_dt = b3d_q12_to_gveh_q8(dt);
    if (sim_dt <= 0) sim_dt = GVEH_FX_ONE / 60;
    gveh_vehicle_world_step(&vehicle->vehicle_world, sim_dt);

    rc = mount89_update(&vehicle->mounts);
    vehicle->last_mount_rc = rc;

    /* Inputs are edge/frame authored by Game Verbs.  Never leave throttle or
       steer latched when the DSL stops emitting a verb. */
    gvpos_zero_input(&vehicle->driver_input);
    vehicle->drive_request = 0;
    vehicle->run_request = 0;
    vehicle->steer_request = 0;
    gveh_vehicle_world_clear_inputs(&vehicle->vehicle_world);
    return rc == MOUNT89_OK ? 1 : 0;
}

int blank3d_mount_vehicle_is_mounted(const Blank3DMountVehicle *vehicle)
{
    if (!vehicle || !vehicle->initialized) return 0;
    return vehicle->mounted &&
           mount89_is_mounted(&vehicle->mounts, vehicle->player_id) ? 1 : 0;
}

int blank3d_mount_vehicle_driver(const Blank3DMountVehicle *vehicle)
{
    GVPos_Vehicle *v;
    if (!vehicle || !vehicle->initialized) return GVPOS_ID_NONE;
    v = gvpos_find_vehicle((GVPos_Context *)&vehicle->possession,
                           vehicle->host_id);
    if (!v) return GVPOS_ID_NONE;
    return v->driver_actor_id;
}

int blank3d_mount_vehicle_seat_count(const Blank3DMountVehicle *vehicle)
{
    if (!vehicle || !vehicle->initialized) return 0;
    return vehicle->possession.seat_count;
}

int blank3d_mount_vehicle_last_possession_result(
    const Blank3DMountVehicle *vehicle)
{
    return vehicle && vehicle->initialized ? vehicle->last_gvpos_rc
                                           : GVPOS_ERR_BAD_STATE;
}

void blank3d_mount_vehicle_forward(Blank3DMountVehicle *vehicle,
                                   g3d_fix dt, int running)
{
    (void)dt;
    if (!blank3d_mount_vehicle_is_mounted(vehicle)) return;
    vehicle->drive_request = 1;
    vehicle->run_request = running ? 1 : 0;
}

void blank3d_mount_vehicle_backward(Blank3DMountVehicle *vehicle,
                                    g3d_fix dt, int running)
{
    (void)dt;
    if (!blank3d_mount_vehicle_is_mounted(vehicle)) return;
    vehicle->drive_request = -1;
    vehicle->run_request = running ? 1 : 0;
}

void blank3d_mount_vehicle_strafe(Blank3DMountVehicle *vehicle,
                                  g3d_fix dt, int right)
{
    (void)dt;
    if (!blank3d_mount_vehicle_is_mounted(vehicle)) return;
    vehicle->steer_request = right ? 1 : -1;
}

void blank3d_mount_vehicle_turn(Blank3DMountVehicle *vehicle,
                                g3d_fix dt, int right)
{
    (void)dt;
    if (!blank3d_mount_vehicle_is_mounted(vehicle)) return;
    vehicle->steer_request = right ? 1 : -1;
}

Transform *blank3d_mount_vehicle_car_transform(Blank3DMountVehicle *vehicle)
{
    if (!vehicle || !vehicle->initialized || vehicle->vehicle_id < 0) return 0;
    return &vehicle->car;
}

const Transform *blank3d_mount_vehicle_car_transform_const(
    const Blank3DMountVehicle *vehicle)
{
    if (!vehicle || !vehicle->initialized || vehicle->vehicle_id < 0) return 0;
    return &vehicle->car;
}

g3d_fix blank3d_mount_vehicle_speed(const Blank3DMountVehicle *vehicle)
{
    return vehicle && vehicle->initialized ? vehicle->drive_speed : 0;
}

g3d_fix blank3d_mount_vehicle_sim_speed(const Blank3DMountVehicle *vehicle)
{
    gveh_vehicle *sim;
    if (!vehicle || !vehicle->initialized || vehicle->vehicle_id < 0) return 0;
    sim = gveh_vehicle_world_get((gveh_vehicle_world *)&vehicle->vehicle_world,
                                 vehicle->vehicle_id);
    if (!sim) return 0;
    return b3d_gveh_q8_to_q12(gveh_fx_abs(sim->speed_forward));
}
