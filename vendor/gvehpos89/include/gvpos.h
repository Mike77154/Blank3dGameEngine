#ifndef GVPOS_H
#define GVPOS_H

/*
   gvehpos89 - C89 vehicle possession / boarding helper
   No malloc, no realloc, no free, no heap ownership, no float/double.
   User provides static arrays or a byte arena.
*/

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GVPOS_VERSION_MAJOR 1
#define GVPOS_VERSION_MINOR 0
#define GVPOS_VERSION_PATCH 0

#define GVPOS_FP_SHIFT 16
#define GVPOS_FP_ONE   65536
#define GVPOS_FP_HALF  32768

#define GVPOS_TRUE  1
#define GVPOS_FALSE 0

#define GVPOS_ID_NONE (-1)

#define GVPOS_FROM_INT(x) ((GVPos_FP)((x) << GVPOS_FP_SHIFT))
#define GVPOS_TO_INT(x)   ((int)((x) >> GVPOS_FP_SHIFT))

typedef int GVPos_FP;
typedef int GVPos_Bool;
typedef int GVPos_Id;

typedef struct GVPos_Vec3 {
    GVPos_FP x;
    GVPos_FP y;
    GVPos_FP z;
} GVPos_Vec3;

typedef enum GVPos_Result {
    GVPOS_OK = 0,
    GVPOS_ERR_NULL = -1,
    GVPOS_ERR_FULL = -2,
    GVPOS_ERR_NOT_FOUND = -3,
    GVPOS_ERR_BAD_STATE = -4,
    GVPOS_ERR_LOCKED = -5,
    GVPOS_ERR_SEAT_BUSY = -6,
    GVPOS_ERR_TOO_FAR = -7,
    GVPOS_ERR_DENIED = -8,
    GVPOS_ERR_BLOCKED_EXIT = -9,
    GVPOS_ERR_BAD_STORAGE = -10,
    GVPOS_ERR_DUPLICATE = -11,
    GVPOS_ERR_BAD_ARG = -12
} GVPos_Result;

typedef enum GVPos_ActorPhase {
    GVPOS_ACTOR_OFF = 0,
    GVPOS_ACTOR_ON_FOOT = 1,
    GVPOS_ACTOR_ENTERING = 2,
    GVPOS_ACTOR_IN_VEHICLE = 3,
    GVPOS_ACTOR_EXITING = 4
} GVPos_ActorPhase;

typedef enum GVPos_EventType {
    GVPOS_EVENT_NONE = 0,
    GVPOS_EVENT_ENTER_BEGIN = 1,
    GVPOS_EVENT_MOUNTED = 2,
    GVPOS_EVENT_EXIT_BEGIN = 3,
    GVPOS_EVENT_EXITED = 4,
    GVPOS_EVENT_DENIED = 5,
    GVPOS_EVENT_DRIVER_CHANGED = 6,
    GVPOS_EVENT_RESERVATION_CANCELLED = 7
} GVPos_EventType;

enum {
    GVPOS_ACTOR_FLAG_PLAYER      = 1,
    GVPOS_ACTOR_FLAG_NPC         = 2,
    GVPOS_ACTOR_FLAG_DISABLED    = 4,
    GVPOS_ACTOR_FLAG_HIDDEN      = 8,
    GVPOS_ACTOR_FLAG_ALLOW_DRIVE = 16,
    GVPOS_ACTOR_FLAG_ALLOW_RIDE  = 32
};

enum {
    GVPOS_VEHICLE_FLAG_USABLE       = 1,
    GVPOS_VEHICLE_FLAG_LOCKED       = 2,
    GVPOS_VEHICLE_FLAG_KEEP_ENGINE  = 4,
    GVPOS_VEHICLE_FLAG_AI_ALLOWED   = 8,
    GVPOS_VEHICLE_FLAG_PLAYER_ONLY  = 16,
    GVPOS_VEHICLE_FLAG_NPC_ONLY     = 32
};

