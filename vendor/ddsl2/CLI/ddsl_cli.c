#include "common/ddsl.h"

#include <stdio.h>
#include <string.h>

#ifndef DDSL_CLI_MAX_SOURCE
#define DDSL_CLI_MAX_SOURCE (256 * 1024)
#endif

#ifndef DDSL_CLI_ARENA_SIZE
#define DDSL_CLI_ARENA_SIZE (256 * 1024)
#endif

static char g_src_buf[DDSL_CLI_MAX_SOURCE + 1];
static unsigned char g_arena_mem[DDSL_CLI_ARENA_SIZE];

static void print_usage(const char *argv0) {
    printf("Uso:\n");
    printf("  %s run <script.ddsl> [-Dkey=value] [--trace] [--dump]\n", argv0);
    printf("  %s tokens <script.ddsl>\n", argv0);
    printf("  %s ast <script.ddsl>\n", argv0);
    printf("  %s ir <script.ddsl>\n", argv0);
    printf("  %s bc <script.ddsl>\n", argv0);
    printf("  %s transpile <script.ddsl>\n", argv0);
    printf("\nNotas:\n");
    printf("  - Identificadores pueden incluir '-' (ej: time-tick).\n");
    printf("  - Para resta usa espacios: a - b (si escribes a-b se lexea como IDENT).\n");
}

static const char *read_file_all(const char *path) {
    FILE *f;
    size_t r;
    int ch;

    f = fopen(path, "rb");
    if (!f) return NULL;

    r = fread(g_src_buf, 1, (size_t)DDSL_CLI_MAX_SOURCE, f);
    ch = fgetc(f);
    fclose(f);

    if (ch != EOF) {
        /* archivo demasiado grande */
        return NULL;
    }

    g_src_buf[r] = '\0';
    return g_src_buf;
}

static int emit_trace(void *user, const char *key_norm, ddsl_value value) {
    (void)user;
    {
        char vbuf[128];
        ddsl_value_to_cstr(value, vbuf, (int)sizeof(vbuf));
        printf("[emit] %s = %s\n", key_norm, vbuf);
    }
    return 1;
}

static int dump_store_iter(const char *key, const char *value, void *user) {
    (void)user;
    printf("%s = %s\n", key, value);
    return 1;
}

static void indent(int n) {
    while (n-- > 0) putchar(' ');
}

static void print_sv(ddsl_strview sv) {
    char buf[128];
    ddsl_sv_to_cstr(sv, buf, (int)sizeof(buf));
    fputs(buf, stdout);
}

static void print_fixed(ddsl_fixed v) {
    char buf[64];
    ddsl_fixed_to_cstr(v, buf, (int)sizeof(buf));
    fputs(buf, stdout);
}

static void print_expr(ddsl_expr *e, int depth) {
    if (!e) {
        indent(depth);
        printf("<null expr>\n");
        return;
    }

    indent(depth);
    switch (e->kind) {
        case DDSL_EXPR_NUMBER:
            printf("NUM ");
            print_fixed(e->as.number);
            printf("\n");
            break;
        case DDSL_EXPR_STRING:
            printf("STR \"");
            print_sv(e->as.string);
            printf("\"\n");
            break;
        case DDSL_EXPR_IDENT:
            printf("IDENT ");
            print_sv(e->as.ident);
            printf("\n");
            break;
        case DDSL_EXPR_UNARY:
            printf("UNARY op=%d\n", (int)e->as.unary.op);
            print_expr(e->as.unary.rhs, depth + 2);
            break;
        case DDSL_EXPR_BINARY:
            printf("BINARY op=%d\n", (int)e->as.binary.op);
            print_expr(e->as.binary.lhs, depth + 2);
            print_expr(e->as.binary.rhs, depth + 2);
            break;
        default:
            printf("<expr kind %d>\n", (int)e->kind);
            break;
    }
}

static void print_actions(ddsl_action *a, int depth) {
    while (a) {
        indent(depth);
        if (a->kind == DDSL_ACTION_FLAG) {
            printf("ACTION FLAG ");
            print_sv(a->name);
            printf("\n");
        } else {
            printf("ACTION SET ");
            print_sv(a->name);
            printf("\n");
            print_expr(a->value, depth + 2);
        }
        a = a->next;
    }
}

static void print_program(ddsl_program *prog) {
    ddsl_stmt *s;
    int idx;

    idx = 0;
    for (s = prog->first; s; s = s->next) {
        printf("STMT #%d kind=%d (line %d)\n", idx++, (int)s->kind, s->at.line);
        if (s->kind == DDSL_STMT_ACTIONS) {
            print_actions(s->as.actions.actions, 2);
        } else if (s->kind == DDSL_STMT_EXPR) {
            print_expr(s->as.expr.expr, 2);
        } else if (s->kind == DDSL_STMT_IF) {
            ddsl_if_clause *c;
            int cidx;

            cidx = 0;
            for (c = s->as.ifs.clauses; c; c = c->next) {
                indent(2);
                printf("CLAUSE #%d %s (line %d)\n", cidx++, c->is_else ? "ELSE" : "IF/ELIF", c->at.line);
                if (!c->is_else) {
                    indent(4);
                    printf("COND\n");
                    print_expr(c->cond, 6);
                }
                indent(4);
                printf("ACTIONS\n");
                print_actions(c->actions, 6);
            }
        }
        printf("\n");
    }
}

