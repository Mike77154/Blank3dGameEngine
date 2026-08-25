#include "var_runtime89.h"

#include <string.h>

static vr89_scope vr89_scope_from_vdsl(vdsl89_scope scope)
{
    switch (scope) {
        case VDSL89_SCOPE_LOCAL: return VR89_SCOPE_LOCAL;
        case VDSL89_SCOPE_GLOBAL: return VR89_SCOPE_GLOBAL;
        case VDSL89_SCOPE_RESOLVE: return VR89_SCOPE_RESOLVE;
        case VDSL89_SCOPE_INSTANCE:
        default: return VR89_SCOPE_INSTANCE;
    }
}

static vm89_scope vr89_scope_to_vm(vr89_scope scope)
{
    switch (scope) {
        case VR89_SCOPE_LOCAL: return VM89_SCOPE_LOCAL;
        case VR89_SCOPE_GLOBAL: return VM89_SCOPE_GLOBAL;
        case VR89_SCOPE_INSTANCE:
        case VR89_SCOPE_RESOLVE:
        default: return VM89_SCOPE_INSTANCE;
    }
}

static void vr89_value_from_vdsl(const vdsl89_value *src, vm89_value *dst)
{
    vm89_value_none(dst);
    if (!src) return;
    if (src->type == VDSL89_VALUE_FIXED)
        vm89_value_fixed_raw(dst, src->fixed_q16);
    else if (src->type == VDSL89_VALUE_BOOL)
        vm89_value_bool(dst, src->boolean);
    else if (src->type == VDSL89_VALUE_STRING)
        (void)vm89_value_string(dst, src->string_value);
}

static const vr89_provider *vr89_claim_provider(const vr89_runtime *runtime,
                                                vr89_scope scope,
                                                vr89_owner owner,
                                                const char *name,
                                                vr89_operation operation,
                                                const vm89_value *value)
{
    int i;
    if (!runtime || !name) return (const vr89_provider *)0;
    for (i = 0; i < runtime->provider_count; ++i) {
        const vr89_provider *p;
        p = &runtime->providers[i];
        if (p->claim && p->claim(p->user, scope, owner, name,
                                 operation, value))
            return p;
    }
    return (const vr89_provider *)0;
}

static int vr89_store_result(int result)
{
    if (result == VM89_OK) return VR89_OK;
    if (result == VM89_ERR_NOT_FOUND) return VR89_ERR_NOT_FOUND;
    if (result == VM89_ERR_NO_FRAME) return VR89_ERR_NO_FRAME;
    if (result == VM89_ERR_BAD_TYPE) return VR89_ERR_BAD_TYPE;
    return VR89_ERR_STORE;
}

void vr89_init(vr89_runtime *runtime)
{
    if (!runtime) return;
    memset(runtime, 0, sizeof(*runtime));
    vm89_init(&runtime->store);
    runtime->last_result = VR89_OK;
}

void vr89_reset_instances(vr89_runtime *runtime)
{
    if (!runtime) return;
    memset(runtime->store.instances, 0, sizeof(runtime->store.instances));
    memset(runtime->store.frames, 0, sizeof(runtime->store.frames));
    runtime->store.frame_count = 0;
    runtime->current_owner = 0UL;
    runtime->current_event = 0UL;
    runtime->event_depth = 0;
    runtime->last_result = VR89_OK;
}

int vr89_add_provider(vr89_runtime *runtime, const vr89_provider *provider)
{
    int i;
    int insert_at;
    if (!runtime || !provider || !provider->claim) return VR89_ERR_NULL;
    if (runtime->provider_count >= VR89_MAX_PROVIDERS)
        return VR89_ERR_PROVIDER_FULL;

    insert_at = runtime->provider_count;
    for (i = 0; i < runtime->provider_count; ++i) {
        if (provider->priority > runtime->providers[i].priority) {
            insert_at = i;
            break;
        }
    }
    for (i = runtime->provider_count; i > insert_at; --i)
        runtime->providers[i] = runtime->providers[i - 1];
    runtime->providers[insert_at] = *provider;
    ++runtime->provider_count;
    return VR89_OK;
}

int vr89_provider_count(const vr89_runtime *runtime)
{
    return runtime ? runtime->provider_count : 0;
}

int vr89_instance_create(vr89_runtime *runtime, vr89_owner owner)
{
    if (!runtime) return VR89_ERR_NULL;
    return vr89_store_result(vm89_instance_create(&runtime->store, owner));
}

int vr89_instance_destroy(vr89_runtime *runtime, vr89_owner owner)
{
    int r;
    if (!runtime) return VR89_ERR_NULL;
    r = vm89_instance_destroy(&runtime->store, owner);
    if (r == VM89_ERR_NOT_FOUND) return VR89_OK;
    return vr89_store_result(r);
}

