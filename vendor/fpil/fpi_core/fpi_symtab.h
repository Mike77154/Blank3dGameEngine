#ifndef FPI_SYMTAB_H
#define FPI_SYMTAB_H

/* Compatibility name: the permanent registry replaces the old per-script symtab. */
#include "fpi_registry.h"
typedef FPI_Registry FPI_SymTab;

void fpi_symtab_init(FPI_SymTab* symtab);
int fpi_symtab_intern_cond(FPI_SymTab* symtab, const char* name);
int fpi_symtab_intern_act(FPI_SymTab* symtab, const char* name);
const char* fpi_symtab_cond_name(const FPI_SymTab* symtab, int id);
const char* fpi_symtab_act_name(const FPI_SymTab* symtab, int id);

#endif /* FPI_SYMTAB_H */
