#include "opcodes/opcodes.h"

const char *ddsl_opcode_bc_name(ddsl_bc_op op) {
    return ddsl_bc_op_name(op);
}

const char *ddsl_opcode_ir_name(ddsl_ir_op op) {
    return ddsl_ir_op_name(op);
}

int ddsl_opcode_is_jump(ddsl_bc_op op) {
    return (op == DDSL_BC_JMP || op == DDSL_BC_JMP_IF_FALSE || op == DDSL_BC_JMP_IF_TRUE) ? 1 : 0;
}