int vr89_begin_event(vr89_runtime *runtime, vr89_owner owner,
                     unsigned long event_id)
{
    int r;
    if (!runtime) return VR89_ERR_NULL;
    r = vm89_begin_event(&runtime->store, owner, event_id);
    if (r != VM89_OK) return vr89_store_result(r);
    runtime->current_owner = owner;
    runtime->current_event = event_id;
    ++runtime->event_depth;
    return VR89_OK;
}

int vr89_end_event(vr89_runtime *runtime)
{
    int r;
    if (!runtime) return VR89_ERR_NULL;
    r = vm89_end_event(&runtime->store);
    if (r != VM89_OK) return vr89_store_result(r);
    if (runtime->event_depth > 0) --runtime->event_depth;
    if (runtime->event_depth == 0) {
        runtime->current_owner = 0UL;
        runtime->current_event = 0UL;
    } else {
        const vm89_local_frame *f;
        f = &runtime->store.frames[runtime->store.frame_count - 1];
        runtime->current_owner = f->owner_instance_id;
        runtime->current_event = f->event_id;
    }
    return VR89_OK;
}

static int vr89_provider_get(vr89_runtime *runtime, vr89_scope scope,
                             vr89_owner owner, const char *name,
                             vm89_value *out_value)
{
    const vr89_provider *p;
    p = vr89_claim_provider(runtime, scope, owner, name,
                            VR89_OP_READ, (const vm89_value *)0);
    if (!p) return VR89_ERR_NOT_FOUND;
    if (!p->get || !p->get(p->user, scope, owner, name, out_value))
        return VR89_ERR_PROVIDER;
    return VR89_OK;
}

int vr89_get(vr89_runtime *runtime, vr89_scope scope, vr89_owner owner,
             const char *name, vm89_value *out_value)
{
    int r;
    if (!runtime || !name || !out_value) return VR89_ERR_NULL;
    if (scope == VR89_SCOPE_RESOLVE) return vr89_resolve(runtime, owner, name, out_value);
    if (scope == VR89_SCOPE_LOCAL)
        return vr89_store_result(vm89_get(&runtime->store, VM89_SCOPE_LOCAL,
                                          owner, name, out_value));

    r = vr89_provider_get(runtime, scope, owner, name, out_value);
    if (r == VR89_OK || r == VR89_ERR_PROVIDER) return r;
    return vr89_store_result(vm89_get(&runtime->store,
                                      vr89_scope_to_vm(scope), owner,
                                      name, out_value));
}

int vr89_resolve(vr89_runtime *runtime, vr89_owner owner,
                 const char *name, vm89_value *out_value)
{
    int r;
    if (!runtime || !name || !out_value) return VR89_ERR_NULL;
    if (runtime->store.frame_count > 0) {
        r = vm89_local_get(&runtime->store, name, out_value);
        if (r == VM89_OK) return VR89_OK;
    }
    r = vr89_provider_get(runtime, VR89_SCOPE_INSTANCE, owner, name, out_value);
    if (r == VR89_OK || r == VR89_ERR_PROVIDER) return r;
    return vr89_store_result(vm89_instance_get(&runtime->store, owner,
                                                name, out_value));
}

static int vr89_write(vr89_runtime *runtime, vr89_scope scope,
                      vr89_owner owner, const char *name,
                      const vm89_value *value, vr89_operation operation)
{
    const vr89_provider *p;
    vr89_write_fn fn;
    int r;
    if (!runtime || !name || !value) return VR89_ERR_NULL;
    if (scope == VR89_SCOPE_RESOLVE) scope = VR89_SCOPE_INSTANCE;

    if (scope == VR89_SCOPE_LOCAL) {
        if (operation == VR89_OP_SET)
            return vr89_store_result(vm89_set(&runtime->store,
                VM89_SCOPE_LOCAL, owner, name, value));
        if (value->type != VM89_VALUE_FIXED) return VR89_ERR_BAD_TYPE;
        r = vm89_local_add_fixed(&runtime->store, name,
            operation == VR89_OP_SUB ? -value->fixed_q16 : value->fixed_q16);
        return vr89_store_result(r);
    }

    p = vr89_claim_provider(runtime, scope, owner, name, operation, value);
    if (p) {
        fn = operation == VR89_OP_SET ? p->set :
             operation == VR89_OP_ADD ? p->add : p->sub;
        if (!fn || !fn(p->user, scope, owner, name, value))
            return VR89_ERR_PROVIDER;
        return VR89_OK;
    }

    if (operation == VR89_OP_SET)
        return vr89_store_result(vm89_set(&runtime->store,
            vr89_scope_to_vm(scope), owner, name, value));
    if (value->type != VM89_VALUE_FIXED) return VR89_ERR_BAD_TYPE;
    if (scope == VR89_SCOPE_GLOBAL)
        r = vm89_global_add_fixed(&runtime->store, name,
            operation == VR89_OP_SUB ? -value->fixed_q16 : value->fixed_q16);
    else
        r = vm89_instance_add_fixed(&runtime->store, owner, name,
            operation == VR89_OP_SUB ? -value->fixed_q16 : value->fixed_q16);
    return vr89_store_result(r);
}

