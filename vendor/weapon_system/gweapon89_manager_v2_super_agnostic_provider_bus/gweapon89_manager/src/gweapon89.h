#ifndef GWEAPON89_H
#define GWEAPON89_H

/*
   gweapon89 - pure weapon system manager
   C89, fixed point, static-storage friendly, no dynamic allocation ownership.

   This module DOES NOT know about players, enemies, muzzle libraries,
   casing systems, trail systems, projectile systems, raytracers, renderers,
   mesh builders, or host inventory implementations. It owns portable weapon
   state and orchestrates those dependencies through a mutable provider bus.
*/

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef GWP89_MAX_WEAPONS
#define GWP89_MAX_WEAPONS       64
#endif
#ifndef GWP89_MAX_USERS
#define GWP89_MAX_USERS         32
#endif
#ifndef GWP89_MAX_EVENTS
#define GWP89_MAX_EVENTS        256
#endif
#ifndef GWP89_MAX_AMMO_TYPES
#define GWP89_MAX_AMMO_TYPES    64
#endif
#ifndef GWP89_NAME_MAX
#define GWP89_NAME_MAX          32
#endif
#ifndef GWP89_MAX_PROVIDERS
#define GWP89_MAX_PROVIDERS     48
#endif
#ifndef GWP89_PROVIDER_NAME_MAX
#define GWP89_PROVIDER_NAME_MAX 32
#endif

#define GWP89_OK                 0
#define GWP89_ERR               -1
#define GWP89_FULL              -2
#define GWP89_NOT_FOUND         -3
#define GWP89_BAD_ARG           -4
#define GWP89_NO_AMMO           -5
#define GWP89_COOLDOWN          -6
#define GWP89_TRIGGER_LOCKED    -7
#define GWP89_CANCELLED         -8
#define GWP89_PROVIDER_ERROR    -9

#define GWP89_FIX_SHIFT         12
#define GWP89_FIX_ONE           (1L << GWP89_FIX_SHIFT)
#define GWP89_FIX_HALF          (1L << (GWP89_FIX_SHIFT - 1))

typedef long gwp89_fx;

typedef struct GWP89_Vec3Tag {
    gwp89_fx x;
    gwp89_fx y;
    gwp89_fx z;
} GWP89_Vec3;

enum GWP89_FireModeTag {
    GWP89_FIRE_SEMI = 0,
    GWP89_FIRE_AUTO = 1,
    GWP89_FIRE_HOLD_ONCE = 2,
    GWP89_FIRE_BURST = 3
};

enum GWP89_ViewStyleTag {
    GWP89_VIEW_UNKNOWN = 0,
    GWP89_VIEW_FPS = 1,
    GWP89_VIEW_OVER_SHOULDER = 2,
    GWP89_VIEW_THIRD_PERSON = 3,
    GWP89_VIEW_TOPDOWN = 4,
    GWP89_VIEW_NPC = 5
};

enum GWP89_EventTypeTag {
    GWP89_EVENT_NONE = 0,
    GWP89_EVENT_FIRE_ACCEPTED = 1,
    GWP89_EVENT_DRY_FIRE = 2,
    GWP89_EVENT_PROJECTILE_REQUEST = 3,
    GWP89_EVENT_MUZZLE_REQUEST = 4,
    GWP89_EVENT_CASING_REQUEST = 5,
    GWP89_EVENT_TRAIL_REQUEST = 6,
    GWP89_EVENT_MESH_ASSIGN_REQUEST = 7,
    GWP89_EVENT_VISUAL_MOD_REQUEST = 8,
    GWP89_EVENT_AMMO_CHANGED = 9,
    GWP89_EVENT_RELOAD_BEGIN = 10,
    GWP89_EVENT_RELOAD_END = 11,
    GWP89_EVENT_WEAPON_CHANGED = 12,
    GWP89_EVENT_ACTIVE_RELOAD_WINDOW = 13,
    GWP89_EVENT_ACTIVE_RELOAD_SUCCESS = 14,
    GWP89_EVENT_ACTIVE_RELOAD_FAIL = 15,
    GWP89_EVENT_PROVIDER_CANCELLED = 16
};

enum GWP89_TriggerTag {
    GWP89_TRIGGER_NONE = 0,
    GWP89_TRIGGER_DOWN = 1,
    GWP89_TRIGGER_PRESSED = 2,
    GWP89_TRIGGER_RELEASED = 4
};

