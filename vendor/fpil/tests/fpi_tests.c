#include <stdio.h>
#include <string.h>

#include "fpi.h"
#include "fpi_builtins.h"

#define TEST_PROGRAM_BYTES (4UL * 1024UL * 1024UL)
#define TEST_SOURCE_BYTES 65536

static FPI_Context test_context;
static unsigned char test_program_memory[TEST_PROGRAM_BYTES];
static char test_source[TEST_SOURCE_BYTES];
static int failures;

#define CHECK(expr) do { if (!(expr)) { \
    printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr); failures++; return; \
} } while (0)

static void setup_defaults(FPI_Context* context, FPI_StateBuiltin* state_builtin, FPI_TruthBuiltin* truth_builtin, FPI_Fixed* state, int* transitioned) {
    int rc;
    fpi_context_init(context);
    *state = 0L;
    *transitioned = 0;
    rc = fpi_state_builtin_init(state_builtin, context, state, transitioned, 0);
    CHECK(rc == FPI_OK);
    rc = fpi_truth_builtin_init(truth_builtin, context, 1UL);
    CHECK(rc == FPI_OK);
}

static void test_fixed(void) {
    FPI_Fixed a;
    FPI_Fixed b;
    FPI_Fixed result;
    int ok;
    char text[64];
    CHECK(fpi_fixed_parse("1.5", &a) == FPI_OK);
    CHECK(fpi_fixed_parse("2.25", &b) == FPI_OK);
    result = fpi_fixed_mul_sat(a, b);
    CHECK(fpi_fixed_format(result, text, (int)sizeof(text), 4) > 0);
    CHECK(strcmp(text, "3.3750") == 0);
    result = fpi_fixed_div_sat(b, a, &ok);
    CHECK(ok == 1);
    CHECK(fpi_fixed_format(result, text, (int)sizeof(text), 4) > 0);
    CHECK(strcmp(text, "1.5000") == 0);
    CHECK(fpi_fixed_sin_deg(90L * FPI_FIXED_ONE) == FPI_FIXED_ONE);
    CHECK(fpi_fixed_cos_deg(0L) == FPI_FIXED_ONE);
    CHECK(fpi_fixed_cos_deg(360L * FPI_FIXED_ONE) == FPI_FIXED_ONE);
    CHECK(fpi_fixed_cos_deg((-360L) * FPI_FIXED_ONE) == FPI_FIXED_ONE);
    CHECK(fpi_fixed_cos_deg(2147483647L) ==
          fpi_fixed_cos_deg(2147483647L % (360L * FPI_FIXED_ONE)));
    CHECK(fpi_fixed_cos_deg((-2147483647L - 1L)) ==
          fpi_fixed_cos_deg((-2147483647L - 1L) % (360L * FPI_FIXED_ONE)));
    CHECK(fpi_fixed_to_int((-2147483647L - 1L)) == -32768L);
    result = fpi_fixed_mod((-2147483647L - 1L), -1L, &ok);
    CHECK(ok == 1 && result == 0L);
    puts("PASS fixed point and signed limits");
}

static void test_configurable_state_and_polysym(void) {
    FPI_StateBuiltin state_builtin;
    FPI_TruthBuiltin truth_builtin;
    FPI_StateWords words;
    FPI_Fixed state;
    int transitioned;
    int rc;
    fpi_context_init(&test_context);
    CHECK(fpi_define_value_symbol(&test_context, "idle", 0L) >= 0);
    CHECK(fpi_define_value_symbol(&test_context, "running", FPI_FIXED_ONE) >= 0);
    CHECK(fpi_state_words_from_base(&words, "mode") == FPI_OK);
    state = 0L;
    transitioned = 0;
    rc = fpi_state_builtin_init(&state_builtin, &test_context, &state, &transitioned, &words);
    CHECK(rc == FPI_OK);
    CHECK(fpi_truth_builtin_init(&truth_builtin, &test_context, 1UL) == FPI_OK);
    rc = fpi_context_load_text(&test_context, ":mode=idle,always:mode=running\n");
    CHECK(rc == FPI_OK);
    CHECK(fpi_tick_bound(&test_context, 0) == 1);
    CHECK(state == FPI_FIXED_ONE);
    puts("PASS configurable state + polysym");
}

