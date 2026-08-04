#ifndef RPYL_VM_H
#define RPYL_VM_H

#include "rpyl_runtime.h"
#include "rpyl_bytecode.h"
#include "rpyl_opcodes.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RpylVmCursor {
    const RpylBytecode* bc;
    int label_index;
    rpyl_u32 ip;
    rpyl_u32 end_ip;
    int active;
} RpylVmCursor;

int rpyl_vm_run(RpylRuntime* rt, const RpylBytecode* bc, const char* entry_label);
int rpyl_vm_find_label(const RpylBytecode* bc, const char* label_name);
const char* rpyl_vm_label_name(const RpylBytecode* bc, int label_index);
int rpyl_vm_begin(RpylVmCursor* cur, const RpylBytecode* bc, const char* entry_label);
int rpyl_vm_step_opcode(RpylVmCursor* cur, rpyl_u32* opcode_out, rpyl_u32* ip_out);
int rpyl_vm_validate(const RpylBytecode* bc);

#ifdef __cplusplus
}
#endif

#endif
