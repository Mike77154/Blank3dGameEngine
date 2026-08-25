#include "blank3d_vehicle_system.h"

#include "../vendor/gvehicle89/vendor/motorcyclemovement89/include/motorcyclemovement89.h"
#include "../vendor/gvehicle89/vendor/busmovement89/include/busmovement89.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <stdlib.h>

static void b3d_vehicle_copy(char *dst, unsigned int cap, const char *src)
{
    unsigned int i;
    if (!dst || cap == 0U) return;
    if (!src) src = "";
    i = 0U;
    while (src[i] != '\0' && i + 1U < cap) {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
}


static void b3d_vehicle_status(char *status, unsigned int cap,
                               const char *prefix, const char *value)
{
    unsigned int used;
    if (!status || cap == 0U) return;
    status[0] = '\0';
    b3d_vehicle_copy(status, cap, prefix ? prefix : "");
    used = (unsigned int)strlen(status);
    if (value && used + 1U < cap)
        b3d_vehicle_copy(status + used, cap - used, value);
}

static char *b3d_vehicle_trim(char *text)
{
    char *end;
    if (!text) return text;
    while (*text && isspace((unsigned char)*text)) ++text;
    end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1])) --end;
    *end = '\0';
    return text;
}

static int b3d_vehicle_parse_bool(const char *text, int *out)
{
    if (!text || !out) return 0;
    if (strcmp(text, "1") == 0 || strcmp(text, "true") == 0 ||
        strcmp(text, "yes") == 0 || strcmp(text, "on") == 0) {
        *out = 1;
        return 1;
    }
    if (strcmp(text, "0") == 0 || strcmp(text, "false") == 0 ||
        strcmp(text, "no") == 0 || strcmp(text, "off") == 0) {
        *out = 0;
        return 1;
    }
    return 0;
}

static int b3d_vehicle_parse_q12(const char *text, g3d_fix *out)
{
    const char *p;
    long whole;
    long frac;
    long scale;
    long value;
    int sign;
    int digits;
    if (!text || !out) return 0;
    p = text;
    sign = 1;
    if (*p == '-') { sign = -1; ++p; }
    else if (*p == '+') ++p;
    whole = 0L;
    digits = 0;
    while (*p >= '0' && *p <= '9') {
        if (whole > LONG_MAX / 10L) return 0;
        whole = whole * 10L + (long)(*p - '0');
        ++p;
        digits = 1;
    }
    frac = 0L;
    scale = 1L;
    if (*p == '.') {
        ++p;
        while (*p >= '0' && *p <= '9' && scale < 1000000L) {
            frac = frac * 10L + (long)(*p - '0');
            scale *= 10L;
            ++p;
            digits = 1;
        }
        while (*p >= '0' && *p <= '9') ++p;
    }
    while (*p && isspace((unsigned char)*p)) ++p;
    if (!digits || *p != '\0') return 0;
    if (whole > LONG_MAX / G3D_FIX_ONE) return 0;
    value = whole * G3D_FIX_ONE;
    if (scale > 1L) value += (frac * G3D_FIX_ONE) / scale;
    if (sign < 0) value = -value;
    if (value > (long)INT_MAX || value < (long)INT_MIN) return 0;
    *out = (g3d_fix)value;
    return 1;
}

static int b3d_vehicle_movement_from_text(const char *text)
{
    if (!text) return 0;
    if (strcmp(text, "car") == 0) return B3D_VEHICLE_MOVE_CAR;
    if (strcmp(text, "motorcycle") == 0 || strcmp(text, "motorbike") == 0 ||
        strcmp(text, "bike") == 0) return B3D_VEHICLE_MOVE_MOTORCYCLE;
    if (strcmp(text, "bus") == 0 || strcmp(text, "heavy") == 0)
        return B3D_VEHICLE_MOVE_BUS;
    if (strcmp(text, "tank") == 0 || strcmp(text, "tracked") == 0)
        return B3D_VEHICLE_MOVE_TANK;
    if (strcmp(text, "water") == 0 || strcmp(text, "boat") == 0)
        return B3D_VEHICLE_MOVE_WATER;
    if (strcmp(text, "air") == 0 || strcmp(text, "aircraft") == 0)
        return B3D_VEHICLE_MOVE_AIR;
    if (strcmp(text, "space") == 0 || strcmp(text, "spacecraft") == 0)
        return B3D_VEHICLE_MOVE_SPACE;
    return 0;
}