enum GWP89_EventFlagsTag {
    GWP89_EVENT_FLAG_FIRST_PELLET = 1,
    GWP89_EVENT_FLAG_LAST_PELLET = 2,
    GWP89_EVENT_FLAG_EXTERNAL_AMMO = 4,
    GWP89_EVENT_FLAG_HIT_VALID = 8,
    GWP89_EVENT_FLAG_PROVIDER_MODIFIED = 16,
    GWP89_EVENT_FLAG_PROVIDER_HANDLED = 32,
    GWP89_EVENT_FLAG_ACTIVE_RELOAD = 64
};

typedef struct GWP89_WeaponProfileTag {
    int active;
    int weapon_id;
    int gun_id;
    int ammo_id;
    int projectile_id;
    int shell_id;
    int muzzle_id;
    int casing_id;
    int trail_id;
    int projectile_mesh_id;
    int shell_mesh_id;

    char name[GWP89_NAME_MAX];
    char gun_name[GWP89_NAME_MAX];
    char ammo_name[GWP89_NAME_MAX];
    char projectile_name[GWP89_NAME_MAX];
    char shell_name[GWP89_NAME_MAX];
    char muzzle_name[GWP89_NAME_MAX];
    char casing_name[GWP89_NAME_MAX];
    char trail_name[GWP89_NAME_MAX];
    char projectile_mesh_name[GWP89_NAME_MAX];
    char shell_mesh_name[GWP89_NAME_MAX];

    int fire_mode;
    int clip_size;
    int ammo_per_shot;
    int infinite_ammo;
    int pellet_count;
    int burst_count;
    int allow_dry_fire_event;
    int active_reload_enabled;

    unsigned short cooldown_ms;
    unsigned short reload_ms;
    unsigned short projectile_life_ms;
    unsigned short active_reload_window_start_ms;
    unsigned short active_reload_window_end_ms;
    unsigned short active_reload_bonus_ms;
    unsigned short active_reload_penalty_ms;

    gwp89_fx damage_fx;
    gwp89_fx speed_fx;
    gwp89_fx range_fx;
    gwp89_fx spread_fx;
    gwp89_fx projectile_radius_fx;
    gwp89_fx projectile_mesh_scale_fx;
    gwp89_fx shell_mesh_scale_fx;
    gwp89_fx recoil_fx;
} GWP89_WeaponProfile;

typedef struct GWP89_UserStateTag {
    int active;
    int actor_id;
    int actor_kind;
    int team_id;
    int weapon_slot;
    int weapon_id;
    int clip_ammo;
    int trigger_latched;
    int burst_left;
    int reload_active;
    unsigned short cooldown_ms_left;
    unsigned short reload_ms_left;
    unsigned short reload_elapsed_ms;
    int active_reload_attempted;
    int active_reload_result;
} GWP89_UserState;

typedef struct GWP89_FireInputTag {
    int actor_id;
    int actor_kind;
    int team_id;
    int view_style;
    int trigger_flags;
    unsigned short dt_ms;
    int input_flags;
    int camera_id;
    int socket_set_id;
    gwp89_fx zoom_fx;

    GWP89_Vec3 socket_origin;
    GWP89_Vec3 socket_forward;
    GWP89_Vec3 socket_right;
    GWP89_Vec3 socket_up;
    GWP89_Vec3 camera_origin;
    GWP89_Vec3 camera_forward;
} GWP89_FireInput;

typedef struct GWP89_PoseRequestTag {
    int actor_id;
    int actor_kind;
    int team_id;
    int view_style;
    int weapon_id;
    int gun_id;
    int pellet_index;
    int pellet_count;
    gwp89_fx spread_fx;
    const GWP89_WeaponProfile *profile;
    GWP89_FireInput input;
} GWP89_PoseRequest;

typedef struct GWP89_PoseResultTag {
    int has_hit;
    GWP89_Vec3 projectile_origin;
    GWP89_Vec3 muzzle_origin;
    GWP89_Vec3 casing_origin;
    GWP89_Vec3 direction;
    GWP89_Vec3 hit_point;
} GWP89_PoseResult;

