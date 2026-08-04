#include "fpi_program.h"

void fpi_program_init(FPI_Program* program) {
    if (!program) return;
    program->code = 0;
    program->code_count = 0;
    program->constants = 0;
    program->constant_count = 0;
    program->rules = 0;
    program->rule_count = 0;
    program->source_generation = 0UL;
}

int fpi_program_validate(const FPI_Program* program, FPI_Error* error) {
    int i;
    const FPI_Instruction* instruction;
    FPI_Span span;
    fpi_span_clear(&span);
    if (!program || !program->code || program->code_count <= 0) {
        fpi_error_set(error, FPI_ERR_PROGRAM_INVALID, span, "program has no code");
        return FPI_ERR_PROGRAM_INVALID;
    }
    if (program->constant_count < 0 || (program->constant_count > 0 && !program->constants)) {
        fpi_error_set(error, FPI_ERR_PROGRAM_INVALID, span, "program constant pool is invalid");
        return FPI_ERR_PROGRAM_INVALID;
    }
    if (program->rule_count < 0 || (program->rule_count > 0 && !program->rules)) {
        fpi_error_set(error, FPI_ERR_PROGRAM_INVALID, span, "program rule table is invalid");
        return FPI_ERR_PROGRAM_INVALID;
    }
    for (i = 0; i < program->code_count; i++) {
        instruction = &program->code[i];
        switch (instruction->opcode) {
            case FPI_OP_HALT:
            case FPI_OP_RULE_FIRED:
                break;
            case FPI_OP_EVAL_COND:
            case FPI_OP_EXEC_ACT:
                if (instruction->b < 0 || instruction->b >= program->constant_count) {
                    fpi_error_set(error, FPI_ERR_PROGRAM_INVALID, span, "constant index out of range");
                    return FPI_ERR_PROGRAM_INVALID;
                }
                break;
            case FPI_OP_JUMP_IF_FALSE:
                if (instruction->a < 0 || instruction->a >= program->code_count) {
                    fpi_error_set(error, FPI_ERR_PROGRAM_INVALID, span, "jump target out of range");
                    return FPI_ERR_PROGRAM_INVALID;
                }
                break;
            default:
                fpi_error_set(error, FPI_ERR_UNKNOWN_OPCODE, span, "program contains an unknown opcode");
                return FPI_ERR_UNKNOWN_OPCODE;
        }
    }
    for (i = 0; i < program->rule_count; i++) {
        if (program->rules[i].condition_ip < 0 ||
            program->rules[i].condition_ip > program->rules[i].action_ip ||
            program->rules[i].action_ip > program->rules[i].end_ip ||
            program->rules[i].end_ip > program->code_count) {
            fpi_error_set(error, FPI_ERR_PROGRAM_INVALID, span, "program rule range is invalid");
            return FPI_ERR_PROGRAM_INVALID;
        }
    }
    if (program->code[program->code_count - 1].opcode != FPI_OP_HALT) {
        fpi_error_set(error, FPI_ERR_PROGRAM_INVALID, span, "program does not end in HALT");
        return FPI_ERR_PROGRAM_INVALID;
    }
    return FPI_OK;
}