void blank3d_vehicle_config_defaults(Blank3DVehicleConfig *config)
{
    if (!config) return;
    memset(config, 0, sizeof(*config));
    b3d_vehicle_copy(config->name, sizeof(config->name), "vehicle");
    b3d_vehicle_copy(config->profile, sizeof(config->profile), "warthog89");
    b3d_vehicle_copy(config->driver_ddsl, sizeof(config->driver_ddsl),
                     "scripts/vehicle_driver.ddsl2");
    config->movement = B3D_VEHICLE_MOVE_CAR;
    config->playerdriving = 1;
    config->npcdriving = 1;
    config->steer_sign = -1;
    config->drive_speed = G3D_FIX_FROM_INT(22);
    config->run_speed = G3D_FIX_FROM_INT(28);
    config->mount_radius = G3D_FIX_FROM_INT(3);
    config->throttle_rise = (g3d_fix)(5L * G3D_FIX_ONE / 2L);
    config->throttle_fall = G3D_FIX_FROM_INT(4);
    config->torque_scale = (g3d_fix)(5L * G3D_FIX_ONE / 2L);
    config->reverse_scale = (g3d_fix)(3L * G3D_FIX_ONE / 2L);
    config->mass_scale = G3D_FIX_ONE;
    config->velocity_retention = (g3d_fix)(94L * G3D_FIX_ONE / 100L);
    config->visual_scale_x = G3D_FIX_ONE;
    config->visual_scale_y = G3D_FIX_ONE;
    config->visual_scale_z = G3D_FIX_ONE;
    config->visual_y_offset = (g3d_fix)(-19L * G3D_FIX_ONE / 10L);
}