typedef struct GWP89_EventTag {
    int type;
    int flags;
    int actor_id;
    int actor_kind;
    int team_id;
    int view_style;
    int weapon_id;
    int gun_id;
    int ammo_id;
    int amount;
    int projectile_id;
    int shell_id;
    int muzzle_id;
    int casing_id;
    int trail_id;
    int projectile_mesh_id;
    int shell_mesh_id;
    int projectile_slot_hint;
    int pellet_index;
    int pellet_count;
    int clip_ammo;
    int reserve_ammo;
    unsigned short cooldown_ms;
    unsigned short life_ms;
    gwp89_fx damage_fx;
    gwp89_fx speed_fx;
    gwp89_fx radius_fx;
    gwp89_fx projectile_mesh_scale_fx;
    gwp89_fx shell_mesh_scale_fx;
    gwp89_fx range_fx;
    gwp89_fx spread_fx;
    gwp89_fx recoil_fx;
    gwp89_fx travel_distance_fx;
    gwp89_fx zoom_fx;
    GWP89_Vec3 origin;
    GWP89_Vec3 muzzle_origin;
    GWP89_Vec3 casing_origin;
    GWP89_Vec3 direction;
    GWP89_Vec3 hit_point;
    /* Immutable fire-time view snapshot. Runtime consumers must use this
       instead of a later camera frame when repairing HUD convergence. */
    GWP89_Vec3 camera_origin;
    GWP89_Vec3 camera_forward;
    GWP89_Vec3 camera_right;
    GWP89_Vec3 camera_up;
    char weapon_name[GWP89_NAME_MAX];
    char muzzle_name[GWP89_NAME_MAX];
    char casing_name[GWP89_NAME_MAX];
    char trail_name[GWP89_NAME_MAX];
    char projectile_mesh_name[GWP89_NAME_MAX];
    char shell_mesh_name[GWP89_NAME_MAX];
} GWP89_Event;


/*
   Provider bus
   ------------
   Providers are registered by service and receive a mutable packet in PRE and
   POST phases. A provider may modify data, claim the fallback, cancel a stage,
   or stop lower-priority providers. The manager still works with zero providers.
*/
enum GWP89_ProviderServiceTag {
    GWP89_SERVICE_ANY = 0,
    GWP89_SERVICE_MATH3D = 1,
    GWP89_SERVICE_TRANSFORM = 2,
    GWP89_SERVICE_NUMERIC = 3,
    GWP89_SERVICE_FLAGS = 4,
    GWP89_SERVICE_CAMERA = 5,
    GWP89_SERVICE_SOCKETS = 6,
    GWP89_SERVICE_INVENTORY = 7,
    GWP89_SERVICE_RAYCAST = 8,
    GWP89_SERVICE_HUD = 9,
    GWP89_SERVICE_CROSSHAIR = 10,
    GWP89_SERVICE_SCOPE = 11,
    GWP89_SERVICE_DRAW = 12,
    GWP89_SERVICE_PROJECTILE = 13,
    GWP89_SERVICE_ZOOM = 14,
    GWP89_SERVICE_SPREAD = 15,
    GWP89_SERVICE_PROJECTILE_LIFE = 16,
    GWP89_SERVICE_HEALTH = 17,
    GWP89_SERVICE_MUZZLE = 18,
    GWP89_SERVICE_RELOAD = 19,
    GWP89_SERVICE_ACTIVE_RELOAD = 20,
    GWP89_SERVICE_CASING = 21,
    GWP89_SERVICE_TRAIL = 22,
    GWP89_SERVICE_MESH = 23,
    GWP89_SERVICE_RECOIL = 24,
    GWP89_SERVICE_POSE = 25,
    GWP89_SERVICE_EVENT_BUS = 26,
    GWP89_SERVICE_IO = 27,
    GWP89_SERVICE_ACTOR_STATE = 28
};

enum GWP89_ProviderPhaseTag {
    GWP89_PHASE_PRE = 1,
    GWP89_PHASE_POST = 2
};

enum GWP89_ProviderResultTag {
    GWP89_PROVIDER_PASS = 0,
    GWP89_PROVIDER_HANDLED = 1,
    GWP89_PROVIDER_CANCEL = 2,
    GWP89_PROVIDER_STOP = 4,
    GWP89_PROVIDER_MODIFIED = 8
};

