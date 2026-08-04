#ifndef DDSL_POLYSYM_H
#define DDSL_POLYSYM_H

#include "config/config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ddsl_polysym_item {
    char domain[32];
    char name[64];
    int id;
    int used;
} ddsl_polysym_item;

typedef struct ddsl_polysym {
    ddsl_polysym_item items[DDSL_MAX_POLYSYMS];
    int count;
} ddsl_polysym;

void ddsl_polysym_init(ddsl_polysym *ps);
int ddsl_polysym_put(ddsl_polysym *ps, const char *domain, const char *name, int id);
int ddsl_polysym_get(const ddsl_polysym *ps, const char *domain, const char *name, int *out_id);

#ifdef __cplusplus
}
#endif

#endif /* DDSL_POLYSYM_H */
