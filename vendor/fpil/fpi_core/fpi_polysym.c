#include "fpi_polysym.h"
#include "fpi_util.h"

void fpi_polysym_init(FPI_PolySym* table) {
    if (!table) return;
    table->pool_used = 0UL;
    table->count = 0;
    table->pool[0] = '\0';
}

int fpi_polysym_resolve(const FPI_PolySym* table, const char* name, FPI_Fixed* out_value) {
    FPI_U32 hash;
    int i;
    const char* stored;
    if (!table || !name || !out_value) return 0;
    hash = fpi_hash_lower(name);
    for (i = 0; i < table->count; i++) {
        if (table->entries[i].hash != hash) continue;
        stored = table->pool + table->entries[i].name_offset;
        if (fpi_str_ieq(stored, name)) {
            *out_value = table->entries[i].value;
            return 1;
        }
    }
    return 0;
}

int fpi_polysym_define(FPI_PolySym* table, const char* name, FPI_Fixed value, FPI_Error* error) {
    FPI_Fixed ignored;
    FPI_U32 hash;
    int i;
    int len;
    FPI_U32 offset;
    FPI_Span span;
    fpi_span_clear(&span);
    if (!table || !name) return FPI_ERR_ARGUMENT;
    hash = fpi_hash_lower(name);
    for (i = 0; i < table->count; i++) {
        const char* stored;
        stored = table->pool + table->entries[i].name_offset;
        if (table->entries[i].hash == hash && fpi_str_ieq(stored, name)) {
            table->entries[i].value = value;
            return i;
        }
    }
    FPI_UNUSED(ignored);
    if (table->count >= FPI_MAX_POLYSYMS) {
        fpi_error_set(error, FPI_ERR_POLYSYM_FULL, span, "polysym table full");
        return FPI_ERR_POLYSYM_FULL;
    }
    len = 0;
    while (name[len]) {
        if (len >= FPI_IDENT_MAX) {
            fpi_error_set(error, FPI_ERR_IDENTIFIER_TOO_LONG, span, "polysym name too long");
            return FPI_ERR_IDENTIFIER_TOO_LONG;
        }
        len++;
    }
    if ((FPI_U32)len + 1UL > FPI_POLYSYM_POOL_BYTES - table->pool_used) {
        fpi_error_set(error, FPI_ERR_STRING_POOL_FULL, span, "polysym string pool full");
        return FPI_ERR_STRING_POOL_FULL;
    }
    offset = table->pool_used;
    for (i = 0; i < len; i++) table->pool[offset + (FPI_U32)i] = fpi_ascii_tolower(name[i]);
    table->pool[offset + (FPI_U32)len] = '\0';
    table->pool_used += (FPI_U32)len + 1UL;
    i = table->count++;
    table->entries[i].name_offset = offset;
    table->entries[i].hash = hash;
    table->entries[i].value = value;
    return i;
}

const char* fpi_polysym_name(const FPI_PolySym* table, int index) {
    if (!table || index < 0 || index >= table->count) return 0;
    return table->pool + table->entries[index].name_offset;
}
