#ifndef INVARIANT_SPECIALOPERATIONS_89_FLAGS89_H
#define INVARIANT_SPECIALOPERATIONS_89_FLAGS89_H

#include "invariantSpecialoperations_89.h"
#include "flagstore.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct iso89_flags89_binding {
    FlagStore *store;
    const char *const *subject_names;
    int subject_count;
    int missing_value_is_zero;
} iso89_flags89_binding;

void iso89_flags89_binding_init(iso89_flags89_binding *binding,
                                FlagStore *store,
                                const char *const *subject_names,
                                int subject_count);
void iso89_flags89_make_provider(iso89_flags89_binding *binding,
                                 iso89_provider *out_provider);

#ifdef __cplusplus
}
#endif

#endif
