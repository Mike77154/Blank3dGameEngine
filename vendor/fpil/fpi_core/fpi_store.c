#include "fpi_store.h"
#include "fpi_util.h"

void fpi_store_init(FPI_Store* store) {
    int i;
    if (!store) return;
    store->pool_used = 0UL;
    store->count = 0;
    store->pool[0] = '\0';
    for (i = 0; i < FPI_STORE_MAX_ENTRIES; i++) store->entries[i].used = 0;
}

int fpi_store_get(const FPI_Store* store, const char* name, FPI_Fixed* out_value) {
    FPI_U32 hash;
    int i;
    if (!store || !name || !out_value) return 0;
    hash = fpi_hash_lower(name);
    for (i = 0; i < store->count; i++) {
        if (!store->entries[i].used || store->entries[i].hash != hash) continue;
        if (fpi_str_ieq(store->pool + store->entries[i].name_offset, name)) {
            *out_value = store->entries[i].value;
            return 1;
        }
    }
    return 0;
}

int fpi_store_set(FPI_Store* store, const char* name, FPI_Fixed value, FPI_Error* error) {
    FPI_U32 hash;
    int i;
    int len;
    FPI_U32 offset;
    FPI_Span span;
    fpi_span_clear(&span);
    if (!store || !name) return FPI_ERR_ARGUMENT;
    hash = fpi_hash_lower(name);
    for (i = 0; i < store->count; i++) {
        if (store->entries[i].used && store->entries[i].hash == hash &&
            fpi_str_ieq(store->pool + store->entries[i].name_offset, name)) {
            store->entries[i].value = value;
            return i;
        }
    }
    if (store->count >= FPI_STORE_MAX_ENTRIES) {
        fpi_error_set(error, FPI_ERR_STRING_POOL_FULL, span, "store entries exhausted");
        return FPI_ERR_STRING_POOL_FULL;
    }
    len = 0;
    while (name[len]) {
        if (len >= FPI_IDENT_MAX) return FPI_ERR_IDENTIFIER_TOO_LONG;
        len++;
    }
    if ((FPI_U32)len + 1UL > FPI_STORE_POOL_BYTES - store->pool_used) {
        fpi_error_set(error, FPI_ERR_STRING_POOL_FULL, span, "store name pool exhausted");
        return FPI_ERR_STRING_POOL_FULL;
    }
    offset = store->pool_used;
    for (i = 0; i < len; i++) store->pool[offset + (FPI_U32)i] = fpi_ascii_tolower(name[i]);
    store->pool[offset + (FPI_U32)len] = '\0';
    store->pool_used += (FPI_U32)len + 1UL;
    i = store->count++;
    store->entries[i].used = 1;
    store->entries[i].name_offset = offset;
    store->entries[i].hash = hash;
    store->entries[i].value = value;
    return i;
}
