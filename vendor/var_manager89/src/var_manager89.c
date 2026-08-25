#include "var_manager89.h"

#include <string.h>

static int vm89_valid_name(const char *name)
{
    size_t n;
    if (name == NULL || name[0] == '\0') return 0;
    n = strlen(name);
    return n < VM89_NAME_MAX;
}

static void vm89_clear_slot(vm89_slot *s)
{
    if (s == NULL) return;
    memset(s, 0, sizeof(*s));
}

static vm89_slot *vm89_find_slot_mut(vm89_slot *slots, int count, const char *name)
{
    int i;
    for (i = 0; i < count; ++i) {
        if (slots[i].used && strcmp(slots[i].name, name) == 0) return &slots[i];
    }
    return NULL;
}

static const vm89_slot *vm89_find_slot(const vm89_slot *slots, int count, const char *name)
{
    int i;
    for (i = 0; i < count; ++i) {
        if (slots[i].used && strcmp(slots[i].name, name) == 0) return &slots[i];
    }
    return NULL;
}

static vm89_slot *vm89_alloc_slot(vm89_slot *slots, int count, const char *name)
{
    int i;
    size_t n;
    vm89_slot *s;

    s = vm89_find_slot_mut(slots, count, name);
    if (s != NULL) return s;

    for (i = 0; i < count; ++i) {
        if (!slots[i].used) {
            s = &slots[i];
            vm89_clear_slot(s);
            n = strlen(name);
            memcpy(s->name, name, n + 1);
            s->used = 1;
            return s;
        }
    }
    return NULL;
}

static vm89_instance *vm89_find_instance_mut(vm89_manager *m, vm89_u32 id)
{
    int i;
    for (i = 0; i < VM89_MAX_INSTANCES; ++i) {
        if (m->instances[i].used && m->instances[i].instance_id == id) {
            return &m->instances[i];
        }
    }
    return NULL;
}

static const vm89_instance *vm89_find_instance(const vm89_manager *m, vm89_u32 id)
{
    int i;
    for (i = 0; i < VM89_MAX_INSTANCES; ++i) {
        if (m->instances[i].used && m->instances[i].instance_id == id) {
            return &m->instances[i];
        }
    }
    return NULL;
}

static vm89_local_frame *vm89_top_frame_mut(vm89_manager *m)
{
    if (m == NULL || m->frame_count <= 0) return NULL;
    return &m->frames[m->frame_count - 1];
}

static const vm89_local_frame *vm89_top_frame(const vm89_manager *m)
{
    if (m == NULL || m->frame_count <= 0) return NULL;
    return &m->frames[m->frame_count - 1];
}

void vm89_init(vm89_manager *m)
{
    if (m == NULL) return;
    memset(m, 0, sizeof(*m));
}

void vm89_value_none(vm89_value *v)
{
    if (v == NULL) return;
    memset(v, 0, sizeof(*v));
}

void vm89_value_fixed_raw(vm89_value *v, vm89_i32 raw_q16)
{
    if (v == NULL) return;
    vm89_value_none(v);
    v->type = VM89_VALUE_FIXED;
    v->fixed_q16 = raw_q16;
}

void vm89_value_fixed_int(vm89_value *v, vm89_i32 whole)
{
    vm89_value_fixed_raw(v, whole * 65536L);
}

void vm89_value_bool(vm89_value *v, int boolean)
{
    if (v == NULL) return;
    vm89_value_none(v);
    v->type = VM89_VALUE_BOOL;
    v->boolean = boolean ? 1 : 0;
}

int vm89_value_string(vm89_value *v, const char *text)
{
    size_t n;
    if (v == NULL || text == NULL) return VM89_ERR_NULL;
    n = strlen(text);
    if (n >= VM89_STRING_MAX) return VM89_ERR_CAPACITY;
    vm89_value_none(v);
    v->type = VM89_VALUE_STRING;
    memcpy(v->string_value, text, n + 1);
    return VM89_OK;
}

int vm89_instance_create(vm89_manager *m, vm89_u32 instance_id)
{
    int i;
    if (m == NULL) return VM89_ERR_NULL;
    if (vm89_find_instance_mut(m, instance_id) != NULL) return VM89_OK;

    for (i = 0; i < VM89_MAX_INSTANCES; ++i) {
        if (!m->instances[i].used) {
            memset(&m->instances[i], 0, sizeof(m->instances[i]));
            m->instances[i].used = 1;
            m->instances[i].instance_id = instance_id;
            return VM89_OK;
        }
    }
    return VM89_ERR_CAPACITY;
}

