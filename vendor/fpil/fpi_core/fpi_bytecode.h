#ifndef FPI_BYTECODE_H
#define FPI_BYTECODE_H

#include <stdio.h>
#include "fpi_compiler.h"
#include "fpi_symtab.h"

typedef FPI_Instruction FPI_BC_Insn;
typedef FPI_Opcode FPI_BC_Op;

#define FPI_BC_OP_HALT FPI_OP_HALT
#define FPI_BC_OP_EVAL_COND FPI_OP_EVAL_COND
#define FPI_BC_OP_JUMP_IF_FALSE FPI_OP_JUMP_IF_FALSE
#define FPI_BC_OP_EXEC_ACT FPI_OP_EXEC_ACT
#define FPI_BC_OP_RULE_FIRED FPI_OP_RULE_FIRED

int fpi_bc_estimate(const AST_Script* script, int* out_code_count, int* out_const_count);
int fpi_bc_compile(const AST_Script* script, FPI_Program* out_program, FPI_Arena* arena);
void fpi_bc_disassemble(FILE* out, const FPI_Program* program, const FPI_SymTab* symtab);

#endif /* FPI_BYTECODE_H */