enum GWP89_ProviderOperationTag {
    GWP89_OP_NONE = 0,
    GWP89_OP_QUERY_INT = 1,
    GWP89_OP_QUERY_FX = 2,
    GWP89_OP_QUERY_FLAG = 3,
    GWP89_OP_MATH_ADD = 4,
    GWP89_OP_MATH_SCALE = 5,
    GWP89_OP_MATH_NORMALIZE = 6,
    GWP89_OP_GET_ACTOR_TRANSFORM = 7,
    GWP89_OP_GET_CAMERA = 8,
    GWP89_OP_GET_SOCKET = 9,
    GWP89_OP_APPLY_SPREAD = 10,
    GWP89_OP_RAYCAST = 11,
    GWP89_OP_AMMO_QUERY = 12,
    GWP89_OP_AMMO_CONSUME = 13,
    GWP89_OP_AMMO_SET = 14,
    GWP89_OP_AMMO_ADD = 15,
    GWP89_OP_CLIP_QUERY = 16,
    GWP89_OP_CLIP_SET = 17,
    GWP89_OP_EQUIP = 18,
    GWP89_OP_FIRE_VALIDATE = 19,
    GWP89_OP_FIRE_ACCEPTED = 20,
    GWP89_OP_RELOAD_BEGIN = 21,
    GWP89_OP_RELOAD_TICK = 22,
    GWP89_OP_RELOAD_COMPLETE = 23,
    GWP89_OP_ACTIVE_RELOAD_PRESS = 24,
    GWP89_OP_EMIT_EVENT = 25,
    GWP89_OP_RESOLVE_POSE = 26,
    GWP89_OP_PROJECTILE_LIFE = 27,
    GWP89_OP_DAMAGE_REQUEST = 28,
    GWP89_OP_UPDATE = 29,
    GWP89_OP_READ_TEXT_FILE = 30
};

enum GWP89_NumericKeyTag {
    GWP89_NUM_CLIP_SIZE = 1,
    GWP89_NUM_AMMO_PER_SHOT = 2,
    GWP89_NUM_PELLET_COUNT = 3,
    GWP89_NUM_BURST_COUNT = 4,
    GWP89_NUM_COOLDOWN_MS = 5,
    GWP89_NUM_RELOAD_MS = 6,
    GWP89_NUM_PROJECTILE_LIFE_MS = 7,
    GWP89_NUM_DAMAGE_FX = 8,
    GWP89_NUM_SPEED_FX = 9,
    GWP89_NUM_RANGE_FX = 10,
    GWP89_NUM_SPREAD_FX = 11,
    GWP89_NUM_PROJECTILE_RADIUS_FX = 12,
    GWP89_NUM_PROJECTILE_MESH_SCALE_FX = 13,
    GWP89_NUM_SHELL_MESH_SCALE_FX = 14,
    GWP89_NUM_RECOIL_FX = 15,
    GWP89_NUM_ACTIVE_RELOAD_WINDOW_START_MS = 16,
    GWP89_NUM_ACTIVE_RELOAD_WINDOW_END_MS = 17,
    GWP89_NUM_ACTIVE_RELOAD_BONUS_MS = 18,
    GWP89_NUM_ACTIVE_RELOAD_PENALTY_MS = 19,
    GWP89_NUM_FIRE_MODE = 20
};

enum GWP89_FlagKeyTag {
    GWP89_FLAG_WEAPON_ENABLED = 1,
    GWP89_FLAG_CAN_FIRE = 2,
    GWP89_FLAG_CAN_RELOAD = 3,
    GWP89_FLAG_ALLOW_DRY_FIRE = 4,
    GWP89_FLAG_USE_INTERNAL_CLIP = 5,
    GWP89_FLAG_ACTIVE_RELOAD_ENABLED = 6,
    GWP89_FLAG_EMIT_PROJECTILE = 7,
    GWP89_FLAG_EMIT_MUZZLE = 8,
    GWP89_FLAG_EMIT_CASING = 9,
    GWP89_FLAG_EMIT_TRAIL = 10,
    GWP89_FLAG_EMIT_VISUALS = 11,
    GWP89_FLAG_APPLY_RECOIL = 12,
    GWP89_FLAG_INFINITE_AMMO = 13
};

