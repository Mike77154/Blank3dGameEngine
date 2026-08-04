#include "fpi_bytecode.h"

int fpi_bc_estimate(const AST_Script* script, int* out_code_count, int* out_const_count) {
    return fpi_compile_estimate(script, out_code_count, out_const_count, 0) == FPI_OK;
}

int fpi_bc_compile(const AST_Script* script, FPI_Program* out_program, FPI_Arena* arena) {
    return fpi_compile_ast(script, 0UL, arena, out_program, 0) == FPI_OK;
}

void fpi_bc_disassemble(FILE* out, const FPI_Program* program, const FPI_SymTab* symtab) {
    int i;
    const FPI_Instruction* instruction;
    const char* name;
    if (!out) out = stdout;
    if (!program || !program->code) {
        fprintf(out, "<empty program>\n");
        return;
    }
    for (i = 0; i < program->code_count; i++) {
        instruction = &program->code[i];
        fprintf(out, "%05d  %-18s", i, fpi_opcode_name(instruction->opcode));
        if (instruction->opcode == FPI_OP_EVAL_COND) {
            name = symtab ? fpi_symtab_cond_name(symtab, instruction->a) : 0;
            fprintf(out, " cond=%d(%s) const=%d", instruction->a, name ? name : "?", instruction->b);
        } else if (instruction->opcode == FPI_OP_EXEC_ACT) {
            name = symtab ? fpi_symtab_act_name(symtab, instruction->a) : 0;
            fprintf(out, " act=%d(%s) const=%d", instruction->a, name ? name : "?", instruction->b);
        } else if (instruction->opcode == FPI_OP_JUMP_IF_FALSE) {
            fprintf(out, " target=%d", instruction->a);
        } else if (instruction->opcode == FPI_OP_RULE_FIRED) {
            fprintf(out, " rule=%d", instruction->a);
        }
        fprintf(out, "\n");
    }
}
