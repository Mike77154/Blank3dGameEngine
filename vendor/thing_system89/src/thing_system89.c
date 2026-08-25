#include "thing_system89.h"
#include <string.h>

static void ts89_set_error(TS89_System *system, int code)
{
    if (system) system->last_error = code;
}

static int ts89_syntax_valid(TS89_Thing thing)
{
    unsigned long raw;
    unsigned long max_raw;
    if (thing < 0) return 0;
    raw = (unsigned long)thing;
    max_raw = (unsigned long)TS89_CAPACITY * (unsigned long)TS89_GENERATION_LIMIT;
    if (max_raw > 0UL) --max_raw;
    return raw <= max_raw;
}

int ts89_index(TS89_Thing thing)
{
    if (!ts89_syntax_valid(thing)) return -1;
    return (int)((unsigned long)thing % (unsigned long)TS89_CAPACITY);
}

unsigned int ts89_generation(TS89_Thing thing)
{
    if (!ts89_syntax_valid(thing)) return 0U;
    return (unsigned int)((unsigned long)thing / (unsigned long)TS89_CAPACITY);
}

static TS89_Thing ts89_pack(int index, unsigned int generation)
{
    unsigned long raw;
    raw = (unsigned long)generation * (unsigned long)TS89_CAPACITY + (unsigned long)index;
    if (raw > (unsigned long)INT_MAX) return TS89_THING_INVALID;
    return (TS89_Thing)raw;
}

static unsigned int ts89_next_generation(unsigned int generation)
{
    ++generation;
    if (generation >= TS89_GENERATION_LIMIT) generation = 0U;
    return generation;
}

static void ts89_zero_slot(TS89_System *system, int index)
{
    system->locked[index] = 0U;
    system->flags[index] = 0UL;
    system->name_space[index] = 0U;
    system->user[index] = 0UL;
}

static void ts89_wipe(TS89_System *system)
{
    int i;
    if (!system) return;
    memset(system, 0, sizeof(*system));
    system->last_error = TS89_OK;
    for (i = 0; i < TS89_CAPACITY; ++i)
        system->state[i] = (unsigned char)TS89_STATE_DISABLED;
}

void ts89_init(TS89_System *system, int max_things)
{
    int i;
    int limit;
    int clamped;
    if (!system) return;
    ts89_wipe(system);
    limit = max_things;
    clamped = 0;
    if (limit < 0) { limit = 0; clamped = 1; }
    if (limit > TS89_CAPACITY) { limit = TS89_CAPACITY; clamped = 1; }
    system->initialized = 1;
    system->limit = limit;
    for (i = 0; i < limit; ++i) {
        system->state[i] = (unsigned char)TS89_STATE_FREE;
        system->free_stack[system->free_top++] = limit - 1 - i;
    }
    if (max_things <= 0) system->last_error = TS89_ERROR_INVALID_LIMIT;
    else if (clamped) system->last_error = TS89_ERROR_CLAMPED_LIMIT;
}

void ts89_reset(TS89_System *system)
{
    int limit;
    if (!system || !system->initialized) {
        ts89_set_error(system, TS89_ERROR_NOT_INITIALIZED);
        return;
    }
    limit = system->limit;
    ts89_init(system, limit);
}

void ts89_shutdown(TS89_System *system) { ts89_wipe(system); }
int ts89_is_initialized(const TS89_System *system) { return system && system->initialized; }
int ts89_limit(const TS89_System *system) { return system ? system->limit : 0; }
int ts89_alive_count(const TS89_System *system) { return system ? system->alive_count : 0; }
int ts89_reserved_count(const TS89_System *system) { return system ? system->reserved_count : 0; }
int ts89_in_use_count(const TS89_System *system) { return system ? system->alive_count + system->reserved_count : 0; }
int ts89_free_count(const TS89_System *system) { return system ? system->free_top : 0; }
int ts89_retired_count(const TS89_System *system) { return system ? system->retired_count : 0; }
int ts89_last_error(const TS89_System *system) { return system ? system->last_error : TS89_ERROR_NOT_INITIALIZED; }

