#include <stdio.h>
#include "fpi.h"
#include "fpi_builtins.h"

static FPI_Context context;
static FPI_StateBuiltin mode_builtin;
static FPI_Fixed mode_value;
static int transitioned;

static void print_error(const FPI_Error* error)
{
    if (!error) return;
    printf("%s at %d:%d: %s\n",
           fpi_result_name(error->code),
           error->span.line,
           error->span.column,
           error->message);
}

int main(void)
{
    FPI_StateWords words;
    int rc;
    int fired;
    char formatted[32];

    fpi_context_init(&context);
    rc = fpi_define_value_symbol(&context, "idle", 0L);
    if (rc < 0) return 1;
    rc = fpi_define_value_symbol(&context, "running", FPI_FIXED_ONE);
    if (rc < 0) return 1;

    rc = fpi_state_words_from_base(&words, "mode");
    if (rc < 0) return 1;
    mode_value = 0L;
    transitioned = 0;
    rc = fpi_state_builtin_init(&mode_builtin, &context, &mode_value,
                                &transitioned, &words);
    if (rc < 0) return 1;

    rc = fpi_context_load_text(&context, ":mode=idle:mode=running\n");
    if (rc < 0) {
        print_error(fpi_context_error(&context));
        return 1;
    }

    fpi_state_builtin_begin_tick(&mode_builtin);
    fired = fpi_tick_bound(&context, 0);
    if (fpi_context_error(&context)->code != FPI_OK) {
        print_error(fpi_context_error(&context));
        return 1;
    }
    (void)fpi_fixed_format(mode_value, formatted,
                           (int)sizeof(formatted), 4);
    printf("fired=%d mode=%s\n", fired, formatted);
    return mode_value == FPI_FIXED_ONE ? 0 : 1;
}
