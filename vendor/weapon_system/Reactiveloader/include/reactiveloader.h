#ifndef REACTIVELOADER_H
#define REACTIVELOADER_H

/*
   Reactiveloader - active reload timing solver
   C89, fixed-point, no malloc/free/realloc, no float/double.

   The library owns no heap memory. The caller may allocate contexts directly,
   pass static arrays, or use the tiny arena wrapper over a caller-provided byte
   buffer.
*/

#ifdef __cplusplus
extern "C" {
#endif

#define RLD_VERSION_MAJOR 1
#define RLD_VERSION_MINOR 0
#define RLD_VERSION_PATCH 0

#define RLD_Q16_SHIFT 16
#define RLD_Q16_ONE   65536
#define RLD_Q16_HALF  32768
#define RLD_TRUE      1
#define RLD_FALSE     0

#define RLD_MAX_NAME  32

#define RLD_FLAG_ENABLED                 1
#define RLD_FLAG_SAFE_FAIL               2
#define RLD_FLAG_ALLOW_AUTO              4
#define RLD_FLAG_ALLOW_INTERRUPT         8
#define RLD_FLAG_SHELL_BY_SHELL          16
#define RLD_FLAG_HARDCORE_JAM            32
#define RLD_FLAG_KEEP_NORMAL_IF_NO_PRESS 64
#define RLD_FLAG_ALLOW_EARLY_PRESS_FAIL  128

#define RLD_INPUT_RELOAD_PRESS 1
#define RLD_INPUT_FIRE_PRESS   2
#define RLD_INPUT_CANCEL_PRESS 4
#define RLD_INPUT_AUTO_RELOAD  8

#define RLD_EVENT_NONE          0
#define RLD_EVENT_RELOAD_BEGIN  1
#define RLD_EVENT_SWEEP_BEGIN   2
#define RLD_EVENT_GOOD          4
#define RLD_EVENT_PERFECT       8
#define RLD_EVENT_FAIL          16
#define RLD_EVENT_NORMAL        32
#define RLD_EVENT_JAM           64
#define RLD_EVENT_SHELL_LOADED  128
#define RLD_EVENT_INTERRUPT     256
#define RLD_EVENT_DONE          512
#define RLD_EVENT_CANCEL        1024

#define RLD_BONUS_DAMAGE   1
#define RLD_BONUS_ACCURACY 2
#define RLD_BONUS_STABILITY 4
#define RLD_BONUS_FIRE_RATE 8
#define RLD_BONUS_SPREAD    16
#define RLD_BONUS_CUSTOM0   32
#define RLD_BONUS_CUSTOM1   64

#define RLD_PROFILE_NOT_FOUND -1
#define RLD_PROFILE_BANK_FULL -2
#define RLD_ARENA_OUT_OF_MEMORY -3

typedef int RLD_Fixed;

typedef enum RLD_StateTag {
    RLD_STATE_IDLE = 0,
    RLD_STATE_RELOAD_BEGIN = 1,
    RLD_STATE_ACTIVE_SWEEP = 2,
    RLD_STATE_FINISHING = 3,
    RLD_STATE_JAMMED = 4,
    RLD_STATE_DONE = 5
} RLD_State;

typedef enum RLD_ResultTag {
    RLD_RESULT_NONE = 0,
    RLD_RESULT_NORMAL = 1,
    RLD_RESULT_GOOD = 2,
    RLD_RESULT_PERFECT = 3,
    RLD_RESULT_FAIL = 4,
    RLD_RESULT_JAM = 5,
    RLD_RESULT_CANCELLED = 6,
    RLD_RESULT_INTERRUPTED = 7
} RLD_Result;

typedef struct RLD_ArenaTag {
    unsigned char *base;
    unsigned long capacity;
    unsigned long used;
    int error;
} RLD_Arena;

typedef struct RLD_ProfileTag {
    char name[RLD_MAX_NAME];
    int id;
    int flags;

    int begin_ticks;
    int normal_reload_ticks;
    int good_finish_ticks;
    int perfect_finish_ticks;
    int fail_penalty_ticks;
    int jam_ticks;

    RLD_Fixed cursor_start_q16;
    RLD_Fixed cursor_end_q16;
    RLD_Fixed cursor_speed_q16;

    RLD_Fixed good_start_q16;
    RLD_Fixed good_end_q16;
    RLD_Fixed perfect_start_q16;
    RLD_Fixed perfect_end_q16;

    int shell_ticks;
    int shells_per_step;
    int max_units_loaded_per_reload;

    int perfect_bonus_ticks;
    int good_bonus_ticks;
    int bonus_mask;
    RLD_Fixed bonus_damage_q16;
    RLD_Fixed bonus_accuracy_q16;
    RLD_Fixed bonus_stability_q16;
    RLD_Fixed bonus_fire_rate_q16;
    RLD_Fixed bonus_spread_q16;

    int hud_x_q16;
    int hud_y_q16;
    int hud_w_q16;
    int hud_h_q16;

    int user0;
    int user1;
} RLD_Profile;

typedef struct RLD_ContextTag {
    int entity_id;
    int weapon_id;
    int profile_id;

    RLD_State state;
    RLD_Result result;
    int events;

    int ticks_in_state;
    int total_ticks;
    int finish_ticks_left;
    int jam_ticks_left;
    int bonus_ticks_left;

    RLD_Fixed cursor_q16;
    RLD_Fixed last_cursor_q16;

    int ammo_before;
    int ammo_after;
    int units_needed;
    int units_loaded;
    int units_loaded_this_step;

    int active_attempted;
    int active_resolved;
    int is_busy;
    int user0;
    int user1;
} RLD_Context;

typedef struct RLD_ProfileBankTag {
    RLD_Profile *items;
    int count;
    int max_count;
} RLD_ProfileBank;

typedef struct RLD_HudSampleTag {
    int visible;
    RLD_Fixed cursor_q16;
    RLD_Fixed good_start_q16;
    RLD_Fixed good_end_q16;
    RLD_Fixed perfect_start_q16;
    RLD_Fixed perfect_end_q16;
    int state;
    int result;
} RLD_HudSample;

typedef struct RLD_CallbacksTag {
    void (*on_reload_begin)(RLD_Context *ctx, void *user);
    void (*on_sweep_begin)(RLD_Context *ctx, void *user);
    void (*on_good)(RLD_Context *ctx, void *user);
    void (*on_perfect)(RLD_Context *ctx, void *user);
    void (*on_fail)(RLD_Context *ctx, void *user);
    void (*on_normal)(RLD_Context *ctx, void *user);
    void (*on_jam)(RLD_Context *ctx, void *user);
    void (*on_shell_loaded)(RLD_Context *ctx, void *user);
    void (*on_interrupt)(RLD_Context *ctx, void *user);
    void (*on_done)(RLD_Context *ctx, void *user);
    void (*on_cancel)(RLD_Context *ctx, void *user);
    void *user;
} RLD_Callbacks;

RLD_Fixed RLD_FromInt(int v);
RLD_Fixed RLD_FromPercent(int pct);
int RLD_ToInt(RLD_Fixed v);
int RLD_ToPercent(RLD_Fixed v);
RLD_Fixed RLD_ClampQ16(RLD_Fixed v, RLD_Fixed lo, RLD_Fixed hi);

void RLD_ArenaInit(RLD_Arena *arena, void *buffer, unsigned long bytes);
void *RLD_ArenaAlloc(RLD_Arena *arena, unsigned long bytes, unsigned long align);
void RLD_ArenaReset(RLD_Arena *arena);

void RLD_DefaultProfile(RLD_Profile *p);
void RLD_MakePistolProfile(RLD_Profile *p, int id);
void RLD_MakeShotgunProfile(RLD_Profile *p, int id);
void RLD_MakeMagnumProfile(RLD_Profile *p, int id);
void RLD_MakeSniperProfile(RLD_Profile *p, int id);
void RLD_MakeLauncherProfile(RLD_Profile *p, int id);

void RLD_ProfileBankInit(RLD_ProfileBank *bank, RLD_Profile *items, int max_count);
int RLD_ProfileBankInitArena(RLD_ProfileBank *bank, RLD_Arena *arena, int max_count);
int RLD_ProfileBankAdd(RLD_ProfileBank *bank, const RLD_Profile *profile);
RLD_Profile *RLD_ProfileBankFindById(RLD_ProfileBank *bank, int id);
RLD_Profile *RLD_ProfileBankFindByName(RLD_ProfileBank *bank, const char *name);

void RLD_ContextInit(RLD_Context *ctx, int entity_id, int weapon_id);
void RLD_Begin(RLD_Context *ctx, const RLD_Profile *profile, int ammo_before, int units_needed);
void RLD_Cancel(RLD_Context *ctx);
void RLD_Tick(RLD_Context *ctx, const RLD_Profile *profile, int input_flags, const RLD_Callbacks *callbacks);
void RLD_Interrupt(RLD_Context *ctx, const RLD_Profile *profile, const RLD_Callbacks *callbacks);
int RLD_IsBusy(const RLD_Context *ctx);
int RLD_IsDone(const RLD_Context *ctx);
void RLD_ClearEvents(RLD_Context *ctx);
void RLD_GetHudSample(const RLD_Context *ctx, const RLD_Profile *profile, RLD_HudSample *sample);

const char *RLD_StateName(int state);
const char *RLD_ResultName(int result);

#ifdef __cplusplus
}
#endif

#endif
