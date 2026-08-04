#ifndef RPYL_STORE_H
#define RPYL_STORE_H

#include <stddef.h>
#include "rpyl_config.h"

#ifndef RPYL_STORE_MAX
#define RPYL_STORE_MAX 256
#endif
#ifndef RPYL_STORE_MAX_KEY
#define RPYL_STORE_MAX_KEY RPYL_RUNTIME_MAX_NAME
#endif
#ifndef RPYL_STORE_MAX_VALUE
#define RPYL_STORE_MAX_VALUE RPYL_RUNTIME_MAX_VALUE
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RpylStoreItem {
    char key[RPYL_STORE_MAX_KEY];
    char value[RPYL_STORE_MAX_VALUE];
    unsigned long flags;
} RpylStoreItem;

typedef struct RpylStore {
    RpylStoreItem items[RPYL_STORE_MAX];
    size_t count;
    int overflowed;
} RpylStore;

void rpyl_store_init(RpylStore* s);
void rpyl_store_clear(RpylStore* s);
int rpyl_store_set_ex(RpylStore* s, const char* key, const char* value, unsigned long flags);
int rpyl_store_set(RpylStore* s, const char* key, const char* value);
const char* rpyl_store_get(const RpylStore* s, const char* key);
const RpylStoreItem* rpyl_store_find(const RpylStore* s, const char* key);
int rpyl_store_remove(RpylStore* s, const char* key);
int rpyl_store_contains(const RpylStore* s, const char* key);
size_t rpyl_store_count(const RpylStore* s);
const RpylStoreItem* rpyl_store_at(const RpylStore* s, size_t index);
int rpyl_store_copy(RpylStore* dst, const RpylStore* src);
int rpyl_store_overflowed(const RpylStore* s);

#ifdef __cplusplus
}
#endif

#endif