const char *ts89_error_string(int code)
{
    switch (code) {
    case TS89_OK: return "ok";
    case TS89_ERROR_NOT_INITIALIZED: return "not initialized";
    case TS89_ERROR_INVALID_LIMIT: return "invalid limit";
    case TS89_ERROR_CLAMPED_LIMIT: return "clamped limit";
    case TS89_ERROR_OUT_OF_SPACE: return "out of space";
    case TS89_ERROR_INVALID_HANDLE: return "invalid handle";
    case TS89_ERROR_STALE_HANDLE: return "stale handle";
    case TS89_ERROR_BAD_STATE: return "bad state";
    case TS89_ERROR_LOCKED: return "locked";
    case TS89_ERROR_OUT_OF_RANGE: return "out of range";
    case TS89_ERROR_INTEGRITY: return "integrity error";
    default: return "unknown";
    }
}

static int ts89_validate_live(const TS89_System *system, TS89_Thing thing, int allow_reserved)
{
    int index;
    unsigned int generation;
    unsigned char state;
    if (!system || !system->initialized || !ts89_syntax_valid(thing)) return 0;
    index = ts89_index(thing);
    if (index < 0 || index >= system->limit) return 0;
    generation = ts89_generation(thing);
    if (generation != system->generation[index]) return 0;
    state = system->state[index];
    if (state == (unsigned char)TS89_STATE_ALIVE) return 1;
    return allow_reserved && state == (unsigned char)TS89_STATE_RESERVED;
}

int ts89_validate(const TS89_System *system, TS89_Thing thing)
{ return ts89_validate_live(system, thing, 1); }
int ts89_exists(const TS89_System *system, TS89_Thing thing)
{ return ts89_validate_live(system, thing, 1); }
int ts89_is_alive(const TS89_System *system, TS89_Thing thing)
{ return ts89_validate_live(system, thing, 0); }
int ts89_is_reserved(const TS89_System *system, TS89_Thing thing)
{
    int index;
    if (!ts89_validate_live(system, thing, 1)) return 0;
    index = ts89_index(thing);
    return system->state[index] == (unsigned char)TS89_STATE_RESERVED;
}
int ts89_is_stale(const TS89_System *system, TS89_Thing thing)
{
    int index;
    if (!system || !system->initialized || !ts89_syntax_valid(thing)) return 0;
    index = ts89_index(thing);
    if (index < 0 || index >= system->limit) return 0;
    return ts89_generation(thing) != system->generation[index];
}

static int ts89_release_one(TS89_System *system)
{
    int index;
    if (!system || system->retired_count <= 0) return 0;
    index = system->retired_queue[system->retired_head++];
    if (system->retired_head >= TS89_CAPACITY) system->retired_head = 0;
    --system->retired_count;
    if (index < 0 || index >= system->limit) return 0;
    system->state[index] = (unsigned char)TS89_STATE_FREE;
    system->free_stack[system->free_top++] = index;
    return 1;
}

static int ts89_prepare_free(TS89_System *system)
{
    while (system->free_top <= 0 && system->retired_count > 0)
        if (!ts89_release_one(system)) break;
    return system->free_top > 0;
}

static void ts89_update_peaks(TS89_System *system)
{
    int in_use;
    if (system->alive_count > system->peak_alive) system->peak_alive = system->alive_count;
    in_use = system->alive_count + system->reserved_count;
    if (in_use > system->peak_in_use) system->peak_in_use = in_use;
}

TS89_Thing ts89_reserve(TS89_System *system)
{
    int index;
    TS89_Thing thing;
    if (!system || !system->initialized) { ts89_set_error(system, TS89_ERROR_NOT_INITIALIZED); return TS89_THING_INVALID; }
    if (!ts89_prepare_free(system)) {
        ++system->failed_creates;
        ts89_set_error(system, TS89_ERROR_OUT_OF_SPACE);
        return TS89_THING_INVALID;
    }
    index = system->free_stack[--system->free_top];
    system->state[index] = (unsigned char)TS89_STATE_RESERVED;
    ts89_zero_slot(system, index);
    ++system->reserved_count;
    ts89_update_peaks(system);
    thing = ts89_pack(index, system->generation[index]);
    ts89_set_error(system, TS89_OK);
    return thing;
}

