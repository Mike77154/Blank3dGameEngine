#ifndef CM89_H
#define CM89_H

#include "cm89_types.h"
#include "cm89_provider.h"

typedef struct cm89_class_record {
    char name[CM89_NAME_MAX];
    cm89_class_h bases[CM89_MAX_BASES];
    cm89_count base_count;
    cm89_class_h mro[CM89_MAX_MRO];
    cm89_count mro_count;
    cm89_member members[CM89_MAX_CLASS_MEMBERS];
    unsigned long revision;
    cm89_bool used;
    cm89_bool finalized;
    unsigned char generation;
} cm89_class_record;

#if CM89_ENABLE_INTERNAL_INSTANCES
typedef struct cm89_instance_attr {
    char name[CM89_NAME_MAX];
    cm89_value value;
    cm89_bool used;
} cm89_instance_attr;

typedef struct cm89_instance_record {
    cm89_class_h class_handle;
    cm89_instance_attr attrs[CM89_MAX_INSTANCE_ATTRS];
    unsigned long revision;
    cm89_bool used;
} cm89_instance_record;
#endif

typedef struct cm89_manager {
    cm89_class_record classes[CM89_MAX_CLASSES];
#if CM89_ENABLE_INTERNAL_INSTANCES
    cm89_instance_record instances[CM89_MAX_INSTANCES];
#endif
    cm89_provider provider;
    unsigned long revision;
    cm89_bool sealed;
} cm89_manager;

typedef struct cm89_lookup {
    cm89_class_h owner_class;
    cm89_member member;
} cm89_lookup;

/* Value constructors. */
cm89_value cm89_value_none(void);
cm89_value cm89_value_sint(long value);
cm89_value cm89_value_uint(unsigned long value);
cm89_value cm89_value_ptr(void *value);
cm89_value cm89_value_host_handle(unsigned long value);
cm89_value cm89_value_class(cm89_class_h value);
cm89_value cm89_value_instance(cm89_instance_h value);

/* Manager lifecycle / authoring gate. */
void cm89_manager_init(cm89_manager *manager, const cm89_provider *provider);
void cm89_manager_reset(cm89_manager *manager);
cm89_result cm89_manager_seal(cm89_manager *manager);
cm89_result cm89_manager_unseal(cm89_manager *manager);
cm89_bool cm89_manager_is_sealed(const cm89_manager *manager);

/* Class registry and hierarchy. */
cm89_result cm89_class_create(
    cm89_manager *manager,
    const char *name,
    const cm89_class_h *bases,
    cm89_count base_count,
    cm89_class_h *out_class
);
cm89_result cm89_class_destroy(cm89_manager *manager, cm89_class_h class_handle);
cm89_result cm89_class_find(
    const cm89_manager *manager,
    const char *name,
    cm89_class_h *out_class
);
cm89_result cm89_class_get_name(
    const cm89_manager *manager,
    cm89_class_h class_handle,
    const char **out_name
);
cm89_result cm89_class_get_bases(
    const cm89_manager *manager,
    cm89_class_h class_handle,
    const cm89_class_h **out_bases,
    cm89_count *out_count
);
cm89_result cm89_class_get_mro(
    const cm89_manager *manager,
    cm89_class_h class_handle,
    const cm89_class_h **out_mro,
    cm89_count *out_count
);
unsigned int cm89_class_handle_index(cm89_class_h class_handle);
unsigned int cm89_class_handle_generation(cm89_class_h class_handle);

/* Class namespace. Assignment always targets the selected class itself. */
cm89_result cm89_class_set_member(
    cm89_manager *manager,
    cm89_class_h class_handle,
    const char *name,
    cm89_member_kind kind,
    cm89_value value,
    cm89_value aux
);
cm89_result cm89_class_remove_member(
    cm89_manager *manager,
    cm89_class_h class_handle,
    const char *name
);
cm89_result cm89_class_lookup(
    const cm89_manager *manager,
    cm89_class_h class_handle,
    const char *name,
    cm89_lookup *out_lookup
);

/* Type relationships. */
cm89_bool cm89_is_subclass(
    const cm89_manager *manager,
    cm89_class_h candidate,
    cm89_class_h expected_base
);
cm89_bool cm89_is_instance(
    const cm89_manager *manager,
    cm89_instance_h instance,
    cm89_class_h expected_class
);

/* Optional standalone instance pool. Returns CM89_ERR_DISABLED when disabled. */
cm89_result cm89_instance_new(
    cm89_manager *manager,
    cm89_class_h class_handle,
    const cm89_call *init_call,
    cm89_instance_h *out_instance
);
cm89_result cm89_instance_release(cm89_manager *manager, cm89_instance_h instance);
cm89_result cm89_instance_get_class(
    const cm89_manager *manager,
    cm89_instance_h instance,
    cm89_class_h *out_class
);
cm89_result cm89_instance_set_attr(
    cm89_manager *manager,
    cm89_instance_h instance,
    const char *name,
    cm89_value value
);
cm89_result cm89_instance_get_attr(
    cm89_manager *manager,
    cm89_instance_h instance,
    const char *name,
    cm89_value *out_value
);
cm89_result cm89_instance_remove_attr(
    cm89_manager *manager,
    cm89_instance_h instance,
    const char *name
);

/* Optional internal-instance dispatch. */
cm89_result cm89_call_method(
    cm89_manager *manager,
    cm89_instance_h instance,
    const char *name,
    const cm89_call *call,
    cm89_value *out_value
);
cm89_result cm89_call_super(
    cm89_manager *manager,
    cm89_instance_h instance,
    cm89_class_h current_owner,
    const char *name,
    const cm89_call *call,
    cm89_value *out_value
);

/*
 * External/bound receiver dispatch.  The host owns the instance identity.
 * For Blank3D, receiver is CM89_VALUE_HOST_HANDLE carrying the Thing handle.
 */
cm89_result cm89_call_bound(
    cm89_manager *manager,
    cm89_class_h dynamic_class,
    cm89_value receiver,
    const char *name,
    const cm89_call *call,
    cm89_value *out_value
);
cm89_result cm89_call_bound_super(
    cm89_manager *manager,
    cm89_class_h dynamic_class,
    cm89_value receiver,
    cm89_class_h current_owner,
    const char *name,
    const cm89_call *call,
    cm89_value *out_value
);
cm89_result cm89_bound_get(
    cm89_manager *manager,
    cm89_class_h dynamic_class,
    cm89_value receiver,
    const char *name,
    cm89_value *out_value
);
cm89_result cm89_bound_set_property(
    cm89_manager *manager,
    cm89_class_h dynamic_class,
    cm89_value receiver,
    const char *name,
    cm89_value value
);

cm89_result cm89_call_class_method(
    cm89_manager *manager,
    cm89_class_h class_handle,
    const char *name,
    const cm89_call *call,
    cm89_value *out_value
);

#endif
