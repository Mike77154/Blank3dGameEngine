#include <stdio.h>
#include <string.h>

#include "rpyl.h"
#include "rpyl_extlang.h"
#include "rpyl_polysym.h"

static int cli_to_int(const char* text) {
    int sign;
    int value;
    const char* p;
    if (!text) return 0;
    p = text;
    sign = 1;
    value = 0;
    if (*p == '-') {
        sign = -1;
        p++;
    }
    while (*p >= '0' && *p <= '9') {
        value = value * 10 + (int)(*p - '0');
        p++;
    }
    return value * sign;
}

static int cli_streq(const char* a, const char* b) {
    if (!a || !b) return 0;
    return strcmp(a, b) == 0;
}

static int cli_read_file(const char* path, char* out, size_t out_sz, size_t* out_len) {
    FILE* f;
    size_t n;
    if (!path || !out || out_sz == 0u || !out_len) return 0;
    f = fopen(path, "rb");
    if (!f) return 0;
    n = fread(out, 1u, out_sz - 1u, f);
    if (ferror(f)) {
        fclose(f);
        return 0;
    }
    if (!feof(f)) {
        fclose(f);
        return 0;
    }
    fclose(f);
    out[n] = 0;
    *out_len = n;
    return 1;
}

static void cli_print_args(const char* tag, const char** args, int argc) {
    int i;
    printf("[%s]", tag ? tag : "cmd");
    for (i = 0; i < argc; i++) {
        printf(" %s", args[i] ? args[i] : "");
    }
    printf("\n");
}

static void cmd_log(RpylContext* ctx, const char** args, int argc) {
    (void)ctx;
    cli_print_args("Log", args, argc);
}

static void cmd_say(RpylContext* ctx, const char** args, int argc) {
    (void)ctx;
    if (argc < 1) return;
    printf("[Say] %s\n", args[0]);
}

static void cmd_show(RpylContext* ctx, const char** args, int argc) {
    (void)ctx;
    cli_print_args("Show", args, argc);
}

static void cmd_draw(RpylContext* ctx, const char** args, int argc) {
    (void)ctx;
    if (argc < 3) return;
    printf("[Draw] x=%d y=%d size=%d\n", cli_to_int(args[0]), cli_to_int(args[1]), cli_to_int(args[2]));
}

static void cmd_scene(RpylContext* ctx, const char** args, int argc) {
    (void)ctx;
    cli_print_args("Scene", args, argc);
}

static void cmd_hide(RpylContext* ctx, const char** args, int argc) {
    (void)ctx;
    cli_print_args("Hide", args, argc);
}

static void cmd_with(RpylContext* ctx, const char** args, int argc) {
    (void)ctx;
    cli_print_args("With", args, argc);
}

static int demo_symbol(RpylContext* ctx, void* userdata, const char* symbol, const char** args, int argc) {
    (void)ctx;
    (void)userdata;
    printf("[PolySym] %s", symbol ? symbol : "(null)");
    if (argc > 0) {
        int i;
        printf("(");
        for (i = 0; i < argc; i++) {
            if (i) printf(", ");
            printf("%s", args[i] ? args[i] : "");
        }
        printf(")");
    }
    printf("\n");
    return 1;
}

static int word_is_export(const char* p) {
    if (!p) return 0;
    while (*p == ' ' || *p == '\t') p++;
    if (strncmp(p, "export", 6) != 0) return 0;
    p += 6;
    return (*p == ' ' || *p == '\t') ? 1 : 0;
}