int ts89_publish(TS89_System *system, TS89_Thing thing)
{
    int index;
    if (!system || !system->initialized) { ts89_set_error(system, TS89_ERROR_NOT_INITIALIZED); return 0; }
    if (!ts89_validate_live(system, thing, 1)) { ts89_set_error(system, TS89_ERROR_INVALID_HANDLE); return 0; }
    index = ts89_index(thing);
    if (system->state[index] != (unsigned char)TS89_STATE_RESERVED) { ts89_set_error(system, TS89_ERROR_BAD_STATE); return 0; }
    system->state[index] = (unsigned char)TS89_STATE_ALIVE;
    --system->reserved_count;
    ++system->alive_count;
    ++system->total_created;
    ts89_update_peaks(system);
    ts89_set_error(system, TS89_OK);
    return 1;
}

TS89_Thing ts89_create(TS89_System *system)
{
    TS89_Thing thing;
    thing = ts89_reserve(system);
    if (thing == TS89_THING_INVALID) return thing;
    if (!ts89_publish(system, thing)) return TS89_THING_INVALID;
    return thing;
}

int ts89_destroy(TS89_System *system, TS89_Thing thing)
{
    int index;
    if (!system || !system->initialized) { ts89_set_error(system, TS89_ERROR_NOT_INITIALIZED); return 0; }
    if (!ts89_validate_live(system, thing, 1)) {
        ts89_set_error(system, ts89_is_stale(system, thing) ? TS89_ERROR_STALE_HANDLE : TS89_ERROR_INVALID_HANDLE);
        return 0;
    }
    index = ts89_index(thing);
    if (system->locked[index]) { ts89_set_error(system, TS89_ERROR_LOCKED); return 0; }
    if (system->state[index] == (unsigned char)TS89_STATE_ALIVE) --system->alive_count;
    else if (system->state[index] == (unsigned char)TS89_STATE_RESERVED) --system->reserved_count;
    system->state[index] = (unsigned char)TS89_STATE_RETIRED;
    system->generation[index] = ts89_next_generation(system->generation[index]);
    ts89_zero_slot(system, index);
    system->retired_queue[system->retired_tail++] = index;
    if (system->retired_tail >= TS89_CAPACITY) system->retired_tail = 0;
    ++system->retired_count;
    ++system->total_destroyed;
    while (system->retired_count > TS89_REUSE_QUARANTINE)
        if (!ts89_release_one(system)) break;
    ts89_set_error(system, TS89_OK);
    return 1;
}

int ts89_collect(TS89_System *system, int max_to_release)
{
    int count;
    if (!system || !system->initialized) { ts89_set_error(system, TS89_ERROR_NOT_INITIALIZED); return 0; }
    count = 0;
    while (system->retired_count > 0 && (max_to_release <= 0 || count < max_to_release)) {
        if (!ts89_release_one(system)) break;
        ++count;
    }
    return count;
}

int ts89_lock(TS89_System *system, TS89_Thing thing)
{
    int index;
    if (!ts89_validate_live(system, thing, 1)) return 0;
    index = ts89_index(thing);
    system->locked[index] = 1U;
    return 1;
}
int ts89_unlock(TS89_System *system, TS89_Thing thing)
{
    int index;
    if (!ts89_validate_live(system, thing, 1)) return 0;
    index = ts89_index(thing);
    system->locked[index] = 0U;
    return 1;
}
int ts89_is_locked(const TS89_System *system, TS89_Thing thing)
{
    int index;
    if (!ts89_validate_live(system, thing, 1)) return 0;
    index = ts89_index(thing);
    return system->locked[index] ? 1 : 0;
}

int ts89_state(const TS89_System *system, TS89_Thing thing)
{
    int index;
    if (!system || !system->initialized || !ts89_syntax_valid(thing)) return TS89_STATE_INVALID;
    index = ts89_index(thing);
    if (index < 0 || index >= system->limit) return TS89_STATE_INVALID;
    if (ts89_generation(thing) != system->generation[index]) return TS89_STATE_INVALID;
    return (int)system->state[index];
}

