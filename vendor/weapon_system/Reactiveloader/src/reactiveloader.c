#include "reactiveloader.h"
#include <string.h>

static void rld_zero(void *ptr, unsigned long bytes)
{
    unsigned char *p;
    unsigned long i;
    p = (unsigned char *)ptr;
    for (i = 0; i < bytes; ++i) {
        p[i] = 0u;
    }
}

static void rld_copy_name(char *dst, const char *src)
{
    int i;
    if (dst == 0) {
        return;
    }
    if (src == 0) {
        dst[0] = '\0';
        return;
    }
    for (i = 0; i < RLD_MAX_NAME - 1; ++i) {
        dst[i] = src[i];
        if (src[i] == '\0') {
            return;
        }
    }
    dst[RLD_MAX_NAME - 1] = '\0';
}

static int rld_streq(const char *a, const char *b)
{
    if (a == 0 || b == 0) {
        return RLD_FALSE;
    }
    while (*a != '\0' && *b != '\0') {
        if (*a != *b) {
            return RLD_FALSE;
        }
        ++a;
        ++b;
    }
    return (*a == '\0' && *b == '\0') ? RLD_TRUE : RLD_FALSE;
}

static void rld_emit(RLD_Context *ctx, int event_bit, const RLD_Callbacks *cb)
{
    if (ctx == 0) {
        return;
    }
    ctx->events |= event_bit;
    if (cb == 0) {
        return;
    }
    if (event_bit == RLD_EVENT_RELOAD_BEGIN && cb->on_reload_begin != 0) {
        cb->on_reload_begin(ctx, cb->user);
    } else if (event_bit == RLD_EVENT_SWEEP_BEGIN && cb->on_sweep_begin != 0) {
        cb->on_sweep_begin(ctx, cb->user);
    } else if (event_bit == RLD_EVENT_GOOD && cb->on_good != 0) {
        cb->on_good(ctx, cb->user);
    } else if (event_bit == RLD_EVENT_PERFECT && cb->on_perfect != 0) {
        cb->on_perfect(ctx, cb->user);
    } else if (event_bit == RLD_EVENT_FAIL && cb->on_fail != 0) {
        cb->on_fail(ctx, cb->user);
    } else if (event_bit == RLD_EVENT_NORMAL && cb->on_normal != 0) {
        cb->on_normal(ctx, cb->user);
    } else if (event_bit == RLD_EVENT_JAM && cb->on_jam != 0) {
        cb->on_jam(ctx, cb->user);
    } else if (event_bit == RLD_EVENT_SHELL_LOADED && cb->on_shell_loaded != 0) {
        cb->on_shell_loaded(ctx, cb->user);
    } else if (event_bit == RLD_EVENT_INTERRUPT && cb->on_interrupt != 0) {
        cb->on_interrupt(ctx, cb->user);
    } else if (event_bit == RLD_EVENT_DONE && cb->on_done != 0) {
        cb->on_done(ctx, cb->user);
    } else if (event_bit == RLD_EVENT_CANCEL && cb->on_cancel != 0) {
        cb->on_cancel(ctx, cb->user);
    }
}

static void rld_enter_finishing(RLD_Context *ctx, const RLD_Profile *profile, int ticks)
{
    if (ctx == 0 || profile == 0) {
        return;
    }
    if (ticks < 1) {
        ticks = 1;
    }
    ctx->state = RLD_STATE_FINISHING;
    ctx->ticks_in_state = 0;
    if ((profile->flags & RLD_FLAG_SHELL_BY_SHELL) != 0) {
        if (profile->shell_ticks > 0) {
            ctx->finish_ticks_left = profile->shell_ticks;
        } else {
            ctx->finish_ticks_left = ticks;
        }
    } else {
        ctx->finish_ticks_left = ticks;
    }
}

static int rld_cursor_inside(RLD_Fixed cursor, RLD_Fixed a, RLD_Fixed b)
{
    if (a <= b) {
        return (cursor >= a && cursor <= b) ? RLD_TRUE : RLD_FALSE;
    }
    return (cursor >= b && cursor <= a) ? RLD_TRUE : RLD_FALSE;
}

