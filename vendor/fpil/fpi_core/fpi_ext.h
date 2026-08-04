#ifndef FPI_EXT_H
#define FPI_EXT_H

#include "fpi.h"
#include "fpi_transpiler.h"
#include "fpi_bytecode.h"

const AST_Script* fpi_get_ast(const FPI_Context* context);
const FPI_SymTab* fpi_get_symtab(const FPI_Context* context);
int fpi_compile_bytecode(FPI_Context* context, FPI_Arena* arena, FPI_Program* out_program);

#endif /* FPI_EXT_H */
