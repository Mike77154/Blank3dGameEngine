#include <limits.h>
#include <string.h>

#include "rpyl.h"
#include "rpyl_config.h"
#include "rpyl_extlang.h"

static char g_order[8][RPYL_EXTLANG_MAX_LANG];
static int g_order_count = 0;
static int g_last_priority = 0;
static char g_last_code[256];

static int ok_callback(RpylContext* ctx, void* userdata, const char* lang, int priority, const char* code, const char* filename, int start_line) {
    (void)ctx;
    (void)userdata;
    (void)filename;
    (void)start_line;
    if (g_order_count < 8) {
        strncpy(g_order[g_order_count], lang, sizeof(g_order[g_order_count]) - 1u);
        g_order[g_order_count][sizeof(g_order[g_order_count]) - 1u] = '\0';
        g_order_count++;
    }
    g_last_priority = priority;
    strncpy(g_last_code, code, sizeof(g_last_code) - 1u);
    g_last_code[sizeof(g_last_code) - 1u] = '\0';
    return 1;
}

static int fail_callback(RpylContext* ctx, void* userdata, const char* lang, int priority, const char* code, const char* filename, int start_line) {
    (void)ctx;
    (void)userdata;
    (void)lang;
    (void)priority;
    (void)code;
    (void)filename;
    (void)start_line;
    return 0;
}

static int detailed_fail_callback(RpylContext* ctx, void* userdata, const char* lang, int priority, const char* code, const char* filename, int start_line) {
    (void)userdata;
    (void)lang;
    (void)priority;
    (void)code;
    rpyl_set_error(ctx, "plugin", filename, start_line, 2, "plugin supplied detail");
    return 0;
}

static int error_contains(RpylContext* ctx, const char* module, const char* filename, int line, const char* needle) {
    const RpylErrorInfo* error;

    error = rpyl_get_last_error(ctx);
    if (!error) return 0;
    if (module && strcmp(error->module, module) != 0) return 0;
    if (filename && strcmp(error->filename, filename) != 0) return 0;
    if (line > 0 && error->line != line) return 0;
    if (needle && !strstr(error->message, needle)) return 0;
    return 1;
}

static RpylContext* make_context(unsigned char* ctx_mem, size_t ctx_sz, unsigned char* work_mem, size_t work_sz) {
    RpylContext* ctx;

    ctx = rpyl_init(ctx_mem, ctx_sz, work_mem, work_sz);
    if (ctx) {
        rpyl_set_label_keyword(ctx, "label");
        rpyl_set_start_block(ctx, "start");
    }
    return ctx;
}

static int test_priority_and_dedent(void) {
    static unsigned char ctx_mem[65536];
    static unsigned char work_mem[65536];
    RpylContext* ctx;
    RpylLangHost* host;
    const char* script;

    ctx = make_context(ctx_mem, sizeof(ctx_mem), work_mem, sizeof(work_mem));
    host = rpyl_langhost_create();
    if (!ctx || !host) return 10;
    if (!rpyl_langhost_register(host, "alpha", ok_callback, (void*)0)) return 11;
    if (!rpyl_langhost_register(host, "beta", ok_callback, (void*)0)) return 12;
    if (!rpyl_langhost_register(host, "gamma", ok_callback, (void*)0)) return 13;

    g_order_count = 0;
    g_last_code[0] = '\0';
    script = "init 10 alpha:\n"
             "    first\n"
             "init -5 beta:\n"
             "    second\n"
             "init 10 gamma:\n"
             "        nested\n"
             "label start:\n"
             "    return\n";
    if (!rpyl_load_buffer_extlang(ctx, host, script, strlen(script), "priority.rpyl")) return 14;
    if (g_order_count != 3) return 15;
    if (strcmp(g_order[0], "beta") != 0) return 16;
    if (strcmp(g_order[1], "alpha") != 0) return 17;
    if (strcmp(g_order[2], "gamma") != 0) return 18;
    if (g_last_priority != 10) return 19;
    if (strcmp(g_last_code, "    nested\n") != 0) return 20;

    rpyl_langhost_destroy(host);
    rpyl_destroy(ctx);
    return 0;
}

static int test_missing_plugin(void) {
    static unsigned char ctx_mem[65536];
    static unsigned char work_mem[65536];
    RpylContext* ctx;
    RpylLangHost* host;
    const char* script;

    ctx = make_context(ctx_mem, sizeof(ctx_mem), work_mem, sizeof(work_mem));
    host = rpyl_langhost_create();
    if (!ctx || !host) return 30;
    script = "init absent:\n"
             "    body\n"
             "label start:\n"
             "    return\n";
    if (rpyl_load_buffer_extlang(ctx, host, script, strlen(script), "missing.rpyl")) return 31;
    if (!error_contains(ctx, "extlang", "missing.rpyl", 1, "no plugin registered")) return 32;
    if (!error_contains(ctx, "extlang", "missing.rpyl", 1, "absent")) return 33;
    rpyl_langhost_destroy(host);
    rpyl_destroy(ctx);
    return 0;
}

