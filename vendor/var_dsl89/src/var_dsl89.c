#include "var_dsl89.h"

#include <ctype.h>
#include <string.h>

static const char *vdsl89_skip_ws(const char *p)
{
    while (*p != '\0' && isspace((unsigned char)*p)) ++p;
    return p;
}

static int vdsl89_is_name_start(int c)
{
    return isalpha((unsigned char)c) || c == '_';
}

static int vdsl89_is_name_char(int c)
{
    return isalnum((unsigned char)c) || c == '_';
}

static int vdsl89_copy_path_name(const char **pp, char *out)
{
    const char *p;
    size_t n;
    int need_start;

    p = *pp;
    n = 0U;
    need_start = 1;
    while (*p != '\0') {
        int c;
        c = (unsigned char)*p;
        if (need_start) {
            if (!vdsl89_is_name_start(c)) break;
            need_start = 0;
        } else if (c == '.') {
            if (n + 1U >= VDSL89_NAME_MAX) return VDSL89_ERR_NAME_TOO_LONG;
            out[n++] = *p++;
            need_start = 1;
            continue;
        } else if (!vdsl89_is_name_char(c)) {
            break;
        }
        if (n + 1U >= VDSL89_NAME_MAX) return VDSL89_ERR_NAME_TOO_LONG;
        out[n++] = *p++;
    }
    if (n == 0U || need_start) return VDSL89_ERR_SYNTAX;
    out[n] = '\0';
    *pp = p;
    return VDSL89_OK;
}

static int vdsl89_parse_scoped_name(const char **pp, vdsl89_scope default_scope,
                                    vdsl89_scope *out_scope, char *out_name)
{
    const char *p;
    int r;
    char temp[VDSL89_NAME_MAX];

    p = *pp;
    *out_scope = default_scope;
    r = vdsl89_copy_path_name(&p, temp);
    if (r != VDSL89_OK) return r;

    if (strncmp(temp, "global.", 7U) == 0) {
        if (temp[7] == '\0') return VDSL89_ERR_SYNTAX;
        *out_scope = VDSL89_SCOPE_GLOBAL;
        strcpy(out_name, temp + 7);
    } else if (strncmp(temp, "self.", 5U) == 0) {
        if (temp[5] == '\0') return VDSL89_ERR_SYNTAX;
        *out_scope = VDSL89_SCOPE_INSTANCE;
        strcpy(out_name, temp + 5);
    } else {
        strcpy(out_name, temp);
    }
    *pp = p;
    return VDSL89_OK;
}

void vdsl89_value_none(vdsl89_value *v)
{
    if (v == NULL) return;
    v->type = VDSL89_VALUE_NONE;
    v->fixed_q16 = 0L;
    v->boolean = 0;
    v->string_value[0] = '\0';
}

void vdsl89_value_fixed_raw(vdsl89_value *v, vdsl89_i32 raw_q16)
{
    if (v == NULL) return;
    vdsl89_value_none(v);
    v->type = VDSL89_VALUE_FIXED;
    v->fixed_q16 = raw_q16;
}

void vdsl89_value_bool(vdsl89_value *v, int boolean)
{
    if (v == NULL) return;
    vdsl89_value_none(v);
    v->type = VDSL89_VALUE_BOOL;
    v->boolean = boolean ? 1 : 0;
}

int vdsl89_value_string(vdsl89_value *v, const char *text)
{
    size_t n;
    if (v == NULL || text == NULL) return VDSL89_ERR_NULL;
    n = strlen(text);
    if (n >= VDSL89_STRING_MAX) return VDSL89_ERR_STRING_TOO_LONG;
    vdsl89_value_none(v);
    v->type = VDSL89_VALUE_STRING;
    memcpy(v->string_value, text, n + 1U);
    return VDSL89_OK;
}

int vdsl89_fixed_from_text(const char *text, vdsl89_i32 *out_q16)
{
    const char *p;
    int negative;
    vdsl89_i32 whole;
    vdsl89_i32 frac;
    vdsl89_i32 scale;
    vdsl89_i32 raw;
    int digits;

    if (text == NULL || out_q16 == NULL) return VDSL89_ERR_NULL;
    p = text;
    negative = 0;
    whole = 0L;
    frac = 0L;
    scale = 1L;
    digits = 0;

    if (*p == '-') { negative = 1; ++p; }
    else if (*p == '+') ++p;

    while (isdigit((unsigned char)*p)) {
        whole = whole * 10L + (vdsl89_i32)(*p - '0');
        ++p;
        digits = 1;
    }
    if (*p == '.') {
        ++p;
        while (isdigit((unsigned char)*p)) {
            if (scale < 1000000L) {
                frac = frac * 10L + (vdsl89_i32)(*p - '0');
                scale *= 10L;
            }
            ++p;
            digits = 1;
        }
    }
    if (!digits || *p != '\0') return VDSL89_ERR_BAD_NUMBER;
    raw = whole * 65536L;
    if (scale > 1L) raw += (frac * 65536L + scale / 2L) / scale;
    if (negative) raw = -raw;
    *out_q16 = raw;
    return VDSL89_OK;
}

