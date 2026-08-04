#include <string.h>

#include "rpyl.h"
#include "rpyl_builtins.h"
#include "rpyl_compiler.h"
#include "rpyl_extlang.h"
#include "rpyl_fixed.h"
#include "rpyl_io.h"
#include "rpyl_parser.h"
#include "rpyl_polysym.h"
#include "rpyl_registry.h"
#include "rpyl_semantics.h"
#include "rpyl_store.h"
#include "rpyl_symtab.h"
#include "rpyl_vm.h"
#include "rpyl_warper.h"

static int g_hit_count = 0;
static int g_registry_count = 0;


static int g_init_seen = 0;
static int g_sym_seen = 0;
static char g_init_code[128];

static int init_any(RpylContext* ctx, void* userdata, const char* lang, int priority, const char* code, const char* filename, int start_line) {
    (void)ctx;
    (void)userdata;
    (void)lang;
    (void)priority;
    (void)filename;
    (void)start_line;
    g_init_seen++;
    strncpy(g_init_code, code, sizeof(g_init_code) - 1u);
    g_init_code[sizeof(g_init_code) - 1u] = '\0';
    return 1;
}

static int sym_any(RpylContext* ctx, void* userdata, const char* symbol, const char** args, int argc) {
    (void)ctx;
    (void)userdata;
    if (strcmp(symbol, "foo") == 0 && argc == 1 && strcmp(args[0], "Monika") == 0) g_sym_seen++;
    return 1;
}

static void hit_cmd(RpylContext* ctx, const char** args, int argc) {
    (void)ctx;
    if (argc == 1 && strcmp(args[0], "ok") == 0) g_hit_count++;
}

static void registry_cmd(void* user, const char** args, int argc) {
    int* value;
    (void)args;
    (void)argc;
    value = (int*)user;
    if (value) (*value)++;
    g_registry_count++;
}

static int test_collections(void) {
    RpylSymtab syms;
    RpylStore store;
    RpylRegistry registry;
    unsigned long value;
    void* user;
    RpylRegistryFn fn;
    int local_count;

    rpyl_symtab_init(&syms);
    if (!rpyl_symtab_put_kind(&syms, "boot", 7UL, RPYL_SYM_LABEL, 3UL)) return 10;
    if (!rpyl_symtab_get_kind(&syms, "boot", RPYL_SYM_LABEL, &value)) return 11;
    if (value != 7UL) return 12;
    if (!rpyl_symtab_remove(&syms, "boot")) return 13;
    if (rpyl_symtab_contains(&syms, "boot")) return 14;

    rpyl_store_init(&store);
    if (!rpyl_store_set_ex(&store, "who", "Monika", 1UL)) return 20;
    if (strcmp(rpyl_store_get(&store, "who"), "Monika") != 0) return 21;
    if (!rpyl_store_set(&store, "who", "Aoi")) return 22;
    if (strcmp(rpyl_store_get(&store, "who"), "Aoi") != 0) return 23;
    if (!rpyl_store_remove(&store, "who")) return 24;
    if (rpyl_store_get(&store, "who") != 0) return 25;

    local_count = 0;
    rpyl_registry_init(&registry);
    if (!rpyl_registry_add_ex(&registry, "tick", registry_cmd, &local_count, 9UL)) return 30;
    user = 0;
    fn = rpyl_registry_find(&registry, "tick", &user);
    if (!fn || user != (void*)&local_count) return 31;
    if (!rpyl_registry_dispatch(&registry, "tick", (const char**)0, 0)) return 32;
    if (local_count != 1 || g_registry_count != 1) return 33;
    if (!rpyl_registry_remove(&registry, "tick")) return 34;
    if (rpyl_registry_find(&registry, "tick", &user) != 0) return 35;
    return 0;
}


