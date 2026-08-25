#include "invariantSpecialoperations_89_flags89.h"

static const char *iso89_flags89_name(const iso89_flags89_binding *binding,
                                      iso89_subject subject)
{
    unsigned long index;
    if (!binding || !binding->subject_names || subject == 0UL) return 0;
    index = subject - 1UL;
    if (index >= (unsigned long)binding->subject_count) return 0;
    return binding->subject_names[index];
}

static int iso89_flags89_get(void *user, iso89_subject subject,
                             iso89_value *out_value)
{
    iso89_flags89_binding *binding;
    const char *name;
    FlagsValue value;
    binding = (iso89_flags89_binding *)user;
    if (!binding || !out_value || !binding->store) return 0;
    name = iso89_flags89_name(binding, subject);
    if (!name) return 0;
    if (!flagstore_get(binding->store, name, &value)) {
        if (binding->missing_value_is_zero) {
            *out_value = 0L;
            return 1;
        }
        return 0;
    }
    if (value.type == FLAGS_VAL_BOOL || value.type == FLAGS_VAL_INT) {
        *out_value = (iso89_value)value.as.i;
        return 1;
    }
    if (value.type == FLAGS_VAL_FX) {
        *out_value = (iso89_value)value.as.fx;
        return 1;
    }
    return 0;
}

static int iso89_flags89_set(void *user, iso89_subject subject,
                             iso89_value value)
{
    iso89_flags89_binding *binding;
    const char *name;
    binding = (iso89_flags89_binding *)user;
    if (!binding || !binding->store) return 0;
    name = iso89_flags89_name(binding, subject);
    if (!name) return 0;
    return flagstore_set_int(binding->store, name, (long)value);
}

void iso89_flags89_binding_init(iso89_flags89_binding *binding,
                                FlagStore *store,
                                const char *const *subject_names,
                                int subject_count)
{
    if (!binding) return;
    binding->store = store;
    binding->subject_names = subject_names;
    binding->subject_count = subject_count > 0 ? subject_count : 0;
    binding->missing_value_is_zero = 1;
}

void iso89_flags89_make_provider(iso89_flags89_binding *binding,
                                 iso89_provider *out_provider)
{
    if (!out_provider) return;
    out_provider->get_value = iso89_flags89_get;
    out_provider->set_value = iso89_flags89_set;
    out_provider->user = binding;
}