static int cmd_tokens(const char *path) {
    const char *src;
    ddsl_token_vec toks;
    ddsl_error err;
    int ok;
    int i;

    src = read_file_all(path);
    if (!src) {
        fprintf(stderr, "No pude leer: %s\n", path);
        return 1;
    }

    ok = ddsl_lex_all(src, &toks, &err);
    if (!ok) {
        fprintf(stderr, "Lexer error: %s (line %d col %d)\n", err.message, err.line, err.col);
        return 1;
    }

    for (i = 0; i < toks.count; ++i) {
        ddsl_token t = toks.items[i];
        char lex[64];
        ddsl_sv_to_cstr(t.lexeme, lex, (int)sizeof(lex));
        printf("%4d:%-3d  %-8s  '%s'\n", t.line, t.col, ddsl_tok_kind_name(t.kind), lex);
    }

    ddsl_tokens_reset(&toks);
    return 0;
}

static int cmd_ast(const char *path) {
    const char *src;
    ddsl_token_vec toks;
    ddsl_program *prog;
    ddsl_error err;
    int ok;
    ddsl_arena arena;

    src = read_file_all(path);
    if (!src) {
        fprintf(stderr, "No pude leer: %s\n", path);
        return 1;
    }

    ok = ddsl_lex_all(src, &toks, &err);
    if (!ok) {
        fprintf(stderr, "Lexer error: %s (line %d col %d)\n", err.message, err.line, err.col);
        return 1;
    }

    prog = NULL;
    ddsl_arena_init(&arena, g_arena_mem, (size_t)sizeof(g_arena_mem));
    ok = ddsl_parse_program(&toks, &arena, &prog, &err);
    if (!ok) {
        fprintf(stderr, "Parser error: %s (line %d col %d)\n", err.message, err.line, err.col);
        ddsl_tokens_reset(&toks);
        return 1;
    }

    print_program(prog);

    ddsl_tokens_reset(&toks);
    return 0;
}

static int cmd_ir(const char *path) {
    const char *src;
    ddsl_token_vec toks;
    ddsl_program *prog;
    ddsl_ir_program *ir;
    ddsl_error err;
    int ok;
    int i;
    ddsl_arena arena;

    src = read_file_all(path);
    if (!src) {
        fprintf(stderr, "No pude leer: %s\n", path);
        return 1;
    }

    ok = ddsl_lex_all(src, &toks, &err);
    if (!ok) {
        fprintf(stderr, "Lexer error: %s (line %d col %d)\n", err.message, err.line, err.col);
        return 1;
    }

    prog = NULL;
    ddsl_arena_init(&arena, g_arena_mem, (size_t)sizeof(g_arena_mem));
    ok = ddsl_parse_program(&toks, &arena, &prog, &err);
    if (!ok) {
        fprintf(stderr, "Parser error: %s (line %d col %d)\n", err.message, err.line, err.col);
        ddsl_tokens_reset(&toks);
        return 1;
    }

    ir = NULL;
    ok = ddsl_ir_build(&arena, prog, &ir, &err);
    if (!ok) {
        fprintf(stderr, "IR error: %s (line %d col %d)\n", err.message, err.line, err.col);
        ddsl_tokens_reset(&toks);
        return 1;
    }

    for (i = 0; i < ir->count; ++i) {
        ddsl_ir_inst in;
        char svbuf[64];
        in = ir->code[i];
        svbuf[0] = '\0';
        ddsl_sv_to_cstr(in.sv, svbuf, (int)sizeof(svbuf));
        {
            char nbuf[64];
            ddsl_fixed_to_cstr(in.num, nbuf, (int)sizeof(nbuf));
            printf("%04d  %-20s  a=%d  num=%s  sv='%s'\n",
                   i, ddsl_ir_op_name(in.op), in.a, nbuf, svbuf);
        }
    }

    ddsl_tokens_reset(&toks);
    return 0;
}

