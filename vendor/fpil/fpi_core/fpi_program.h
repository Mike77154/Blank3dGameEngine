#ifndef FPI_PROGRAM_H
#define FPI_PROGRAM_H

#include "fpi_opcodes.h"
#include "fpi_arena.h"
#include "fpi_ast.h"

typedef struct FPI_Instruction {
    FPI_U8 opcode;
    int a;
    int b;
} FPI_Instruction;

typedef struct FPI_ProgramRule {
    int condition_ip;
    int action_ip;
    int end_ip;
} FPI_ProgramRule;

typedef struct FPI_Program {
    FPI_Instruction* code;
    int code_count;
    FPI_Value* constants;
    int constant_count;
    FPI_ProgramRule* rules;
    int rule_count;
    FPI_U32 source_generation;
} FPI_Program;

void fpi_program_init(FPI_Program* program);
int fpi_program_validate(const FPI_Program* program, FPI_Error* error);

#endif /* FPI_PROGRAM_H */