static int vdsl89_parse_string(const char **pp, vdsl89_value *out)
{
    const char *p;
    char temp[VDSL89_STRING_MAX];
    size_t n;
    char quote;

    p = *pp;
    quote = *p++;
    n = 0U;
    while (*p != '\0' && *p != quote) {
        char c;
        c = *p++;
        if (c == '\\' && *p != '\0') {
            c = *p++;
            if (c == 'n') c = '\n';
            else if (c == 't') c = '\t';
        }
        if (n + 1U >= VDSL89_STRING_MAX) return VDSL89_ERR_STRING_TOO_LONG;
        temp[n++] = c;
    }
    if (*p != quote) return VDSL89_ERR_SYNTAX;
    ++p;
    temp[n] = '\0';
    *pp = p;
    return vdsl89_value_string(out, temp);
}

static int vdsl89_token_is_number(const char *token)
{
    const char *p;
    int digit;
    p = token;
    digit = 0;
    if (*p == '+' || *p == '-') ++p;
    while (isdigit((unsigned char)*p)) { digit = 1; ++p; }
    if (*p == '.') {
        ++p;
        while (isdigit((unsigned char)*p)) { digit = 1; ++p; }
    }
    return digit && *p == '\0';
}

static int vdsl89_parse_rhs(const char **pp, vdsl89_command *out)
{
    const char *p;
    char token[VDSL89_NAME_MAX];
    size_t n;
    vdsl89_i32 q;
    int r;

    p = vdsl89_skip_ws(*pp);
    out->rhs_kind = VDSL89_RHS_LITERAL;
    out->rhs_scope = VDSL89_SCOPE_RESOLVE;
    out->rhs_name[0] = '\0';

    if (*p == '"' || *p == '\'') {
        r = vdsl89_parse_string(&p, &out->value);
        if (r != VDSL89_OK) return r;
        *pp = p;
        return VDSL89_OK;
    }

    n = 0U;
    while (*p != '\0' && *p != ';' && !isspace((unsigned char)*p)) {
        if (n + 1U >= sizeof(token)) return VDSL89_ERR_NAME_TOO_LONG;
        token[n++] = *p++;
    }
    token[n] = '\0';
    if (n == 0U) return VDSL89_ERR_SYNTAX;

    if (strcmp(token, "true") == 0) {
        vdsl89_value_bool(&out->value, 1);
    } else if (strcmp(token, "false") == 0) {
        vdsl89_value_bool(&out->value, 0);
    } else if (vdsl89_token_is_number(token)) {
        r = vdsl89_fixed_from_text(token, &q);
        if (r != VDSL89_OK) return r;
        vdsl89_value_fixed_raw(&out->value, q);
    } else {
        const char *qptr;
        qptr = token;
        out->rhs_kind = VDSL89_RHS_VARIABLE;
        r = vdsl89_parse_scoped_name(&qptr, VDSL89_SCOPE_RESOLVE,
                                     &out->rhs_scope, out->rhs_name);
        if (r != VDSL89_OK || *qptr != '\0') return VDSL89_ERR_SYNTAX;
        vdsl89_value_none(&out->value);
    }
    *pp = p;
    return VDSL89_OK;
}