int blank3d_vehicle_config_load(const char *path,
                                Blank3DVehicleConfig *config,
                                char *status,
                                unsigned int status_capacity)
{
    FILE *file;
    char line[256];
    int line_no;
    Blank3DVehicleConfig temp;
    if (status && status_capacity > 0U) status[0] = '\0';
    if (!path || !config) return 0;
    blank3d_vehicle_config_defaults(&temp);
    file = fopen(path, "rb");
    if (!file) {
        b3d_vehicle_status(status, status_capacity, "vehicle INI open failed: ", path);
        return 0;
    }
    line_no = 0;
    while (fgets(line, sizeof(line), file)) {
        char *key;
        char *value;
        char *eq;
        int b;
        int movement;
        ++line_no;
        key = b3d_vehicle_trim(line);
        if (*key == '\0' || *key == '#' || *key == ';' || *key == '[') continue;
        eq = strchr(key, '=');
        if (!eq) continue;
        *eq = '\0';
        value = b3d_vehicle_trim(eq + 1);
        key = b3d_vehicle_trim(key);
        if (strcmp(key, "name") == 0) b3d_vehicle_copy(temp.name, sizeof(temp.name), value);
        else if (strcmp(key, "profile") == 0) b3d_vehicle_copy(temp.profile, sizeof(temp.profile), value);
        else if (strcmp(key, "driver_ddsl") == 0 || strcmp(key, "driver_script") == 0)
            b3d_vehicle_copy(temp.driver_ddsl, sizeof(temp.driver_ddsl), value);
        else if (strcmp(key, "movement") == 0) {
            movement = b3d_vehicle_movement_from_text(value);
            if (movement == 0) { fclose(file); return 0; }
            temp.movement = movement;
        } else if (strcmp(key, "playerdriving") == 0 || strcmp(key, "allow_player_driver") == 0) {
            if (!b3d_vehicle_parse_bool(value, &b)) { fclose(file); return 0; }
            temp.playerdriving = b;
        } else if (strcmp(key, "npcdriving") == 0 || strcmp(key, "allow_npc_driver") == 0) {
            if (!b3d_vehicle_parse_bool(value, &b)) { fclose(file); return 0; }
            temp.npcdriving = b;
        } else if (strcmp(key, "invert_steer") == 0) {
            if (!b3d_vehicle_parse_bool(value, &b)) { fclose(file); return 0; }
            temp.steer_sign = b ? -1 : 1;
        } else if (strcmp(key, "steer_sign") == 0) {
            temp.steer_sign = atoi(value) < 0 ? -1 : 1;
        } else if (strcmp(key, "drive_speed") == 0) {
            if (!b3d_vehicle_parse_q12(value, &temp.drive_speed)) { fclose(file); return 0; }
        } else if (strcmp(key, "run_speed") == 0 || strcmp(key, "max_speed") == 0) {
            if (!b3d_vehicle_parse_q12(value, &temp.run_speed)) { fclose(file); return 0; }
        } else if (strcmp(key, "mount_radius") == 0) {
            if (!b3d_vehicle_parse_q12(value, &temp.mount_radius)) { fclose(file); return 0; }
        } else if (strcmp(key, "throttle_rise") == 0) {
            if (!b3d_vehicle_parse_q12(value, &temp.throttle_rise)) { fclose(file); return 0; }
        } else if (strcmp(key, "throttle_fall") == 0) {
            if (!b3d_vehicle_parse_q12(value, &temp.throttle_fall)) { fclose(file); return 0; }
        } else if (strcmp(key, "torque_scale") == 0) {
            if (!b3d_vehicle_parse_q12(value, &temp.torque_scale)) { fclose(file); return 0; }
        } else if (strcmp(key, "reverse_scale") == 0) {
            if (!b3d_vehicle_parse_q12(value, &temp.reverse_scale)) { fclose(file); return 0; }
        } else if (strcmp(key, "mass_scale") == 0) {
            if (!b3d_vehicle_parse_q12(value, &temp.mass_scale)) { fclose(file); return 0; }
        } else if (strcmp(key, "velocity_retention") == 0 ||
                   strcmp(key, "linear_retention") == 0) {
            if (!b3d_vehicle_parse_q12(value, &temp.velocity_retention)) { fclose(file); return 0; }
        } else if (strcmp(key, "scale_x") == 0) {
            if (!b3d_vehicle_parse_q12(value, &temp.visual_scale_x)) { fclose(file); return 0; }
        } else if (strcmp(key, "scale_y") == 0) {
            if (!b3d_vehicle_parse_q12(value, &temp.visual_scale_y)) { fclose(file); return 0; }
        } else if (strcmp(key, "scale_z") == 0) {
            if (!b3d_vehicle_parse_q12(value, &temp.visual_scale_z)) { fclose(file); return 0; }
        } else if (strcmp(key, "visual_y_offset") == 0) {
            if (!b3d_vehicle_parse_q12(value, &temp.visual_y_offset)) { fclose(file); return 0; }
        }
    }
    fclose(file);
    *config = temp;
    b3d_vehicle_status(status, status_capacity, "vehicle INI loaded: ", temp.name);
    return 1;
}

static gveh_i16 b3d_vehicle_scale_i16(gveh_i16 value, g3d_fix scale)
{
    long out;
    out = ((long)value * (long)scale) / G3D_FIX_ONE;
    if (out > 32767L) out = 32767L;
    if (out < -32768L) out = -32768L;
    return (gveh_i16)out;
}

static gveh_fx b3d_vehicle_scale_q8(gveh_fx value, g3d_fix scale)
{
    long out;
    out = ((long)value * (long)scale) / G3D_FIX_ONE;
    if (out > (long)INT_MAX) out = INT_MAX;
    if (out < (long)INT_MIN) out = INT_MIN;
    return (gveh_fx)out;
}

static gveh_fx b3d_vehicle_q12_to_q8(g3d_fix value)
{
    long out;
    out = ((long)value * (long)GVEH_FX_ONE) / G3D_FIX_ONE;
    if (out > (long)INT_MAX) out = INT_MAX;
    if (out < (long)INT_MIN) out = INT_MIN;
    return (gveh_fx)out;
}

