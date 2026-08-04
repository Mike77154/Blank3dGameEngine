#ifndef DDSL_OPCODES_H
#define DDSL_OPCODES_H

#include "bytecode/bytecode.h"
#include "IR/ir.h"

#ifdef __cplusplus
extern "C" {
#endif

const char *ddsl_opcode_bc_name(ddsl_bc_op op);
const char *ddsl_opcode_ir_name(ddsl_ir_op op);
int ddsl_opcode_is_jump(ddsl_bc_op op);

#ifdef __cplusplus
}
#endif

#endif /* DDSL_OPCODES_H */