static void test_reload_stability(void) {
    FPI_StateBuiltin state_builtin;
    FPI_TruthBuiltin truth_builtin;
    FPI_Fixed state;
    int transitioned;
    int state_cond_before;
    int always_before;
    FPI_U32 generation;
    setup_defaults(&test_context, &state_builtin, &truth_builtin, &state, &transitioned);
    CHECK(fpi_context_load_text(&test_context, ":state=0,always:state=1\n") == FPI_OK);
    state_cond_before = fpi_get_cond_id(&test_context, "state");
    always_before = fpi_get_cond_id(&test_context, "always");
    CHECK(fpi_tick_bound(&test_context, 0) == 1);
    CHECK(state == FPI_FIXED_ONE);
    state = 0L;
    CHECK(fpi_context_load_text(&test_context, ":always,state=0:state=2\n") == FPI_OK);
    CHECK(fpi_get_cond_id(&test_context, "state") == state_cond_before);
    CHECK(fpi_get_cond_id(&test_context, "always") == always_before);
    CHECK(fpi_tick_bound(&test_context, 0) == 1);
    CHECK(state == 2L * FPI_FIXED_ONE);
    generation = fpi_context_generation(&test_context);
    CHECK(fpi_context_load_text(&test_context, ":state=2,always state=3\n") != FPI_OK);
    CHECK(fpi_context_generation(&test_context) == generation);
    state = 0L;
    CHECK(fpi_tick_bound(&test_context, 0) == 1);
    CHECK(state == 2L * FPI_FIXED_ONE);
    puts("PASS permanent registry + transactional reload");
}

static void build_many_rules(int count) {
    int i;
    int pos;
    const char* line;
    int j;
    pos = 0;
    line = ":always:none\n";
    for (i = 0; i < count; i++) {
        j = 0;
        while (line[j]) {
            if (pos < TEST_SOURCE_BYTES - 1) test_source[pos++] = line[j];
            j++;
        }
    }
    test_source[pos] = '\0';
}

static void test_two_phase_and_limits(void) {
    FPI_StateBuiltin state_builtin;
    FPI_TruthBuiltin truth_builtin;
    FPI_Fixed state;
    int transitioned;
    FPI_RunOptions options;
    FPI_Arena arena;
    FPI_Program program;
    setup_defaults(&test_context, &state_builtin, &truth_builtin, &state, &transitioned);
    build_many_rules(100);
    CHECK(fpi_context_load_text(&test_context, test_source) == FPI_OK);
    options.stop_on_first_match = 0;
    options.exec_mode = FPI_EXEC_TWO_PHASE;
    CHECK(fpi_tick_bound_ex(&test_context, 0, &options) == 100);
    fpi_arena_init(&arena, test_program_memory, TEST_PROGRAM_BYTES);
    fpi_program_init(&program);
    CHECK(fpi_compile(&test_context, &arena, &program) == FPI_OK);
    CHECK(fpi_tick_vm_bound(&test_context, &program, 0, &options) == 100);
    build_many_rules(FPI_MAX_RULES + 1);
    CHECK(fpi_context_load_text(&test_context, test_source) == FPI_ERR_TOO_MANY_RULES);
    CHECK(fpi_context_error(&test_context)->code == FPI_ERR_TOO_MANY_RULES);
    puts("PASS interpreter/VM two-phase capacity + explicit rule limit");
}

static char captured[128];
static void capture_action(void* bind_user, void* run_user, int symbol_id, const FPI_Value* value) {
    FPI_UNUSED(bind_user);
    FPI_UNUSED(run_user);
    FPI_UNUSED(symbol_id);
    if (!value || !value->has) captured[0] = '\0';
    else {
        int i;
        i = 0;
        while (value->s[i] && i < (int)sizeof(captured) - 1) { captured[i] = value->s[i]; i++; }
        captured[i] = '\0';
    }
}

static void test_quoted_values(void) {
    FPI_StateBuiltin state_builtin;
    FPI_TruthBuiltin truth_builtin;
    FPI_Fixed state;
    int transitioned;
    setup_defaults(&test_context, &state_builtin, &truth_builtin, &state, &transitioned);
    CHECK(fpi_bind_act(&test_context, "emit", capture_action, 0) >= 0);
    CHECK(fpi_context_load_text(&test_context, ":always:emit=\"a,b:c\\nline\"\n") == FPI_OK);
    captured[0] = '\0';
    CHECK(fpi_tick_bound(&test_context, 0) == 1);
    CHECK(strcmp(captured, "a,b:c\nline") == 0);
    puts("PASS quoted/escaped RHS");
}

static void test_vm_parity(void) {
    FPI_StateBuiltin state_builtin;
    FPI_TruthBuiltin truth_builtin;
    FPI_Fixed state;
    int transitioned;
    FPI_Arena arena;
    FPI_Program program;
    FPI_RunOptions options;
    int interpreter_fired;
    int vm_fired;
    setup_defaults(&test_context, &state_builtin, &truth_builtin, &state, &transitioned);
    CHECK(fpi_context_load_text(&test_context,
        ":state=0,always:state=1\n"
        ":state=1,always:state=2\n") == FPI_OK);
    fpi_arena_init(&arena, test_program_memory, TEST_PROGRAM_BYTES);
    fpi_program_init(&program);
    CHECK(fpi_compile(&test_context, &arena, &program) == FPI_OK);
    options.stop_on_first_match = 0;
    options.exec_mode = FPI_EXEC_SEQUENTIAL_IMMEDIATE;
    state = 0L;
    interpreter_fired = fpi_tick_bound_ex(&test_context, 0, &options);
    CHECK(state == 2L * FPI_FIXED_ONE);
    state = 0L;
    vm_fired = fpi_tick_vm_bound(&test_context, &program, 0, &options);
    CHECK(state == 2L * FPI_FIXED_ONE);
    CHECK(interpreter_fired == vm_fired && vm_fired == 2);
    options.exec_mode = FPI_EXEC_TWO_PHASE;
    state = 0L;
    interpreter_fired = fpi_tick_bound_ex(&test_context, 0, &options);
    CHECK(state == FPI_FIXED_ONE && interpreter_fired == 1);
    state = 0L;
    vm_fired = fpi_tick_vm_bound(&test_context, &program, 0, &options);
    CHECK(state == FPI_FIXED_ONE && vm_fired == 1);
    CHECK(interpreter_fired == vm_fired);
    puts("PASS interpreter/VM parity");
}

