#ifndef FPI_WARPER_H
#define FPI_WARPER_H

/* "warper" is kept as the requested protocol name; it is a no-heap wrapper. */
#include "fpi.h"

typedef struct FPI_Warper {
    FPI_Context* context;
    FPI_Arena arena;
    FPI_Program program;
    int compiled;
} FPI_Warper;

void fpi_warper_init(FPI_Warper* warper, FPI_Context* context, void* program_memory, FPI_U32 program_memory_bytes);
int fpi_warper_load_text(FPI_Warper* warper, const char* source_text);
int fpi_warper_compile(FPI_Warper* warper);
int fpi_warper_tick_bound(FPI_Warper* warper, void* run_user, const FPI_RunOptions* options);

#endif /* FPI_WARPER_H */
