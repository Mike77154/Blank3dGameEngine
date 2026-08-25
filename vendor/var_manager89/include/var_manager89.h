#ifndef VAR_MANAGER89_H
#define VAR_MANAGER89_H

/*
 * var_manager89 - fixed-capacity variable scope manager.
 * C89, no heap, no float/double. CC0-1.0.
 *
 * Models three useful GameMaker-like scopes:
 *   GLOBAL   - shared by the whole manager
 *   INSTANCE - persistent per instance id
 *   LOCAL    - belongs to active event/function frame
 *
 * It does not parse any language.
 *
 * ZERO HARDCODED VARIABLES:
 * - The manager starts with no variable names registered.
 * - Names are supplied at runtime.
 * - A SET operation creates a variable if the name does not exist.
 * - Capacity is fixed, but variable identity/content is fully dynamic.
 */

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef VM89_NAME_MAX
#define VM89_NAME_MAX 48
#endif

#ifndef VM89_STRING_MAX
#define VM89_STRING_MAX 96
#endif

#ifndef VM89_MAX_GLOBALS
#define VM89_MAX_GLOBALS 128
#endif

#ifndef VM89_MAX_INSTANCES
#define VM89_MAX_INSTANCES 64
#endif

#ifndef VM89_MAX_INSTANCE_VARS
#define VM89_MAX_INSTANCE_VARS 64
#endif

#ifndef VM89_MAX_LOCAL_FRAMES
#define VM89_MAX_LOCAL_FRAMES 16
#endif

#ifndef VM89_MAX_LOCALS_PER_FRAME
#define VM89_MAX_LOCALS_PER_FRAME 64
#endif

typedef long vm89_i32;
typedef unsigned long vm89_u32;

typedef enum vm89_value_type {
    VM89_VALUE_NONE = 0,
    VM89_VALUE_FIXED = 1,
    VM89_VALUE_BOOL = 2,
    VM89_VALUE_STRING = 3
} vm89_value_type;

typedef struct vm89_value {
    vm89_value_type type;
    vm89_i32 fixed_q16;
    int boolean;
    char string_value[VM89_STRING_MAX];
} vm89_value;

typedef struct vm89_slot {
    int used;
    char name[VM89_NAME_MAX];
    vm89_value value;
} vm89_slot;

typedef struct vm89_instance {
    int used;
    vm89_u32 instance_id;
    vm89_slot vars[VM89_MAX_INSTANCE_VARS];
} vm89_instance;

typedef struct vm89_local_frame {
    int used;
    vm89_u32 owner_instance_id;
    vm89_u32 event_id;
    vm89_slot vars[VM89_MAX_LOCALS_PER_FRAME];
} vm89_local_frame;

typedef struct vm89_manager {
    vm89_slot globals[VM89_MAX_GLOBALS];
    vm89_instance instances[VM89_MAX_INSTANCES];
    vm89_local_frame frames[VM89_MAX_LOCAL_FRAMES];
    int frame_count;
} vm89_manager;

typedef enum vm89_result {
    VM89_OK = 0,
    VM89_ERR_NULL = -1,
    VM89_ERR_NOT_FOUND = -2,
    VM89_ERR_CAPACITY = -3,
    VM89_ERR_BAD_NAME = -4,
    VM89_ERR_BAD_TYPE = -5,
    VM89_ERR_NO_FRAME = -6,
    VM89_ERR_FRAME_ORDER = -7,
    VM89_ERR_DIV_ZERO = -8
} vm89_result;

typedef enum vm89_scope {
    VM89_SCOPE_LOCAL = 0,
    VM89_SCOPE_INSTANCE = 1,
    VM89_SCOPE_GLOBAL = 2
} vm89_scope;

void vm89_init(vm89_manager *m);

/*
 * Generic dynamic interface.
 * No registry/schema is required beforehand.
 *
 * VM89_SCOPE_LOCAL:
 *   target_instance_id is ignored; requires an active event frame.
 *
 * VM89_SCOPE_INSTANCE:
 *   target_instance_id selects/creates the instance on SET.
 *
 * VM89_SCOPE_GLOBAL:
 *   target_instance_id is ignored.
 */
int vm89_set(vm89_manager *m, vm89_scope scope, vm89_u32 target_instance_id,
             const char *runtime_name, const vm89_value *value);
int vm89_get(const vm89_manager *m, vm89_scope scope, vm89_u32 target_instance_id,
             const char *runtime_name, vm89_value *out);
int vm89_exists(const vm89_manager *m, vm89_scope scope,
                vm89_u32 target_instance_id, const char *runtime_name);
int vm89_unset(vm89_manager *m, vm89_scope scope, vm89_u32 target_instance_id,
               const char *runtime_name);

void vm89_value_none(vm89_value *v);
void vm89_value_fixed_raw(vm89_value *v, vm89_i32 raw_q16);
void vm89_value_fixed_int(vm89_value *v, vm89_i32 whole);
void vm89_value_bool(vm89_value *v, int boolean);
int vm89_value_string(vm89_value *v, const char *text);

/* Instance lifetime. */
int vm89_instance_create(vm89_manager *m, vm89_u32 instance_id);
int vm89_instance_destroy(vm89_manager *m, vm89_u32 instance_id);
int vm89_instance_exists(const vm89_manager *m, vm89_u32 instance_id);

/* Global variables. set() creates on first assignment. */
int vm89_global_set(vm89_manager *m, const char *name, const vm89_value *value);
int vm89_global_get(const vm89_manager *m, const char *name, vm89_value *out);
int vm89_global_exists(const vm89_manager *m, const char *name);
int vm89_global_unset(vm89_manager *m, const char *name);

/* Per-instance persistent variables. */
int vm89_instance_set(vm89_manager *m, vm89_u32 instance_id,
                      const char *name, const vm89_value *value);
int vm89_instance_get(const vm89_manager *m, vm89_u32 instance_id,
                      const char *name, vm89_value *out);
int vm89_instance_var_exists(const vm89_manager *m, vm89_u32 instance_id,
                             const char *name);
int vm89_instance_unset(vm89_manager *m, vm89_u32 instance_id,
                        const char *name);

/*
 * Local frame lifetime.
 * begin_event pushes one local scope.
 * end_event pops it and clears every local automatically.
 * Nested frames are supported up to VM89_MAX_LOCAL_FRAMES.
 */
int vm89_begin_event(vm89_manager *m, vm89_u32 instance_id, vm89_u32 event_id);
int vm89_end_event(vm89_manager *m);
int vm89_local_set(vm89_manager *m, const char *name, const vm89_value *value);
int vm89_local_get(const vm89_manager *m, const char *name, vm89_value *out);
int vm89_local_exists(const vm89_manager *m, const char *name);

/*
 * GML-like unqualified lookup:
 * local first, then current instance, then optional global is NOT implicit.
 * GameMaker global variables normally require global. prefix, so globals are
 * intentionally accessed explicitly through vm89_global_*.
 */
int vm89_resolve(const vm89_manager *m, vm89_u32 current_instance_id,
                 const char *name, vm89_value *out);

/* Numeric operations for FIXED values only. Create-on-set, not create-on-add. */
int vm89_global_add_fixed(vm89_manager *m, const char *name, vm89_i32 delta_q16);
int vm89_instance_add_fixed(vm89_manager *m, vm89_u32 instance_id,
                            const char *name, vm89_i32 delta_q16);
int vm89_local_add_fixed(vm89_manager *m, const char *name, vm89_i32 delta_q16);

const char *vm89_result_string(int result);

#ifdef __cplusplus
}
#endif

#endif
