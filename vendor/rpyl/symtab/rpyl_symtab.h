#ifndef RPYL_SYMTAB_H
#define RPYL_SYMTAB_H

#include <stddef.h>
#include "rpyl_config.h"

#ifndef RPYL_SYMTAB_MAX
#define RPYL_SYMTAB_MAX 256
#endif
#ifndef RPYL_SYMTAB_MAX_NAME
#define RPYL_SYMTAB_MAX_NAME RPYL_AST_MAX_NAME
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum RpylSymKind {
    RPYL_SYM_OTHER = 0,
    RPYL_SYM_LABEL = 1,
    RPYL_SYM_DEFINE = 2,
    RPYL_SYM_COMMAND = 3,
    RPYL_SYM_EXTERN = 4,
    RPYL_SYM_VARIABLE = 5
} RpylSymKind;

typedef struct RpylSym {
    char name[RPYL_SYMTAB_MAX_NAME];
    unsigned long value;
    RpylSymKind kind;
    unsigned long flags;
} RpylSym;

typedef struct RpylSymtab {
    RpylSym items[RPYL_SYMTAB_MAX];
    size_t count;
    int overflowed;
} RpylSymtab;

void rpyl_symtab_init(RpylSymtab* t);
void rpyl_symtab_clear(RpylSymtab* t);
int rpyl_symtab_put_kind(RpylSymtab* t, const char* name, unsigned long value, RpylSymKind kind, unsigned long flags);
int rpyl_symtab_put(RpylSymtab* t, const char* name, unsigned long value);
int rpyl_symtab_get(const RpylSymtab* t, const char* name, unsigned long* value_out);
int rpyl_symtab_get_kind(const RpylSymtab* t, const char* name, RpylSymKind kind, unsigned long* value_out);
int rpyl_symtab_contains(const RpylSymtab* t, const char* name);
int rpyl_symtab_remove(RpylSymtab* t, const char* name);
size_t rpyl_symtab_count(const RpylSymtab* t);
const RpylSym* rpyl_symtab_at(const RpylSymtab* t, size_t index);
int rpyl_symtab_overflowed(const RpylSymtab* t);

#ifdef __cplusplus
}
#endif

#endif