int vm89_instance_destroy(vm89_manager *m, vm89_u32 instance_id)
{
    vm89_instance *inst;
    if (m == NULL) return VM89_ERR_NULL;
    inst = vm89_find_instance_mut(m, instance_id);
    if (inst == NULL) return VM89_ERR_NOT_FOUND;
    memset(inst, 0, sizeof(*inst));
    return VM89_OK;
}

int vm89_instance_exists(const vm89_manager *m, vm89_u32 instance_id)
{
    if (m == NULL) return 0;
    return vm89_find_instance(m, instance_id) != NULL;
}

static int vm89_set_in_slots(vm89_slot *slots, int count,
                             const char *name, const vm89_value *value)
{
    vm89_slot *s;
    if (!vm89_valid_name(name)) return VM89_ERR_BAD_NAME;
    if (value == NULL) return VM89_ERR_NULL;
    s = vm89_alloc_slot(slots, count, name);
    if (s == NULL) return VM89_ERR_CAPACITY;
    s->value = *value;
    return VM89_OK;
}

static int vm89_get_from_slots(const vm89_slot *slots, int count,
                               const char *name, vm89_value *out)
{
    const vm89_slot *s;
    if (!vm89_valid_name(name)) return VM89_ERR_BAD_NAME;
    if (out == NULL) return VM89_ERR_NULL;
    s = vm89_find_slot(slots, count, name);
    if (s == NULL) return VM89_ERR_NOT_FOUND;
    *out = s->value;
    return VM89_OK;
}

static int vm89_unset_in_slots(vm89_slot *slots, int count, const char *name)
{
    vm89_slot *s;
    if (!vm89_valid_name(name)) return VM89_ERR_BAD_NAME;
    s = vm89_find_slot_mut(slots, count, name);
    if (s == NULL) return VM89_ERR_NOT_FOUND;
    vm89_clear_slot(s);
    return VM89_OK;
}

int vm89_global_set(vm89_manager *m, const char *name, const vm89_value *value)
{
    if (m == NULL) return VM89_ERR_NULL;
    return vm89_set_in_slots(m->globals, VM89_MAX_GLOBALS, name, value);
}

int vm89_global_get(const vm89_manager *m, const char *name, vm89_value *out)
{
    if (m == NULL) return VM89_ERR_NULL;
    return vm89_get_from_slots(m->globals, VM89_MAX_GLOBALS, name, out);
}

int vm89_global_exists(const vm89_manager *m, const char *name)
{
    if (m == NULL || !vm89_valid_name(name)) return 0;
    return vm89_find_slot(m->globals, VM89_MAX_GLOBALS, name) != NULL;
}

int vm89_global_unset(vm89_manager *m, const char *name)
{
    if (m == NULL) return VM89_ERR_NULL;
    return vm89_unset_in_slots(m->globals, VM89_MAX_GLOBALS, name);
}

int vm89_instance_set(vm89_manager *m, vm89_u32 instance_id,
                      const char *name, const vm89_value *value)
{
    vm89_instance *inst;
    int r;
    if (m == NULL) return VM89_ERR_NULL;
    inst = vm89_find_instance_mut(m, instance_id);
    if (inst == NULL) {
        r = vm89_instance_create(m, instance_id);
        if (r != VM89_OK) return r;
        inst = vm89_find_instance_mut(m, instance_id);
    }
    return vm89_set_in_slots(inst->vars, VM89_MAX_INSTANCE_VARS, name, value);
}

int vm89_instance_get(const vm89_manager *m, vm89_u32 instance_id,
                      const char *name, vm89_value *out)
{
    const vm89_instance *inst;
    if (m == NULL) return VM89_ERR_NULL;
    inst = vm89_find_instance(m, instance_id);
    if (inst == NULL) return VM89_ERR_NOT_FOUND;
    return vm89_get_from_slots(inst->vars, VM89_MAX_INSTANCE_VARS, name, out);
}

int vm89_instance_var_exists(const vm89_manager *m, vm89_u32 instance_id,
                             const char *name)
{
    const vm89_instance *inst;
    if (m == NULL || !vm89_valid_name(name)) return 0;
    inst = vm89_find_instance(m, instance_id);
    if (inst == NULL) return 0;
    return vm89_find_slot(inst->vars, VM89_MAX_INSTANCE_VARS, name) != NULL;
}