int vr89_set(vr89_runtime *runtime, vr89_scope scope, vr89_owner owner,
             const char *name, const vm89_value *value)
{
    return vr89_write(runtime, scope, owner, name, value, VR89_OP_SET);
}

int vr89_add(vr89_runtime *runtime, vr89_scope scope, vr89_owner owner,
             const char *name, const vm89_value *value)
{
    return vr89_write(runtime, scope, owner, name, value, VR89_OP_ADD);
}

int vr89_sub(vr89_runtime *runtime, vr89_scope scope, vr89_owner owner,
             const char *name, const vm89_value *value)
{
    return vr89_write(runtime, scope, owner, name, value, VR89_OP_SUB);
}

int vr89_unset(vr89_runtime *runtime, vr89_scope scope, vr89_owner owner,
               const char *name)
{
    const vr89_provider *p;
    int r;
    if (!runtime || !name) return VR89_ERR_NULL;
    if (scope == VR89_SCOPE_RESOLVE) scope = VR89_SCOPE_INSTANCE;
    if (scope != VR89_SCOPE_LOCAL) {
        p = vr89_claim_provider(runtime, scope, owner, name,
                                VR89_OP_UNSET, (const vm89_value *)0);
        if (p) {
            if (!p->unset || !p->unset(p->user, scope, owner, name))
                return VR89_ERR_PROVIDER;
            return VR89_OK;
        }
    }
    r = vm89_unset(&runtime->store, vr89_scope_to_vm(scope), owner, name);
    return vr89_store_result(r);
}

static int vr89_emit_dsl(void *user, const vdsl89_command *command)
{
    vr89_runtime *runtime;
    vm89_value value;
    vr89_scope scope;
    int r;
    runtime = (vr89_runtime *)user;
    if (!runtime || !command) return 0;

    if (command->rhs_kind == VDSL89_RHS_VARIABLE) {
        r = vr89_get(runtime, vr89_scope_from_vdsl(command->rhs_scope),
                     runtime->current_owner, command->rhs_name, &value);
        if (r != VR89_OK) { runtime->last_result = r; return 0; }
    } else {
        vr89_value_from_vdsl(&command->value, &value);
    }

    scope = vr89_scope_from_vdsl(command->scope);
    if (command->opcode == VDSL89_OP_SET)
        r = vr89_set(runtime, scope, runtime->current_owner,
                     command->name, &value);
    else if (command->opcode == VDSL89_OP_ADD)
        r = vr89_add(runtime, scope, runtime->current_owner,
                     command->name, &value);
    else
        r = vr89_sub(runtime, scope, runtime->current_owner,
                     command->name, &value);
    runtime->last_result = r;
    return r == VR89_OK;
}

int vr89_execute_line(vr89_runtime *runtime, vr89_owner owner,
                      const char *line)
{
    vdsl89_provider provider;
    int r;
    vr89_owner previous_owner;
    if (!runtime || !line) return VR89_ERR_NULL;
    provider.emit = vr89_emit_dsl;
    provider.user = runtime;
    previous_owner = runtime->current_owner;
    runtime->current_owner = owner;
    runtime->last_result = VR89_OK;
    r = vdsl89_execute_line(line, &provider);
    runtime->current_owner = previous_owner;
    if (r != VDSL89_OK)
        return runtime->last_result != VR89_OK ? runtime->last_result : VR89_ERR_DSL;
    return runtime->last_result;
}

int vr89_execute_buffer(vr89_runtime *runtime, vr89_owner owner,
                        const char *text, int *out_statement_count)
{
    vdsl89_provider provider;
    int r;
    vr89_owner previous_owner;
    if (!runtime || !text) return VR89_ERR_NULL;
    provider.emit = vr89_emit_dsl;
    provider.user = runtime;
    previous_owner = runtime->current_owner;
    runtime->current_owner = owner;
    runtime->last_result = VR89_OK;
    r = vdsl89_execute_buffer(text, &provider, out_statement_count);
    runtime->current_owner = previous_owner;
    if (r != VDSL89_OK)
        return runtime->last_result != VR89_OK ? runtime->last_result : VR89_ERR_DSL;
    return runtime->last_result;
}

const char *vr89_result_string(int result)
{
    switch (result) {
        case VR89_OK: return "ok";
        case VR89_ERR_NULL: return "null argument";
        case VR89_ERR_PROVIDER_FULL: return "provider table full";
        case VR89_ERR_NOT_FOUND: return "variable not found";
        case VR89_ERR_PROVIDER: return "provider rejected operation";
        case VR89_ERR_BAD_TYPE: return "bad value type";
        case VR89_ERR_NO_FRAME: return "no local frame";
        case VR89_ERR_DSL: return "variable DSL error";
        case VR89_ERR_STORE: return "variable store error";
        default: return "unknown error";
    }
}
