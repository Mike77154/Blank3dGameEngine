#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fpi.h"
#include "fpi_builtins.h"
#include "fpi_bytecode.h"
#include "fpi_lexer.h"
#include "fpi_transpiler.h"
#include "activation_fpi.h"
#include "time_binding_fpi.h"
#include "var_system_fpi.h"

#define CLI_SOURCE_BYTES FPI_STATIC_FILE_BYTES
#define CLI_PROGRAM_BYTES (4UL * 1024UL * 1024UL)
#define CLI_MAX_DEFINES 64

typedef struct CLI_Define {
    char name[FPI_IDENT_MAX + 1];
    char value[64];
} CLI_Define;

static FPI_Context cli_context;
static char cli_source[CLI_SOURCE_BYTES];
static unsigned char cli_program_memory[CLI_PROGRAM_BYTES];
static CLI_Define cli_defines[CLI_MAX_DEFINES];
static FPI_VarSystem cli_vars;
static FPI_TimeBinding cli_time;
static FPI_Activation cli_activation;
static FPI_U32 cli_timer_start;
static FPI_U32 cli_etimer_start;
static FPI_U32 cli_now;
static FPI_Fixed cli_activation_value;

static FPI_U32 cli_now_ms(void* user) {
    FPI_UNUSED(user);
    return cli_now;
}

static void print_help(void) {
    puts("fpil_cli - no-heap C89 FPI tool");
    puts("usage: fpi_cli [options] script.fpi");
    puts("  --tokens                 print lexer tokens");
    puts("  --ast                    print compact AST summary");
    puts("  --bytecode               compile and disassemble");
    puts("  --transpile              emit normalized FPI");
    puts("  --json                   emit JSON");
    puts("  --run N                  run N ticks with generic standard library");
    puts("  --vm                     use VM for --run");
    puts("  --two-phase              evaluate all rules before actions");
    puts("  --stop-first             stop after first matching rule");
    puts("  --state-word NAME        use NAME instead of default state keyword");
    puts("  --state VALUE            initial state (fixed or polysym)");
    puts("  --define NAME=VALUE      define symbolic fixed value");
    puts("Example:");
    puts("  fpi_cli --define idle=0 --define run=1 --state-word mode --run 2 demo.fpi");
}

static void print_error(const FPI_Error* error) {
    if (!error) return;
    fprintf(stderr, "error %s", fpi_result_name(error->code));
    if (error->span.line) fprintf(stderr, " at %u:%u", (unsigned)error->span.line, (unsigned)error->span.column);
    if (error->message[0]) fprintf(stderr, ": %s", error->message);
    fputc('\n', stderr);
}

static int split_define(const char* text, CLI_Define* out) {
    int i;
    int j;
    if (!text || !out) return 0;
    i = 0;
    while (text[i] && text[i] != '=') {
        if (i >= FPI_IDENT_MAX) return 0;
        out->name[i] = text[i];
        i++;
    }
    if (i == 0 || text[i] != '=') return 0;
    out->name[i] = '\0';
    i++;
    j = 0;
    while (text[i]) {
        if (j >= (int)sizeof(out->value) - 1) return 0;
        out->value[j++] = text[i++];
    }
    out->value[j] = '\0';
    return j > 0;
}

static int cli_rhs_delimiter(char c) {
    return c == ',' || c == ':' || c == ';' || c == '\r' || c == '\n' || c == '\0';
}

static void print_source_span(const char* source, FPI_U32 offset, FPI_U32 length) {
    FPI_U32 i;
    char c;
    if (!source || length == 0UL) return;
    putchar(' ');
    for (i = 0UL; i < length; i++) {
        c = source[offset + i];
        if (c == '\t') fputs("\\t", stdout);
        else putchar((unsigned char)c);
    }
}

