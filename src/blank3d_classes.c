#include "blank3d_classes.h"

#include <string.h>
#include <ctype.h>

static void b3d_cls_copy(char *dst, unsigned int cap, const char *src)
{
    unsigned int i;
    if (!dst || cap == 0U) return;
    if (!src) src = "";
    i = 0U;
    while (src[i] != '\0' && i + 1U < cap) {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
}

static char *b3d_cls_trim(char *text)
{
    char *end;
    if (!text) return text;
    while (*text && isspace((unsigned char)*text)) ++text;
    end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1])) --end;
    *end = '\0';
    return text;
}

int blank3d_classes_init(Blank3DClassSystem *classes,
                         const cm89_provider *provider)
{
    cm89_result result;
    if (!classes) return 0;
    memset(classes, 0, sizeof(*classes));
    cm89_manager_init(&classes->manager, provider);
    result = cm89_class_create(&classes->manager, "Object", 0, 0,
                               &classes->object_class);
    if (result != CM89_OK) {
        b3d_cls_copy(classes->status, sizeof(classes->status),
                     "ClassSystem root Object creation failed");
        return 0;
    }
    b3d_cls_copy(classes->status, sizeof(classes->status),
                 "ClassSystem initialized");
    return 1;
}

void blank3d_classes_reset(Blank3DClassSystem *classes,
                           const cm89_provider *provider)
{
    (void)blank3d_classes_init(classes, provider);
}

cm89_result blank3d_classes_find(const Blank3DClassSystem *classes,
                                 const char *name,
                                 cm89_class_h *out_class)
{
    if (!classes) return CM89_ERR_ARGUMENT;
    return cm89_class_find(&classes->manager, name, out_class);
}

cm89_result blank3d_classes_resolve_or_create(
    Blank3DClassSystem *classes,
    const char *class_name,
    const char *base_names,
    cm89_class_h *out_class)
{
    char bases_text[B3D_CLASS_BASES_TEXT_CAP];
    cm89_class_h bases[CM89_MAX_BASES];
    cm89_count base_count;
    char *cursor;
    char *comma;
    char *name;
    cm89_class_h found;
    cm89_result result;

    if (!classes || !class_name || !class_name[0] || !out_class)
        return CM89_ERR_ARGUMENT;

    result = cm89_class_find(&classes->manager, class_name, &found);
    if (result == CM89_OK) {
        *out_class = found;
        return CM89_OK;
    }
    if (result != CM89_ERR_NOT_FOUND) return result;

    base_count = 0;
    if (!base_names || !base_names[0]) {
        if (strcmp(class_name, "Object") != 0) {
            bases[0] = classes->object_class;
            base_count = 1;
        }
    } else {
        b3d_cls_copy(bases_text, sizeof(bases_text), base_names);
        cursor = bases_text;
        while (*cursor) {
            if (base_count >= (cm89_count)CM89_MAX_BASES)
                return CM89_ERR_CAPACITY;
            comma = strchr(cursor, ',');
            if (comma) *comma = '\0';
            name = b3d_cls_trim(cursor);
            if (!name[0]) return CM89_ERR_ARGUMENT;
            result = cm89_class_find(&classes->manager, name,
                                     &bases[base_count]);
            if (result != CM89_OK) {
                b3d_cls_copy(classes->status, sizeof(classes->status),
                             "ClassSystem base class not registered");
                return result;
            }
            ++base_count;
            if (!comma) break;
            cursor = comma + 1;
        }
    }

    result = cm89_class_create(&classes->manager, class_name,
                               base_count ? bases : 0, base_count, out_class);
    if (result == CM89_OK)
        b3d_cls_copy(classes->status, sizeof(classes->status),
                     "ClassSystem class registered");
    return result;
}

cm89_result blank3d_classes_seal(Blank3DClassSystem *classes)
{
    if (!classes) return CM89_ERR_ARGUMENT;
    return cm89_manager_seal(&classes->manager);
}

cm89_result blank3d_classes_unseal(Blank3DClassSystem *classes)
{
    if (!classes) return CM89_ERR_ARGUMENT;
    return cm89_manager_unseal(&classes->manager);
}

cm89_bool blank3d_classes_is_subclass(const Blank3DClassSystem *classes,
                                      cm89_class_h candidate,
                                      cm89_class_h expected_base)
{
    if (!classes) return CM89_FALSE;
    return cm89_is_subclass(&classes->manager, candidate, expected_base);
}

cm89_result blank3d_classes_call_bound(
    Blank3DClassSystem *classes,
    cm89_class_h dynamic_class,
    unsigned long thing_handle,
    const char *method_name,
    const cm89_call *call,
    cm89_value *out_value)
{
    if (!classes || thing_handle == 0UL) return CM89_ERR_ARGUMENT;
    return cm89_call_bound(&classes->manager, dynamic_class,
                           cm89_value_host_handle(thing_handle),
                           method_name, call, out_value);
}

const char *blank3d_classes_status(const Blank3DClassSystem *classes)
{
    return classes ? classes->status : "ClassSystem unavailable";
}