static int cmd_bc(const char *path) {
    const char *src;
    ddsl_arena arena;
    ddsl_bc_program *bc;
    ddsl_error err;
    int ok;
    int i;

    src = read_file_all(path);
    if (!src) {
        fprintf(stderr, "No pude leer: %s\n", path);
        return 1;
    }

    ddsl_arena_init(&arena, g_arena_mem, (size_t)sizeof(g_arena_mem));
    bc = NULL;
    ok = ddsl_compile_source_to_bytecode(&arena, src, &bc, &err);
    if (!ok) {
        fprintf(stderr, "Compile error: %s (line %d col %d)\n", err.message, err.line, err.col);
        return 1;
    }

    for (i = 0; i < bc->count; ++i) {
        ddsl_bc_ins in;
        char svbuf[64];
        in = bc->code[i];
        svbuf[0] = '\0';
        ddsl_sv_to_cstr(in.sv, svbuf, (int)sizeof(svbuf));
        {
            char nbuf[64];
            ddsl_fixed_to_cstr(in.num, nbuf, (int)sizeof(nbuf));
            printf("%04d  %-20s  a=%d  num=%s  sv='%s'  key='%s'\n",
                   i, ddsl_bc_op_name(in.op), in.a, nbuf, svbuf, in.key ? in.key : "");
        }
    }

    return 0;
}

static int cmd_transpile(const char *path) {
    const char *src;
    ddsl_arena arena;
    ddsl_bc_program *bc;
    ddsl_error err;
    int ok;
    ddsl_transpile_opts opt;

    /* buffer grande para la salida */
    static char out[128 * 1024];

    src = read_file_all(path);
    if (!src) {
        fprintf(stderr, "No pude leer: %s\n", path);
        return 1;
    }

    ddsl_arena_init(&arena, g_arena_mem, (size_t)sizeof(g_arena_mem));
    bc = NULL;
    ok = ddsl_compile_source_to_bytecode(&arena, src, &bc, &err);
    if (!ok) {
        fprintf(stderr, "Compile error: %s (line %d col %d)\n", err.message, err.line, err.col);
        return 1;
    }

    opt.func_name = "ddsl2_script_run";
    opt.prog_name = "ddsl2_script_prog";

    ok = ddsl_transpile_bytecode_to_c(bc, &opt, out, (int)sizeof(out), &err);
    if (!ok) {
        fprintf(stderr, "Transpile error: %s\n", err.message);
        return 1;
    }

    fputs(out, stdout);
    return 0;
}

static int cmd_run(int argc, char **argv) {
    const char *path;
    int trace;
    int dump;
    int i;

    ddsl_store st;
    ddsl_runtime rt;
    ddsl_error err;
    const char *src;
    ddsl_arena arena;

    if (argc < 1) return 1;
    path = argv[0];

    trace = 0;
    dump = 0;

    ddsl_store_init(&st);

    /* parse flags */
    for (i = 1; i < argc; ++i) {
        const char *a = argv[i];
        if (strcmp(a, "--trace") == 0) trace = 1;
        else if (strcmp(a, "--dump") == 0) dump = 1;
        else if (strncmp(a, "-D", 2) == 0) {
            const char *kv = a + 2;
            const char *eq = strchr(kv, '=');
            if (eq) {
                char k[64];
                char v[128];
                int klen = (int)(eq - kv);
                if (klen > (int)sizeof(k) - 1) klen = (int)sizeof(k) - 1;
                memcpy(k, kv, (size_t)klen);
                k[klen] = '\0';
                strncpy(v, eq + 1, sizeof(v) - 1);
                v[sizeof(v) - 1] = '\0';
                ddsl_store_set(&st, k, v);
            }
        }
    }

    src = read_file_all(path);
    if (!src) {
        fprintf(stderr, "No pude leer: %s\n", path);
        return 1;
    }

    ddsl_runtime_init(&rt, &st);
    ddsl_arena_init(&arena, g_arena_mem, (size_t)sizeof(g_arena_mem));
    ddsl_runtime_set_arena(&rt, &arena);
    if (trace) ddsl_runtime_set_emit(&rt, emit_trace, NULL);

    if (!ddsl_exec_source(&rt, src, &err)) {
        fprintf(stderr, "Runtime error: %s (line %d col %d)\n", err.message, err.line, err.col);
        return 1;
    }

    if (dump) {
        printf("\n--- STORE ---\n");
        ddsl_store_foreach(&st, dump_store_iter, NULL);
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "run") == 0) {
        if (argc < 3) { print_usage(argv[0]); return 1; }
        return cmd_run(argc - 2, argv + 2);
    }

    if (strcmp(argv[1], "tokens") == 0) {
        if (argc < 3) { print_usage(argv[0]); return 1; }
        return cmd_tokens(argv[2]);
    }

    if (strcmp(argv[1], "ast") == 0) {
        if (argc < 3) { print_usage(argv[0]); return 1; }
        return cmd_ast(argv[2]);
    }

    if (strcmp(argv[1], "ir") == 0) {
        if (argc < 3) { print_usage(argv[0]); return 1; }
        return cmd_ir(argv[2]);
    }

    if (strcmp(argv[1], "bc") == 0) {
        if (argc < 3) { print_usage(argv[0]); return 1; }
        return cmd_bc(argv[2]);
    }

    if (strcmp(argv[1], "transpile") == 0) {
        if (argc < 3) { print_usage(argv[0]); return 1; }
        return cmd_transpile(argv[2]);
    }

    print_usage(argv[0]);
    return 1;
}
