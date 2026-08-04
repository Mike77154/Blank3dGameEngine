#include "gprojectiletravel89.h"

#define GPT89_INF_FX 0x3fffffffL

static GPT89_Fx gpt89_abs_fx(GPT89_Fx value)
{
    return value < 0L ? -value : value;
}

static GPT89_Fx gpt89_fx_mul(GPT89_Fx a, GPT89_Fx b)
{
    return (a * b) >> GPT89_FIX_SHIFT;
}

static GPT89_Vec3 gpt89_v3_sub(GPT89_Vec3 a, GPT89_Vec3 b)
{
    return gpt89_v3(a.x - b.x, a.y - b.y, a.z - b.z);
}

static GPT89_Vec3 gpt89_v3_add(GPT89_Vec3 a, GPT89_Vec3 b)
{
    return gpt89_v3(a.x + b.x, a.y + b.y, a.z + b.z);
}

static GPT89_Vec3 gpt89_v3_scale(GPT89_Vec3 value, GPT89_Fx scale)
{
    return gpt89_v3(gpt89_fx_mul(value.x, scale),
                    gpt89_fx_mul(value.y, scale),
                    gpt89_fx_mul(value.z, scale));
}

static GPT89_Fx gpt89_length_approx(GPT89_Vec3 value)
{
    GPT89_Fx a;
    GPT89_Fx b;
    GPT89_Fx c;
    GPT89_Fx t;

    a = gpt89_abs_fx(value.x);
    b = gpt89_abs_fx(value.y);
    c = gpt89_abs_fx(value.z);

    if (a < b) { t = a; a = b; b = t; }
    if (b < c) { t = b; b = c; c = t; }
    if (a < b) { t = a; a = b; b = t; }

    return a + ((b * 3L) >> 3) + (c >> 2);
}

static GPT89_Vec3 gpt89_normalize(GPT89_Vec3 value)
{
    GPT89_Fx length;
    length = gpt89_length_approx(value);
    if (length <= 0L) return gpt89_v3(0L, 0L, GPT89_FIX_ONE);
    return gpt89_v3((value.x << GPT89_FIX_SHIFT) / length,
                    (value.y << GPT89_FIX_SHIFT) / length,
                    (value.z << GPT89_FIX_SHIFT) / length);
}

static void gpt89_zero_bytes(void *ptr, unsigned long size)
{
    unsigned char *bytes;
    unsigned long i;
    if (!ptr) return;
    bytes = (unsigned char *)ptr;
    for (i = 0UL; i < size; i++) bytes[i] = 0u;
}

static int gpt89_valid_slot(const GPT89_Pool *pool, int slot)
{
    if (!pool) return 0;
    if (slot < 0 || slot >= GPT89_MAX_PROJECTILES) return 0;
    return pool->projectiles[slot].active ? 1 : 0;
}

static int gpt89_finish(GPT89_Pool *pool, int slot, int reason)
{
    GPT89_State *state;
    if (!gpt89_valid_slot(pool, slot)) return GPT89_NOT_FOUND;
    state = &pool->projectiles[slot];
    state->active = 0;
    state->end_reason = reason;
    if (pool->active_count > 0) pool->active_count--;
    if (pool->end_fn) pool->end_fn(pool->end_ctx, state);
    return reason;
}

static GPT89_Fx gpt89_step_distance(GPT89_Fx speed_fx, unsigned long dt_ms)
{
    unsigned long limited_dt;
    if (speed_fx <= 0L || dt_ms == 0UL) return 0L;
    limited_dt = dt_ms;
    if (limited_dt > 60000UL) limited_dt = 60000UL;
    return (GPT89_Fx)((speed_fx * (GPT89_Fx)limited_dt) / 1000L);
}

static unsigned long gpt89_ms_for_distance(GPT89_Fx distance_fx, GPT89_Fx speed_fx)
{
    if (distance_fx <= 0L || speed_fx <= 0L) return 0UL;
    return (unsigned long)((distance_fx * 1000L) / speed_fx);
}

static GPT89_Fx gpt89_destination_remaining(const GPT89_State *state)
{
    GPT89_Fx distance;
    if (!state || !state->has_destination) return GPT89_INF_FX;
    distance = gpt89_distance_approx(state->position, state->destination);
    distance -= state->arrival_radius_fx;
    if (distance < 0L) distance = 0L;
    return distance;
}

GPT89_Fx gpt89_fx_from_int(int value)
{
    return ((GPT89_Fx)value) << GPT89_FIX_SHIFT;
}