static int test_fixed32_and_alignment(void) {
    static unsigned char arena_mem[64];
    RpylArena* arena;
    rpyl_fx ten;
    rpyl_fx half;
    rpyl_fx value;
    rpyl_fx parsed;
    int ok;

    if (sizeof(rpyl_u32) != 4u) return 36;
    if (sizeof(rpyl_i32) != 4u) return 37;
    if (sizeof(rpyl_fx) != 4u) return 38;

    ten = rpyl_fx_from_int(10L);
    half = (rpyl_fx)(RPYL_FX_ONE / 2L);
    value = rpyl_fx_mul(ten, half);
    if (rpyl_fx_to_int(value) != 5L) return 39;

    value = rpyl_fx_mul(rpyl_fx_from_int(30000L), rpyl_fx_from_int(1L));
    if (rpyl_fx_to_int(value) != 30000L) return 45;

    value = rpyl_fx_div(ten, rpyl_fx_from_int(2L), &ok);
    if (!ok || rpyl_fx_to_int(value) != 5L) return 46;
    value = rpyl_fx_div(rpyl_fx_from_int(-10L), rpyl_fx_from_int(4L), &ok);
    if (!ok || rpyl_fx_to_int(value) != -2L) return 47;
    value = rpyl_fx_div(ten, 0, &ok);
    if (ok || value != 0) return 48;

    parsed = rpyl_fx_from_decimal_text("12.5", &ok);
    if (!ok || parsed != rpyl_fx_add(rpyl_fx_from_int(12L), half)) return 49;
    parsed = rpyl_fx_from_decimal_text("32768", &ok);
    if (!ok || parsed != RPYL_I32_MAX) return 61;
    parsed = rpyl_fx_from_decimal_text("-32768", &ok);
    if (!ok || parsed != RPYL_I32_MIN) return 62;

    if (rpyl_fx_mul(RPYL_I32_MAX, rpyl_fx_from_int(2L)) != RPYL_I32_MAX) return 63;
    if (rpyl_fx_mul(RPYL_I32_MIN, rpyl_fx_from_int(2L)) != RPYL_I32_MIN) return 64;
    if (rpyl_fx_add(RPYL_I32_MAX, 1) != RPYL_I32_MAX) return 65;
    if (rpyl_fx_sub(RPYL_I32_MIN, 1) != RPYL_I32_MIN) return 66;

    arena = rpyl_arena_create_with_buffer(arena_mem, sizeof(arena_mem));
    if (!arena) return 67;
    if (!rpyl_arena_alloc(arena, 1u, 1u)) return 68;
    if (!rpyl_arena_alloc(arena, 1u, 3u)) return 69;
    if (rpyl_arena_used(arena) != 4u) return 74;
    rpyl_arena_destroy(arena);
    return 0;
}

static int test_io_and_warper(void) {
    RpylIoBuffer buffer;
    RpylIoSource src;
    char line[16];
    rpyl_fx a;
    rpyl_fx b;
    rpyl_fx half;
    rpyl_fx out;

    rpyl_io_buffer_init(&buffer, "abc\ndef", 7u);
    rpyl_io_source_init(&src, &buffer, rpyl_io_buffer_getc, rpyl_io_buffer_ungetc);
    if (rpyl_io_source_read_line(&src, line, sizeof(line)) != 4u) return 40;
    if (strcmp(line, "abc\n") != 0) return 41;
    if (src.line != 2UL) return 42;

    a = rpyl_fx_from_int(0L);
    b = rpyl_fx_from_int(10L);
    half = (rpyl_fx)(RPYL_FX_ONE / 2L);
    out = rpyl_warper_apply(a, b, half, "linear");
    if (rpyl_fx_to_int(out) != 5L) return 43;
    out = rpyl_warper_clamp01((rpyl_fx)(2L * RPYL_FX_ONE));
    if (out != (rpyl_fx)RPYL_FX_ONE) return 44;
    return 0;
}

