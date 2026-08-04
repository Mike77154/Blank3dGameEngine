#include "API/ddsl_api.h"

#include <stdio.h>
#include <string.h>

#define TEST_ARENA_SIZE (512 * 1024)
#define LONG_CHAIN_SIZE (64 * 1024)

static unsigned char g_mem1[TEST_ARENA_SIZE];
static unsigned char g_mem2[TEST_ARENA_SIZE];
static char g_chain[LONG_CHAIN_SIZE];

static int g_failures = 0;

static void fail(const char *name, const char *detail) {
    g_failures++;
    printf("FAIL %-28s %s\n", name, detail ? detail : "");
}

static void pass(const char *name) {
    printf("PASS %s\n", name);
}

static int compile_rejected(const char *source) {
    ddsl_arena arena;
    ddsl_bc_program *bc;
    ddsl_error err;
    ddsl_arena_init(&arena, g_mem1, (size_t)sizeof(g_mem1));
    bc = NULL;
    return ddsl_compile_source_to_bytecode(&arena, source, &bc, &err) ? 0 : 1;
}

static int compile_rejected_with(const char *source, const char *needle) {
    ddsl_arena arena;
    ddsl_bc_program *bc;
    ddsl_error err;
    ddsl_arena_init(&arena, g_mem1, (size_t)sizeof(g_mem1));
    bc = NULL;
    if (ddsl_compile_source_to_bytecode(&arena, source, &bc, &err)) return 0;
    if (needle && strstr(err.message, needle) == NULL) return 0;
    return 1;
}

static void test_missing_comma(void) {
    if (!compile_rejected("if false then guarded = 1 leaked = 2\n")) {
        fail("missing comma in if", "accepted unsafe syntax");
        return;
    }
    if (!compile_rejected("a = 1 b = 2\n")) {
        fail("missing comma standalone", "accepted unsafe syntax");
        return;
    }
    pass("missing comma rejection");
}

static void test_invalid_numbers(void) {
    if (!compile_rejected("value = 1.2.3\n")) {
        fail("multiple decimal points", "accepted malformed number");
        return;
    }
    if (!compile_rejected("value = 99999999999999999999999999999999999999999999999999999999999999999\n")) {
        fail("oversized number", "accepted truncated/overflowed number");
        return;
    }
    pass("invalid number rejection");
}

static int append_text(char *dst, int cap, int *io_len, const char *text) {
    int n;
    n = (int)strlen(text);
    if (*io_len + n >= cap) return 0;
    memcpy(dst + *io_len, text, (size_t)n);
    *io_len += n;
    dst[*io_len] = '\0';
    return 1;
}

static void build_long_chain(void) {
    int i;
    int len;
    char line[128];
    len = 0;
    g_chain[0] = '\0';
    append_text(g_chain, (int)sizeof(g_chain), &len, "x = 66\n");
    for (i = 0; i < 80; ++i) {
        if (i == 0) sprintf(line, "if x = %d then hit = %d\n", i, i);
        else sprintf(line, "elif x = %d then hit = %d\n", i, i);
        append_text(g_chain, (int)sizeof(g_chain), &len, line);
    }
    append_text(g_chain, (int)sizeof(g_chain), &len, "else hit = -1\n");
    append_text(g_chain, (int)sizeof(g_chain), &len, "after = 1\n");
}

static void test_long_elif_runtime_vm(void) {
    ddsl_store runtime_store;
    ddsl_store vm_store;
    ddsl_runtime rt;
    ddsl_vm vm;
    ddsl_arena a1;
    ddsl_arena a2;
    ddsl_error err;
    const char *r_hit;
    const char *v_hit;
    const char *r_after;
    const char *v_after;

    build_long_chain();
    ddsl_store_init(&runtime_store);
    ddsl_store_init(&vm_store);
    ddsl_arena_init(&a1, g_mem1, (size_t)sizeof(g_mem1));
    ddsl_arena_init(&a2, g_mem2, (size_t)sizeof(g_mem2));
    ddsl_runtime_init(&rt, &runtime_store);
    ddsl_runtime_set_arena(&rt, &a1);
    ddsl_vm_init(&vm, &vm_store);

    if (!ddsl_exec_source(&rt, g_chain, &err)) {
        fail("long elif runtime", err.message);
        return;
    }
    if (!ddsl_vm_exec_source(&vm, &a2, g_chain, &err)) {
        fail("long elif vm", err.message);
        return;
    }

    r_hit = ddsl_store_get(&runtime_store, "hit");
    v_hit = ddsl_store_get(&vm_store, "hit");
    r_after = ddsl_store_get(&runtime_store, "after");
    v_after = ddsl_store_get(&vm_store, "after");
    if (!r_hit || !v_hit || strcmp(r_hit, "66") != 0 || strcmp(v_hit, "66") != 0 ||
        !r_after || !v_after || strcmp(r_after, "1") != 0 || strcmp(v_after, "1") != 0) {
        fail("long elif runtime/vm", "results diverged after clause 64");
        return;
    }
    pass("long elif runtime/vm parity");
}

static void test_bytecode_owns_text(void) {
    char source[128];
    ddsl_arena arena;
    ddsl_bc_program *bc;
    ddsl_store st;
    ddsl_vm vm;
    ddsl_error err;
    const char *value;

    strcpy(source, "answer = persistent_literal\n");
    ddsl_arena_init(&arena, g_mem1, (size_t)sizeof(g_mem1));
    bc = NULL;
    if (!ddsl_compile_source_to_bytecode(&arena, source, &bc, &err)) {
        fail("bytecode ownership compile", err.message);
        return;
    }
    memset(source, 'X', strlen(source));
    source[strlen("answer = persistent_literal\n")] = '\0';

    ddsl_store_init(&st);
    ddsl_vm_init(&vm, &st);
    if (!ddsl_vm_run(&vm, bc, &err)) {
        fail("bytecode ownership run", err.message);
        return;
    }
    value = ddsl_store_get(&st, "answer");
    if (!value || strcmp(value, "persistent_literal") != 0) {
        fail("bytecode ownership", "literal still depended on source buffer");
        return;
    }
    pass("bytecode owns literals");
}