static int rld_sweep_finished(const RLD_Context *ctx, const RLD_Profile *profile)
{
    if (ctx == 0 || profile == 0) {
        return RLD_TRUE;
    }
    if (profile->cursor_speed_q16 >= 0) {
        return (ctx->cursor_q16 >= profile->cursor_end_q16) ? RLD_TRUE : RLD_FALSE;
    }
    return (ctx->cursor_q16 <= profile->cursor_end_q16) ? RLD_TRUE : RLD_FALSE;
}

static void rld_resolve_normal(RLD_Context *ctx, const RLD_Profile *profile, const RLD_Callbacks *callbacks)
{
    ctx->result = RLD_RESULT_NORMAL;
    ctx->active_resolved = RLD_TRUE;
    rld_emit(ctx, RLD_EVENT_NORMAL, callbacks);
    rld_enter_finishing(ctx, profile, profile->normal_reload_ticks);
}

static void rld_resolve_fail(RLD_Context *ctx, const RLD_Profile *profile, const RLD_Callbacks *callbacks)
{
    ctx->active_resolved = RLD_TRUE;
    if ((profile->flags & RLD_FLAG_SAFE_FAIL) != 0) {
        ctx->result = RLD_RESULT_NORMAL;
        rld_emit(ctx, RLD_EVENT_NORMAL, callbacks);
        rld_enter_finishing(ctx, profile, profile->normal_reload_ticks);
        return;
    }
    ctx->result = RLD_RESULT_FAIL;
    rld_emit(ctx, RLD_EVENT_FAIL, callbacks);
    if ((profile->flags & RLD_FLAG_HARDCORE_JAM) != 0 && profile->jam_ticks > 0) {
        ctx->state = RLD_STATE_JAMMED;
        ctx->ticks_in_state = 0;
        ctx->jam_ticks_left = profile->jam_ticks;
        ctx->result = RLD_RESULT_JAM;
        rld_emit(ctx, RLD_EVENT_JAM, callbacks);
    } else {
        rld_enter_finishing(ctx, profile, profile->normal_reload_ticks + profile->fail_penalty_ticks);
    }
}

static void rld_resolve_cursor(RLD_Context *ctx, const RLD_Profile *profile, const RLD_Callbacks *callbacks)
{
    if (rld_cursor_inside(ctx->cursor_q16, profile->perfect_start_q16, profile->perfect_end_q16) != 0) {
        ctx->result = RLD_RESULT_PERFECT;
        ctx->active_resolved = RLD_TRUE;
        ctx->bonus_ticks_left = profile->perfect_bonus_ticks;
        rld_emit(ctx, RLD_EVENT_PERFECT, callbacks);
        rld_enter_finishing(ctx, profile, profile->perfect_finish_ticks);
    } else if (rld_cursor_inside(ctx->cursor_q16, profile->good_start_q16, profile->good_end_q16) != 0) {
        ctx->result = RLD_RESULT_GOOD;
        ctx->active_resolved = RLD_TRUE;
        ctx->bonus_ticks_left = profile->good_bonus_ticks;
        rld_emit(ctx, RLD_EVENT_GOOD, callbacks);
        rld_enter_finishing(ctx, profile, profile->good_finish_ticks);
    } else {
        rld_resolve_fail(ctx, profile, callbacks);
    }
}

static void rld_load_units(RLD_Context *ctx, const RLD_Profile *profile, int amount, const RLD_Callbacks *callbacks)
{
    int max_step;
    if (ctx == 0 || profile == 0) {
        return;
    }
    if (amount < 1) {
        amount = 1;
    }
    max_step = profile->max_units_loaded_per_reload;
    if (max_step > 0) {
        if (ctx->units_loaded + amount > max_step) {
            amount = max_step - ctx->units_loaded;
        }
    }
    if (ctx->units_loaded + amount > ctx->units_needed) {
        amount = ctx->units_needed - ctx->units_loaded;
    }
    if (amount < 0) {
        amount = 0;
    }
    ctx->units_loaded_this_step = amount;
    ctx->units_loaded += amount;
    ctx->ammo_after = ctx->ammo_before + ctx->units_loaded;
    if (amount > 0) {
        rld_emit(ctx, RLD_EVENT_SHELL_LOADED, callbacks);
    }
}