static int read_word_after_export(const char* p, char* out, size_t out_sz) {
    size_t i;
    if (!p || !out || out_sz == 0u) return 0;
    out[0] = 0;
    while (*p == ' ' || *p == '\t') p++;
    if (strncmp(p, "export", 6) != 0) return 0;
    p += 6;
    while (*p == ' ' || *p == '\t') p++;
    i = 0u;
    while ((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9') || *p == '_' || *p == '.') {
        if (i + 1u < out_sz) out[i++] = *p;
        p++;
    }
    out[i] = 0;
    return out[0] ? 1 : 0;
}

static int demo_init_lang(RpylContext* ctx, void* userdata, const char* lang, int priority, const char* code, const char* filename, int start_line) {
    RpylSymbolHost* sh;
    const char* p;
    char line[256];
    char sym[128];
    size_t i;
    (void)priority;
    (void)filename;
    (void)start_line;
    sh = (RpylSymbolHost*)userdata;
    printf("[Init:%s]\n", lang ? lang : "unknown");
    p = code ? code : "";
    while (*p) {
        i = 0u;
        while (*p && *p != '\n' && i + 1u < sizeof(line)) line[i++] = *p++;
        if (*p == '\n') p++;
        line[i] = 0;
        if (word_is_export(line)) {
            if (read_word_after_export(line, sym, sizeof(sym))) {
                if (rpyl_symbolhost_export(sh, sym, demo_symbol, (void*)0)) {
                    printf("[Init:%s] export %s\n", lang ? lang : "unknown", sym);
                }
            }
        }
    }
    (void)ctx;
    return 1;
}

static void register_demo_commands(RpylContext* rpyl) {
    rpyl_register_command(rpyl, "log", cmd_log);
    rpyl_register_command(rpyl, "say", cmd_say);
    rpyl_register_command(rpyl, "scene", cmd_scene);
    rpyl_register_command(rpyl, "show", cmd_show);
    rpyl_register_command(rpyl, "hide", cmd_hide);
    rpyl_register_command(rpyl, "with", cmd_with);
    rpyl_register_command(rpyl, "Show", cmd_show);
    rpyl_register_command(rpyl, "Draw", cmd_draw);
}

static void print_usage(void) {
    printf("rpyl_cli [options] <script> [entry] [frames]\n");
    printf("Options:\n");
    printf("  --label-keyword <kw>   keyword for labels, e.g. task\n");
    printf("  --start-block <name>   default entry block, e.g. boot\n");
    printf("  --frames <n>           run frames/iterations\n");
    printf("  --extlang              enable init <lang>: preprocessing (default on)\n");
    printf("  --polysym              enable $ symbol(...) dispatch (default on)\n");
    printf("  --no-extlang           disable init <lang>: preprocessing\n");
    printf("  --no-polysym           disable $ symbol(...) dispatch\n");
    printf("  --help                 show this message\n");
}

int main(int argc, char** argv) {
    const char* script;
    const char* entry;
    const char* label_kw;
    int frames;
    int use_extlang;
    int use_polysym;
    int positional;
    int i;
    int loaded;
    RpylContext* rpyl;
    RpylLangHost* lang_host;
    RpylSymbolHost* sym_host;
    static char script_buffer[RPYL_FILE_MAX_BYTES];
    size_t script_len;

    script = "script.rpy";
    entry = (const char*)0;
    label_kw = (const char*)0;
    frames = 3;
    use_extlang = 1;
    use_polysym = 1;
    positional = 0;
    lang_host = (RpylLangHost*)0;
    sym_host = (RpylSymbolHost*)0;
    script_len = 0u;

    for (i = 1; i < argc; i++) {
        if (cli_streq(argv[i], "--help") || cli_streq(argv[i], "-h")) {
            print_usage();
            return 0;
        } else if (cli_streq(argv[i], "--label-keyword") && i + 1 < argc) {
            label_kw = argv[++i];
        } else if (cli_streq(argv[i], "--start-block") && i + 1 < argc) {
            entry = argv[++i];
        } else if (cli_streq(argv[i], "--frames") && i + 1 < argc) {
            frames = cli_to_int(argv[++i]);
        } else if (cli_streq(argv[i], "--extlang")) {
            use_extlang = 1;
        } else if (cli_streq(argv[i], "--polysym")) {
            use_polysym = 1;
        } else if (cli_streq(argv[i], "--no-extlang")) {
            use_extlang = 0;
        } else if (cli_streq(argv[i], "--no-polysym")) {
            use_polysym = 0;
        } else if (argv[i][0] == '-' && argv[i][1] == '-') {
            fprintf(stderr, "unknown option: %s\n", argv[i]);
            return 1;
        } else {
            if (positional == 0) script = argv[i];
            else if (positional == 1 && !entry) entry = argv[i];
            else if (positional == 2) frames = cli_to_int(argv[i]);
            positional++;
        }
    }

    if (!entry) entry = "start";
    if (frames <= 0) frames = 1;

    rpyl = rpyl_create();
    if (!rpyl) {
        fprintf(stderr, "failed to create rpyl\n");
        return 1;
    }

    if (label_kw) rpyl_set_label_keyword(rpyl, label_kw);
    if (entry) rpyl_set_start_block(rpyl, entry);
    register_demo_commands(rpyl);

    if (use_extlang) {
        lang_host = rpyl_langhost_create();
        if (!lang_host) {
            fprintf(stderr, "failed to create lang host\n");
            rpyl_destroy(rpyl);
            return 1;
        }
    }
    if (use_polysym) {
        sym_host = rpyl_symbolhost_create();
        if (!sym_host) {
            fprintf(stderr, "failed to create symbol host\n");
            if (lang_host) rpyl_langhost_destroy(lang_host);
            rpyl_destroy(rpyl);
            return 1;
        }
    }
    if (lang_host) {
        void* u;
        u = sym_host ? (void*)sym_host : (void*)0;
        (void)rpyl_langhost_register(lang_host, "anydsl", demo_init_lang, u);
        (void)rpyl_langhost_register(lang_host, "cualquierdsl", demo_init_lang, u);
    }

    loaded = 0;
    if (use_extlang || use_polysym) {
        if (!cli_read_file(script, script_buffer, sizeof(script_buffer), &script_len)) {
            const RpylErrorInfo* err;
            err = rpyl_get_last_error(rpyl);
            fprintf(stderr, "failed to read script into host buffer: %s\n", script);
            if (err && err->message[0]) {
                fprintf(stderr, "%s:%d:%d [%s] %s\n", err->filename, err->line, err->column, err->module, err->message);
            }
        } else if (use_extlang && use_polysym && lang_host && sym_host) {
            loaded = rpyl_load_buffer_extlang_symbols(rpyl, lang_host, sym_host, script_buffer, script_len, script);
        } else if (use_extlang && lang_host) {
            loaded = rpyl_load_buffer_extlang(rpyl, lang_host, script_buffer, script_len, script);
        } else {
            loaded = rpyl_load_buffer(rpyl, script_buffer, script_len);
        }
    } else {
        loaded = rpyl_load_path_buffered(rpyl, script);
    }

    if (!loaded) {
        const RpylErrorInfo* err;
        err = rpyl_get_last_error(rpyl);
        fprintf(stderr, "failed to load script: %s\n", script);
        if (err && err->message[0]) {
            fprintf(stderr, "%s:%d:%d [%s] %s\n", err->filename, err->line, err->column, err->module, err->message);
        }
        if (sym_host) rpyl_symbolhost_destroy(sym_host);
        if (lang_host) rpyl_langhost_destroy(lang_host);
        rpyl_destroy(rpyl);
        return 1;
    }

    for (i = 0; i < frames; i++) {
        int rc;
        printf("--- frame %d ---\n", i);
        rc = rpyl_run(rpyl, entry);
        if (rc == RPYL_EXEC_ERROR) {
            const RpylErrorInfo* err;
            err = rpyl_get_last_error(rpyl);
            if (err && err->message[0]) {
                fprintf(stderr, "%s:%d:%d [%s] %s\n", err->filename, err->line, err->column, err->module, err->message);
            }
            break;
        }
    }

    if (sym_host) rpyl_symbolhost_destroy(sym_host);
    if (lang_host) rpyl_langhost_destroy(lang_host);
    rpyl_destroy(rpyl);
    return 0;
}
