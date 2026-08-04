#ifndef DDSL_STDLIB_H
#define DDSL_STDLIB_H

#include "store/store.h"

#ifdef __cplusplus
extern "C" {
#endif

int ddsl_stdlib_load_defaults(ddsl_store *st);
int ddsl_stdlib_set_usage_flag(ddsl_store *st, const char *name);

#ifdef __cplusplus
}
#endif

#endif /* DDSL_STDLIB_H */