static int test_callback_errors(void) {
    static unsigned char ctx_mem[65536];
    static unsigned char work_mem[65536];
    RpylContext* ctx;
    RpylLangHost* host;
    const char* script;

    script = "init alpha:\n"
             "    body\n"
             "label start:\n"
             "    return\n";

    ctx = make_context(ctx_mem, sizeof(ctx_mem), work_mem, sizeof(work_mem));
    host = rpyl_langhost_create();
    if (!ctx || !host) return 40;
    if (!rpyl_langhost_register(host, "alpha", fail_callback, (void*)0)) return 41;
    if (rpyl_load_buffer_extlang(ctx, host, script, strlen(script), "callback.rpyl")) return 42;
    if (!error_contains(ctx, "extlang", "callback.rpyl", 1, "callback failed")) return 43;
    rpyl_langhost_destroy(host);
    rpyl_destroy(ctx);

    ctx = make_context(ctx_mem, sizeof(ctx_mem), work_mem, sizeof(work_mem));
    host = rpyl_langhost_create();
    if (!ctx || !host) return 44;
    if (!rpyl_langhost_register(host, "alpha", detailed_fail_callback, (void*)0)) return 45;
    if (rpyl_load_buffer_extlang(ctx, host, script, strlen(script), "detail.rpyl")) return 46;
    if (!error_contains(ctx, "plugin", "detail.rpyl", 1, "plugin supplied detail")) return 47;
    rpyl_langhost_destroy(host);
    rpyl_destroy(ctx);
    return 0;
}

static int test_priority_overflow(void) {
    static unsigned char ctx_mem[65536];
    static unsigned char work_mem[65536];
    RpylContext* ctx;
    RpylLangHost* host;
    const char* script;

    ctx = make_context(ctx_mem, sizeof(ctx_mem), work_mem, sizeof(work_mem));
    host = rpyl_langhost_create();
    if (!ctx || !host) return 50;
    script = "init 999999999999999999999999 alpha:\n"
             "    body\n";
    if (rpyl_load_buffer_extlang(ctx, host, script, strlen(script), "overflow.rpyl")) return 51;
    if (!error_contains(ctx, "extlang", "overflow.rpyl", 1, "outside the C int range")) return 52;
    rpyl_langhost_destroy(host);
    rpyl_destroy(ctx);
    return 0;
}

static int test_long_line(void) {
    static unsigned char ctx_mem[65536];
    static unsigned char work_mem[65536];
    static char script[RPYL_EXTLANG_MAX_LINE + 512u];
    RpylContext* ctx;
    RpylLangHost* host;
    size_t pos;
    size_t i;

    ctx = make_context(ctx_mem, sizeof(ctx_mem), work_mem, sizeof(work_mem));
    host = rpyl_langhost_create();
    if (!ctx || !host) return 60;
    if (!rpyl_langhost_register(host, "alpha", ok_callback, (void*)0)) return 61;

    pos = 0u;
    memcpy(script + pos, "init alpha:\n    ", 16u);
    pos += 16u;
    for (i = 0u; i < (size_t)RPYL_EXTLANG_MAX_LINE; i++) script[pos++] = 'x';
    script[pos++] = '\n';
    script[pos] = '\0';
    if (rpyl_load_buffer_extlang(ctx, host, script, pos, "longline.rpyl")) return 62;
    if (!error_contains(ctx, "extlang", "longline.rpyl", 2, "input was not truncated")) return 63;
    rpyl_langhost_destroy(host);
    rpyl_destroy(ctx);
    return 0;
}

