#ifndef BLANK3D_CLASSES_H
#define BLANK3D_CLASSES_H

#include "cm89.h"

#ifdef __cplusplus
extern "C" {
#endif

#define B3D_CLASS_BASES_TEXT_CAP 128

typedef struct Blank3DClassSystemTag {
    cm89_manager manager;
    cm89_class_h object_class;
    char status[160];
} Blank3DClassSystem;

int blank3d_classes_init(Blank3DClassSystem *classes,
                         const cm89_provider *provider);
void blank3d_classes_reset(Blank3DClassSystem *classes,
                           const cm89_provider *provider);
cm89_result blank3d_classes_resolve_or_create(
    Blank3DClassSystem *classes,
    const char *class_name,
    const char *base_names,
    cm89_class_h *out_class);
cm89_result blank3d_classes_find(const Blank3DClassSystem *classes,
                                 const char *name,
                                 cm89_class_h *out_class);
cm89_result blank3d_classes_seal(Blank3DClassSystem *classes);
cm89_result blank3d_classes_unseal(Blank3DClassSystem *classes);
cm89_bool blank3d_classes_is_subclass(const Blank3DClassSystem *classes,
                                      cm89_class_h candidate,
                                      cm89_class_h expected_base);
cm89_result blank3d_classes_call_bound(
    Blank3DClassSystem *classes,
    cm89_class_h dynamic_class,
    unsigned long thing_handle,
    const char *method_name,
    const cm89_call *call,
    cm89_value *out_value);
const char *blank3d_classes_status(const Blank3DClassSystem *classes);

#ifdef __cplusplus
}
#endif

#endif