static int dump_rhs_token(FPI_Lexer* lexer, const char* source, FPI_Error* error) {
    FPI_Stream* stream;
    FPI_U32 start;
    FPI_U32 end;
    int line;
    int column;
    char c;
    char quote;
    FPI_Span span;
    if (!lexer) return 0;
    stream = &lexer->stream;
    while (fpi_stream_peek(stream) == ' ' || fpi_stream_peek(stream) == '\t')
        (void)fpi_stream_advance(stream);
    start = stream->position;
    line = stream->line;
    column = stream->column;
    c = fpi_stream_peek(stream);
    if (c == '"' || c == '\'') {
        quote = fpi_stream_advance(stream);
        for (;;) {
            c = fpi_stream_peek(stream);
            if (c == '\0' || c == '\r' || c == '\n') {
                span = fpi_stream_span_from(stream, start, line, column);
                fpi_error_set(error, FPI_ERR_UNTERMINATED_STRING, span,
                              "quoted RHS is not terminated");
                return 0;
            }
            (void)fpi_stream_advance(stream);
            if (c == '\\') {
                c = fpi_stream_peek(stream);
                if (c == '\0' || c == '\r' || c == '\n') {
                    span = fpi_stream_span_from(stream, start, line, column);
                    fpi_error_set(error, FPI_ERR_UNTERMINATED_STRING, span,
                                  "quoted RHS is not terminated");
                    return 0;
                }
                (void)fpi_stream_advance(stream);
            } else if (c == quote) {
                break;
            }
        }
        end = stream->position;
        while (fpi_stream_peek(stream) == ' ' || fpi_stream_peek(stream) == '\t')
            (void)fpi_stream_advance(stream);
    } else {
        while (!cli_rhs_delimiter(fpi_stream_peek(stream)))
            (void)fpi_stream_advance(stream);
        end = stream->position;
        while (end > start && (source[end - 1UL] == ' ' || source[end - 1UL] == '\t')) end--;
    }
    printf("%4u:%-3u %-12s", (unsigned)line, (unsigned)column, "RHS_VALUE");
    print_source_span(source, start, end - start);
    putchar('\n');
    return 1;
}

static void dump_tokens(const char* source) {
    FPI_Lexer lexer;
    FPI_Error error;
    FPI_Token token;
    int ok;
    fpi_error_clear(&error);
    fpi_lexer_init(&lexer, source, &error);
    ok = 1;
    do {
        token = fpi_lexer_next(&lexer);
        printf("%4u:%-3u %-12s", (unsigned)token.span.line, (unsigned)token.span.column, fpi_token_type_name(token.type));
        if (token.type == FPI_TOK_IDENTIFIER) printf(" %s", token.text);
        putchar('\n');
        if (token.type == FPI_TOK_EQUAL) ok = dump_rhs_token(&lexer, source, &error);
    } while (ok && token.type != FPI_TOK_EOF && token.type != FPI_TOK_ERROR);
    if (!ok || token.type == FPI_TOK_ERROR) print_error(&error);
}

static void dump_ast(const FPI_Context* context) {
    const FPI_AST* ast;
    const FPI_Registry* registry;
    int i;
    int j;
    const FPI_Rule* rule;
    const FPI_Term* term;
    const char* name;
    ast = fpi_context_ast(context);
    registry = fpi_context_registry(context);
    printf("rules=%d terms=%d pool=%lu bytes compact_used=%lu bytes\n",
        ast->rule_count, ast->term_count, (unsigned long)ast->value_pool_used,
        (unsigned long)fpi_ast_memory_used(ast));
    for (i = 0; i < ast->rule_count; i++) {
        rule = &ast->rules[i];
        printf("rule %d @%u:%u\n", i, (unsigned)rule->span.line, (unsigned)rule->span.column);
        for (j = 0; j < (int)rule->condition_count; j++) {
            term = fpi_ast_condition_at(ast, rule, j);
            name = fpi_registry_cond_name(registry, term->symbol_id);
            printf("  if   %s", name ? name : "?");
            if (term->value.has) printf("=%s", fpi_ast_value_text(ast, &term->value));
            putchar('\n');
        }
        for (j = 0; j < (int)rule->action_count; j++) {
            term = fpi_ast_action_at(ast, rule, j);
            name = fpi_registry_act_name(registry, term->symbol_id);
            printf("  then %s", name ? name : "?");
            if (term->value.has) printf("=%s", fpi_ast_value_text(ast, &term->value));
            putchar('\n');
        }
    }
}

