#ifndef FPI_OPCODES_H
#define FPI_OPCODES_H

typedef enum FPI_Opcode {
    FPI_OP_HALT = 0,
    FPI_OP_EVAL_COND = 1,
    FPI_OP_JUMP_IF_FALSE = 2,
    FPI_OP_EXEC_ACT = 3,
    FPI_OP_RULE_FIRED = 4
} FPI_Opcode;

const char* fpi_opcode_name(int opcode);

#endif /* FPI_OPCODES_H */
