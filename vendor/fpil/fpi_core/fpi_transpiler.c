#include "fpi_transpiler.h"

static int value_needs_quotes(const char* text) {
    int i;
    if (!text) return 0;
    i = 0;
    while (text[i]) {
        if (text[i] == ',' || text[i] == ':' || text[i] == ';' ||
            text[i] == '\n' || text[i] == '\r' || text[i] == '"') return 1;
        i++;
    }
    return 0;
}

static void emit_escaped(FILE* out, const char* text) {
    int i;
    char c;
    fputc('"', out);
    i = 0;
    while (text && text[i]) {
        c = text[i++];
        if (c == '\\' || c == '"') { fputc('\\', out); fputc(c, out); }
        else if (c == '\n') fputs("\\n", out);
        else if (c == '\r') fputs("\\r", out);
        else if (c == '\t') fputs("\\t", out);
        else fputc(c, out);
    }
    fputc('"', out);
}

static void emit_value(FILE* out, const FPI_AST* ast, const FPI_ValueRef* ref) {
    const char* text;
    if (!ref || !ref->has) return;
    text = fpi_ast_value_text(ast, ref);
    fputc('=', out);
    if (value_needs_quotes(text)) emit_escaped(out, text);
    else fputs(text, out);
}

int fpi_transpile_fpi(FILE* out, const FPI_AST* ast, const FPI_Registry* registry) {
    int i;
    int j;
    const FPI_Rule* rule;
    const FPI_Term* term;
    const char* name;
    if (!out || !ast || !registry) return FPI_ERR_ARGUMENT;
    for (i = 0; i < ast->rule_count; i++) {
        rule = &ast->rules[i];
        fputc(':', out);
        for (j = 0; j < (int)rule->condition_count; j++) {
            if (j) fputc(',', out);
            term = fpi_ast_condition_at(ast, rule, j);
            name = term ? fpi_registry_cond_name(registry, term->symbol_id) : 0;
            fputs(name ? name : "?", out);
            if (term) emit_value(out, ast, &term->value);
        }
        fputc(':', out);
        for (j = 0; j < (int)rule->action_count; j++) {
            if (j) fputc(',', out);
            term = fpi_ast_action_at(ast, rule, j);
            name = term ? fpi_registry_act_name(registry, term->symbol_id) : 0;
            fputs(name ? name : "?", out);
            if (term) emit_value(out, ast, &term->value);
        }
        fputc('\n', out);
    }
    return FPI_OK;
}

static void json_string(FILE* out, const char* text) {
    int i;
    char c;
    fputc('"', out);
    i = 0;
    while (text && text[i]) {
        c = text[i++];
        if (c == '"' || c == '\\') { fputc('\\', out); fputc(c, out); }
        else if (c == '\n') fputs("\\n", out);
        else if (c == '\r') fputs("\\r", out);
        else if (c == '\t') fputs("\\t", out);
        else fputc(c, out);
    }
    fputc('"', out);
}

static void json_term(FILE* out, const FPI_AST* ast, const FPI_Registry* registry, const FPI_Term* term, int name_space) {
    const char* name;
    name = name_space == FPI_NS_CONDITION ?
        fpi_registry_cond_name(registry, term->symbol_id) :
        fpi_registry_act_name(registry, term->symbol_id);
    fputs("{\"name\":", out);
    json_string(out, name ? name : "?");
    fputs(",\"has_value\":", out);
    fputs(term->value.has ? "true" : "false", out);
    if (term->value.has) {
        fputs(",\"value\":", out);
        json_string(out, fpi_ast_value_text(ast, &term->value));
        if (term->value.kind == FPI_VALUE_FIXED) fprintf(out, ",\"fixed_raw\":%ld", term->value.fixed);
    }
    fputc('}', out);
}

int fpi_transpile_json(FILE* out, const FPI_AST* ast, const FPI_Registry* registry) {
    int i;
    int j;
    const FPI_Rule* rule;
    const FPI_Term* term;
    if (!out || !ast || !registry) return FPI_ERR_ARGUMENT;
    fputs("{\"rules\":[", out);
    for (i = 0; i < ast->rule_count; i++) {
        if (i) fputc(',', out);
        rule = &ast->rules[i];
        fputs("{\"conditions\":[", out);
        for (j = 0; j < (int)rule->condition_count; j++) {
            if (j) fputc(',', out);
            term = fpi_ast_condition_at(ast, rule, j);
            if (term) json_term(out, ast, registry, term, FPI_NS_CONDITION);
        }
        fputs("],\"actions\":[", out);
        for (j = 0; j < (int)rule->action_count; j++) {
            if (j) fputc(',', out);
            term = fpi_ast_action_at(ast, rule, j);
            if (term) json_term(out, ast, registry, term, FPI_NS_ACTION);
        }
        fputs("]}", out);
    }
    fputs("]}\n", out);
    return FPI_OK;
}

int fpi_emit_normalized(FILE* out, const AST_Script* ast, const FPI_SymTab* symtab) {
    return fpi_transpile_fpi(out, ast, symtab);
}

int fpi_emit_json(FILE* out, const AST_Script* ast, const FPI_SymTab* symtab) {
    return fpi_transpile_json(out, ast, symtab);
}
