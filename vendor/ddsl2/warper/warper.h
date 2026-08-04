#ifndef DDSL_WARPER_H
#define DDSL_WARPER_H

#include "arena/arena.h"
#include "store/store.h"
#include "error/error.h"

#ifdef __cplusplus
extern "C" {
#endif

int ddsl_warper_exec_to_store(ddsl_arena *arena, const char *source, ddsl_store *store, ddsl_error *err);

#ifdef __cplusplus
}
#endif

#endif /* DDSL_WARPER_H */
