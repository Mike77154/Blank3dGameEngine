#ifndef VAR_RUNTIME89_H
#define VAR_RUNTIME89_H

/*
 * var_runtime89 - provider-routed GameMaker-like variable runtime.
 * C89, fixed-capacity, no heap, no float/double. CC0-1.0.
 *
 * Roles:
 *   var_dsl89     authoring/parser surface
 *   var_runtime89 scope + provider resolver
 *   var_manager89 fallback storage for dynamic/ad-hoc variables
 *
 * External systems (numeric systems, flags, save data, etc.) may claim names
 * through providers. Unknown SETs fall back to var_manager89 and therefore
 * create variables at runtime without registration.
 */

#include "var_manager89.h"
#include "var_dsl89.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef VR89_MAX_PROVIDERS
#define VR89_MAX_PROVIDERS 8
#endif

typedef unsigned long vr89_owner;

typedef enum vr89_operation {
    VR89_OP_READ = 1,
    VR89_OP_SET = 2,
    VR89_OP_ADD = 3,
    VR89_OP_SUB = 4,
    VR89_OP_UNSET = 5
} vr89_operation;

typedef enum vr89_scope {
    VR89_SCOPE_LOCAL = 0,
    VR89_SCOPE_INSTANCE = 1,
    VR89_SCOPE_GLOBAL = 2,
    VR89_SCOPE_RESOLVE = 3
} vr89_scope;

typedef int (*vr89_claim_fn)(void *user, vr89_scope scope,
                             vr89_owner owner, const char *name,
                             vr89_operation operation,
                             const vm89_value *value);
typedef int (*vr89_get_fn)(void *user, vr89_scope scope,
                           vr89_owner owner, const char *name,
                           vm89_value *out_value);
typedef int (*vr89_write_fn)(void *user, vr89_scope scope,
                             vr89_owner owner, const char *name,
                             const vm89_value *value);
typedef int (*vr89_unset_fn)(void *user, vr89_scope scope,
                             vr89_owner owner, const char *name);

typedef struct vr89_provider {
    const char *name;
    int priority;
    void *user;
    vr89_claim_fn claim;
    vr89_get_fn get;
    vr89_write_fn set;
    vr89_write_fn add;
    vr89_write_fn sub;
    vr89_unset_fn unset;
} vr89_provider;

typedef struct vr89_runtime {
    vm89_manager store;
    vr89_provider providers[VR89_MAX_PROVIDERS];
    int provider_count;
    vr89_owner current_owner;
    unsigned long current_event;
    int event_depth;
    int last_result;
} vr89_runtime;

typedef enum vr89_result {
    VR89_OK = 0,
    VR89_ERR_NULL = -1,
    VR89_ERR_PROVIDER_FULL = -2,
    VR89_ERR_NOT_FOUND = -3,
    VR89_ERR_PROVIDER = -4,
    VR89_ERR_BAD_TYPE = -5,
    VR89_ERR_NO_FRAME = -6,
    VR89_ERR_DSL = -7,
    VR89_ERR_STORE = -8
} vr89_result;

void vr89_init(vr89_runtime *runtime);
void vr89_reset_instances(vr89_runtime *runtime);
int vr89_add_provider(vr89_runtime *runtime, const vr89_provider *provider);
int vr89_provider_count(const vr89_runtime *runtime);

int vr89_instance_create(vr89_runtime *runtime, vr89_owner owner);
int vr89_instance_destroy(vr89_runtime *runtime, vr89_owner owner);
int vr89_begin_event(vr89_runtime *runtime, vr89_owner owner,
                     unsigned long event_id);
int vr89_end_event(vr89_runtime *runtime);

int vr89_get(vr89_runtime *runtime, vr89_scope scope, vr89_owner owner,
             const char *name, vm89_value *out_value);
int vr89_set(vr89_runtime *runtime, vr89_scope scope, vr89_owner owner,
             const char *name, const vm89_value *value);
int vr89_add(vr89_runtime *runtime, vr89_scope scope, vr89_owner owner,
             const char *name, const vm89_value *value);
int vr89_sub(vr89_runtime *runtime, vr89_scope scope, vr89_owner owner,
             const char *name, const vm89_value *value);
int vr89_unset(vr89_runtime *runtime, vr89_scope scope, vr89_owner owner,
               const char *name);
int vr89_resolve(vr89_runtime *runtime, vr89_owner owner,
                 const char *name, vm89_value *out_value);

/* Execute one/buffer of var_dsl89 source using owner as self/instance. */
int vr89_execute_line(vr89_runtime *runtime, vr89_owner owner,
                      const char *line);
int vr89_execute_buffer(vr89_runtime *runtime, vr89_owner owner,
                        const char *text, int *out_statement_count);

const char *vr89_result_string(int result);

#ifdef __cplusplus
}
#endif

#endif
