#include "fpi_opcodes.h"

const char* fpi_opcode_name(int opcode) {
    switch (opcode) {
        case FPI_OP_HALT: return "HALT";
        case FPI_OP_EVAL_COND: return "EVAL_COND";
        case FPI_OP_JUMP_IF_FALSE: return "JUMP_IF_FALSE";
        case FPI_OP_EXEC_ACT: return "EXEC_ACT";
        case FPI_OP_RULE_FIRED: return "RULE_FIRED";
        default: return "UNKNOWN";
    }
}
