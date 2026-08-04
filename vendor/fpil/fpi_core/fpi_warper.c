#include "fpi_warper.h"

void fpi_warper_init(FPI_Warper* warper, FPI_Context* context, void* program_memory, FPI_U32 program_memory_bytes) {
    if (!warper) return;
    warper->context = context;
    fpi_arena_init(&warper->arena, program_memory, program_memory_bytes);
    fpi_program_init(&warper->program);
    warper->compiled = 0;
}

int fpi_warper_load_text(FPI_Warper* warper, const char* source_text) {
    int rc;
    if (!warper || !warper->context) return FPI_ERR_ARGUMENT;
    rc = fpi_context_load_text(warper->context, source_text);
    if (rc != FPI_OK) return rc;
    warper->compiled = 0;
    return FPI_OK;
}

int fpi_warper_compile(FPI_Warper* warper) {
    int rc;
    if (!warper || !warper->context) return FPI_ERR_ARGUMENT;
    fpi_arena_reset(&warper->arena);
    fpi_program_init(&warper->program);
    rc = fpi_compile(warper->context, &warper->arena, &warper->program);
    warper->compiled = rc == FPI_OK;
    return rc;
}

int fpi_warper_tick_bound(FPI_Warper* warper, void* run_user, const FPI_RunOptions* options) {
    if (!warper || !warper->context) return 0;
    if (!warper->compiled && fpi_warper_compile(warper) != FPI_OK) return 0;
    return fpi_tick_vm_bound(warper->context, &warper->program, run_user, options);
}
