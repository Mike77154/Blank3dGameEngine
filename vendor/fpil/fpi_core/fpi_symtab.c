#include "fpi_symtab.h"

void fpi_symtab_init(FPI_SymTab* symtab) { fpi_registry_init(symtab); }
int fpi_symtab_intern_cond(FPI_SymTab* symtab, const char* name) { return fpi_registry_register_cond(symtab, name, 0); }
int fpi_symtab_intern_act(FPI_SymTab* symtab, const char* name) { return fpi_registry_register_act(symtab, name, 0); }
const char* fpi_symtab_cond_name(const FPI_SymTab* symtab, int id) { return fpi_registry_cond_name(symtab, id); }
const char* fpi_symtab_act_name(const FPI_SymTab* symtab, int id) { return fpi_registry_act_name(symtab, id); }