static void b3d_vehicle_apply_tuning(Blank3DVehicleInstance *instance)
{
    int i;
    gveh_profile *profile;
    if (!instance) return;
    profile = &instance->mount.vehicle_profile;
    for (i = 0; i < 16; ++i)
        profile->drive.engine.torque_nm[i] = b3d_vehicle_scale_i16(
            profile->drive.engine.torque_nm[i], instance->config.torque_scale);
    profile->drive.gearbox.gear_ratio[0] = b3d_vehicle_scale_q8(
        profile->drive.gearbox.gear_ratio[0], instance->config.reverse_scale);
    profile->mass = b3d_vehicle_scale_q8(profile->mass,
                                         instance->config.mass_scale);
    profile->velocity_retention = b3d_vehicle_q12_to_q8(
        instance->config.velocity_retention);
}

static gveh_i32 b3d_vehicle_specialized_movement(
    void *user, const vehicleprovider89_movement_request *request)
{
    Blank3DVehicleInstance *instance;
    gveh_vehicle *vehicle;
    gveh_runtime *runtime;
    if (!user || !request || !request->vehicle || !request->runtime)
        return VEHICLEPROVIDER89_DECLINED;
    instance = (Blank3DVehicleInstance *)user;
    vehicle = request->vehicle;
    runtime = request->runtime;
    if (request->module != VEHICLEPROVIDER89_MODULE_CAR)
        return VEHICLEPROVIDER89_DECLINED;
    if (instance->config.movement == B3D_VEHICLE_MOVE_MOTORCYCLE) {
        motorcyclemovement89_step(runtime->surfaces, &runtime->world,
            &vehicle->profile, &vehicle->body, request->basis, request->input,
            request->dt, &vehicle->grounded_count, &vehicle->grounded_ratio,
            &vehicle->fxq);
        return VEHICLEPROVIDER89_HANDLED;
    }
    if (instance->config.movement == B3D_VEHICLE_MOVE_BUS) {
        busmovement89_step(runtime->surfaces, &runtime->world,
            &vehicle->profile, &vehicle->body, request->basis, request->input,
            request->dt, &vehicle->grounded_count, &vehicle->grounded_ratio,
            &vehicle->fxq);
        return VEHICLEPROVIDER89_HANDLED;
    }
    return VEHICLEPROVIDER89_DECLINED;
}

int blank3d_vehicle_system_init(Blank3DVehicleSystem *system,
                                Transform *player,
                                int player_id)
{
    if (!system || !player || player_id <= 0) return 0;
    memset(system, 0, sizeof(*system));
    system->initialized = 1;
    system->player = player;
    system->player_id = player_id;
    system->active_player_slot = -1;
    return 1;
}

void blank3d_vehicle_system_clear(Blank3DVehicleSystem *system)
{
    Transform *player;
    int player_id;
    if (!system) return;
    player = system->player;
    player_id = system->player_id;
    memset(system, 0, sizeof(*system));
    system->initialized = player && player_id > 0 ? 1 : 0;
    system->player = player;
    system->player_id = player_id;
    system->active_player_slot = -1;
}

