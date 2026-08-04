#ifndef RPYL_REGISTRY_H
#define RPYL_REGISTRY_H

#include <stddef.h>
#include "rpyl_config.h"

#ifndef RPYL_REGISTRY_MAX
#define RPYL_REGISTRY_MAX 256
#endif
#ifndef RPYL_REGISTRY_MAX_NAME
#define RPYL_REGISTRY_MAX_NAME RPYL_RUNTIME_MAX_NAME
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*RpylRegistryFn)(void* user, const char** args, int argc);

typedef struct RpylRegistryEntry {
    char name[RPYL_REGISTRY_MAX_NAME];
    RpylRegistryFn fn;
    void* user;
    unsigned long flags;
} RpylRegistryEntry;

typedef struct RpylRegistry {
    RpylRegistryEntry entries[RPYL_REGISTRY_MAX];
    size_t count;
    int overflowed;
} RpylRegistry;

void rpyl_registry_init(RpylRegistry* r);
void rpyl_registry_clear(RpylRegistry* r);
int rpyl_registry_add_ex(RpylRegistry* r, const char* name, RpylRegistryFn fn, void* user, unsigned long flags);
int rpyl_registry_add(RpylRegistry* r, const char* name, RpylRegistryFn fn, void* user);
RpylRegistryFn rpyl_registry_find(const RpylRegistry* r, const char* name, void** user_out);
const RpylRegistryEntry* rpyl_registry_entry(const RpylRegistry* r, const char* name);
int rpyl_registry_remove(RpylRegistry* r, const char* name);
int rpyl_registry_dispatch(const RpylRegistry* r, const char* name, const char** args, int argc);
size_t rpyl_registry_count(const RpylRegistry* r);
const RpylRegistryEntry* rpyl_registry_at(const RpylRegistry* r, size_t index);
int rpyl_registry_overflowed(const RpylRegistry* r);

#ifdef __cplusplus
}
#endif

#endif
