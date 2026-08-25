#include <string.h>
#include "cm89.h"

static void cm89_copy_name(char *dst, const char *src)
{
    unsigned int i;
    if (dst == 0) {
        return;
    }
    if (src == 0) {
        dst[0] = '\0';
        return;
    }
    i = 0U;
    while (src[i] != '\0' && i + 1U < (unsigned int)CM89_NAME_MAX) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

static cm89_bool cm89_name_equal(const char *a, const char *b)
{
    if (a == 0 || b == 0) {
        return CM89_FALSE;
    }
    return (strcmp(a, b) == 0) ? CM89_TRUE : CM89_FALSE;
}

#define CM89_CLASS_SLOT_MASK 0x00FFU
#define CM89_CLASS_GENERATION_SHIFT 8U

static cm89_class_h cm89_class_make_handle(unsigned int index, unsigned int generation)
{
    unsigned int slot;
    slot = index + 1U;
    if (slot == 0U || slot > 255U) return CM89_CLASS_NONE;
    generation &= 0xFFU;
    if (generation == 0U) generation = 1U;
    return (cm89_class_h)((generation << CM89_CLASS_GENERATION_SHIFT) | slot);
}

unsigned int cm89_class_handle_index(cm89_class_h class_handle)
{
    unsigned int slot;
    if (class_handle == CM89_CLASS_NONE) return (unsigned int)CM89_MAX_CLASSES;
    slot = ((unsigned int)class_handle) & CM89_CLASS_SLOT_MASK;
    if (slot == 0U) return (unsigned int)CM89_MAX_CLASSES;
    return slot - 1U;
}

unsigned int cm89_class_handle_generation(cm89_class_h class_handle)
{
    return (((unsigned int)class_handle) >> CM89_CLASS_GENERATION_SHIFT) & 0xFFU;
}

static unsigned char cm89_next_generation(unsigned char generation)
{
    unsigned int next;
    next = ((unsigned int)generation + 1U) & 0xFFU;
    if (next == 0U) next = 1U;
    return (unsigned char)next;
}

static cm89_class_h cm89_class_slot_handle(
    const cm89_manager *manager, unsigned int index
)
{
    unsigned int generation;
    if (manager == 0 || index >= (unsigned int)CM89_MAX_CLASSES)
        return CM89_CLASS_NONE;
    generation = (unsigned int)manager->classes[index].generation;
    if (generation == 0U) generation = 1U;
    return cm89_class_make_handle(index, generation);
}

static cm89_bool cm89_class_valid(const cm89_manager *manager, cm89_class_h h)
{
    unsigned int index;
    unsigned int generation;
    if (manager == 0 || h == CM89_CLASS_NONE) return CM89_FALSE;
    index = cm89_class_handle_index(h);
    generation = cm89_class_handle_generation(h);
    if (index >= (unsigned int)CM89_MAX_CLASSES || generation == 0U)
        return CM89_FALSE;
    if (!manager->classes[index].used) return CM89_FALSE;
    return ((unsigned int)manager->classes[index].generation == generation)
        ? CM89_TRUE : CM89_FALSE;
}

static cm89_class_record *cm89_class_mut(cm89_manager *manager, cm89_class_h h)
{
    if (!cm89_class_valid(manager, h)) {
        return 0;
    }
    return &manager->classes[cm89_class_handle_index(h)];
}

static const cm89_class_record *cm89_class_ref(
    const cm89_manager *manager,
    cm89_class_h h
)
{
    if (!cm89_class_valid(manager, h)) {
        return 0;
    }
    return &manager->classes[cm89_class_handle_index(h)];
}

#if CM89_ENABLE_INTERNAL_INSTANCES
static cm89_bool cm89_instance_valid(const cm89_manager *manager, cm89_instance_h h)
{
    unsigned int index;
    if (manager == 0 || h == CM89_INSTANCE_NONE) {
        return CM89_FALSE;
    }
    index = (unsigned int)h - 1U;
    if (index >= (unsigned int)CM89_MAX_INSTANCES) {
        return CM89_FALSE;
    }
    return manager->instances[index].used;
}

static cm89_instance_record *cm89_instance_mut(
    cm89_manager *manager,
    cm89_instance_h h
)
{
    if (!cm89_instance_valid(manager, h)) {
        return 0;
    }
    return &manager->instances[(unsigned int)h - 1U];
}

static const cm89_instance_record *cm89_instance_ref(
    const cm89_manager *manager,
    cm89_instance_h h
)
{
    if (!cm89_instance_valid(manager, h)) {
        return 0;
    }
    return &manager->instances[(unsigned int)h - 1U];
}

#endif

cm89_value cm89_value_none(void)
{
    cm89_value v;
    v.kind = CM89_VALUE_NONE;
    v.data.uint_value = 0UL;
    return v;
}

cm89_value cm89_value_sint(long value)
{
    cm89_value v;
    v.kind = CM89_VALUE_SINT;
    v.data.sint_value = value;
    return v;
}

cm89_value cm89_value_uint(unsigned long value)
{
    cm89_value v;
    v.kind = CM89_VALUE_UINT;
    v.data.uint_value = value;
    return v;
}

cm89_value cm89_value_ptr(void *value)
{
    cm89_value v;
    v.kind = CM89_VALUE_PTR;
    v.data.ptr_value = value;
    return v;
}

cm89_value cm89_value_host_handle(unsigned long value)
{
    cm89_value v;
    v.kind = CM89_VALUE_HOST_HANDLE;
    v.data.uint_value = value;
    return v;
}

cm89_value cm89_value_class(cm89_class_h value)
{
    cm89_value v;
    v.kind = CM89_VALUE_CLASS;
    v.data.uint_value = (unsigned long)value;
    return v;
}

cm89_value cm89_value_instance(cm89_instance_h value)
{
    cm89_value v;
    v.kind = CM89_VALUE_INSTANCE;
    v.data.uint_value = (unsigned long)value;
    return v;
}

void cm89_manager_init(cm89_manager *manager, const cm89_provider *provider)
{
    if (manager == 0) {
        return;
    }
    memset(manager, 0, sizeof(*manager));
    if (provider != 0) {
        manager->provider = *provider;
    }
    manager->revision = 1UL;
}

void cm89_manager_reset(cm89_manager *manager)
{
    cm89_provider provider;
    if (manager == 0) {
        return;
    }
    provider = manager->provider;
    memset(manager, 0, sizeof(*manager));
    manager->provider = provider;
    manager->revision = 1UL;
}

cm89_result cm89_manager_seal(cm89_manager *manager)
{
    if (manager == 0) return CM89_ERR_ARGUMENT;
    manager->sealed = CM89_TRUE;
    return CM89_OK;
}

cm89_result cm89_manager_unseal(cm89_manager *manager)
{
    if (manager == 0) return CM89_ERR_ARGUMENT;
    manager->sealed = CM89_FALSE;
    return CM89_OK;
}

cm89_bool cm89_manager_is_sealed(const cm89_manager *manager)
{
    if (manager == 0) return CM89_FALSE;
    return manager->sealed;
}

cm89_result cm89_class_find(
    const cm89_manager *manager,
    const char *name,
    cm89_class_h *out_class
)
{
    unsigned int i;
    if (manager == 0 || name == 0 || out_class == 0) {
        return CM89_ERR_ARGUMENT;
    }
    for (i = 0U; i < (unsigned int)CM89_MAX_CLASSES; ++i) {
        if (manager->classes[i].used &&
            cm89_name_equal(manager->classes[i].name, name)) {
            *out_class = cm89_class_slot_handle(manager, i);
            return CM89_OK;
        }
    }
    return CM89_ERR_NOT_FOUND;
}

static cm89_bool cm89_mro_contains(
    const cm89_class_record *record,
    cm89_class_h h
)
{
    cm89_count i;
    if (record == 0) {
        return CM89_FALSE;
    }
    for (i = 0; i < record->mro_count; ++i) {
        if (record->mro[i] == h) {
            return CM89_TRUE;
        }
    }
    return CM89_FALSE;
}

static cm89_result cm89_build_mro(
    const cm89_manager *manager,
    cm89_class_h self,
    const cm89_class_h *bases,
    cm89_count base_count,
    cm89_class_h *out_mro,
    cm89_count *out_count
)
{
    cm89_class_h seq[CM89_MAX_BASES + 1][CM89_MAX_MRO];
    cm89_count len[CM89_MAX_BASES + 1];
    cm89_count pos[CM89_MAX_BASES + 1];
    cm89_count seq_count;
    cm89_count i;
    cm89_count j;
    cm89_count result_count;
    cm89_bool any_left;
    cm89_bool selected;
    cm89_bool in_tail;
    cm89_class_h candidate;
    const cm89_class_record *base_record;

    if (out_mro == 0 || out_count == 0) {
        return CM89_ERR_ARGUMENT;
    }
    if (base_count > (cm89_count)CM89_MAX_BASES) {
        return CM89_ERR_CAPACITY;
    }
    if ((cm89_count)1 + base_count > (cm89_count)CM89_MAX_MRO) {
        return CM89_ERR_CAPACITY;
    }

    for (i = 0; i < base_count; ++i) {
        if (!cm89_class_valid(manager, bases[i])) {
            return CM89_ERR_INVALID_HANDLE;
        }
        base_record = cm89_class_ref(manager, bases[i]);
        if (base_record == 0 || !base_record->finalized) {
            return CM89_ERR_CLASS_NOT_FINAL;
        }
        if (bases[i] == self || cm89_mro_contains(base_record, self)) {
            return CM89_ERR_CYCLE;
        }
        for (j = 0; j < i; ++j) {
            if (bases[j] == bases[i]) {
                return CM89_ERR_DUPLICATE_BASE;
            }
        }
    }

    result_count = 0;
    out_mro[result_count++] = self;

    if (base_count == 0) {
        *out_count = result_count;
        return CM89_OK;
    }

    seq_count = (cm89_count)(base_count + 1);
    for (i = 0; i < seq_count; ++i) {
        len[i] = 0;
        pos[i] = 0;
    }

    for (i = 0; i < base_count; ++i) {
        base_record = cm89_class_ref(manager, bases[i]);
        if (base_record == 0) {
            return CM89_ERR_INVALID_HANDLE;
        }
        if (base_record->mro_count > (cm89_count)CM89_MAX_MRO) {
            return CM89_ERR_CAPACITY;
        }
        len[i] = base_record->mro_count;
        for (j = 0; j < base_record->mro_count; ++j) {
            seq[i][j] = base_record->mro[j];
        }
    }

    len[base_count] = base_count;
    for (i = 0; i < base_count; ++i) {
        seq[base_count][i] = bases[i];
    }

    for (;;) {
        any_left = CM89_FALSE;
        for (i = 0; i < seq_count; ++i) {
            if (pos[i] < len[i]) {
                any_left = CM89_TRUE;
                break;
            }
        }
        if (!any_left) {
            break;
        }

        selected = CM89_FALSE;
        candidate = CM89_CLASS_NONE;

        for (i = 0; i < seq_count; ++i) {
            if (pos[i] >= len[i]) {
                continue;
            }

            candidate = seq[i][pos[i]];
            in_tail = CM89_FALSE;

            for (j = 0; j < seq_count; ++j) {
                cm89_count k;
                if (pos[j] >= len[j]) {
                    continue;
                }
                for (k = (cm89_count)(pos[j] + 1); k < len[j]; ++k) {
                    if (seq[j][k] == candidate) {
                        in_tail = CM89_TRUE;
                        break;
                    }
                }
                if (in_tail) {
                    break;
                }
            }

            if (!in_tail) {
                selected = CM89_TRUE;
                break;
            }
        }

        if (!selected) {
            return CM89_ERR_MRO_CONFLICT;
        }

        if (result_count >= (cm89_count)CM89_MAX_MRO) {
            return CM89_ERR_CAPACITY;
        }
        out_mro[result_count++] = candidate;

        for (i = 0; i < seq_count; ++i) {
            if (pos[i] < len[i] && seq[i][pos[i]] == candidate) {
                pos[i]++;
            }
        }
    }

    *out_count = result_count;
    return CM89_OK;
}

cm89_result cm89_class_create(
    cm89_manager *manager,
    const char *name,
    const cm89_class_h *bases,
    cm89_count base_count,
    cm89_class_h *out_class
)
{
    unsigned int i;
    cm89_class_h existing;
    cm89_class_h handle;
    cm89_class_h mro[CM89_MAX_MRO];
    cm89_count mro_count;
    cm89_result result;
    cm89_class_record *record;

    if (manager == 0 || name == 0 || out_class == 0) {
        return CM89_ERR_ARGUMENT;
    }
    if (manager->sealed) return CM89_ERR_SEALED;
    if (name[0] == '\0') {
        return CM89_ERR_ARGUMENT;
    }
    if (base_count > 0 && bases == 0) {
        return CM89_ERR_ARGUMENT;
    }
    if (cm89_class_find(manager, name, &existing) == CM89_OK) {
        return CM89_ERR_DUPLICATE;
    }

    handle = CM89_CLASS_NONE;
    for (i = 0U; i < (unsigned int)CM89_MAX_CLASSES; ++i) {
        if (!manager->classes[i].used) {
            handle = cm89_class_slot_handle(manager, i);
            break;
        }
    }
    if (handle == CM89_CLASS_NONE) {
        return CM89_ERR_CAPACITY;
    }

    result = cm89_build_mro(
        manager, handle, bases, base_count, mro, &mro_count
    );
    if (result != CM89_OK) {
        return result;
    }

    record = &manager->classes[cm89_class_handle_index(handle)];
    {
        unsigned char generation;
        generation = record->generation;
        if (generation == 0U) generation = 1U;
        memset(record, 0, sizeof(*record));
        record->generation = generation;
    }
    record->used = CM89_TRUE;
    record->finalized = CM89_TRUE;
    cm89_copy_name(record->name, name);
    record->base_count = base_count;
    for (i = 0U; i < (unsigned int)base_count; ++i) {
        record->bases[i] = bases[i];
    }
    record->mro_count = mro_count;
    for (i = 0U; i < (unsigned int)mro_count; ++i) {
        record->mro[i] = mro[i];
    }
    record->revision = ++manager->revision;
    *out_class = handle;
    return CM89_OK;
}

cm89_result cm89_class_destroy(cm89_manager *manager, cm89_class_h class_handle)
{
    unsigned int i;
    cm89_count j;
    cm89_class_record *record;

    if (manager == 0) return CM89_ERR_ARGUMENT;
    if (manager->sealed) return CM89_ERR_SEALED;
    if (!cm89_class_valid(manager, class_handle)) {
        return CM89_ERR_INVALID_HANDLE;
    }

    for (i = 0U; i < (unsigned int)CM89_MAX_CLASSES; ++i) {
        if (!manager->classes[i].used) {
            continue;
        }
        for (j = 0; j < manager->classes[i].base_count; ++j) {
            if (manager->classes[i].bases[j] == class_handle) {
                return CM89_ERR_PROVIDER;
            }
        }
    }

#if CM89_ENABLE_INTERNAL_INSTANCES
    for (i = 0U; i < (unsigned int)CM89_MAX_INSTANCES; ++i) {
        if (manager->instances[i].used &&
            manager->instances[i].class_handle == class_handle) {
            return CM89_ERR_PROVIDER;
        }
    }

#endif

    record = cm89_class_mut(manager, class_handle);
    if (record == 0) {
        return CM89_ERR_INVALID_HANDLE;
    }
    {
        unsigned char next_generation;
        next_generation = cm89_next_generation(record->generation);
        memset(record, 0, sizeof(*record));
        record->generation = next_generation;
    }
    manager->revision++;
    return CM89_OK;
}

cm89_result cm89_class_get_name(
    const cm89_manager *manager,
    cm89_class_h class_handle,
    const char **out_name
)
{
    const cm89_class_record *record;
    if (out_name == 0) {
        return CM89_ERR_ARGUMENT;
    }
    record = cm89_class_ref(manager, class_handle);
    if (record == 0) {
        return CM89_ERR_INVALID_HANDLE;
    }
    *out_name = record->name;
    return CM89_OK;
}

cm89_result cm89_class_get_bases(
    const cm89_manager *manager,
    cm89_class_h class_handle,
    const cm89_class_h **out_bases,
    cm89_count *out_count
)
{
    const cm89_class_record *record;
    if (out_bases == 0 || out_count == 0) {
        return CM89_ERR_ARGUMENT;
    }
    record = cm89_class_ref(manager, class_handle);
    if (record == 0) {
        return CM89_ERR_INVALID_HANDLE;
    }
    *out_bases = record->bases;
    *out_count = record->base_count;
    return CM89_OK;
}

cm89_result cm89_class_get_mro(
    const cm89_manager *manager,
    cm89_class_h class_handle,
    const cm89_class_h **out_mro,
    cm89_count *out_count
)
{
    const cm89_class_record *record;
    if (out_mro == 0 || out_count == 0) {
        return CM89_ERR_ARGUMENT;
    }
    record = cm89_class_ref(manager, class_handle);
    if (record == 0) {
        return CM89_ERR_INVALID_HANDLE;
    }
    *out_mro = record->mro;
    *out_count = record->mro_count;
    return CM89_OK;
}

static cm89_member *cm89_find_own_member_mut(
    cm89_class_record *record,
    const char *name
)
{
    unsigned int i;
    if (record == 0 || name == 0) {
        return 0;
    }
    for (i = 0U; i < (unsigned int)CM89_MAX_CLASS_MEMBERS; ++i) {
        if (record->members[i].used &&
            cm89_name_equal(record->members[i].name, name)) {
            return &record->members[i];
        }
    }
    return 0;
}

static const cm89_member *cm89_find_own_member_ref(
    const cm89_class_record *record,
    const char *name
)
{
    unsigned int i;
    if (record == 0 || name == 0) {
        return 0;
    }
    for (i = 0U; i < (unsigned int)CM89_MAX_CLASS_MEMBERS; ++i) {
        if (record->members[i].used &&
            cm89_name_equal(record->members[i].name, name)) {
            return &record->members[i];
        }
    }
    return 0;
}

cm89_result cm89_class_set_member(
    cm89_manager *manager,
    cm89_class_h class_handle,
    const char *name,
    cm89_member_kind kind,
    cm89_value value,
    cm89_value aux
)
{
    cm89_class_record *record;
    cm89_member *member;
    unsigned int i;

    if (manager == 0 || name == 0 || name[0] == '\0') {
        return CM89_ERR_ARGUMENT;
    }
    if (manager->sealed) return CM89_ERR_SEALED;
    record = cm89_class_mut(manager, class_handle);
    if (record == 0) {
        return CM89_ERR_INVALID_HANDLE;
    }

    member = cm89_find_own_member_mut(record, name);
    if (member == 0) {
        for (i = 0U; i < (unsigned int)CM89_MAX_CLASS_MEMBERS; ++i) {
            if (!record->members[i].used) {
                member = &record->members[i];
                memset(member, 0, sizeof(*member));
                member->used = CM89_TRUE;
                cm89_copy_name(member->name, name);
                break;
            }
        }
    }
    if (member == 0) {
        return CM89_ERR_CAPACITY;
    }

    member->kind = kind;
    member->value = value;
    member->aux = aux;
    record->revision = ++manager->revision;
    return CM89_OK;
}

cm89_result cm89_class_remove_member(
    cm89_manager *manager,
    cm89_class_h class_handle,
    const char *name
)
{
    cm89_class_record *record;
    cm89_member *member;
    if (manager == 0 || name == 0) {
        return CM89_ERR_ARGUMENT;
    }
    if (manager->sealed) return CM89_ERR_SEALED;
    record = cm89_class_mut(manager, class_handle);
    if (record == 0) {
        return CM89_ERR_INVALID_HANDLE;
    }
    member = cm89_find_own_member_mut(record, name);
    if (member == 0) {
        return CM89_ERR_NOT_FOUND;
    }
    memset(member, 0, sizeof(*member));
    record->revision = ++manager->revision;
    return CM89_OK;
}

cm89_result cm89_class_lookup(
    const cm89_manager *manager,
    cm89_class_h class_handle,
    const char *name,
    cm89_lookup *out_lookup
)
{
    const cm89_class_record *record;
    const cm89_class_record *owner;
    const cm89_member *member;
    cm89_count i;
    cm89_class_h owner_h;

    if (manager == 0 || name == 0 || out_lookup == 0) {
        return CM89_ERR_ARGUMENT;
    }
    record = cm89_class_ref(manager, class_handle);
    if (record == 0) {
        return CM89_ERR_INVALID_HANDLE;
    }

    for (i = 0; i < record->mro_count; ++i) {
        owner_h = record->mro[i];
        owner = cm89_class_ref(manager, owner_h);
        if (owner == 0) {
            return CM89_ERR_INVALID_HANDLE;
        }
        member = cm89_find_own_member_ref(owner, name);
        if (member != 0) {
            out_lookup->owner_class = owner_h;
            out_lookup->member = *member;
            return CM89_OK;
        }
    }
    return CM89_ERR_NOT_FOUND;
}

cm89_bool cm89_is_subclass(
    const cm89_manager *manager,
    cm89_class_h candidate,
    cm89_class_h expected_base
)
{
    const cm89_class_record *record;
    cm89_count i;
    if (!cm89_class_valid(manager, candidate) ||
        !cm89_class_valid(manager, expected_base)) {
        return CM89_FALSE;
    }
    record = cm89_class_ref(manager, candidate);
    if (record == 0) {
        return CM89_FALSE;
    }
    for (i = 0; i < record->mro_count; ++i) {
        if (record->mro[i] == expected_base) {
            return CM89_TRUE;
        }
    }
    return CM89_FALSE;
}

static cm89_result cm89_invoke_member(
    cm89_manager *manager,
    const cm89_lookup *lookup,
    cm89_class_h dynamic_class,
    cm89_value receiver,
    const cm89_call *call,
    cm89_value *out_value
)
{
    cm89_call empty_call;
    cm89_value sink;
    cm89_value actual_receiver;

    if (manager == 0 || lookup == 0) {
        return CM89_ERR_ARGUMENT;
    }
    if (manager->provider.invoke == 0) {
        return CM89_ERR_PROVIDER;
    }

    empty_call.args = 0;
    empty_call.arg_count = 0;
    empty_call.kwargs_handle = cm89_value_none();

    if (call == 0) {
        call = &empty_call;
    }
    if (out_value == 0) {
        out_value = &sink;
    }

    actual_receiver = receiver;
    if (lookup->member.kind == CM89_MEMBER_STATIC_METHOD) {
        actual_receiver = cm89_value_none();
    } else if (lookup->member.kind == CM89_MEMBER_CLASS_METHOD) {
        actual_receiver = cm89_value_class(dynamic_class);
    }

    return manager->provider.invoke(
        manager->provider.user,
        lookup->member.value,
        actual_receiver,
        lookup->owner_class,
        call,
        out_value
    );
}

#if CM89_ENABLE_INTERNAL_INSTANCES
cm89_bool cm89_is_instance(
    const cm89_manager *manager,
    cm89_instance_h instance,
    cm89_class_h expected_class
)
{
    const cm89_instance_record *record;
    record = cm89_instance_ref(manager, instance);
    if (record == 0) {
        return CM89_FALSE;
    }
    return cm89_is_subclass(manager, record->class_handle, expected_class);
}

static cm89_instance_attr *cm89_find_instance_attr_mut(
    cm89_instance_record *record,
    const char *name
)
{
    unsigned int i;
    if (record == 0 || name == 0) {
        return 0;
    }
    for (i = 0U; i < (unsigned int)CM89_MAX_INSTANCE_ATTRS; ++i) {
        if (record->attrs[i].used &&
            cm89_name_equal(record->attrs[i].name, name)) {
            return &record->attrs[i];
        }
    }
    return 0;
}

static const cm89_instance_attr *cm89_find_instance_attr_ref(
    const cm89_instance_record *record,
    const char *name
)
{
    unsigned int i;
    if (record == 0 || name == 0) {
        return 0;
    }
    for (i = 0U; i < (unsigned int)CM89_MAX_INSTANCE_ATTRS; ++i) {
        if (record->attrs[i].used &&
            cm89_name_equal(record->attrs[i].name, name)) {
            return &record->attrs[i];
        }
    }
    return 0;
}

cm89_result cm89_instance_new(
    cm89_manager *manager,
    cm89_class_h class_handle,
    const cm89_call *init_call,
    cm89_instance_h *out_instance
)
{
    unsigned int i;
    cm89_instance_h handle;
    cm89_instance_record *record;
    cm89_lookup lookup;
    cm89_result result;
    cm89_value ignored;

    if (manager == 0 || out_instance == 0) {
        return CM89_ERR_ARGUMENT;
    }
    if (!cm89_class_valid(manager, class_handle)) {
        return CM89_ERR_INVALID_HANDLE;
    }

    handle = CM89_INSTANCE_NONE;
    for (i = 0U; i < (unsigned int)CM89_MAX_INSTANCES; ++i) {
        if (!manager->instances[i].used) {
            handle = (cm89_instance_h)(i + 1U);
            break;
        }
    }
    if (handle == CM89_INSTANCE_NONE) {
        return CM89_ERR_CAPACITY;
    }

    record = &manager->instances[(unsigned int)handle - 1U];
    memset(record, 0, sizeof(*record));
    record->used = CM89_TRUE;
    record->class_handle = class_handle;
    record->revision = ++manager->revision;

    result = cm89_class_lookup(manager, class_handle, "__init__", &lookup);
    if (result == CM89_OK) {
        if (lookup.member.kind != CM89_MEMBER_METHOD) {
            memset(record, 0, sizeof(*record));
            return CM89_ERR_NOT_CALLABLE;
        }
        result = cm89_invoke_member(
            manager,
            &lookup,
            class_handle,
            cm89_value_instance(handle),
            init_call,
            &ignored
        );
        if (result != CM89_OK) {
            memset(record, 0, sizeof(*record));
            return result;
        }
    } else if (result != CM89_ERR_NOT_FOUND) {
        memset(record, 0, sizeof(*record));
        return result;
    }

    *out_instance = handle;
    return CM89_OK;
}

cm89_result cm89_instance_release(
    cm89_manager *manager,
    cm89_instance_h instance
)
{
    cm89_instance_record *record;
    if (manager == 0) {
        return CM89_ERR_ARGUMENT;
    }
    record = cm89_instance_mut(manager, instance);
    if (record == 0) {
        return CM89_ERR_INVALID_HANDLE;
    }
    memset(record, 0, sizeof(*record));
    manager->revision++;
    return CM89_OK;
}

cm89_result cm89_instance_get_class(
    const cm89_manager *manager,
    cm89_instance_h instance,
    cm89_class_h *out_class
)
{
    const cm89_instance_record *record;
    if (out_class == 0) {
        return CM89_ERR_ARGUMENT;
    }
    record = cm89_instance_ref(manager, instance);
    if (record == 0) {
        return CM89_ERR_INVALID_HANDLE;
    }
    *out_class = record->class_handle;
    return CM89_OK;
}

cm89_result cm89_instance_set_attr(
    cm89_manager *manager,
    cm89_instance_h instance,
    const char *name,
    cm89_value value
)
{
    cm89_instance_record *record;
    cm89_instance_attr *attr;
    cm89_lookup lookup;
    cm89_result result;
    unsigned int i;
    cm89_value arg;
    cm89_call call;
    cm89_value ignored;

    if (manager == 0 || name == 0 || name[0] == '\0') {
        return CM89_ERR_ARGUMENT;
    }
    record = cm89_instance_mut(manager, instance);
    if (record == 0) {
        return CM89_ERR_INVALID_HANDLE;
    }

    result = cm89_class_lookup(manager, record->class_handle, name, &lookup);
    if (result == CM89_OK && lookup.member.kind == CM89_MEMBER_PROPERTY) {
        if (lookup.member.aux.kind == CM89_VALUE_NONE) {
            return CM89_ERR_READ_ONLY;
        }
        if (manager->provider.invoke == 0) {
            return CM89_ERR_PROVIDER;
        }
        arg = value;
        call.args = &arg;
        call.arg_count = 1;
        call.kwargs_handle = cm89_value_none();
        return manager->provider.invoke(
            manager->provider.user,
            lookup.member.aux,
            cm89_value_instance(instance),
            lookup.owner_class,
            &call,
            &ignored
        );
    }

    attr = cm89_find_instance_attr_mut(record, name);
    if (attr == 0) {
        for (i = 0U; i < (unsigned int)CM89_MAX_INSTANCE_ATTRS; ++i) {
            if (!record->attrs[i].used) {
                attr = &record->attrs[i];
                memset(attr, 0, sizeof(*attr));
                attr->used = CM89_TRUE;
                cm89_copy_name(attr->name, name);
                break;
            }
        }
    }
    if (attr == 0) {
        return CM89_ERR_CAPACITY;
    }
    attr->value = value;
    record->revision = ++manager->revision;
    return CM89_OK;
}

cm89_result cm89_instance_get_attr(
    cm89_manager *manager,
    cm89_instance_h instance,
    const char *name,
    cm89_value *out_value
)
{
    cm89_instance_record *record;
    const cm89_instance_attr *attr;
    cm89_lookup lookup;
    cm89_result result;
    cm89_call empty_call;

    if (manager == 0 || name == 0 || out_value == 0) {
        return CM89_ERR_ARGUMENT;
    }
    record = cm89_instance_mut(manager, instance);
    if (record == 0) {
        return CM89_ERR_INVALID_HANDLE;
    }

    result = cm89_class_lookup(manager, record->class_handle, name, &lookup);
    if (result == CM89_OK && lookup.member.kind == CM89_MEMBER_PROPERTY) {
        if (lookup.member.value.kind == CM89_VALUE_NONE) {
            return CM89_ERR_NOT_FOUND;
        }
        empty_call.args = 0;
        empty_call.arg_count = 0;
        empty_call.kwargs_handle = cm89_value_none();
        return manager->provider.invoke(
            manager->provider.user,
            lookup.member.value,
            cm89_value_instance(instance),
            lookup.owner_class,
            &empty_call,
            out_value
        );
    }

    attr = cm89_find_instance_attr_ref(record, name);
    if (attr != 0) {
        *out_value = attr->value;
        return CM89_OK;
    }

    if (result != CM89_OK) {
        return result;
    }

    *out_value = lookup.member.value;
    return CM89_OK;
}

cm89_result cm89_instance_remove_attr(
    cm89_manager *manager,
    cm89_instance_h instance,
    const char *name
)
{
    cm89_instance_record *record;
    cm89_instance_attr *attr;
    if (manager == 0 || name == 0) {
        return CM89_ERR_ARGUMENT;
    }
    record = cm89_instance_mut(manager, instance);
    if (record == 0) {
        return CM89_ERR_INVALID_HANDLE;
    }
    attr = cm89_find_instance_attr_mut(record, name);
    if (attr == 0) {
        return CM89_ERR_NOT_FOUND;
    }
    memset(attr, 0, sizeof(*attr));
    record->revision = ++manager->revision;
    return CM89_OK;
}

cm89_result cm89_call_method(
    cm89_manager *manager,
    cm89_instance_h instance,
    const char *name,
    const cm89_call *call,
    cm89_value *out_value
)
{
    const cm89_instance_record *record;
    cm89_lookup lookup;
    cm89_result result;

    if (manager == 0 || name == 0) {
        return CM89_ERR_ARGUMENT;
    }
    record = cm89_instance_ref(manager, instance);
    if (record == 0) {
        return CM89_ERR_INVALID_HANDLE;
    }
    result = cm89_class_lookup(manager, record->class_handle, name, &lookup);
    if (result != CM89_OK) {
        return result;
    }
    if (lookup.member.kind != CM89_MEMBER_METHOD &&
        lookup.member.kind != CM89_MEMBER_STATIC_METHOD &&
        lookup.member.kind != CM89_MEMBER_CLASS_METHOD) {
        return CM89_ERR_NOT_CALLABLE;
    }
    return cm89_invoke_member(
        manager, &lookup, record->class_handle,
        cm89_value_instance(instance), call, out_value
    );
}

cm89_result cm89_call_super(
    cm89_manager *manager,
    cm89_instance_h instance,
    cm89_class_h current_owner,
    const char *name,
    const cm89_call *call,
    cm89_value *out_value
)
{
    const cm89_instance_record *instance_record;
    const cm89_class_record *class_record;
    const cm89_class_record *owner_record;
    const cm89_member *member;
    cm89_lookup lookup;
    cm89_count i;
    cm89_bool found_current;

    if (manager == 0 || name == 0) {
        return CM89_ERR_ARGUMENT;
    }
    instance_record = cm89_instance_ref(manager, instance);
    if (instance_record == 0) {
        return CM89_ERR_INVALID_HANDLE;
    }
    class_record = cm89_class_ref(manager, instance_record->class_handle);
    if (class_record == 0 || !cm89_class_valid(manager, current_owner)) {
        return CM89_ERR_INVALID_HANDLE;
    }

    found_current = CM89_FALSE;
    for (i = 0; i < class_record->mro_count; ++i) {
        if (!found_current) {
            if (class_record->mro[i] == current_owner) {
                found_current = CM89_TRUE;
            }
            continue;
        }

        owner_record = cm89_class_ref(manager, class_record->mro[i]);
        if (owner_record == 0) {
            return CM89_ERR_INVALID_HANDLE;
        }
        member = cm89_find_own_member_ref(owner_record, name);
        if (member != 0) {
            if (member->kind != CM89_MEMBER_METHOD &&
                member->kind != CM89_MEMBER_STATIC_METHOD &&
                member->kind != CM89_MEMBER_CLASS_METHOD) {
                return CM89_ERR_NOT_CALLABLE;
            }
            lookup.owner_class = class_record->mro[i];
            lookup.member = *member;
            return cm89_invoke_member(
                manager,
                &lookup,
                instance_record->class_handle,
                cm89_value_instance(instance),
                call,
                out_value
            );
        }
    }

    if (!found_current) {
        return CM89_ERR_ARGUMENT;
    }
    return CM89_ERR_NOT_FOUND;
}

#else
cm89_bool cm89_is_instance(
    const cm89_manager *manager, cm89_instance_h instance,
    cm89_class_h expected_class
)
{
    (void)manager; (void)instance; (void)expected_class;
    return CM89_FALSE;
}

cm89_result cm89_instance_new(
    cm89_manager *manager, cm89_class_h class_handle,
    const cm89_call *init_call, cm89_instance_h *out_instance
)
{
    (void)manager; (void)class_handle; (void)init_call;
    if (out_instance) *out_instance = CM89_INSTANCE_NONE;
    return CM89_ERR_DISABLED;
}
cm89_result cm89_instance_release(cm89_manager *manager, cm89_instance_h instance)
{ (void)manager; (void)instance; return CM89_ERR_DISABLED; }
cm89_result cm89_instance_get_class(
    const cm89_manager *manager, cm89_instance_h instance, cm89_class_h *out_class
)
{ (void)manager; (void)instance; if (out_class) *out_class = CM89_CLASS_NONE; return CM89_ERR_DISABLED; }
cm89_result cm89_instance_set_attr(
    cm89_manager *manager, cm89_instance_h instance, const char *name, cm89_value value
)
{ (void)manager; (void)instance; (void)name; (void)value; return CM89_ERR_DISABLED; }
cm89_result cm89_instance_get_attr(
    cm89_manager *manager, cm89_instance_h instance, const char *name, cm89_value *out_value
)
{ (void)manager; (void)instance; (void)name; if (out_value) *out_value = cm89_value_none(); return CM89_ERR_DISABLED; }
cm89_result cm89_instance_remove_attr(
    cm89_manager *manager, cm89_instance_h instance, const char *name
)
{ (void)manager; (void)instance; (void)name; return CM89_ERR_DISABLED; }
cm89_result cm89_call_method(
    cm89_manager *manager, cm89_instance_h instance, const char *name,
    const cm89_call *call, cm89_value *out_value
)
{ (void)manager; (void)instance; (void)name; (void)call; (void)out_value; return CM89_ERR_DISABLED; }
cm89_result cm89_call_super(
    cm89_manager *manager, cm89_instance_h instance, cm89_class_h current_owner,
    const char *name, const cm89_call *call, cm89_value *out_value
)
{ (void)manager; (void)instance; (void)current_owner; (void)name; (void)call; (void)out_value; return CM89_ERR_DISABLED; }
#endif

cm89_result cm89_call_class_method(
    cm89_manager *manager,
    cm89_class_h class_handle,
    const char *name,
    const cm89_call *call,
    cm89_value *out_value
)
{
    cm89_lookup lookup;
    cm89_result result;
    cm89_value receiver;
    if (manager == 0 || name == 0) return CM89_ERR_ARGUMENT;
    if (!cm89_class_valid(manager, class_handle)) return CM89_ERR_INVALID_HANDLE;
    result = cm89_class_lookup(manager, class_handle, name, &lookup);
    if (result != CM89_OK) return result;
    if (lookup.member.kind != CM89_MEMBER_CLASS_METHOD &&
        lookup.member.kind != CM89_MEMBER_STATIC_METHOD)
        return CM89_ERR_NOT_CALLABLE;
    receiver = cm89_value_class(class_handle);
    return cm89_invoke_member(manager, &lookup, class_handle, receiver, call, out_value);
}

cm89_result cm89_call_bound(
    cm89_manager *manager, cm89_class_h dynamic_class, cm89_value receiver,
    const char *name, const cm89_call *call, cm89_value *out_value
)
{
    cm89_lookup lookup;
    cm89_result result;
    if (manager == 0 || name == 0) return CM89_ERR_ARGUMENT;
    if (!cm89_class_valid(manager, dynamic_class)) return CM89_ERR_INVALID_HANDLE;
    result = cm89_class_lookup(manager, dynamic_class, name, &lookup);
    if (result != CM89_OK) return result;
    if (lookup.member.kind != CM89_MEMBER_METHOD &&
        lookup.member.kind != CM89_MEMBER_STATIC_METHOD &&
        lookup.member.kind != CM89_MEMBER_CLASS_METHOD)
        return CM89_ERR_NOT_CALLABLE;
    return cm89_invoke_member(manager, &lookup, dynamic_class, receiver, call, out_value);
}

cm89_result cm89_call_bound_super(
    cm89_manager *manager, cm89_class_h dynamic_class, cm89_value receiver,
    cm89_class_h current_owner, const char *name,
    const cm89_call *call, cm89_value *out_value
)
{
    const cm89_class_record *class_record;
    const cm89_class_record *owner_record;
    const cm89_member *member;
    cm89_lookup lookup;
    cm89_count i;
    cm89_bool found_current;
    if (manager == 0 || name == 0) return CM89_ERR_ARGUMENT;
    class_record = cm89_class_ref(manager, dynamic_class);
    if (class_record == 0 || !cm89_class_valid(manager, current_owner))
        return CM89_ERR_INVALID_HANDLE;
    found_current = CM89_FALSE;
    for (i = 0; i < class_record->mro_count; ++i) {
        if (!found_current) {
            if (class_record->mro[i] == current_owner) found_current = CM89_TRUE;
            continue;
        }
        owner_record = cm89_class_ref(manager, class_record->mro[i]);
        if (owner_record == 0) return CM89_ERR_INVALID_HANDLE;
        member = cm89_find_own_member_ref(owner_record, name);
        if (member != 0) {
            if (member->kind != CM89_MEMBER_METHOD &&
                member->kind != CM89_MEMBER_STATIC_METHOD &&
                member->kind != CM89_MEMBER_CLASS_METHOD)
                return CM89_ERR_NOT_CALLABLE;
            lookup.owner_class = class_record->mro[i];
            lookup.member = *member;
            return cm89_invoke_member(manager, &lookup, dynamic_class, receiver, call, out_value);
        }
    }
    if (!found_current) return CM89_ERR_ARGUMENT;
    return CM89_ERR_NOT_FOUND;
}

cm89_result cm89_bound_get(
    cm89_manager *manager, cm89_class_h dynamic_class, cm89_value receiver,
    const char *name, cm89_value *out_value
)
{
    cm89_lookup lookup;
    cm89_call empty_call;
    cm89_result result;
    if (manager == 0 || name == 0 || out_value == 0) return CM89_ERR_ARGUMENT;
    if (!cm89_class_valid(manager, dynamic_class)) return CM89_ERR_INVALID_HANDLE;
    result = cm89_class_lookup(manager, dynamic_class, name, &lookup);
    if (result != CM89_OK) return result;
    if (lookup.member.kind == CM89_MEMBER_PROPERTY) {
        if (lookup.member.value.kind == CM89_VALUE_NONE) return CM89_ERR_NOT_FOUND;
        if (manager->provider.invoke == 0) return CM89_ERR_PROVIDER;
        empty_call.args = 0;
        empty_call.arg_count = 0;
        empty_call.kwargs_handle = cm89_value_none();
        return manager->provider.invoke(manager->provider.user, lookup.member.value,
            receiver, lookup.owner_class, &empty_call, out_value);
    }
    if (lookup.member.kind != CM89_MEMBER_VALUE) return CM89_ERR_NOT_CALLABLE;
    *out_value = lookup.member.value;
    return CM89_OK;
}

cm89_result cm89_bound_set_property(
    cm89_manager *manager, cm89_class_h dynamic_class, cm89_value receiver,
    const char *name, cm89_value value
)
{
    cm89_lookup lookup;
    cm89_value arg;
    cm89_call call;
    cm89_value ignored;
    cm89_result result;
    if (manager == 0 || name == 0) return CM89_ERR_ARGUMENT;
    if (!cm89_class_valid(manager, dynamic_class)) return CM89_ERR_INVALID_HANDLE;
    result = cm89_class_lookup(manager, dynamic_class, name, &lookup);
    if (result != CM89_OK) return result;
    if (lookup.member.kind != CM89_MEMBER_PROPERTY) return CM89_ERR_READ_ONLY;
    if (lookup.member.aux.kind == CM89_VALUE_NONE) return CM89_ERR_READ_ONLY;
    if (manager->provider.invoke == 0) return CM89_ERR_PROVIDER;
    arg = value;
    call.args = &arg;
    call.arg_count = 1;
    call.kwargs_handle = cm89_value_none();
    return manager->provider.invoke(manager->provider.user, lookup.member.aux,
        receiver, lookup.owner_class, &call, &ignored);
}
