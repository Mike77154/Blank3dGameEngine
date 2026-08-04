#include "fpi_registry.h"
#include "fpi_util.h"

static const char* registry_pool_at(const FPI_Registry* registry, FPI_U32 offset) {
    if (!registry || offset >= registry->pool_used) return 0;
    return registry->pool + offset;
}

static int registry_name_valid(const char* name) {
    int i;
    if (!name || !fpi_ascii_is_ident_start(name[0])) return 0;
    i = 1;
    while (name[i]) {
        if (!fpi_ascii_is_ident_continue(name[i])) return 0;
        if (i >= FPI_IDENT_MAX) return 0;
        i++;
    }
    return 1;
}

static int registry_store_name(FPI_Registry* registry, const char* name, FPI_U32* out_offset, FPI_Error* error) {
    int len;
    int i;
    FPI_U32 needed;
    FPI_U32 offset;
    FPI_Span span;
    fpi_span_clear(&span);
    if (!registry || !name || !out_offset) return FPI_ERR_ARGUMENT;
    if (!registry_name_valid(name)) {
        fpi_error_set(error, FPI_ERR_IDENTIFIER_TOO_LONG, span, "invalid or oversized symbol name");
        return FPI_ERR_IDENTIFIER_TOO_LONG;
    }
    len = 0;
    while (name[len]) len++;
    needed = (FPI_U32)len + 1UL;
    if (needed > FPI_REGISTRY_POOL_BYTES - registry->pool_used) {
        fpi_error_set(error, FPI_ERR_STRING_POOL_FULL, span, "registry string pool exhausted");
        return FPI_ERR_STRING_POOL_FULL;
    }
    offset = registry->pool_used;
    for (i = 0; i < len; i++) registry->pool[offset + (FPI_U32)i] = fpi_ascii_tolower(name[i]);
    registry->pool[offset + (FPI_U32)len] = '\0';
    registry->pool_used += needed;
    *out_offset = offset;
    return FPI_OK;
}

void fpi_registry_init(FPI_Registry* registry) {
    int i;
    if (!registry) return;
    registry->pool_used = 0UL;
    registry->condition_count = 0;
    registry->action_count = 0;
    registry->pool[0] = '\0';
    for (i = 0; i < FPI_MAX_COND_SYMBOLS; i++) {
        registry->conditions[i].name_offset = 0UL;
        registry->conditions[i].hash = 0UL;
        registry->conditions[i].canonical_id = i;
        registry->conditions[i].fn = 0;
        registry->conditions[i].bind_user = 0;
    }
    for (i = 0; i < FPI_MAX_ACT_SYMBOLS; i++) {
        registry->actions[i].name_offset = 0UL;
        registry->actions[i].hash = 0UL;
        registry->actions[i].canonical_id = i;
        registry->actions[i].fn = 0;
        registry->actions[i].bind_user = 0;
    }
}

int fpi_registry_find_cond(const FPI_Registry* registry, const char* name) {
    FPI_U32 hash;
    int i;
    const char* stored;
    if (!registry || !name) return -1;
    hash = fpi_hash_lower(name);
    for (i = 0; i < registry->condition_count; i++) {
        if (registry->conditions[i].hash != hash) continue;
        stored = registry_pool_at(registry, registry->conditions[i].name_offset);
        if (stored && fpi_str_ieq(stored, name)) return i;
    }
    return -1;
}

int fpi_registry_find_act(const FPI_Registry* registry, const char* name) {
    FPI_U32 hash;
    int i;
    const char* stored;
    if (!registry || !name) return -1;
    hash = fpi_hash_lower(name);
    for (i = 0; i < registry->action_count; i++) {
        if (registry->actions[i].hash != hash) continue;
        stored = registry_pool_at(registry, registry->actions[i].name_offset);
        if (stored && fpi_str_ieq(stored, name)) return i;
    }
    return -1;
}

int fpi_registry_register_cond(FPI_Registry* registry, const char* name, FPI_Error* error) {
    int id;
    FPI_U32 offset;
    int rc;
    FPI_Span span;
    fpi_span_clear(&span);
    if (!registry || !name) return FPI_ERR_ARGUMENT;
    id = fpi_registry_find_cond(registry, name);
    if (id >= 0) return id;
    if (registry->condition_count >= FPI_MAX_COND_SYMBOLS) {
        fpi_error_set(error, FPI_ERR_SYMBOL_TABLE_FULL, span, "condition registry full");
        return FPI_ERR_SYMBOL_TABLE_FULL;
    }
    rc = registry_store_name(registry, name, &offset, error);
    if (rc != FPI_OK) return rc;
    id = registry->condition_count++;
    registry->conditions[id].name_offset = offset;
    registry->conditions[id].hash = fpi_hash_lower(name);
    registry->conditions[id].canonical_id = id;
    return id;
}