static int test_language_pipeline(void) {
    static unsigned char arena_mem[65536];
    RpylArena* arena;
    AstNode* root;
    RpylSemanticsReport sem;
    RpylBytecode bc;
    RpylCompilerResult result;
    RpylVmCursor cursor;
    rpyl_u32 opcode;
    int saw_command;
    const char* script;

    script = "define who = Monika\n"
             "task boot:\n"
             "    setg target done\n"
             "    hit ok\n"
             "    call sub\n"
             "    jump $target\n"
             "task sub:\n"
             "    return\n"
             "task done:\n"
             "    return\n";

    arena = rpyl_arena_create_with_buffer(arena_mem, sizeof(arena_mem));
    if (!arena) return 50;
    root = rpyl_parse_buffer_ex_with_label(script, strlen(script), arena, "task");
    if (!root) return 51;
    if (!rpyl_semantics_validate_ex(root, &sem)) return 52;
    if (sem.block_count != 3) return 53;
    if (sem.define_count != 1) return 54;

    rpyl_bytecode_clear(&bc);
    if (!rpyl_compiler_compile_ast_ex(&bc, root, (const RpylCompilerOptions*)0, &result)) return 55;
    if (!result.success || !result.had_ir) return 56;
    if (result.ir.label_count != 3u) return 57;
    if (!rpyl_vm_validate(&bc)) return 58;
    if (!rpyl_vm_begin(&cursor, &bc, "boot")) return 59;
    saw_command = 0;
    while (rpyl_vm_step_opcode(&cursor, &opcode, (rpyl_u32*)0)) {
        if (opcode == (rpyl_u32)RPYL_BC_OP_CMD) saw_command = 1;
    }
    if (!saw_command) return 60;

    rpyl_arena_destroy(arena);
    return 0;
}


static int test_extlang_polysym(void) {
    static unsigned char ctx_mem[65536];
    static unsigned char work_mem[65536];
    RpylContext* ctx;
    RpylLangHost* lh;
    RpylSymbolHost* sh;
    const char* script;

    g_init_seen = 0;
    g_sym_seen = 0;
    g_hit_count = 0;
    g_init_code[0] = '\0';

    ctx = rpyl_init(ctx_mem, sizeof(ctx_mem), work_mem, sizeof(work_mem));
    lh = rpyl_langhost_create();
    sh = rpyl_symbolhost_create();
    if (!ctx || !lh || !sh) return 80;
    rpyl_set_label_keyword(ctx, "task");
    rpyl_set_start_block(ctx, "boot");
    rpyl_register_command(ctx, "hit", hit_cmd);
    if (!rpyl_langhost_register(lh, "cualquierdsl", init_any, (void*)0)) return 81;
    if (!rpyl_symbolhost_export(sh, "foo", sym_any, (void*)0)) return 82;

    script = "init cualquierdsl:\n"
             "    export foo\n"
             "    $ kept_raw(yes)\n"
             "define who = Monika\n"
             "task boot:\n"
             "    $ foo($who)\n"
             "    hit ok\n";
    if (!rpyl_load_buffer_extlang_symbols(ctx, lh, sh, script, strlen(script), "memory.rpyl")) return 83;
    if (strstr(g_init_code, "$ kept_raw") == (char*)0) return 84;
    if (!rpyl_compile(ctx)) return 85;
    if (rpyl_run_compiled(ctx, (const char*)0) != RPYL_EXEC_DONE) return 86;
    if (g_init_seen != 1) return 87;
    if (g_sym_seen != 1) return 88;
    if (g_hit_count != 1) return 89;
    rpyl_symbolhost_destroy(sh);
    rpyl_langhost_destroy(lh);
    rpyl_destroy(ctx);
    return 0;
}