static void rld_finish_done(RLD_Context *ctx, const RLD_Callbacks *callbacks)
{
    ctx->state = RLD_STATE_DONE;
    ctx->is_busy = RLD_FALSE;
    ctx->ticks_in_state = 0;
    rld_emit(ctx, RLD_EVENT_DONE, callbacks);
}

RLD_Fixed RLD_FromInt(int v)
{
    return (RLD_Fixed)(v * RLD_Q16_ONE);
}

RLD_Fixed RLD_FromPercent(int pct)
{
    return (RLD_Fixed)((pct * RLD_Q16_ONE) / 100);
}

int RLD_ToInt(RLD_Fixed v)
{
    return (int)(v / RLD_Q16_ONE);
}

int RLD_ToPercent(RLD_Fixed v)
{
    return (int)((v * 100) / RLD_Q16_ONE);
}

RLD_Fixed RLD_ClampQ16(RLD_Fixed v, RLD_Fixed lo, RLD_Fixed hi)
{
    if (lo > hi) {
        RLD_Fixed t;
        t = lo;
        lo = hi;
        hi = t;
    }
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

void RLD_ArenaInit(RLD_Arena *arena, void *buffer, unsigned long bytes)
{
    if (arena == 0) {
        return;
    }
    arena->base = (unsigned char *)buffer;
    arena->capacity = bytes;
    arena->used = 0u;
    arena->error = 0;
    if (buffer == 0 || bytes == 0u) {
        arena->error = RLD_ARENA_OUT_OF_MEMORY;
    }
}

void *RLD_ArenaAlloc(RLD_Arena *arena, unsigned long bytes, unsigned long align)
{
    unsigned long mask;
    unsigned long pos;
    unsigned long aligned;
    void *out;
    if (arena == 0 || arena->base == 0 || bytes == 0u) {
        if (arena != 0) {
            arena->error = RLD_ARENA_OUT_OF_MEMORY;
        }
        return 0;
    }
    if (align < 1u) {
        align = 1u;
    }
    mask = align - 1u;
    pos = arena->used;
    aligned = (pos + mask) & ~mask;
    if (aligned + bytes > arena->capacity) {
        arena->error = RLD_ARENA_OUT_OF_MEMORY;
        return 0;
    }
    out = (void *)(arena->base + aligned);
    arena->used = aligned + bytes;
    rld_zero(out, bytes);
    return out;
}

void RLD_ArenaReset(RLD_Arena *arena)
{
    if (arena == 0) {
        return;
    }
    arena->used = 0u;
    arena->error = 0;
}

void RLD_DefaultProfile(RLD_Profile *p)
{
    if (p == 0) {
        return;
    }
    rld_zero(p, (unsigned long)sizeof(RLD_Profile));
    rld_copy_name(p->name, "default");
    p->id = 0;
    p->flags = RLD_FLAG_ENABLED | RLD_FLAG_KEEP_NORMAL_IF_NO_PRESS;
    p->begin_ticks = 8;
    p->normal_reload_ticks = 72;
    p->good_finish_ticks = 42;
    p->perfect_finish_ticks = 24;
    p->fail_penalty_ticks = 16;
    p->jam_ticks = 0;
    p->cursor_start_q16 = RLD_FromPercent(0);
    p->cursor_end_q16 = RLD_FromPercent(100);
    p->cursor_speed_q16 = RLD_FromPercent(4);
    p->good_start_q16 = RLD_FromPercent(45);
    p->good_end_q16 = RLD_FromPercent(72);
    p->perfect_start_q16 = RLD_FromPercent(58);
    p->perfect_end_q16 = RLD_FromPercent(63);
    p->shell_ticks = 16;
    p->shells_per_step = 1;
    p->max_units_loaded_per_reload = 0;
    p->perfect_bonus_ticks = 120;
    p->good_bonus_ticks = 60;
    p->bonus_mask = RLD_BONUS_ACCURACY | RLD_BONUS_STABILITY;
    p->bonus_damage_q16 = RLD_FromPercent(0);
    p->bonus_accuracy_q16 = RLD_FromPercent(8);
    p->bonus_stability_q16 = RLD_FromPercent(8);
    p->bonus_fire_rate_q16 = RLD_FromPercent(0);
    p->bonus_spread_q16 = RLD_FromPercent(0);
    p->hud_x_q16 = RLD_FromPercent(50);
    p->hud_y_q16 = RLD_FromPercent(55);
    p->hud_w_q16 = RLD_FromPercent(34);
    p->hud_h_q16 = RLD_FromPercent(3);
}

void RLD_MakePistolProfile(RLD_Profile *p, int id)
{
    RLD_DefaultProfile(p);
    if (p == 0) {
        return;
    }
    rld_copy_name(p->name, "pistol_9mm");
    p->id = id;
    p->normal_reload_ticks = 72;
    p->good_finish_ticks = 38;
    p->perfect_finish_ticks = 26;
    p->fail_penalty_ticks = 12;
    p->good_start_q16 = RLD_FromPercent(45);
    p->good_end_q16 = RLD_FromPercent(70);
    p->perfect_start_q16 = RLD_FromPercent(56);
    p->perfect_end_q16 = RLD_FromPercent(61);
    p->bonus_mask = RLD_BONUS_ACCURACY | RLD_BONUS_STABILITY;
    p->bonus_accuracy_q16 = RLD_FromPercent(8);
    p->bonus_stability_q16 = RLD_FromPercent(6);
}

void RLD_MakeShotgunProfile(RLD_Profile *p, int id)
{
    RLD_DefaultProfile(p);
    if (p == 0) {
        return;
    }
    rld_copy_name(p->name, "shotgun_shells");
    p->id = id;
    p->flags = RLD_FLAG_ENABLED | RLD_FLAG_KEEP_NORMAL_IF_NO_PRESS | RLD_FLAG_ALLOW_INTERRUPT | RLD_FLAG_SHELL_BY_SHELL;
    p->normal_reload_ticks = 64;
    p->good_finish_ticks = 14;
    p->perfect_finish_ticks = 10;
    p->fail_penalty_ticks = 8;
    p->shell_ticks = 16;
    p->shells_per_step = 1;
    p->good_start_q16 = RLD_FromPercent(40);
    p->good_end_q16 = RLD_FromPercent(75);
    p->perfect_start_q16 = RLD_FromPercent(55);
    p->perfect_end_q16 = RLD_FromPercent(63);
    p->bonus_mask = RLD_BONUS_SPREAD | RLD_BONUS_STABILITY;
    p->bonus_spread_q16 = RLD_FromPercent(10);
    p->bonus_stability_q16 = RLD_FromPercent(8);
}

void RLD_MakeMagnumProfile(RLD_Profile *p, int id)
{
    RLD_DefaultProfile(p);
    if (p == 0) {
        return;
    }
    rld_copy_name(p->name, "magnum_risk");
    p->id = id;
    p->flags = RLD_FLAG_ENABLED | RLD_FLAG_KEEP_NORMAL_IF_NO_PRESS | RLD_FLAG_HARDCORE_JAM;
    p->normal_reload_ticks = 110;
    p->good_finish_ticks = 60;
    p->perfect_finish_ticks = 36;
    p->fail_penalty_ticks = 40;
    p->jam_ticks = 24;
    p->cursor_speed_q16 = RLD_FromPercent(3);
    p->good_start_q16 = RLD_FromPercent(50);
    p->good_end_q16 = RLD_FromPercent(68);
    p->perfect_start_q16 = RLD_FromPercent(59);
    p->perfect_end_q16 = RLD_FromPercent(62);
    p->bonus_mask = RLD_BONUS_DAMAGE | RLD_BONUS_STABILITY;
    p->bonus_damage_q16 = RLD_FromPercent(15);
    p->bonus_stability_q16 = RLD_FromPercent(5);
}

void RLD_MakeSniperProfile(RLD_Profile *p, int id)
{
    RLD_DefaultProfile(p);
    if (p == 0) {
        return;
    }
    rld_copy_name(p->name, "sniper_precision");
    p->id = id;
    p->normal_reload_ticks = 96;
    p->good_finish_ticks = 50;
    p->perfect_finish_ticks = 30;
    p->fail_penalty_ticks = 24;
    p->good_start_q16 = RLD_FromPercent(48);
    p->good_end_q16 = RLD_FromPercent(72);
    p->perfect_start_q16 = RLD_FromPercent(61);
    p->perfect_end_q16 = RLD_FromPercent(65);
    p->bonus_mask = RLD_BONUS_ACCURACY | RLD_BONUS_STABILITY;
    p->bonus_accuracy_q16 = RLD_FromPercent(15);
    p->bonus_stability_q16 = RLD_FromPercent(12);
}

void RLD_MakeLauncherProfile(RLD_Profile *p, int id)
{
    RLD_DefaultProfile(p);
    if (p == 0) {
        return;
    }
    rld_copy_name(p->name, "launcher_heavy");
    p->id = id;
    p->flags = RLD_FLAG_ENABLED | RLD_FLAG_KEEP_NORMAL_IF_NO_PRESS;
    p->normal_reload_ticks = 150;
    p->good_finish_ticks = 90;
    p->perfect_finish_ticks = 60;
    p->fail_penalty_ticks = 35;
    p->cursor_speed_q16 = RLD_FromPercent(2);
    p->good_start_q16 = RLD_FromPercent(35);
    p->good_end_q16 = RLD_FromPercent(58);
    p->perfect_start_q16 = RLD_FromPercent(47);
    p->perfect_end_q16 = RLD_FromPercent(51);
    p->bonus_mask = RLD_BONUS_STABILITY | RLD_BONUS_CUSTOM0;
    p->bonus_stability_q16 = RLD_FromPercent(10);
}

void RLD_ProfileBankInit(RLD_ProfileBank *bank, RLD_Profile *items, int max_count)
{
    if (bank == 0) {
        return;
    }
    bank->items = items;
    bank->count = 0;
    bank->max_count = max_count;
    if (items == 0 || max_count < 1) {
        bank->max_count = 0;
    }
}

int RLD_ProfileBankInitArena(RLD_ProfileBank *bank, RLD_Arena *arena, int max_count)
{
    RLD_Profile *items;
    unsigned long bytes;
    if (bank == 0 || arena == 0 || max_count < 1) {
        return RLD_ARENA_OUT_OF_MEMORY;
    }
    bytes = (unsigned long)sizeof(RLD_Profile) * (unsigned long)max_count;
    items = (RLD_Profile *)RLD_ArenaAlloc(arena, bytes, (unsigned long)sizeof(void *));
    if (items == 0) {
        RLD_ProfileBankInit(bank, 0, 0);
        return RLD_ARENA_OUT_OF_MEMORY;
    }
    RLD_ProfileBankInit(bank, items, max_count);
    return 0;
}

int RLD_ProfileBankAdd(RLD_ProfileBank *bank, const RLD_Profile *profile)
{
    if (bank == 0 || profile == 0 || bank->items == 0) {
        return RLD_PROFILE_BANK_FULL;
    }
    if (bank->count >= bank->max_count) {
        return RLD_PROFILE_BANK_FULL;
    }
    bank->items[bank->count] = *profile;
    bank->count += 1;
    return bank->count - 1;
}

RLD_Profile *RLD_ProfileBankFindById(RLD_ProfileBank *bank, int id)
{
    int i;
    if (bank == 0 || bank->items == 0) {
        return 0;
    }
    for (i = 0; i < bank->count; ++i) {
        if (bank->items[i].id == id) {
            return &bank->items[i];
        }
    }
    return 0;
}

RLD_Profile *RLD_ProfileBankFindByName(RLD_ProfileBank *bank, const char *name)
{
    int i;
    if (bank == 0 || bank->items == 0) {
        return 0;
    }
    for (i = 0; i < bank->count; ++i) {
        if (rld_streq(bank->items[i].name, name) != 0) {
            return &bank->items[i];
        }
    }
    return 0;
}

void RLD_ContextInit(RLD_Context *ctx, int entity_id, int weapon_id)
{
    if (ctx == 0) {
        return;
    }
    rld_zero(ctx, (unsigned long)sizeof(RLD_Context));
    ctx->entity_id = entity_id;
    ctx->weapon_id = weapon_id;
    ctx->state = RLD_STATE_IDLE;
    ctx->result = RLD_RESULT_NONE;
}

void RLD_Begin(RLD_Context *ctx, const RLD_Profile *profile, int ammo_before, int units_needed)
{
    if (ctx == 0 || profile == 0) {
        return;
    }
    if (units_needed < 1) {
        units_needed = 1;
    }
    ctx->profile_id = profile->id;
    ctx->state = RLD_STATE_RELOAD_BEGIN;
    ctx->result = RLD_RESULT_NONE;
    ctx->events = RLD_EVENT_RELOAD_BEGIN;
    ctx->ticks_in_state = 0;
    ctx->total_ticks = 0;
    ctx->finish_ticks_left = 0;
    ctx->jam_ticks_left = 0;
    ctx->bonus_ticks_left = 0;
    ctx->cursor_q16 = profile->cursor_start_q16;
    ctx->last_cursor_q16 = ctx->cursor_q16;
    ctx->ammo_before = ammo_before;
    ctx->ammo_after = ammo_before;
    ctx->units_needed = units_needed;
    ctx->units_loaded = 0;
    ctx->units_loaded_this_step = 0;
    ctx->active_attempted = RLD_FALSE;
    ctx->active_resolved = RLD_FALSE;
    ctx->is_busy = RLD_TRUE;
}

void RLD_Cancel(RLD_Context *ctx)
{
    if (ctx == 0) {
        return;
    }
    ctx->state = RLD_STATE_DONE;
    ctx->result = RLD_RESULT_CANCELLED;
    ctx->is_busy = RLD_FALSE;
    ctx->events |= RLD_EVENT_CANCEL | RLD_EVENT_DONE;
}

void RLD_Interrupt(RLD_Context *ctx, const RLD_Profile *profile, const RLD_Callbacks *callbacks)
{
    if (ctx == 0 || profile == 0) {
        return;
    }
    if ((profile->flags & RLD_FLAG_ALLOW_INTERRUPT) == 0) {
        return;
    }
    if (ctx->state != RLD_STATE_FINISHING && ctx->state != RLD_STATE_ACTIVE_SWEEP) {
        return;
    }
    ctx->result = RLD_RESULT_INTERRUPTED;
    ctx->state = RLD_STATE_DONE;
    ctx->is_busy = RLD_FALSE;
    rld_emit(ctx, RLD_EVENT_INTERRUPT, callbacks);
    rld_emit(ctx, RLD_EVENT_DONE, callbacks);
}

void RLD_Tick(RLD_Context *ctx, const RLD_Profile *profile, int input_flags, const RLD_Callbacks *callbacks)
{
    int step;
    int active_on;
    int press_reload;
    int press_fire;
    int press_cancel;
    int auto_reload;
    if (ctx == 0 || profile == 0) {
        return;
    }
    ctx->events = RLD_EVENT_NONE;
    ctx->units_loaded_this_step = 0;
    if (ctx->state == RLD_STATE_IDLE || ctx->state == RLD_STATE_DONE) {
        return;
    }
    ctx->total_ticks += 1;
    if (ctx->bonus_ticks_left > 0) {
        ctx->bonus_ticks_left -= 1;
    }
    active_on = ((profile->flags & RLD_FLAG_ENABLED) != 0) ? RLD_TRUE : RLD_FALSE;
    press_reload = ((input_flags & RLD_INPUT_RELOAD_PRESS) != 0) ? RLD_TRUE : RLD_FALSE;
    press_fire = ((input_flags & RLD_INPUT_FIRE_PRESS) != 0) ? RLD_TRUE : RLD_FALSE;
    press_cancel = ((input_flags & RLD_INPUT_CANCEL_PRESS) != 0) ? RLD_TRUE : RLD_FALSE;
    auto_reload = ((input_flags & RLD_INPUT_AUTO_RELOAD) != 0) ? RLD_TRUE : RLD_FALSE;

    if (press_cancel != 0) {
        RLD_Cancel(ctx);
        if (callbacks != 0 && callbacks->on_cancel != 0) {
            callbacks->on_cancel(ctx, callbacks->user);
        }
        return;
    }

    if (press_fire != 0 && (profile->flags & RLD_FLAG_ALLOW_INTERRUPT) != 0) {
        if (ctx->state == RLD_STATE_FINISHING || ctx->state == RLD_STATE_ACTIVE_SWEEP) {
            RLD_Interrupt(ctx, profile, callbacks);
            return;
        }
    }

    if (ctx->state == RLD_STATE_RELOAD_BEGIN) {
        if (ctx->ticks_in_state == 0) {
            rld_emit(ctx, RLD_EVENT_RELOAD_BEGIN, callbacks);
        }
        ctx->ticks_in_state += 1;
        if (ctx->ticks_in_state >= profile->begin_ticks) {
            if (active_on != 0 && auto_reload == 0) {
                ctx->state = RLD_STATE_ACTIVE_SWEEP;
                ctx->ticks_in_state = 0;
                ctx->cursor_q16 = profile->cursor_start_q16;
                ctx->last_cursor_q16 = ctx->cursor_q16;
                rld_emit(ctx, RLD_EVENT_SWEEP_BEGIN, callbacks);
            } else {
                rld_resolve_normal(ctx, profile, callbacks);
            }
        }
        return;
    }

    if (ctx->state == RLD_STATE_ACTIVE_SWEEP) {
        if (press_reload != 0) {
            ctx->active_attempted = RLD_TRUE;
            rld_resolve_cursor(ctx, profile, callbacks);
            return;
        }
        if (auto_reload != 0 && (profile->flags & RLD_FLAG_ALLOW_AUTO) != 0) {
            rld_resolve_normal(ctx, profile, callbacks);
            return;
        }
        ctx->ticks_in_state += 1;
        ctx->last_cursor_q16 = ctx->cursor_q16;
        ctx->cursor_q16 += profile->cursor_speed_q16;
        if (rld_sweep_finished(ctx, profile) != 0) {
            ctx->cursor_q16 = profile->cursor_end_q16;
            if ((profile->flags & RLD_FLAG_KEEP_NORMAL_IF_NO_PRESS) != 0) {
                rld_resolve_normal(ctx, profile, callbacks);
            } else {
                rld_resolve_fail(ctx, profile, callbacks);
            }
        }
        return;
    }

    if (ctx->state == RLD_STATE_JAMMED) {
        if (ctx->jam_ticks_left > 0) {
            ctx->jam_ticks_left -= 1;
        }
        ctx->ticks_in_state += 1;
        if (ctx->jam_ticks_left <= 0) {
            rld_enter_finishing(ctx, profile, profile->normal_reload_ticks + profile->fail_penalty_ticks);
        }
        return;
    }

    if (ctx->state == RLD_STATE_FINISHING) {
        if (ctx->finish_ticks_left > 0) {
            ctx->finish_ticks_left -= 1;
        }
        ctx->ticks_in_state += 1;
        if (ctx->finish_ticks_left > 0) {
            return;
        }
        if ((profile->flags & RLD_FLAG_SHELL_BY_SHELL) != 0) {
            step = profile->shells_per_step;
            if (step < 1) {
                step = 1;
            }
            rld_load_units(ctx, profile, step, callbacks);
            if (ctx->units_loaded >= ctx->units_needed) {
                rld_finish_done(ctx, callbacks);
            } else if (profile->max_units_loaded_per_reload > 0 && ctx->units_loaded >= profile->max_units_loaded_per_reload) {
                rld_finish_done(ctx, callbacks);
            } else {
                if (ctx->result == RLD_RESULT_PERFECT && profile->perfect_finish_ticks > 0) {
                    ctx->finish_ticks_left = profile->perfect_finish_ticks;
                } else if (ctx->result == RLD_RESULT_GOOD && profile->good_finish_ticks > 0) {
                    ctx->finish_ticks_left = profile->good_finish_ticks;
                } else {
                    ctx->finish_ticks_left = profile->shell_ticks;
                }
            }
        } else {
            rld_load_units(ctx, profile, ctx->units_needed, callbacks);
            rld_finish_done(ctx, callbacks);
        }
        return;
    }
}

int RLD_IsBusy(const RLD_Context *ctx)
{
    if (ctx == 0) {
        return RLD_FALSE;
    }
    return ctx->is_busy;
}

int RLD_IsDone(const RLD_Context *ctx)
{
    if (ctx == 0) {
        return RLD_FALSE;
    }
    return (ctx->state == RLD_STATE_DONE) ? RLD_TRUE : RLD_FALSE;
}

void RLD_ClearEvents(RLD_Context *ctx)
{
    if (ctx != 0) {
        ctx->events = RLD_EVENT_NONE;
    }
}

void RLD_GetHudSample(const RLD_Context *ctx, const RLD_Profile *profile, RLD_HudSample *sample)
{
    if (sample == 0) {
        return;
    }
    rld_zero(sample, (unsigned long)sizeof(RLD_HudSample));
    if (ctx == 0 || profile == 0) {
        return;
    }
    sample->visible = (ctx->state == RLD_STATE_ACTIVE_SWEEP) ? RLD_TRUE : RLD_FALSE;
    sample->cursor_q16 = ctx->cursor_q16;
    sample->good_start_q16 = profile->good_start_q16;
    sample->good_end_q16 = profile->good_end_q16;
    sample->perfect_start_q16 = profile->perfect_start_q16;
    sample->perfect_end_q16 = profile->perfect_end_q16;
    sample->state = (int)ctx->state;
    sample->result = (int)ctx->result;
}

const char *RLD_StateName(int state)
{
    switch (state) {
    case RLD_STATE_IDLE:
        return "IDLE";
    case RLD_STATE_RELOAD_BEGIN:
        return "RELOAD_BEGIN";
    case RLD_STATE_ACTIVE_SWEEP:
        return "ACTIVE_SWEEP";
    case RLD_STATE_FINISHING:
        return "FINISHING";
    case RLD_STATE_JAMMED:
        return "JAMMED";
    case RLD_STATE_DONE:
        return "DONE";
    default:
        return "UNKNOWN";
    }
}

const char *RLD_ResultName(int result)
{
    switch (result) {
    case RLD_RESULT_NONE:
        return "NONE";
    case RLD_RESULT_NORMAL:
        return "NORMAL";
    case RLD_RESULT_GOOD:
        return "GOOD";
    case RLD_RESULT_PERFECT:
        return "PERFECT";
    case RLD_RESULT_FAIL:
        return "FAIL";
    case RLD_RESULT_JAM:
        return "JAM";
    case RLD_RESULT_CANCELLED:
        return "CANCELLED";
    case RLD_RESULT_INTERRUPTED:
        return "INTERRUPTED";
    default:
        return "UNKNOWN";
    }
}