enum {
    GVPOS_SEAT_FLAG_DRIVER       = 1,
    GVPOS_SEAT_FLAG_PASSENGER    = 2,
    GVPOS_SEAT_FLAG_LOCKED       = 4,
    GVPOS_SEAT_FLAG_ALLOW_PLAYER = 8,
    GVPOS_SEAT_FLAG_ALLOW_NPC    = 16,
    GVPOS_SEAT_FLAG_NO_ANIM      = 32
};

enum {
    GVPOS_REQ_CHECK_DISTANCE = 1,
    GVPOS_REQ_CHECK_CALLBACK = 2,
    GVPOS_REQ_FORCE          = 4,
    GVPOS_REQ_ALLOW_SWAP     = 8
};

typedef struct GVPos_Input {
    GVPos_FP throttle;
    GVPos_FP brake;
    GVPos_FP steer;
    GVPos_FP handbrake;
    int interact_pressed;
    int exit_pressed;
    int horn_pressed;
} GVPos_Input;

typedef struct GVPos_Config {
    int default_enter_ticks;
    int default_exit_ticks;
    int default_cooldown_ticks;
    GVPos_FP default_entry_radius;
    GVPos_FP default_exit_check_radius;
} GVPos_Config;

typedef struct GVPos_Actor {
    GVPos_Id id;
    int flags;
    int phase;
    GVPos_Id vehicle_id;
    int seat_slot;
    int timer_ticks;
    int cooldown_ticks;
    GVPos_Input input;
    GVPos_Vec3 cached_pos;
} GVPos_Actor;

typedef struct GVPos_Vehicle {
    GVPos_Id id;
    int flags;
    int driver_slot;
    GVPos_Id driver_actor_id;
    int engine_on;
    GVPos_Vec3 cached_pos;
} GVPos_Vehicle;

typedef struct GVPos_Seat {
    GVPos_Id vehicle_id;
    int slot;
    int flags;
    GVPos_FP entry_radius;
    GVPos_FP exit_check_radius;
    GVPos_Vec3 local_mount_pos;
    GVPos_Vec3 local_exit_pos;
    GVPos_Id occupant_actor_id;
    GVPos_Id reserved_by_actor_id;
} GVPos_Seat;

typedef struct GVPos_Event {
    int type;
    GVPos_Id actor_id;
    GVPos_Id vehicle_id;
    int seat_slot;
    int result;
} GVPos_Event;

typedef struct GVPos_Arena {
    unsigned char *base;
    size_t capacity;
    size_t used;
} GVPos_Arena;

struct GVPos_Context;

typedef GVPos_Bool (*GVPos_CanEnterFn)(struct GVPos_Context *ctx, GVPos_Id actor_id, GVPos_Id vehicle_id, int seat_slot, void *user);
typedef GVPos_Bool (*GVPos_CanExitFn)(struct GVPos_Context *ctx, GVPos_Id actor_id, GVPos_Id vehicle_id, int seat_slot, void *user);
typedef GVPos_Bool (*GVPos_GetPosFn)(struct GVPos_Context *ctx, GVPos_Id object_id, GVPos_Vec3 *out_pos, void *user);
typedef GVPos_Bool (*GVPos_TestExitFn)(struct GVPos_Context *ctx, GVPos_Id actor_id, GVPos_Id vehicle_id, int seat_slot, GVPos_Vec3 exit_pos, GVPos_FP radius, void *user);
typedef void (*GVPos_EventFn)(struct GVPos_Context *ctx, const GVPos_Event *event, void *user);
typedef void (*GVPos_ActorHiddenFn)(struct GVPos_Context *ctx, GVPos_Id actor_id, int hidden, void *user);
typedef void (*GVPos_ActorAttachFn)(struct GVPos_Context *ctx, GVPos_Id actor_id, GVPos_Id vehicle_id, int seat_slot, int attached, void *user);
typedef void (*GVPos_VehicleInputFn)(struct GVPos_Context *ctx, GVPos_Id vehicle_id, GVPos_Id driver_actor_id, const GVPos_Input *input, void *user);

typedef struct GVPos_Callbacks {
    GVPos_CanEnterFn can_enter;
    GVPos_CanExitFn can_exit;
    GVPos_GetPosFn get_actor_pos;
    GVPos_GetPosFn get_vehicle_pos;
    GVPos_TestExitFn test_exit_clear;
    GVPos_EventFn on_event;
    GVPos_ActorHiddenFn set_actor_hidden;
    GVPos_ActorAttachFn set_actor_attached;
    GVPos_VehicleInputFn apply_vehicle_input;
} GVPos_Callbacks;