static int b3d_vehicle_system_spawn_config(Blank3DVehicleSystem *system,
                                           const Blank3DVehicleConfig *config,
                                           g3d_fix x, g3d_fix y, g3d_fix z)
{
    Blank3DVehicleInstance *instance;
    int slot;
    int host_id;
    int owner_id;
    int seat_base;
    if (!system || !system->initialized || !config) return 0;
    if (system->count >= B3D_VEHICLE_SYSTEM_MAX) return 0;
    slot = system->count;
    instance = &system->vehicles[slot];
    memset(instance, 0, sizeof(*instance));
    instance->config = *config;
    host_id = 92000 + slot;
    owner_id = 9200 + slot;
    seat_base = 200 + slot * GVEH_MAX_SEATS;
    if (!blank3d_mount_vehicle_init_ex(&instance->mount, system->player,
            system->player_id, host_id, owner_id, seat_base)) return 0;
    if (!blank3d_mount_vehicle_set_profile(&instance->mount,
                                           instance->config.profile)) return 0;
    blank3d_mount_vehicle_set_driver_policy(&instance->mount,
        instance->config.playerdriving, instance->config.npcdriving);
    blank3d_mount_vehicle_set_control_tuning(&instance->mount,
        instance->config.throttle_rise, instance->config.throttle_fall,
        instance->config.steer_sign);
    instance->mount.mount_radius = instance->config.mount_radius;
    blank3d_mount_vehicle_set_speeds(&instance->mount,
        instance->config.drive_speed, instance->config.run_speed);
    b3d_vehicle_apply_tuning(instance);
    vehicleprovider89_movement_clear(&instance->specialized_movement);
    if (instance->config.movement == B3D_VEHICLE_MOVE_MOTORCYCLE ||
        instance->config.movement == B3D_VEHICLE_MOVE_BUS) {
        instance->specialized_movement.user = instance;
        instance->specialized_movement.module_mask = VEHICLEPROVIDER89_MODULE_CAR;
        instance->specialized_movement.step = b3d_vehicle_specialized_movement;
        blank3d_mount_vehicle_set_movement_provider(&instance->mount,
            &instance->specialized_movement);
    }
    blank3d_mount_vehicle_reset(&instance->mount, x, y, z);
    instance->used = 1;
    instance->slot = slot;
    system->count++;
    return 1;
}

int blank3d_vehicle_system_spawn_ini(Blank3DVehicleSystem *system,
                                     const char *path,
                                     g3d_fix x, g3d_fix y, g3d_fix z,
                                     char *status,
                                     unsigned int status_capacity)
{
    Blank3DVehicleConfig config;
    if (!blank3d_vehicle_config_load(path, &config, status, status_capacity))
        return 0;
    return b3d_vehicle_system_spawn_config(system, &config, x, y, z);
}

int blank3d_vehicle_system_spawn_profile(Blank3DVehicleSystem *system,
                                         const char *profile_name,
                                         g3d_fix x, g3d_fix y, g3d_fix z,
                                         g3d_fix drive_speed,
                                         g3d_fix run_speed)
{
    Blank3DVehicleConfig config;
    if (!profile_name) return 0;
    blank3d_vehicle_config_defaults(&config);
    b3d_vehicle_copy(config.name, sizeof(config.name), profile_name);
    b3d_vehicle_copy(config.profile, sizeof(config.profile), profile_name);
    if (drive_speed > 0) config.drive_speed = drive_speed;
    if (run_speed > 0) config.run_speed = run_speed;
    if (strstr(profile_name, "tank") != 0) config.movement = B3D_VEHICLE_MOVE_TANK;
    else if (strstr(profile_name, "jetski") != 0) config.movement = B3D_VEHICLE_MOVE_WATER;
    else if (strstr(profile_name, "chopper") != 0 || strstr(profile_name, "heli") != 0 ||
             strstr(profile_name, "fighter") != 0 || strstr(profile_name, "plane") != 0)
        config.movement = B3D_VEHICLE_MOVE_AIR;
    return b3d_vehicle_system_spawn_config(system, &config, x, y, z);
}

int blank3d_vehicle_system_update(Blank3DVehicleSystem *system, g3d_fix dt)
{
    int i;
    int ok;
    if (!system || !system->initialized) return 0;
    ok = 1;
    for (i = 0; i < system->count; ++i) {
        if (!system->vehicles[i].used) continue;
        if (!blank3d_mount_vehicle_update(&system->vehicles[i].mount, dt))
            ok = 0;
        if (blank3d_mount_vehicle_is_mounted(&system->vehicles[i].mount))
            system->active_player_slot = i;
    }
    if (system->active_player_slot >= 0 &&
        !blank3d_mount_vehicle_is_mounted(
            &system->vehicles[system->active_player_slot].mount))
        system->active_player_slot = -1;
    return ok;
}