static void fill_store(ddsl_store *st) {
    int i;
    char key[32];
    for (i = 0; i < DDSL_MAX_FLAGS; ++i) {
        sprintf(key, "slot_%d", i);
        ddsl_store_set(st, key, "1");
    }
}

static void test_store_errors(void) {
    ddsl_store st;
    ddsl_runtime rt;
    ddsl_vm vm;
    ddsl_arena a1;
    ddsl_arena a2;
    ddsl_error err;

    ddsl_store_init(&st);
    fill_store(&st);
    ddsl_arena_init(&a1, g_mem1, (size_t)sizeof(g_mem1));
    ddsl_runtime_init(&rt, &st);
    ddsl_runtime_set_arena(&rt, &a1);
    if (ddsl_exec_source(&rt, "overflow = 1\n", &err)) {
        fail("runtime full store", "write failure was ignored");
        return;
    }

    ddsl_store_init(&st);
    fill_store(&st);
    ddsl_arena_init(&a2, g_mem2, (size_t)sizeof(g_mem2));
    ddsl_vm_init(&vm, &st);
    if (ddsl_vm_exec_source(&vm, &a2, "overflow = 1\n", &err)) {
        fail("vm full store", "write failure was ignored");
        return;
    }
    pass("store write errors propagate");
}

static void test_utf8_transpile_escape(void) {
    static const char source[] = "text = \"\303\251A\"\n";
    char out[32768];
    ddsl_arena arena;
    ddsl_bc_program *bc;
    ddsl_transpile_opts opts;
    ddsl_error err;

    ddsl_arena_init(&arena, g_mem1, (size_t)sizeof(g_mem1));
    bc = NULL;
    if (!ddsl_compile_source_to_bytecode(&arena, source, &bc, &err)) {
        fail("utf8 transpile compile", err.message);
        return;
    }
    opts.func_name = "test_script_run";
    opts.prog_name = "test_script_prog";
    if (!ddsl_transpile_bytecode_to_c(bc, &opts, out, (int)sizeof(out), &err)) {
        fail("utf8 transpile", err.message);
        return;
    }
    if (strstr(out, "\\x") != NULL || strstr(out, "\\303\\251A") == NULL) {
        fail("utf8 transpile escape", "did not emit bounded octal escapes");
        return;
    }
    pass("utf8 transpile escaping");
}

static void test_public_version_api(void) {
    if (ddsl_api_version_major() != DDSL_STANDARD_MAJOR ||
        ddsl_api_version_minor() != DDSL_STANDARD_MINOR) {
        fail("public version API", "version functions disagree with config");
        return;
    }
    if (strcmp(DDSL_VERSION, "0.3.2") != 0 ||
        DDSL_VERSION_MAJOR != 0 || DDSL_VERSION_MINOR != 3 ||
        DDSL_VERSION_PATCH != 2) {
        fail("library version macros", "unexpected package version");
        return;
    }
    pass("public version API");
}

static void test_semantic_limits(void) {
    char source[512];
    int i;
    int pos;

    if (!compile_rejected_with("true = 1\n", "true/false") ||
        !compile_rejected_with("FALSE = 1\n", "true/false")) {
        fail("reserved assignment", "allowed assignment to boolean literal");
        return;
    }

    pos = 0;
    for (i = 0; i < DDSL_MAX_KEY_LEN; ++i) source[pos++] = 'a';
    source[pos++] = '=';
    source[pos++] = '1';
    source[pos++] = '\n';
    source[pos] = '\0';
    if (!compile_rejected_with(source, "DDSL_MAX_KEY_LEN")) {
        fail("identifier length limit", "long key was silently truncated");
        return;
    }

    pos = 0;
    source[pos++] = 'x';
    source[pos++] = '=';
    source[pos++] = '"';
    for (i = 0; i < DDSL_MAX_VALUE_LEN; ++i) source[pos++] = 'b';
    source[pos++] = '"';
    source[pos++] = '\n';
    source[pos] = '\0';
    if (!compile_rejected_with(source, "DDSL_MAX_VALUE_LEN")) {
        fail("string length limit", "long value was silently truncated");
        return;
    }

    pass("semantic fixed-buffer limits");
}

static void test_semantic_ast_invariants(void) {
    ddsl_program prog;
    ddsl_stmt stmt;
    ddsl_error err;

    memset(&prog, 0, sizeof(prog));
    memset(&stmt, 0, sizeof(stmt));
    prog.first = &stmt;
    prog.last = &stmt;
    stmt.kind = (ddsl_stmt_kind)999;
    if (ddsl_semantics_check_program(&prog, &err)) {
        fail("semantic AST invariants", "accepted unknown statement kind");
        return;
    }
    pass("semantic AST invariants");
}

int main(void) {
    test_missing_comma();
    test_invalid_numbers();
    test_long_elif_runtime_vm();
    test_bytecode_owns_text();
    test_store_errors();
    test_utf8_transpile_escape();
    test_public_version_api();
    test_semantic_limits();
    test_semantic_ast_invariants();

    if (g_failures != 0) {
        printf("%d regression test(s) failed\n", g_failures);
        return 1;
    }
    printf("All regression tests passed.\n");
    return 0;
}