int gpt89_fx_to_int_round(GPT89_Fx value)
{
    if (value >= 0L) return (int)((value + GPT89_FIX_HALF) >> GPT89_FIX_SHIFT);
    return (int)(-(((-value) + GPT89_FIX_HALF) >> GPT89_FIX_SHIFT));
}

GPT89_Vec3 gpt89_v3(GPT89_Fx x, GPT89_Fx y, GPT89_Fx z)
{
    GPT89_Vec3 value;
    value.x = x;
    value.y = y;
    value.z = z;
    return value;
}

GPT89_Fx gpt89_distance_approx(GPT89_Vec3 a, GPT89_Vec3 b)
{
    return gpt89_length_approx(gpt89_v3_sub(a, b));
}

void gpt89_desc_defaults(GPT89_Desc *desc)
{
    if (!desc) return;
    gpt89_zero_bytes(desc, (unsigned long)sizeof(*desc));
    desc->mode = GPT89_MODE_INTERNAL_MOVE;
    desc->direction = gpt89_v3(0L, 0L, GPT89_FIX_ONE);
    desc->arrival_radius_fx = GPT89_FIX_ONE / 8L;
}

void gpt89_pool_init(GPT89_Pool *pool)
{
    if (!pool) return;
    gpt89_zero_bytes(pool, (unsigned long)sizeof(*pool));
}

void gpt89_set_end_hook(GPT89_Pool *pool, GPT89_EndFn end_fn, void *ctx)
{
    if (!pool) return;
    pool->end_fn = end_fn;
    pool->end_ctx = ctx;
}

int gpt89_spawn(GPT89_Pool *pool, const GPT89_Desc *desc)
{
    GPT89_State *state;
    int slot;

    if (!pool || !desc) return GPT89_BAD_ARG;
    slot = -1;
    {
        int i;
        for (i = 0; i < GPT89_MAX_PROJECTILES; i++) {
            if (!pool->projectiles[i].active) {
                slot = i;
                break;
            }
        }
    }
    if (slot < 0) return GPT89_FULL;

    state = &pool->projectiles[slot];
    gpt89_zero_bytes(state, (unsigned long)sizeof(*state));
    state->active = 1;
    state->end_reason = GPT89_END_NONE;
    state->slot = slot;
    state->projectile_id = desc->projectile_id;
    state->owner_id = desc->owner_id;
    state->weapon_id = desc->weapon_id;
    state->team_id = desc->team_id;
    state->user_tag = desc->user_tag;
    state->mode = desc->mode;
    state->origin = desc->origin;
    state->previous_position = desc->origin;
    state->position = desc->origin;
    state->direction = gpt89_normalize(desc->direction);
    state->destination = desc->destination;
    state->has_destination = desc->has_destination ? 1 : 0;
    state->speed_fx = desc->speed_fx;
    state->max_distance_fx = desc->max_distance_fx;
    state->arrival_radius_fx = desc->arrival_radius_fx;
    if (state->arrival_radius_fx < 0L) state->arrival_radius_fx = 0L;
    state->life_ms = desc->life_ms;
    pool->active_count++;

    if (state->has_destination && gpt89_destination_remaining(state) <= 0L) {
        gpt89_finish(pool, slot, GPT89_END_DESTINATION);
    } else if (state->max_distance_fx < 0L) {
        state->max_distance_fx = 0L;
    }

    return slot;
}

int gpt89_kill(GPT89_Pool *pool, int slot, int reason)
{
    if (reason == GPT89_END_NONE) reason = GPT89_END_MANUAL;
    return gpt89_finish(pool, slot, reason);
}