int blank3d_vehicle_system_toggle_player(Blank3DVehicleSystem *system)
{
    int i;
    int best;
    g3d_fix best_d;
    if (!system || !system->initialized) return B3D_MOUNT_VEHICLE_NOT_READY;
    if (system->active_player_slot >= 0) {
        i = system->active_player_slot;
        if (blank3d_mount_vehicle_toggle(&system->vehicles[i].mount)
            != B3D_MOUNT_VEHICLE_OK) return B3D_MOUNT_VEHICLE_ERROR;
        system->active_player_slot = -1;
        return B3D_MOUNT_VEHICLE_OK;
    }
    best = -1;
    best_d = INT_MAX;
    for (i = 0; i < system->count; ++i) {
        g3d_fix dx;
        g3d_fix dz;
        g3d_fix d;
        Blank3DVehicleInstance *instance;
        instance = &system->vehicles[i];
        if (!instance->used || !instance->config.playerdriving ||
            !blank3d_mount_vehicle_player_near(&instance->mount)) continue;
        dx = g3d_fix_abs(g3d_fix_sub_sat(system->player->position.x,
            instance->mount.car.position.x));
        dz = g3d_fix_abs(g3d_fix_sub_sat(system->player->position.z,
            instance->mount.car.position.z));
        d = g3d_fix_max(dx, dz);
        if (best < 0 || d < best_d) { best = i; best_d = d; }
    }
    if (best < 0) return B3D_MOUNT_VEHICLE_NOT_NEAR;
    if (blank3d_mount_vehicle_toggle(&system->vehicles[best].mount)
        != B3D_MOUNT_VEHICLE_OK) return B3D_MOUNT_VEHICLE_ERROR;
    system->active_player_slot = best;
    return B3D_MOUNT_VEHICLE_OK;
}

int blank3d_vehicle_system_playerdriving(const Blank3DVehicleSystem *system)
{
    if (!system || !system->initialized || system->active_player_slot < 0 ||
        system->active_player_slot >= system->count) return 0;
    return system->vehicles[system->active_player_slot].config.playerdriving &&
        blank3d_mount_vehicle_is_mounted(
            &system->vehicles[system->active_player_slot].mount);
}

Blank3DVehicleInstance *blank3d_vehicle_system_active(Blank3DVehicleSystem *system)
{
    if (!system || system->active_player_slot < 0 ||
        system->active_player_slot >= system->count) return 0;
    return &system->vehicles[system->active_player_slot];
}

const Blank3DVehicleInstance *blank3d_vehicle_system_active_const(
    const Blank3DVehicleSystem *system)
{
    if (!system || system->active_player_slot < 0 ||
        system->active_player_slot >= system->count) return 0;
    return &system->vehicles[system->active_player_slot];
}

int blank3d_vehicle_system_count(const Blank3DVehicleSystem *system)
{
    return system && system->initialized ? system->count : 0;
}

Blank3DVehicleInstance *blank3d_vehicle_system_at(Blank3DVehicleSystem *system,
                                                  int index)
{
    if (!system || index < 0 || index >= system->count) return 0;
    return &system->vehicles[index];
}

const Blank3DVehicleInstance *blank3d_vehicle_system_at_const(
    const Blank3DVehicleSystem *system, int index)
{
    if (!system || index < 0 || index >= system->count) return 0;
    return &system->vehicles[index];
}

void blank3d_vehicle_system_forward(Blank3DVehicleSystem *system,
                                    g3d_fix dt, int running)
{
    Blank3DVehicleInstance *instance;
    instance = blank3d_vehicle_system_active(system);
    if (!instance || !instance->config.playerdriving) return;
    blank3d_mount_vehicle_forward(&instance->mount, dt, running);
}

void blank3d_vehicle_system_backward(Blank3DVehicleSystem *system,
                                     g3d_fix dt, int running)
{
    Blank3DVehicleInstance *instance;
    instance = blank3d_vehicle_system_active(system);
    if (!instance || !instance->config.playerdriving) return;
    blank3d_mount_vehicle_backward(&instance->mount, dt, running);
}

void blank3d_vehicle_system_steer(Blank3DVehicleSystem *system,
                                  g3d_fix dt, int right)
{
    Blank3DVehicleInstance *instance;
    instance = blank3d_vehicle_system_active(system);
    if (!instance || !instance->config.playerdriving) return;
    blank3d_mount_vehicle_turn(&instance->mount, dt, right);
}
