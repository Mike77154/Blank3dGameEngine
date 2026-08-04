#ifndef FPI_COMPILER_H
#define FPI_COMPILER_H

#include "fpi_ir.h"

int fpi_compile_estimate(const FPI_AST* ast, int* out_code_count, int* out_constant_count, FPI_U32* out_string_bytes);
int fpi_compile_ast(const FPI_AST* ast, FPI_U32 generation, FPI_Arena* arena, FPI_Program* out_program, FPI_Error* error);
int fpi_compile_ir(const FPI_IR* ir, FPI_U32 generation, FPI_Arena* arena, FPI_Program* out_program, FPI_Error* error);

#endif /* FPI_COMPILER_H */