static int test_indentation_guards(void) {
    static unsigned char ctx_mem[65536];
    static unsigned char work_mem[65536];
    RpylContext* ctx;
    RpylLangHost* host;
    const char* short_indent;
    const char* tab_indent;
    const char* header_indent;

    short_indent = "init alpha:\n"
                   "  body\n";
    tab_indent = "init alpha:\n"
                 "\tbody\n";
    header_indent = "    init alpha:\n"
                    "        body\n";

    ctx = make_context(ctx_mem, sizeof(ctx_mem), work_mem, sizeof(work_mem));
    host = rpyl_langhost_create();
    if (!ctx || !host) return 70;
    if (!rpyl_langhost_register(host, "alpha", ok_callback, (void*)0)) return 71;
    if (rpyl_load_buffer_extlang(ctx, host, short_indent, strlen(short_indent), "indent.rpyl")) return 72;
    if (!error_contains(ctx, "extlang", "indent.rpyl", 2, "at least")) return 73;
    if (rpyl_load_buffer_extlang(ctx, host, tab_indent, strlen(tab_indent), "tab.rpyl")) return 74;
    if (!error_contains(ctx, "extlang", "tab.rpyl", 2, "tabs are not allowed")) return 75;
    if (rpyl_load_buffer_extlang(ctx, host, header_indent, strlen(header_indent), "header.rpyl")) return 76;
    if (!error_contains(ctx, "extlang", "header.rpyl", 1, "column 1")) return 77;
    rpyl_langhost_destroy(host);
    rpyl_destroy(ctx);
    return 0;
}

static int test_init_limits(void) {
    static unsigned char ctx_mem[65536];
    static unsigned char work_mem[65536];
    static char many_blocks[8192];
    static char large_block[RPYL_EXTLANG_MAX_INIT_CODE + RPYL_EXTLANG_MAX_LINE];
    RpylContext* ctx;
    RpylLangHost* host;
    size_t pos;
    size_t i;
    int n;

    ctx = make_context(ctx_mem, sizeof(ctx_mem), work_mem, sizeof(work_mem));
    host = rpyl_langhost_create();
    if (!ctx || !host) return 80;
    if (!rpyl_langhost_register(host, "alpha", ok_callback, (void*)0)) return 81;

    pos = 0u;
    for (n = 0; n < RPYL_EXTLANG_MAX_INITS + 1; n++) {
        const char* block;
        size_t block_len;
        block = "init alpha:\n    x\n";
        block_len = strlen(block);
        if (pos + block_len + 1u >= sizeof(many_blocks)) return 82;
        memcpy(many_blocks + pos, block, block_len);
        pos += block_len;
    }
    many_blocks[pos] = '\0';
    if (rpyl_load_buffer_extlang(ctx, host, many_blocks, pos, "many.rpyl")) return 83;
    if (!error_contains(ctx, "extlang", "many.rpyl", 0, "too many init blocks")) return 84;

    pos = 0u;
    memcpy(large_block + pos, "init alpha:\n", 12u);
    pos += 12u;
    for (n = 0; n < 3; n++) {
        memcpy(large_block + pos, "    ", 4u);
        pos += 4u;
        for (i = 0u; i < 3000u; i++) large_block[pos++] = 'z';
        large_block[pos++] = '\n';
    }
    large_block[pos] = '\0';
    if (rpyl_load_buffer_extlang(ctx, host, large_block, pos, "largeblock.rpyl")) return 85;
    if (!error_contains(ctx, "extlang", "largeblock.rpyl", 0, "MAX_INIT_CODE")) return 86;

    rpyl_langhost_destroy(host);
    rpyl_destroy(ctx);
    return 0;
}

static int test_registration_and_parser_filename(void) {
    static unsigned char ctx_mem[65536];
    static unsigned char work_mem[64];
    char long_name[RPYL_EXTLANG_MAX_LANG + 16];
    RpylContext* ctx;
    RpylLangHost* host;
    size_t i;
    const char* bad_core;

    ctx = make_context(ctx_mem, sizeof(ctx_mem), work_mem, sizeof(work_mem));
    host = rpyl_langhost_create();
    if (!ctx || !host) return 90;
    for (i = 0u; i + 1u < sizeof(long_name); i++) long_name[i] = 'a';
    long_name[sizeof(long_name) - 1u] = '\0';
    if (rpyl_langhost_register(host, long_name, ok_callback, (void*)0)) return 91;
    if (rpyl_langhost_register(host, "bad name", ok_callback, (void*)0)) return 92;

    bad_core = "label start\n"
               "    return\n";
    if (rpyl_load_buffer_extlang(ctx, host, bad_core, strlen(bad_core), "actual_name.rpyl")) return 93;
    if (!error_contains(ctx, "parser", "actual_name.rpyl", 1, "parse failed")) return 94;
    rpyl_langhost_destroy(host);
    rpyl_destroy(ctx);
    return 0;
}

int main(void) {
    int result;

    result = test_priority_and_dedent();
    if (result) return result;
    result = test_missing_plugin();
    if (result) return result;
    result = test_callback_errors();
    if (result) return result;
    result = test_priority_overflow();
    if (result) return result;
    result = test_long_line();
    if (result) return result;
    result = test_indentation_guards();
    if (result) return result;
    result = test_init_limits();
    if (result) return result;
    result = test_registration_and_parser_filename();
    if (result) return result;
    return 0;
}