int vm89_instance_unset(vm89_manager *m, vm89_u32 instance_id,
                        const char *name)
{
    vm89_instance *inst;
    if (m == NULL) return VM89_ERR_NULL;
    inst = vm89_find_instance_mut(m, instance_id);
    if (inst == NULL) return VM89_ERR_NOT_FOUND;
    return vm89_unset_in_slots(inst->vars, VM89_MAX_INSTANCE_VARS, name);
}

int vm89_begin_event(vm89_manager *m, vm89_u32 instance_id, vm89_u32 event_id)
{
    vm89_local_frame *f;
    int r;
    if (m == NULL) return VM89_ERR_NULL;
    if (m->frame_count >= VM89_MAX_LOCAL_FRAMES) return VM89_ERR_CAPACITY;

    if (!vm89_instance_exists(m, instance_id)) {
        r = vm89_instance_create(m, instance_id);
        if (r != VM89_OK) return r;
    }

    f = &m->frames[m->frame_count];
    memset(f, 0, sizeof(*f));
    f->used = 1;
    f->owner_instance_id = instance_id;
    f->event_id = event_id;
    ++m->frame_count;
    return VM89_OK;
}

int vm89_end_event(vm89_manager *m)
{
    if (m == NULL) return VM89_ERR_NULL;
    if (m->frame_count <= 0) return VM89_ERR_NO_FRAME;
    --m->frame_count;
    memset(&m->frames[m->frame_count], 0, sizeof(m->frames[m->frame_count]));
    return VM89_OK;
}

int vm89_local_set(vm89_manager *m, const char *name, const vm89_value *value)
{
    vm89_local_frame *f;
    if (m == NULL) return VM89_ERR_NULL;
    f = vm89_top_frame_mut(m);
    if (f == NULL) return VM89_ERR_NO_FRAME;
    return vm89_set_in_slots(f->vars, VM89_MAX_LOCALS_PER_FRAME, name, value);
}

int vm89_local_get(const vm89_manager *m, const char *name, vm89_value *out)
{
    const vm89_local_frame *f;
    if (m == NULL) return VM89_ERR_NULL;
    f = vm89_top_frame(m);
    if (f == NULL) return VM89_ERR_NO_FRAME;
    return vm89_get_from_slots(f->vars, VM89_MAX_LOCALS_PER_FRAME, name, out);
}

int vm89_local_exists(const vm89_manager *m, const char *name)
{
    const vm89_local_frame *f;
    if (m == NULL || !vm89_valid_name(name)) return 0;
    f = vm89_top_frame(m);
    if (f == NULL) return 0;
    return vm89_find_slot(f->vars, VM89_MAX_LOCALS_PER_FRAME, name) != NULL;
}

int vm89_resolve(const vm89_manager *m, vm89_u32 current_instance_id,
                 const char *name, vm89_value *out)
{
    int r;
    if (m == NULL || out == NULL) return VM89_ERR_NULL;

    if (m->frame_count > 0) {
        r = vm89_local_get(m, name, out);
        if (r == VM89_OK) return VM89_OK;
    }

    return vm89_instance_get(m, current_instance_id, name, out);
}

static int vm89_add_fixed_slot(vm89_slot *s, vm89_i32 delta)
{
    if (s == NULL) return VM89_ERR_NOT_FOUND;
    if (s->value.type != VM89_VALUE_FIXED) return VM89_ERR_BAD_TYPE;
    s->value.fixed_q16 += delta;
    return VM89_OK;
}

int vm89_global_add_fixed(vm89_manager *m, const char *name, vm89_i32 delta_q16)
{
    vm89_slot *s;
    if (m == NULL) return VM89_ERR_NULL;
    if (!vm89_valid_name(name)) return VM89_ERR_BAD_NAME;
    s = vm89_find_slot_mut(m->globals, VM89_MAX_GLOBALS, name);
    return vm89_add_fixed_slot(s, delta_q16);
}

int vm89_instance_add_fixed(vm89_manager *m, vm89_u32 instance_id,
                            const char *name, vm89_i32 delta_q16)
{
    vm89_instance *inst;
    vm89_slot *s;
    if (m == NULL) return VM89_ERR_NULL;
    if (!vm89_valid_name(name)) return VM89_ERR_BAD_NAME;
    inst = vm89_find_instance_mut(m, instance_id);
    if (inst == NULL) return VM89_ERR_NOT_FOUND;
    s = vm89_find_slot_mut(inst->vars, VM89_MAX_INSTANCE_VARS, name);
    return vm89_add_fixed_slot(s, delta_q16);
}

