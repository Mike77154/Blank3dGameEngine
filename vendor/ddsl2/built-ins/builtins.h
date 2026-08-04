#ifndef DDSL_BUILTINS_H
#define DDSL_BUILTINS_H

#include "registry/registry.h"

#ifdef __cplusplus
extern "C" {
#endif

int ddsl_builtin_truthy(void *user, const ddsl_value *args, int argc, ddsl_value *out);
int ddsl_builtins_register_core(ddsl_registry *r);

#ifdef __cplusplus
}
#endif

#endif /* DDSL_BUILTINS_H */