enum GWP89_SocketKindTag {
    GWP89_SOCKET_PROJECTILE = 1,
    GWP89_SOCKET_MUZZLE = 2,
    GWP89_SOCKET_CASING = 3,
    GWP89_SOCKET_AIM = 4
};

typedef struct GWP89_TransformTag {
    GWP89_Vec3 position;
    GWP89_Vec3 forward;
    GWP89_Vec3 right;
    GWP89_Vec3 up;
    GWP89_Vec3 scale;
} GWP89_Transform;

typedef struct GWP89_CameraStateTag {
    int valid;
    int camera_id;
    int view_style;
    GWP89_Vec3 origin;
    GWP89_Vec3 forward;
    GWP89_Vec3 right;
    GWP89_Vec3 up;
    gwp89_fx zoom_fx;
} GWP89_CameraState;

typedef struct GWP89_RaycastHitTag {
    int hit;
    int actor_id;
    int material_id;
    GWP89_Vec3 point;
    GWP89_Vec3 normal;
    gwp89_fx distance_fx;
} GWP89_RaycastHit;

typedef struct GWP89_ProviderPacketTag {
    int phase;
    int service;
    int operation;
    int key;
    int actor_id;
    int actor_kind;
    int team_id;
    int weapon_id;
    int gun_id;
    int ammo_id;
    int socket_kind;
    int i_value;
    int i_value2;
    int amount;
    int result_code;
    unsigned short ms_value;
    gwp89_fx fx_value;
    gwp89_fx fx_value2;
    GWP89_Vec3 vec_a;
    GWP89_Vec3 vec_b;
    GWP89_Vec3 vec_c;
    GWP89_Transform transform;
    GWP89_CameraState camera;
    GWP89_RaycastHit hit;
    const GWP89_WeaponProfile *profile;
    GWP89_UserState *user;
    GWP89_FireInput *input;
    GWP89_PoseRequest *pose_request;
    GWP89_PoseResult *pose_result;
    GWP89_Event *event;
    const char *text_in;
    char *text_out;
    int text_capacity;
    int text_length;
    void *payload;
} GWP89_ProviderPacket;

typedef int (*GWP89_ProviderFn)(void *ctx, GWP89_ProviderPacket *packet);

typedef struct GWP89_ProviderSlotTag {
    int active;
    int service;
    int priority;
    unsigned long serial;
    void *ctx;
    GWP89_ProviderFn fn;
    char name[GWP89_PROVIDER_NAME_MAX];
} GWP89_ProviderSlot;

typedef struct GWP89_HooksTag {
    void *ctx;

    int  (*resolve_pose)(void *ctx, const GWP89_PoseRequest *request, GWP89_PoseResult *out_pose);

    int  (*ammo_query)(void *ctx, int actor_id, int ammo_id, int weapon_id);
    int  (*ammo_consume)(void *ctx, int actor_id, int ammo_id, int weapon_id, int amount);
    void (*ammo_changed)(void *ctx, const GWP89_Event *event);

    void (*emit_projectile)(void *ctx, const GWP89_Event *event);
    void (*emit_muzzle)(void *ctx, const GWP89_Event *event);
    void (*emit_casing)(void *ctx, const GWP89_Event *event);
    void (*emit_trail)(void *ctx, const GWP89_Event *event);
    void (*assign_mesh)(void *ctx, const GWP89_Event *event);
    void (*visual_mod)(void *ctx, const GWP89_Event *event);
    void (*event)(void *ctx, const GWP89_Event *event);
} GWP89_Hooks;

typedef struct GWP89_ManagerTag {
    GWP89_WeaponProfile weapons[GWP89_MAX_WEAPONS];
    GWP89_UserState users[GWP89_MAX_USERS];
    int ammo_bank[GWP89_MAX_USERS][GWP89_MAX_AMMO_TYPES];
    GWP89_Event events[GWP89_MAX_EVENTS];
    int event_head;
    int event_tail;
    int event_count;
    int weapon_count;
    GWP89_Hooks hooks;
    GWP89_ProviderSlot providers[GWP89_MAX_PROVIDERS];
    int provider_count;
    unsigned long provider_serial;
    char status[160];
} GWP89_Manager;