int gpt89_update_one(GPT89_Pool *pool, int slot, unsigned long dt_ms)
{
    GPT89_State *state;
    unsigned long move_ms;
    unsigned long remaining_life_ms;
    unsigned long used_ms;
    GPT89_Fx requested_step;
    GPT89_Fx actual_step;
    GPT89_Fx range_remaining;
    GPT89_Fx destination_remaining;
    int end_reason;

    if (!gpt89_valid_slot(pool, slot)) return GPT89_NOT_FOUND;
    state = &pool->projectiles[slot];
    if (state->mode == GPT89_MODE_EXTERNAL_POSITION) return GPT89_OK;

    move_ms = dt_ms;
    if (state->life_ms > 0UL) {
        if (state->elapsed_ms >= state->life_ms) {
            return gpt89_finish(pool, slot, GPT89_END_TIMEOUT);
        }
        remaining_life_ms = state->life_ms - state->elapsed_ms;
        if (move_ms > remaining_life_ms) move_ms = remaining_life_ms;
    }

    requested_step = gpt89_step_distance(state->speed_fx, move_ms);
    actual_step = requested_step;
    end_reason = GPT89_END_NONE;

    range_remaining = GPT89_INF_FX;
    if (state->max_distance_fx > 0L) {
        range_remaining = state->max_distance_fx - state->traveled_fx;
        if (range_remaining < 0L) range_remaining = 0L;
        if (range_remaining <= actual_step) {
            actual_step = range_remaining;
            end_reason = GPT89_END_MAX_DISTANCE;
        }
    }

    destination_remaining = gpt89_destination_remaining(state);
    if (state->has_destination && destination_remaining <= actual_step) {
        if (end_reason == GPT89_END_NONE || destination_remaining <= range_remaining) {
            actual_step = destination_remaining;
            end_reason = GPT89_END_DESTINATION;
        }
    }

    state->previous_position = state->position;
    if (actual_step > 0L) {
        state->position = gpt89_v3_add(state->position,
                                      gpt89_v3_scale(state->direction, actual_step));
        state->traveled_fx += actual_step;
    }

    used_ms = move_ms;
    if (end_reason != GPT89_END_NONE && actual_step < requested_step && state->speed_fx > 0L) {
        used_ms = gpt89_ms_for_distance(actual_step, state->speed_fx);
        if (used_ms > move_ms) used_ms = move_ms;
    }
    state->elapsed_ms += used_ms;

    if (end_reason == GPT89_END_DESTINATION) {
        state->position = state->destination;
        return gpt89_finish(pool, slot, end_reason);
    }
    if (end_reason == GPT89_END_MAX_DISTANCE) {
        return gpt89_finish(pool, slot, end_reason);
    }

    if (state->life_ms > 0UL && state->elapsed_ms >= state->life_ms) {
        return gpt89_finish(pool, slot, GPT89_END_TIMEOUT);
    }

    return GPT89_OK;
}

int gpt89_update_all(GPT89_Pool *pool, unsigned long dt_ms)
{
    int i;
    int updated;
    if (!pool) return GPT89_BAD_ARG;
    updated = 0;
    for (i = 0; i < GPT89_MAX_PROJECTILES; i++) {
        if (pool->projectiles[i].active &&
            pool->projectiles[i].mode == GPT89_MODE_INTERNAL_MOVE) {
            gpt89_update_one(pool, i, dt_ms);
            updated++;
        }
    }
    return updated;
}

int gpt89_observe_position(GPT89_Pool *pool,
                           int slot,
                           GPT89_Vec3 new_position,
                           unsigned long dt_ms)
{
    GPT89_State *state;
    GPT89_Fx moved;

    if (!gpt89_valid_slot(pool, slot)) return GPT89_NOT_FOUND;
    state = &pool->projectiles[slot];
    state->previous_position = state->position;
    state->position = new_position;
    moved = gpt89_distance_approx(state->previous_position, state->position);
    state->traveled_fx += moved;
    state->elapsed_ms += dt_ms;

    if (state->has_destination && gpt89_destination_remaining(state) <= 0L) {
        state->position = state->destination;
        return gpt89_finish(pool, slot, GPT89_END_DESTINATION);
    }
    if (state->max_distance_fx > 0L && state->traveled_fx >= state->max_distance_fx) {
        return gpt89_finish(pool, slot, GPT89_END_MAX_DISTANCE);
    }
    if (state->life_ms > 0UL && state->elapsed_ms >= state->life_ms) {
        return gpt89_finish(pool, slot, GPT89_END_TIMEOUT);
    }
    return GPT89_OK;
}

const GPT89_State *gpt89_get(const GPT89_Pool *pool, int slot)
{
    if (!gpt89_valid_slot(pool, slot)) return 0;
    return &pool->projectiles[slot];
}

GPT89_State *gpt89_get_mutable(GPT89_Pool *pool, int slot)
{
    if (!gpt89_valid_slot(pool, slot)) return 0;
    return &pool->projectiles[slot];
}

GPT89_Fx gpt89_remaining_distance(const GPT89_State *state)
{
    GPT89_Fx remaining;
    if (!state || state->max_distance_fx <= 0L) return GPT89_INF_FX;
    remaining = state->max_distance_fx - state->traveled_fx;
    return remaining > 0L ? remaining : 0L;
}

unsigned long gpt89_remaining_life_ms(const GPT89_State *state)
{
    if (!state || state->life_ms == 0UL) return 0xffffffffUL;
    if (state->elapsed_ms >= state->life_ms) return 0UL;
    return state->life_ms - state->elapsed_ms;
}
