#ifndef DDSL_SYMTAB_H
#define DDSL_SYMTAB_H

#include "config/config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ddsl_sym {
    char name[64];
    int id;
    int used;
} ddsl_sym;

typedef struct ddsl_symtab {
    ddsl_sym items[DDSL_MAX_SYMBOLS];
    int count;
} ddsl_symtab;

void ddsl_symtab_init(ddsl_symtab *tab);
int ddsl_symtab_put(ddsl_symtab *tab, const char *name, int id);
int ddsl_symtab_get(const ddsl_symtab *tab, const char *name, int *out_id);
int ddsl_symtab_count(const ddsl_symtab *tab);

#ifdef __cplusplus
}
#endif

#endif /* DDSL_SYMTAB_H */