int vdsl89_parse_line(const char *line, vdsl89_command *out_command)
{
    const char *p;
    int r;

    if (line == NULL || out_command == NULL) return VDSL89_ERR_NULL;
    memset(out_command, 0, sizeof(*out_command));
    out_command->scope = VDSL89_SCOPE_INSTANCE;
    out_command->rhs_scope = VDSL89_SCOPE_RESOLVE;
    out_command->opcode = VDSL89_OP_SET;
    out_command->rhs_kind = VDSL89_RHS_LITERAL;

    p = vdsl89_skip_ws(line);
    if (*p == '\0' || *p == '#') return VDSL89_ERR_SYNTAX;
    if (p[0] == '/' && p[1] == '/') return VDSL89_ERR_SYNTAX;

    if (strncmp(p, "var", 3U) == 0 &&
        (p[3] == '\0' || isspace((unsigned char)p[3]))) {
        p += 3;
        p = vdsl89_skip_ws(p);
        out_command->scope = VDSL89_SCOPE_LOCAL;
        out_command->declares_local = 1;
        r = vdsl89_copy_path_name(&p, out_command->name);
    } else {
        r = vdsl89_parse_scoped_name(&p, VDSL89_SCOPE_INSTANCE,
                                     &out_command->scope,
                                     out_command->name);
    }
    if (r != VDSL89_OK) return r;

    p = vdsl89_skip_ws(p);
    if (p[0] == '+' && p[1] == '=') {
        out_command->opcode = VDSL89_OP_ADD;
        p += 2;
    } else if (p[0] == '-' && p[1] == '=') {
        out_command->opcode = VDSL89_OP_SUB;
        p += 2;
    } else if (p[0] == '=') {
        out_command->opcode = VDSL89_OP_SET;
        ++p;
    } else {
        return VDSL89_ERR_SYNTAX;
    }

    r = vdsl89_parse_rhs(&p, out_command);
    if (r != VDSL89_OK) return r;
    p = vdsl89_skip_ws(p);
    if (*p == ';') { ++p; p = vdsl89_skip_ws(p); }
    if (*p != '\0' && *p != '#' && !(p[0] == '/' && p[1] == '/'))
        return VDSL89_ERR_SYNTAX;
    return VDSL89_OK;
}

int vdsl89_execute_line(const char *line, const vdsl89_provider *provider)
{
    vdsl89_command command;
    int r;
    if (provider == NULL || provider->emit == NULL) return VDSL89_ERR_NULL;
    r = vdsl89_parse_line(line, &command);
    if (r != VDSL89_OK) return r;
    if (!provider->emit(provider->user, &command)) return VDSL89_ERR_PROVIDER;
    return VDSL89_OK;
}

static int vdsl89_statement_has_text(const char *s)
{
    while (*s != '\0') {
        if (!isspace((unsigned char)*s)) return 1;
        ++s;
    }
    return 0;
}

int vdsl89_execute_buffer(const char *text, const vdsl89_provider *provider,
                          int *out_statement_count)
{
    char statement[VDSL89_STATEMENT_MAX];
    size_t n;
    const char *p;
    char quote;
    int count;
    int r;

    if (out_statement_count) *out_statement_count = 0;
    if (text == NULL || provider == NULL || provider->emit == NULL)
        return VDSL89_ERR_NULL;

    p = text;
    n = 0U;
    quote = '\0';
    count = 0;
    while (1) {
        char c;
        c = *p;
        if (quote != '\0') {
            if (c == '\0') return VDSL89_ERR_SYNTAX;
            if (n + 1U >= sizeof(statement)) return VDSL89_ERR_STATEMENT_TOO_LONG;
            statement[n++] = c;
            if (c == '\\' && p[1] != '\0') {
                ++p;
                if (n + 1U >= sizeof(statement)) return VDSL89_ERR_STATEMENT_TOO_LONG;
                statement[n++] = *p;
            } else if (c == quote) {
                quote = '\0';
            }
            ++p;
            continue;
        }

        if (c == '"' || c == '\'') {
            quote = c;
            if (n + 1U >= sizeof(statement)) return VDSL89_ERR_STATEMENT_TOO_LONG;
            statement[n++] = c;
            ++p;
            continue;
        }

        if (c == '#' || (c == '/' && p[1] == '/')) {
            while (*p != '\0' && *p != '\n') ++p;
            c = *p;
        }

        if (c == ';' || c == '\n' || c == '\0') {
            statement[n] = '\0';
            if (vdsl89_statement_has_text(statement)) {
                r = vdsl89_execute_line(statement, provider);
                if (r != VDSL89_OK) return r;
                ++count;
            }
            n = 0U;
            if (c == '\0') break;
            ++p;
            continue;
        }
        if (c == '\r') { ++p; continue; }
        if (n + 1U >= sizeof(statement)) return VDSL89_ERR_STATEMENT_TOO_LONG;
        statement[n++] = c;
        ++p;
    }
    if (out_statement_count) *out_statement_count = count;
    return VDSL89_OK;
}

const char *vdsl89_result_string(int result)
{
    switch (result) {
        case VDSL89_OK: return "ok";
        case VDSL89_ERR_NULL: return "null argument";
        case VDSL89_ERR_SYNTAX: return "syntax error";
        case VDSL89_ERR_NAME_TOO_LONG: return "name too long";
        case VDSL89_ERR_STRING_TOO_LONG: return "string too long";
        case VDSL89_ERR_BAD_NUMBER: return "bad number";
        case VDSL89_ERR_PROVIDER: return "provider rejected command";
        case VDSL89_ERR_STATEMENT_TOO_LONG: return "statement too long";
        default: return "unknown error";
    }
}