static int test_public_api(void) {
    static unsigned char ctx_mem[65536];
    static unsigned char work_mem[65536];
    RpylContext* ctx;
    const char* script;

    g_hit_count = 0;
    ctx = rpyl_init(ctx_mem, sizeof(ctx_mem), work_mem, sizeof(work_mem));
    if (!ctx) return 70;
    rpyl_set_label_keyword(ctx, "task");
    rpyl_set_start_block(ctx, "boot");
    rpyl_register_command(ctx, "hit", hit_cmd);
    script = "task boot:\n"
             "    hit ok\n";
    if (!rpyl_load_buffer(ctx, script, strlen(script))) return 71;
    if (rpyl_run(ctx, (const char*)0) != RPYL_EXEC_DONE) return 72;
    if (g_hit_count != 1) return 73;
    rpyl_destroy(ctx);
    return 0;
}


static int test_validation_guards(void) {
    static unsigned char ctx_mem[65536];
    static unsigned char work_mem[65536];
    static unsigned char byte_buffer[16384];
    RpylContext* ctx;
    const RpylBytecode* compiled;
    RpylBytecode bad;
    const char* invalid_script;
    const char* valid_script;
    size_t needed;
    size_t pos;
    size_t i;

    ctx = rpyl_init(ctx_mem, sizeof(ctx_mem), work_mem, sizeof(work_mem));
    if (!ctx) return 90;
    rpyl_set_label_keyword(ctx, "task");
    rpyl_set_start_block(ctx, "boot");

    invalid_script = "task boot:\n"
                     "    jump missing\n";
    if (!rpyl_load_buffer(ctx, invalid_script, strlen(invalid_script))) return 91;
    if (rpyl_compile(ctx)) return 92;
    if (!rpyl_get_last_error(ctx)) return 93;

    valid_script = "task boot:\n"
                   "    hit ok\n";
    if (!rpyl_load_buffer(ctx, valid_script, strlen(valid_script))) return 94;
    if (!rpyl_compile(ctx)) return 95;
    compiled = rpyl_get_bytecode(ctx);
    if (!compiled) return 96;

    rpyl_bytecode_clear(&bad);
    if (!rpyl_bytecode_copy_into(&bad, compiled)) return 97;
    if (bad.code_count < 2u) return 98;
    bad.code[1] = (rpyl_u32)bad.string_count;
    if (rpyl_use_bytecode(ctx, &bad)) return 99;

    if (!rpyl_load_buffer(ctx, valid_script, strlen(valid_script))) return 100;
    if (!rpyl_compile(ctx)) return 101;
    compiled = rpyl_get_bytecode(ctx);
    if (!compiled) return 102;
    needed = rpyl_save_bytecode_buffer(ctx, (void*)0, 0u);
    if (needed == 0u || needed > sizeof(byte_buffer)) return 103;
    if (rpyl_save_bytecode_buffer(ctx, byte_buffer, sizeof(byte_buffer)) != needed) return 104;

    pos = 24u;
    for (i = 0u; i < compiled->string_count; i++) {
        pos += 4u + strlen(compiled->strings[i]);
    }
    if (pos + 4u > needed) return 105;
    byte_buffer[pos + 0u] = 0xFFu;
    byte_buffer[pos + 1u] = 0xFFu;
    byte_buffer[pos + 2u] = 0xFFu;
    byte_buffer[pos + 3u] = 0x7Fu;
    if (rpyl_load_bytecode_buffer(ctx, byte_buffer, needed)) return 106;

    rpyl_destroy(ctx);
    return 0;
}

int main(void) {
    int r;
    if (rpyl_builtins_count() < 10) return 1;
    if (!rpyl_builtins_find("show")) return 2;
    if (!rpyl_builtins_is_renpyish("jump")) return 3;

    r = test_collections();
    if (r != 0) return r;
    r = test_fixed32_and_alignment();
    if (r != 0) return r;
    r = test_io_and_warper();
    if (r != 0) return r;
    r = test_language_pipeline();
    if (r != 0) return r;
    r = test_public_api();
    if (r != 0) return r;
    r = test_extlang_polysym();
    if (r != 0) return r;
    r = test_validation_guards();
    if (r != 0) return r;
    return 0;
}
