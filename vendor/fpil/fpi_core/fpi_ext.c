#include "fpi_ext.h"

const AST_Script* fpi_get_ast(const FPI_Context* context) { return fpi_context_ast(context); }
const FPI_SymTab* fpi_get_symtab(const FPI_Context* context) { return fpi_context_registry(context); }
int fpi_compile_bytecode(FPI_Context* context, FPI_Arena* arena, FPI_Program* out_program) {
    return fpi_compile(context, arena, out_program) == FPI_OK;
}
