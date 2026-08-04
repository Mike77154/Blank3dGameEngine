#include "value_resolver_fpi.h"
#include "var_system_fpi.h"

static int is_space_char(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

int fpi_split_2tokens_ws(const char* text, char* token1, int token1_capacity, char* token2, int token2_capacity) {
    int i;
    int j;
    if (!text || !token1 || token1_capacity <= 0 || !token2 || token2_capacity <= 0) return 0;
    token1[0] = '\0';
    token2[0] = '\0';
    while (*text && is_space_char(*text)) text++;
    if (!*text) return 0;
    i = 0;
    while (*text && !is_space_char(*text)) {
        if (i >= token1_capacity - 1) return 0;
        token1[i++] = *text++;
    }
    token1[i] = '\0';
    while (*text && is_space_char(*text)) text++;
    j = 0;
    while (*text && !is_space_char(*text)) {
        if (j >= token2_capacity - 1) return 0;
        token2[j++] = *text++;
    }
    token2[j] = '\0';
    while (*text && is_space_char(*text)) text++;
    return *text == '\0';
}

void fpi_value_resolver_init(FPI_ValueResolver* resolver, FPI_Context* context, struct FPI_VarSystem* vars, FPI_InternalVarFn internal_fn, void* internal_user) {
    if (!resolver) return;
    resolver->context = context;
    resolver->vars = vars;
    resolver->internal_fn = internal_fn;
    resolver->internal_user = internal_user;
}

int fpi_resolve_fixed_token(const FPI_ValueResolver* resolver, const char* token, FPI_Fixed* out_value) {
    FPI_Value value;
    if (!resolver || !token || !*token || !out_value) return 0;
    if (token[0] == '%' && resolver->vars)
        return fpi_var_try_get(resolver->vars, token + 1, out_value);
    if (token[0] == '$' && resolver->internal_fn)
        return resolver->internal_fn(resolver->internal_user, token + 1, out_value);
    if (fpi_fixed_parse(token, out_value) == FPI_OK) return 1;
    fpi_value_clear(&value);
    value.has = 1;
    value.kind = FPI_VALUE_TEXT;
    value.s = token;
    while (token[value.len]) value.len++;
    return resolver->context ? fpi_resolve_value_fixed(resolver->context, &value, out_value) : 0;
}

int fpi_resolve_int_token(const FPI_ValueResolver* resolver, const char* token, int* out_value) {
    FPI_Fixed fixed;
    if (!out_value || !fpi_resolve_fixed_token(resolver, token, &fixed)) return 0;
    *out_value = (int)fpi_fixed_to_int(fixed);
    return 1;
}

int fpi_resolve_fixed_value(const FPI_ValueResolver* resolver, const FPI_Value* value, FPI_Fixed* out_value) {
    if (!resolver || !value || !value->has || !out_value) return 0;
    if (value->kind == FPI_VALUE_FIXED) { *out_value = value->fixed; return 1; }
    return fpi_resolve_fixed_token(resolver, value->s, out_value);
}

int fpi_resolve_int_value(const FPI_ValueResolver* resolver, const FPI_Value* value, int* out_value) {
    FPI_Fixed fixed;
    if (!out_value || !fpi_resolve_fixed_value(resolver, value, &fixed)) return 0;
    *out_value = (int)fpi_fixed_to_int(fixed);
    return 1;
}