typedef struct GVPos_Context {
    GVPos_Config cfg;
    GVPos_Callbacks cb;
    void *user;
    GVPos_Actor *actors;
    int actor_cap;
    int actor_count;
    GVPos_Vehicle *vehicles;
    int vehicle_cap;
    int vehicle_count;
    GVPos_Seat *seats;
    int seat_cap;
    int seat_count;
    GVPos_Event *events;
    int event_cap;
    int event_head;
    int event_tail;
    unsigned int tick;
} GVPos_Context;

void gvpos_default_config(GVPos_Config *cfg);
void gvpos_zero_input(GVPos_Input *input);
void gvpos_init(GVPos_Context *ctx, const GVPos_Config *cfg);
void gvpos_set_callbacks(GVPos_Context *ctx, const GVPos_Callbacks *cb, void *user);

void gvpos_arena_init(GVPos_Arena *arena, void *memory, size_t bytes);
void *gvpos_arena_push(GVPos_Arena *arena, size_t bytes);
size_t gvpos_required_bytes(int actor_cap, int vehicle_cap, int seat_cap, int event_cap);
int gvpos_bind_arena(GVPos_Context *ctx, GVPos_Arena *arena, int actor_cap, int vehicle_cap, int seat_cap, int event_cap);
int gvpos_bind_storage(GVPos_Context *ctx,
                       GVPos_Actor *actors, int actor_cap,
                       GVPos_Vehicle *vehicles, int vehicle_cap,
                       GVPos_Seat *seats, int seat_cap,
                       GVPos_Event *events, int event_cap);

GVPos_Actor *gvpos_find_actor(GVPos_Context *ctx, GVPos_Id actor_id);
GVPos_Vehicle *gvpos_find_vehicle(GVPos_Context *ctx, GVPos_Id vehicle_id);
GVPos_Seat *gvpos_find_seat(GVPos_Context *ctx, GVPos_Id vehicle_id, int seat_slot);

int gvpos_add_actor(GVPos_Context *ctx, GVPos_Id actor_id, int flags, GVPos_Vec3 pos);
int gvpos_add_vehicle(GVPos_Context *ctx, GVPos_Id vehicle_id, int flags, int driver_slot, GVPos_Vec3 pos);
int gvpos_add_seat(GVPos_Context *ctx, GVPos_Id vehicle_id, int seat_slot, int flags, GVPos_FP entry_radius, GVPos_Vec3 mount_pos, GVPos_Vec3 exit_pos);

int gvpos_set_actor_input(GVPos_Context *ctx, GVPos_Id actor_id, const GVPos_Input *input);
int gvpos_set_actor_pos(GVPos_Context *ctx, GVPos_Id actor_id, GVPos_Vec3 pos);
int gvpos_set_vehicle_pos(GVPos_Context *ctx, GVPos_Id vehicle_id, GVPos_Vec3 pos);

int gvpos_request_mount(GVPos_Context *ctx, GVPos_Id actor_id, GVPos_Id vehicle_id, int seat_slot, int req_flags);
int gvpos_request_mount_nearest(GVPos_Context *ctx, GVPos_Id actor_id, GVPos_Id vehicle_id, int req_flags);
int gvpos_request_exit(GVPos_Context *ctx, GVPos_Id actor_id, int req_flags);
int gvpos_cancel_enter(GVPos_Context *ctx, GVPos_Id actor_id);

void gvpos_update(GVPos_Context *ctx);
int gvpos_pop_event(GVPos_Context *ctx, GVPos_Event *out_event);

GVPos_Bool gvpos_actor_is_mounted(GVPos_Context *ctx, GVPos_Id actor_id);
GVPos_Id gvpos_vehicle_driver(GVPos_Context *ctx, GVPos_Id vehicle_id);

const char *gvpos_result_name(int result);
const char *gvpos_event_name(int event_type);

#ifdef __cplusplus
}
#endif

#endif
