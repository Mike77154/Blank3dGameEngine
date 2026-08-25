#ifndef THING_SYSTEM89_H
#define THING_SYSTEM89_H

#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef TS89_CAPACITY
#define TS89_CAPACITY 256
#endif
#ifndef TS89_REUSE_QUARANTINE
#define TS89_REUSE_QUARANTINE 8
#endif
#ifndef TS89_NAMESPACE_LIMIT
#define TS89_NAMESPACE_LIMIT 256
#endif

#if TS89_CAPACITY <= 0
#error "TS89_CAPACITY must be positive"
#endif
#if TS89_REUSE_QUARANTINE < 0 || TS89_REUSE_QUARANTINE > TS89_CAPACITY
#error "TS89_REUSE_QUARANTINE out of range"
#endif

#define TS89_GENERATION_LIMIT ((unsigned int)(((unsigned long)INT_MAX / (unsigned long)TS89_CAPACITY) + 1UL))

typedef int TS89_Thing;
#define TS89_THING_INVALID ((TS89_Thing)-1)

enum {
    TS89_OK = 0,
    TS89_ERROR_NOT_INITIALIZED = 1,
    TS89_ERROR_INVALID_LIMIT = 2,
    TS89_ERROR_CLAMPED_LIMIT = 3,
    TS89_ERROR_OUT_OF_SPACE = 4,
    TS89_ERROR_INVALID_HANDLE = 5,
    TS89_ERROR_STALE_HANDLE = 6,
    TS89_ERROR_BAD_STATE = 7,
    TS89_ERROR_LOCKED = 8,
    TS89_ERROR_OUT_OF_RANGE = 9,
    TS89_ERROR_INTEGRITY = 10
};

enum {
    TS89_STATE_INVALID = -1,
    TS89_STATE_DISABLED = 0,
    TS89_STATE_FREE = 1,
    TS89_STATE_RESERVED = 2,
    TS89_STATE_ALIVE = 3,
    TS89_STATE_RETIRED = 4
};

typedef struct TS89_SystemTag {
    int initialized;
    int limit;
    int last_error;
    unsigned int generation[TS89_CAPACITY];
    unsigned char state[TS89_CAPACITY];
    unsigned char locked[TS89_CAPACITY];
    unsigned long flags[TS89_CAPACITY];
    unsigned int name_space[TS89_CAPACITY];
    unsigned long user[TS89_CAPACITY];
    int free_stack[TS89_CAPACITY];
    int free_top;
    int retired_queue[TS89_CAPACITY];
    int retired_head;
    int retired_tail;
    int retired_count;
    int alive_count;
    int reserved_count;
    int peak_alive;
    int peak_in_use;
    unsigned long total_created;
    unsigned long total_destroyed;
    unsigned long failed_creates;
} TS89_System;

void ts89_init(TS89_System *system, int max_things);
void ts89_reset(TS89_System *system);
void ts89_shutdown(TS89_System *system);
int ts89_is_initialized(const TS89_System *system);
int ts89_limit(const TS89_System *system);
int ts89_alive_count(const TS89_System *system);
int ts89_reserved_count(const TS89_System *system);
int ts89_in_use_count(const TS89_System *system);
int ts89_free_count(const TS89_System *system);
int ts89_retired_count(const TS89_System *system);
int ts89_last_error(const TS89_System *system);
const char *ts89_error_string(int error_code);

TS89_Thing ts89_reserve(TS89_System *system);
TS89_Thing ts89_create(TS89_System *system);
int ts89_publish(TS89_System *system, TS89_Thing thing);
int ts89_destroy(TS89_System *system, TS89_Thing thing);
int ts89_validate(const TS89_System *system, TS89_Thing thing);
int ts89_exists(const TS89_System *system, TS89_Thing thing);
int ts89_is_alive(const TS89_System *system, TS89_Thing thing);
int ts89_is_reserved(const TS89_System *system, TS89_Thing thing);
int ts89_is_stale(const TS89_System *system, TS89_Thing thing);
int ts89_lock(TS89_System *system, TS89_Thing thing);
int ts89_unlock(TS89_System *system, TS89_Thing thing);
int ts89_is_locked(const TS89_System *system, TS89_Thing thing);
int ts89_collect(TS89_System *system, int max_to_release);
int ts89_check_integrity(const TS89_System *system);

int ts89_index(TS89_Thing thing);
unsigned int ts89_generation(TS89_Thing thing);
int ts89_state(const TS89_System *system, TS89_Thing thing);
TS89_Thing ts89_from_index(const TS89_System *system, int index);

unsigned long ts89_get_flags(const TS89_System *system, TS89_Thing thing);
int ts89_set_flags(TS89_System *system, TS89_Thing thing, unsigned long flags);
int ts89_add_flags(TS89_System *system, TS89_Thing thing, unsigned long mask);
int ts89_clear_flags(TS89_System *system, TS89_Thing thing, unsigned long mask);
int ts89_has_flags(const TS89_System *system, TS89_Thing thing, unsigned long mask);
int ts89_get_namespace(const TS89_System *system, TS89_Thing thing);
int ts89_set_namespace(TS89_System *system, TS89_Thing thing, int name_space);
unsigned long ts89_get_user(const TS89_System *system, TS89_Thing thing);
int ts89_set_user(TS89_System *system, TS89_Thing thing, unsigned long value);

TS89_Thing ts89_first_alive(const TS89_System *system);
TS89_Thing ts89_next_alive(const TS89_System *system, TS89_Thing after);

#ifdef __cplusplus
}
#endif

#endif
