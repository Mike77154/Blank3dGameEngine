#ifndef FPI_TRANSPILER_H
#define FPI_TRANSPILER_H

#include <stdio.h>
#include "fpi_ast.h"
#include "fpi_registry.h"
#include "fpi_symtab.h"

int fpi_transpile_fpi(FILE* out, const FPI_AST* ast, const FPI_Registry* registry);
int fpi_transpile_json(FILE* out, const FPI_AST* ast, const FPI_Registry* registry);

/* Compatibility names. */
int fpi_emit_normalized(FILE* out, const AST_Script* ast, const FPI_SymTab* symtab);
int fpi_emit_json(FILE* out, const AST_Script* ast, const FPI_SymTab* symtab);

#endif /* FPI_TRANSPILER_H */