static int define_symbols(FPI_Context* context, int define_count) {
    int i;
    FPI_Fixed value;
    int rc;
    for (i = 0; i < define_count; i++) {
        rc = fpi_fixed_parse(cli_defines[i].value, &value);
        if (rc != FPI_OK) {
            fprintf(stderr, "invalid --define value: %s=%s\n", cli_defines[i].name, cli_defines[i].value);
            return 0;
        }
        if (fpi_define_value_symbol(context, cli_defines[i].name, value) < 0) {
            print_error(fpi_context_error(context));
            return 0;
        }
    }
    return 1;
}

static int parse_initial_state(FPI_Context* context, const char* text, FPI_Fixed* out_state) {
    FPI_Value value;
    if (!text) { *out_state = 0L; return 1; }
    fpi_value_clear(&value);
    value.has = 1;
    value.s = text;
    value.len = (int)strlen(text);
    if (fpi_fixed_parse(text, &value.fixed) == FPI_OK) value.kind = FPI_VALUE_FIXED;
    else value.kind = FPI_VALUE_TEXT;
    return fpi_resolve_value_fixed(context, &value, out_state);
}

int main(int argc, char** argv) {
    const char* filename;
    const char* state_word;
    const char* initial_state_text;
    int show_tokens;
    int show_ast;
    int show_bytecode;
    int show_transpile;
    int show_json;
    int use_vm;
    int run_ticks;
    int define_count;
    int i;
    int rc;
    FPI_U32 source_length;
    FPI_Arena arena;
    FPI_Program program;
    FPI_RunOptions options;
    FPI_StateBuiltin state_builtin;
    FPI_TruthBuiltin truth_builtin;
    FPI_StateWords words;
    FPI_Fixed state;
    int transitioned;
    char state_text[64];

    filename = 0;
    state_word = 0;
    initial_state_text = 0;
    show_tokens = 0;
    show_ast = 0;
    show_bytecode = 0;
    show_transpile = 0;
    show_json = 0;
    use_vm = 0;
    run_ticks = 0;
    define_count = 0;
    options.stop_on_first_match = 0;
    options.exec_mode = FPI_EXEC_SEQUENTIAL_IMMEDIATE;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) { print_help(); return 0; }
        else if (strcmp(argv[i], "--tokens") == 0) show_tokens = 1;
        else if (strcmp(argv[i], "--ast") == 0) show_ast = 1;
        else if (strcmp(argv[i], "--bytecode") == 0) show_bytecode = 1;
        else if (strcmp(argv[i], "--transpile") == 0) show_transpile = 1;
        else if (strcmp(argv[i], "--json") == 0) show_json = 1;
        else if (strcmp(argv[i], "--vm") == 0) use_vm = 1;
        else if (strcmp(argv[i], "--two-phase") == 0) options.exec_mode = FPI_EXEC_TWO_PHASE;
        else if (strcmp(argv[i], "--stop-first") == 0) options.stop_on_first_match = 1;
        else if (strcmp(argv[i], "--run") == 0 && i + 1 < argc) run_ticks = atoi(argv[++i]);
        else if (strcmp(argv[i], "--state-word") == 0 && i + 1 < argc) state_word = argv[++i];
        else if (strcmp(argv[i], "--state") == 0 && i + 1 < argc) initial_state_text = argv[++i];
        else if (strcmp(argv[i], "--define") == 0 && i + 1 < argc) {
            if (define_count >= CLI_MAX_DEFINES || !split_define(argv[++i], &cli_defines[define_count])) {
                fprintf(stderr, "invalid --define\n");
                return 2;
            }
            define_count++;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "unknown option: %s\n", argv[i]);
            return 2;
        } else filename = argv[i];
    }
    if (!filename) { print_help(); return 2; }

    fpi_context_init(&cli_context);
    if (!define_symbols(&cli_context, define_count)) return 1;
    if (state_word) {
        rc = fpi_state_words_from_base(&words, state_word);
        if (rc != FPI_OK) { fprintf(stderr, "invalid state word\n"); return 1; }
    } else fpi_state_words_default(&words);
    if (!parse_initial_state(&cli_context, initial_state_text, &state)) {
        fprintf(stderr, "unable to resolve initial state: %s\n", initial_state_text ? initial_state_text : "");
        return 1;
    }
    transitioned = 0;
    rc = fpi_state_builtin_init(&state_builtin, &cli_context, &state, &transitioned, &words);
    if (rc != FPI_OK) { print_error(fpi_context_error(&cli_context)); return 1; }
    rc = fpi_truth_builtin_init(&truth_builtin, &cli_context, 1UL);
    if (rc != FPI_OK) { print_error(fpi_context_error(&cli_context)); return 1; }
    fpi_var_system_init(&cli_vars, &cli_context);
    cli_timer_start = 0UL;
    cli_etimer_start = 0UL;
    cli_now = 0UL;
    rc = fpi_time_binding_init3(&cli_time, &cli_context, &cli_timer_start,
        &cli_etimer_start, cli_now_ms, 0, &cli_vars, 0, 0);
    if (rc != FPI_OK) { print_error(fpi_context_error(&cli_context)); return 1; }
    cli_activation_value = 0L;
    rc = fpi_activation_init2(&cli_activation, &cli_context,
        &cli_activation_value, &cli_vars, 0, 0);
    if (rc != FPI_OK) { print_error(fpi_context_error(&cli_context)); return 1; }

    rc = fpi_io_read_file(filename, cli_source, CLI_SOURCE_BYTES, &source_length, &cli_context.last_error);
    FPI_UNUSED(source_length);
    if (rc != FPI_OK) { print_error(fpi_context_error(&cli_context)); return 1; }
    if (show_tokens) dump_tokens(cli_source);
    rc = fpi_context_load_text(&cli_context, cli_source);
    if (rc != FPI_OK) { print_error(fpi_context_error(&cli_context)); return 1; }

    if (show_ast) dump_ast(&cli_context);
    if (show_transpile) (void)fpi_transpile_fpi(stdout, fpi_context_ast(&cli_context), fpi_context_registry(&cli_context));
    if (show_json) (void)fpi_transpile_json(stdout, fpi_context_ast(&cli_context), fpi_context_registry(&cli_context));

    fpi_arena_init(&arena, cli_program_memory, CLI_PROGRAM_BYTES);
    fpi_program_init(&program);
    if (show_bytecode || (run_ticks > 0 && use_vm)) {
        rc = fpi_compile(&cli_context, &arena, &program);
        if (rc != FPI_OK) { print_error(fpi_context_error(&cli_context)); return 1; }
        if (show_bytecode) fpi_bc_disassemble(stdout, &program, fpi_context_registry(&cli_context));
    }

    if (run_ticks > 0) {
        for (i = 0; i < run_ticks; i++) {
            int fired;
            char var_text[64];
            cli_now = (FPI_U32)i * 1000UL;
            fpi_state_builtin_begin_tick(&state_builtin);
            fired = use_vm ? fpi_tick_vm_bound(&cli_context, &program, 0, &options) :
                             fpi_tick_bound_ex(&cli_context, 0, &options);
            (void)fpi_fixed_format(state, state_text, (int)sizeof(state_text), 4);
            (void)fpi_fixed_format(cli_vars.gvars[0], var_text, (int)sizeof(var_text), 4);
            printf("tick=%d now_ms=%lu fired=%d state=%s var0=%s\n",
                i, (unsigned long)cli_now, fired, state_text, var_text);
        }
    }
    return 0;
}