static void test_generation_guard(void) {
    FPI_StateBuiltin state_builtin;
    FPI_TruthBuiltin truth_builtin;
    FPI_Fixed state;
    int transitioned;
    FPI_Arena arena;
    FPI_Program program;
    setup_defaults(&test_context, &state_builtin, &truth_builtin, &state, &transitioned);
    CHECK(fpi_context_load_text(&test_context, ":always:none\n") == FPI_OK);
    fpi_arena_init(&arena, test_program_memory, TEST_PROGRAM_BYTES);
    CHECK(fpi_compile(&test_context, &arena, &program) == FPI_OK);
    CHECK(fpi_context_load_text(&test_context, ":always:none\n:always:none\n") == FPI_OK);
    CHECK(fpi_tick_vm_bound(&test_context, &program, 0, 0) == 0);
    CHECK(fpi_context_error(&test_context)->code == FPI_ERR_GENERATION_MISMATCH);
    puts("PASS stale bytecode guard");
}

static void test_explicit_parser_and_binding_errors(void) {
    FPI_TruthBuiltin truth_builtin;
    FPI_Arena arena;
    FPI_Program program;
    FPI_U32 generation;

    fpi_context_init(&test_context);
    CHECK(fpi_context_load_text(&test_context, ":missing:missing\n") == FPI_OK);
    CHECK(fpi_tick_bound(&test_context, 0) == 0);
    CHECK(fpi_context_error(&test_context)->code == FPI_ERR_UNBOUND_SYMBOL);

    fpi_context_init(&test_context);
    CHECK(fpi_truth_builtin_init(&truth_builtin, &test_context, 1UL) == FPI_OK);
    CHECK(fpi_context_load_text(&test_context, ":always:missing\n") == FPI_OK);
    CHECK(fpi_tick_bound(&test_context, 0) == 0);
    CHECK(fpi_context_error(&test_context)->code == FPI_ERR_UNBOUND_SYMBOL);
    fpi_arena_init(&arena, test_program_memory, TEST_PROGRAM_BYTES);
    fpi_program_init(&program);
    CHECK(fpi_compile(&test_context, &arena, &program) == FPI_OK);
    CHECK(fpi_tick_vm_bound(&test_context, &program, 0, 0) == 0);
    CHECK(fpi_context_error(&test_context)->code == FPI_ERR_UNBOUND_SYMBOL);

    generation = fpi_context_generation(&test_context);
    CHECK(fpi_context_load_text(&test_context, "garbage outside a rule\n") == FPI_ERR_SYNTAX);
    CHECK(fpi_context_generation(&test_context) == generation);
    puts("PASS explicit parser and binding errors");
}

static void test_buffered_io_no_truncation(void) {
    FPI_StateBuiltin state_builtin;
    FPI_TruthBuiltin truth_builtin;
    FPI_Fixed state;
    int transitioned;
    FILE* file;
    char small_buffer[32];
    FPI_U32 generation;
    setup_defaults(&test_context, &state_builtin, &truth_builtin, &state, &transitioned);
    CHECK(fpi_context_load_text(&test_context, ":always:none\n") == FPI_OK);
    generation = fpi_context_generation(&test_context);
    file = fopen("tests/oversized_test.fpi", "wb");
    CHECK(file != 0);
    fputs(":always:none\n:always:none\n:always:none\n", file);
    fclose(file);
    CHECK(fpi_context_load_buffered(&test_context, "tests/oversized_test.fpi", small_buffer, (FPI_U32)sizeof(small_buffer)) == FPI_ERR_FILE_TOO_LARGE);
    CHECK(fpi_context_generation(&test_context) == generation);
    (void)remove("tests/oversized_test.fpi");
    puts("PASS file overflow is explicit");
}

int main(void) {
    failures = 0;
    test_fixed();
    test_configurable_state_and_polysym();
    test_reload_stability();
    test_two_phase_and_limits();
    test_quoted_values();
    test_vm_parity();
    test_generation_guard();
    test_explicit_parser_and_binding_errors();
    test_buffered_io_no_truncation();
    if (failures) {
        printf("%d test group(s) failed\n", failures);
        return 1;
    }
    printf("ALL TESTS PASSED context_size=%lu bytes\n", (unsigned long)fpi_context_size());
    return 0;
}