int fpi_registry_register_act(FPI_Registry* registry, const char* name, FPI_Error* error) {
    int id;
    FPI_U32 offset;
    int rc;
    FPI_Span span;
    fpi_span_clear(&span);
    if (!registry || !name) return FPI_ERR_ARGUMENT;
    id = fpi_registry_find_act(registry, name);
    if (id >= 0) return id;
    if (registry->action_count >= FPI_MAX_ACT_SYMBOLS) {
        fpi_error_set(error, FPI_ERR_SYMBOL_TABLE_FULL, span, "action registry full");
        return FPI_ERR_SYMBOL_TABLE_FULL;
    }
    rc = registry_store_name(registry, name, &offset, error);
    if (rc != FPI_OK) return rc;
    id = registry->action_count++;
    registry->actions[id].name_offset = offset;
    registry->actions[id].hash = fpi_hash_lower(name);
    registry->actions[id].canonical_id = id;
    return id;
}

const char* fpi_registry_cond_name(const FPI_Registry* registry, int id) {
    if (!registry || id < 0 || id >= registry->condition_count) return 0;
    return registry_pool_at(registry, registry->conditions[id].name_offset);
}

const char* fpi_registry_act_name(const FPI_Registry* registry, int id) {
    if (!registry || id < 0 || id >= registry->action_count) return 0;
    return registry_pool_at(registry, registry->actions[id].name_offset);
}

int fpi_registry_canonical_cond(const FPI_Registry* registry, int id) {
    int canonical;
    if (!registry || id < 0 || id >= registry->condition_count) return -1;
    canonical = registry->conditions[id].canonical_id;
    if (canonical < 0 || canonical >= registry->condition_count) return -1;
    return canonical;
}

int fpi_registry_canonical_act(const FPI_Registry* registry, int id) {
    int canonical;
    if (!registry || id < 0 || id >= registry->action_count) return -1;
    canonical = registry->actions[id].canonical_id;
    if (canonical < 0 || canonical >= registry->action_count) return -1;
    return canonical;
}

int fpi_registry_bind_cond_id(FPI_Registry* registry, int id, FPI_BoundCondFn fn, void* bind_user) {
    int canonical;
    canonical = fpi_registry_canonical_cond(registry, id);
    if (canonical < 0) return FPI_ERR_ARGUMENT;
    registry->conditions[canonical].fn = fn;
    registry->conditions[canonical].bind_user = bind_user;
    return FPI_OK;
}

int fpi_registry_bind_act_id(FPI_Registry* registry, int id, FPI_BoundActFn fn, void* bind_user) {
    int canonical;
    canonical = fpi_registry_canonical_act(registry, id);
    if (canonical < 0) return FPI_ERR_ARGUMENT;
    registry->actions[canonical].fn = fn;
    registry->actions[canonical].bind_user = bind_user;
    return FPI_OK;
}

int fpi_registry_bind_cond(FPI_Registry* registry, const char* name, FPI_BoundCondFn fn, void* bind_user, FPI_Error* error) {
    int id;
    id = fpi_registry_register_cond(registry, name, error);
    if (id < 0) return id;
    if (fpi_registry_bind_cond_id(registry, id, fn, bind_user) != FPI_OK) return FPI_ERR_ARGUMENT;
    return id;
}

int fpi_registry_bind_act(FPI_Registry* registry, const char* name, FPI_BoundActFn fn, void* bind_user, FPI_Error* error) {
    int id;
    id = fpi_registry_register_act(registry, name, error);
    if (id < 0) return id;
    if (fpi_registry_bind_act_id(registry, id, fn, bind_user) != FPI_OK) return FPI_ERR_ARGUMENT;
    return id;
}

int fpi_registry_alias_cond(FPI_Registry* registry, const char* alias_name, const char* target_name, FPI_Error* error) {
    int target;
    int alias;
    target = fpi_registry_register_cond(registry, target_name, error);
    if (target < 0) return target;
    target = fpi_registry_canonical_cond(registry, target);
    alias = fpi_registry_register_cond(registry, alias_name, error);
    if (alias < 0) return alias;
    registry->conditions[alias].canonical_id = target;
    return alias;
}

int fpi_registry_alias_act(FPI_Registry* registry, const char* alias_name, const char* target_name, FPI_Error* error) {
    int target;
    int alias;
    target = fpi_registry_register_act(registry, target_name, error);
    if (target < 0) return target;
    target = fpi_registry_canonical_act(registry, target);
    alias = fpi_registry_register_act(registry, alias_name, error);
    if (alias < 0) return alias;
    registry->actions[alias].canonical_id = target;
    return alias;
}

int fpi_registry_dispatch_cond(const FPI_Registry* registry, void* run_user, int id, const FPI_Value* value) {
    int canonical;
    const FPI_CondSymbol* symbol;
    canonical = fpi_registry_canonical_cond(registry, id);
    if (canonical < 0) return FPI_ERR_ARGUMENT;
    symbol = &registry->conditions[canonical];
    if (!symbol->fn) return FPI_ERR_UNBOUND_SYMBOL;
    return symbol->fn(symbol->bind_user, run_user, id, value);
}

int fpi_registry_dispatch_act(const FPI_Registry* registry, void* run_user, int id, const FPI_Value* value) {
    int canonical;
    const FPI_ActSymbol* symbol;
    canonical = fpi_registry_canonical_act(registry, id);
    if (canonical < 0) return FPI_ERR_ARGUMENT;
    symbol = &registry->actions[canonical];
    if (!symbol->fn) return FPI_ERR_UNBOUND_SYMBOL;
    symbol->fn(symbol->bind_user, run_user, id, value);
    return FPI_OK;
}
