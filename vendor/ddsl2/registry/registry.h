#ifndef DDSL_REGISTRY_H
#define DDSL_REGISTRY_H

#include "config/config.h"
#include "value/value.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*ddsl_registry_fn)(void *user, const ddsl_value *args, int argc, ddsl_value *out);

typedef struct ddsl_registry_item {
    char name[64];
    ddsl_registry_fn fn;
    void *user;
    int used;
} ddsl_registry_item;

typedef struct ddsl_registry {
    ddsl_registry_item items[DDSL_MAX_REGISTRY_ITEMS];
} ddsl_registry;

void ddsl_registry_init(ddsl_registry *r);
int ddsl_registry_add(ddsl_registry *r, const char *name, ddsl_registry_fn fn, void *user);
int ddsl_registry_find(const ddsl_registry *r, const char *name, ddsl_registry_fn *out_fn, void **out_user);

#ifdef __cplusplus
}
#endif

#endif /* DDSL_REGISTRY_H */