/* fixed helpers */
gwp89_fx    gwp89_fx_from_int(int v);
gwp89_fx    gwp89_fx_from_text(const char *text);
int         gwp89_fx_to_int_round(gwp89_fx v);
GWP89_Vec3  gwp89_v3(gwp89_fx x, gwp89_fx y, gwp89_fx z);
GWP89_Vec3  gwp89_v3_zero(void);

/* lifecycle */
void gwp89_init(GWP89_Manager *m);
void gwp89_set_hooks(GWP89_Manager *m, const GWP89_Hooks *hooks);
int  gwp89_add_provider(GWP89_Manager *m, int service, int priority, const char *name, void *ctx, GWP89_ProviderFn fn);
int  gwp89_remove_provider(GWP89_Manager *m, int provider_slot);
void gwp89_clear_providers(GWP89_Manager *m);
int  gwp89_provider_call(GWP89_Manager *m, int service, int operation, int phase, GWP89_ProviderPacket *packet);
int  gwp89_provider_count(const GWP89_Manager *m, int service);
const char *gwp89_service_name(int service);
const char *gwp89_status(const GWP89_Manager *m);

/* profiles */
void gwp89_profile_defaults(GWP89_WeaponProfile *p);
int  gwp89_add_weapon(GWP89_Manager *m, const GWP89_WeaponProfile *profile);
int  gwp89_find_weapon_slot(const GWP89_Manager *m, const char *name);
int  gwp89_find_weapon_slot_by_id(const GWP89_Manager *m, int weapon_id);
const GWP89_WeaponProfile *gwp89_get_weapon(const GWP89_Manager *m, int weapon_slot);
int  gwp89_load_ini_text(GWP89_Manager *m, const char *text);
int  gwp89_load_ini_file(GWP89_Manager *m, const char *path);
int  gwp89_load_default_profiles(GWP89_Manager *m);

/* actor/user binding */
int gwp89_bind_actor(GWP89_Manager *m, int actor_id, int actor_kind, int team_id);
int gwp89_find_user_slot(const GWP89_Manager *m, int actor_id);
int gwp89_equip_slot(GWP89_Manager *m, int actor_id, int weapon_slot, int fill_clip);
int gwp89_equip_name(GWP89_Manager *m, int actor_id, const char *weapon_name, int fill_clip);
int gwp89_cycle_next(GWP89_Manager *m, int actor_id, int fill_clip);
int gwp89_cycle_prev(GWP89_Manager *m, int actor_id, int fill_clip);
int gwp89_begin_reload(GWP89_Manager *m, int actor_id);
int gwp89_active_reload_press(GWP89_Manager *m, int actor_id);

/* ammo: internal bank by default, external hooks may override query/consume */
int gwp89_set_ammo(GWP89_Manager *m, int actor_id, int ammo_id, int amount);
int gwp89_add_ammo(GWP89_Manager *m, int actor_id, int ammo_id, int amount);
int gwp89_query_ammo(const GWP89_Manager *m, int actor_id, int ammo_id, int weapon_id);
int gwp89_query_clip(GWP89_Manager *m, int actor_id, int weapon_id);
int gwp89_set_clip(GWP89_Manager *m, int actor_id, int weapon_id, int amount);

/* frame/update/fire */
int gwp89_update_actor(GWP89_Manager *m, const GWP89_FireInput *input);
int gwp89_try_fire(GWP89_Manager *m, const GWP89_FireInput *input);

/* queued events, useful when callbacks are not wanted */
int  gwp89_resolve_numeric_int(GWP89_Manager *m, int actor_id, const GWP89_WeaponProfile *profile, int key, int fallback);
gwp89_fx gwp89_resolve_numeric_fx(GWP89_Manager *m, int actor_id, const GWP89_WeaponProfile *profile, int key, gwp89_fx fallback);
int  gwp89_resolve_flag(GWP89_Manager *m, int actor_id, const GWP89_WeaponProfile *profile, int key, int fallback);

int  gwp89_poll_event(GWP89_Manager *m, GWP89_Event *out_event);
void gwp89_clear_events(GWP89_Manager *m);

/* utility */
int  gwp89_streq_id(const char *a, const char *b);
void gwp89_copy_id(char *dst, int cap, const char *src);
int  gwp89_fire_mode_from_name(const char *name);
const char *gwp89_fire_mode_name(int fire_mode);

#ifdef __cplusplus
}
#endif

#endif
