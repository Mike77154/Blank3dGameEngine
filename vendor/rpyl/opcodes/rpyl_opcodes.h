#ifndef RPYL_OPCODES_H
#define RPYL_OPCODES_H

#include "rpyl_bytecode.h"

#ifdef __cplusplus
extern "C" {
#endif

const char* rpyl_opcode_name(unsigned long opcode);
int rpyl_opcode_operand_count(unsigned long opcode);
int rpyl_opcode_is_control(unsigned long opcode);
int rpyl_opcode_next_ip(const RpylBytecode* bc, rpyl_u32 ip, rpyl_u32* next_ip);
int rpyl_opcode_validate_stream(const RpylBytecode* bc, rpyl_u32 start_ip, rpyl_u32 end_ip, rpyl_u32* bad_ip);
int rpyl_opcode_format(const RpylBytecode* bc, rpyl_u32 ip, char* out, size_t out_size);

#ifdef __cplusplus
}
#endif

#endif