int vm89_local_add_fixed(vm89_manager *m, const char *name, vm89_i32 delta_q16)
{
    vm89_local_frame *f;
    vm89_slot *s;
    if (m == NULL) return VM89_ERR_NULL;
    if (!vm89_valid_name(name)) return VM89_ERR_BAD_NAME;
    f = vm89_top_frame_mut(m);
    if (f == NULL) return VM89_ERR_NO_FRAME;
    s = vm89_find_slot_mut(f->vars, VM89_MAX_LOCALS_PER_FRAME, name);
    return vm89_add_fixed_slot(s, delta_q16);
}


int vm89_set(vm89_manager *m, vm89_scope scope, vm89_u32 target_instance_id,
             const char *runtime_name, const vm89_value *value)
{
    if (m == NULL || runtime_name == NULL || value == NULL) return VM89_ERR_NULL;

    switch (scope) {
        case VM89_SCOPE_LOCAL:
            return vm89_local_set(m, runtime_name, value);
        case VM89_SCOPE_INSTANCE:
            return vm89_instance_set(m, target_instance_id, runtime_name, value);
        case VM89_SCOPE_GLOBAL:
            return vm89_global_set(m, runtime_name, value);
        default:
            return VM89_ERR_BAD_TYPE;
    }
}

int vm89_get(const vm89_manager *m, vm89_scope scope, vm89_u32 target_instance_id,
             const char *runtime_name, vm89_value *out)
{
    if (m == NULL || runtime_name == NULL || out == NULL) return VM89_ERR_NULL;

    switch (scope) {
        case VM89_SCOPE_LOCAL:
            return vm89_local_get(m, runtime_name, out);
        case VM89_SCOPE_INSTANCE:
            return vm89_instance_get(m, target_instance_id, runtime_name, out);
        case VM89_SCOPE_GLOBAL:
            return vm89_global_get(m, runtime_name, out);
        default:
            return VM89_ERR_BAD_TYPE;
    }
}

int vm89_exists(const vm89_manager *m, vm89_scope scope,
                vm89_u32 target_instance_id, const char *runtime_name)
{
    if (m == NULL || runtime_name == NULL) return 0;

    switch (scope) {
        case VM89_SCOPE_LOCAL:
            return vm89_local_exists(m, runtime_name);
        case VM89_SCOPE_INSTANCE:
            return vm89_instance_var_exists(m, target_instance_id, runtime_name);
        case VM89_SCOPE_GLOBAL:
            return vm89_global_exists(m, runtime_name);
        default:
            return 0;
    }
}

int vm89_unset(vm89_manager *m, vm89_scope scope, vm89_u32 target_instance_id,
               const char *runtime_name)
{
    if (m == NULL || runtime_name == NULL) return VM89_ERR_NULL;

    switch (scope) {
        case VM89_SCOPE_LOCAL:
        {
            vm89_local_frame *f;
            f = vm89_top_frame_mut(m);
            if (f == NULL) return VM89_ERR_NO_FRAME;
            return vm89_unset_in_slots(f->vars, VM89_MAX_LOCALS_PER_FRAME, runtime_name);
        }
        case VM89_SCOPE_INSTANCE:
            return vm89_instance_unset(m, target_instance_id, runtime_name);
        case VM89_SCOPE_GLOBAL:
            return vm89_global_unset(m, runtime_name);
        default:
            return VM89_ERR_BAD_TYPE;
    }
}

const char *vm89_result_string(int result)
{
    switch (result) {
        case VM89_OK: return "ok";
        case VM89_ERR_NULL: return "null argument";
        case VM89_ERR_NOT_FOUND: return "not found";
        case VM89_ERR_CAPACITY: return "capacity exhausted";
        case VM89_ERR_BAD_NAME: return "bad name";
        case VM89_ERR_BAD_TYPE: return "bad value type";
        case VM89_ERR_NO_FRAME: return "no active local frame";
        case VM89_ERR_FRAME_ORDER: return "bad frame order";
        case VM89_ERR_DIV_ZERO: return "division by zero";
        default: return "unknown error";
    }
}