TS89_Thing ts89_from_index(const TS89_System *system, int index)
{
    if (!system || !system->initialized || index < 0 || index >= system->limit) return TS89_THING_INVALID;
    if (system->state[index] != (unsigned char)TS89_STATE_ALIVE && system->state[index] != (unsigned char)TS89_STATE_RESERVED)
        return TS89_THING_INVALID;
    return ts89_pack(index, system->generation[index]);
}

unsigned long ts89_get_flags(const TS89_System *system, TS89_Thing thing)
{
    int index;
    if (!ts89_validate_live(system, thing, 1)) return 0UL;
    index = ts89_index(thing);
    return system->flags[index];
}
int ts89_set_flags(TS89_System *system, TS89_Thing thing, unsigned long flags)
{
    int index;
    if (!ts89_validate_live(system, thing, 1)) return 0;
    index = ts89_index(thing); system->flags[index] = flags; return 1;
}
int ts89_add_flags(TS89_System *system, TS89_Thing thing, unsigned long mask)
{ return ts89_set_flags(system, thing, ts89_get_flags(system, thing) | mask); }
int ts89_clear_flags(TS89_System *system, TS89_Thing thing, unsigned long mask)
{ return ts89_set_flags(system, thing, ts89_get_flags(system, thing) & ~mask); }
int ts89_has_flags(const TS89_System *system, TS89_Thing thing, unsigned long mask)
{ return (ts89_get_flags(system, thing) & mask) == mask; }

int ts89_get_namespace(const TS89_System *system, TS89_Thing thing)
{
    int index;
    if (!ts89_validate_live(system, thing, 1)) return -1;
    index = ts89_index(thing); return (int)system->name_space[index];
}
int ts89_set_namespace(TS89_System *system, TS89_Thing thing, int name_space)
{
    int index;
    if (!ts89_validate_live(system, thing, 1) || name_space < 0 || name_space >= TS89_NAMESPACE_LIMIT) return 0;
    index = ts89_index(thing); system->name_space[index] = (unsigned int)name_space; return 1;
}
unsigned long ts89_get_user(const TS89_System *system, TS89_Thing thing)
{
    int index;
    if (!ts89_validate_live(system, thing, 1)) return 0UL;
    index = ts89_index(thing); return system->user[index];
}
int ts89_set_user(TS89_System *system, TS89_Thing thing, unsigned long value)
{
    int index;
    if (!ts89_validate_live(system, thing, 1)) return 0;
    index = ts89_index(thing); system->user[index] = value; return 1;
}

TS89_Thing ts89_first_alive(const TS89_System *system)
{
    int i;
    if (!system || !system->initialized) return TS89_THING_INVALID;
    for (i = 0; i < system->limit; ++i)
        if (system->state[i] == (unsigned char)TS89_STATE_ALIVE)
            return ts89_pack(i, system->generation[i]);
    return TS89_THING_INVALID;
}
TS89_Thing ts89_next_alive(const TS89_System *system, TS89_Thing after)
{
    int i;
    if (!system || !system->initialized) return TS89_THING_INVALID;
    i = ts89_index(after);
    if (i < 0) i = -1;
    for (++i; i < system->limit; ++i)
        if (system->state[i] == (unsigned char)TS89_STATE_ALIVE)
            return ts89_pack(i, system->generation[i]);
    return TS89_THING_INVALID;
}

int ts89_check_integrity(const TS89_System *system)
{
    int i;
    int alive;
    int reserved;
    int retired;
    int free_count;
    if (!system || !system->initialized) return 0;
    alive = reserved = retired = free_count = 0;
    for (i = 0; i < system->limit; ++i) {
        switch ((int)system->state[i]) {
        case TS89_STATE_ALIVE: ++alive; break;
        case TS89_STATE_RESERVED: ++reserved; break;
        case TS89_STATE_RETIRED: ++retired; break;
        case TS89_STATE_FREE: ++free_count; break;
        default: return 0;
        }
    }
    if (alive != system->alive_count || reserved != system->reserved_count || retired != system->retired_count || free_count != system->free_top) return 0;
    return alive + reserved + retired + free_count == system->limit;
}
