#include "rpyl_vm.h"
#include "rpyl_common.h"

int rpyl_vm_run(RpylRuntime* rt, const RpylBytecode* bc, const char* entry_label) {
    return rpyl_runtime_execute_bytecode(rt, bc, entry_label);
}

int rpyl_vm_find_label(const RpylBytecode* bc, const char* label_name) {
    size_t i;
    const char* s;
    if (!bc || !label_name) return -1;
    for (i = 0u; i < bc->label_count; i++) {
        if ((size_t)bc->labels[i].name_sid >= bc->string_count) continue;
        s = bc->strings[bc->labels[i].name_sid];
        if (rpyl_common_streq(s, label_name)) return (int)i;
    }
    return -1;
}

const char* rpyl_vm_label_name(const RpylBytecode* bc, int label_index) {
    size_t idx;
    rpyl_u32 sid;
    if (!bc || label_index < 0) return 0;
    idx = (size_t)label_index;
    if (idx >= bc->label_count) return 0;
    sid = bc->labels[idx].name_sid;
    if ((size_t)sid >= bc->string_count) return 0;
    return bc->strings[sid];
}

int rpyl_vm_begin(RpylVmCursor* cur, const RpylBytecode* bc, const char* entry_label) {
    int label_index;
    if (!cur || !bc || !entry_label) return 0;
    label_index = rpyl_vm_find_label(bc, entry_label);
    if (label_index < 0) return 0;
    cur->bc = bc;
    cur->label_index = label_index;
    cur->ip = bc->labels[label_index].ip;
    cur->end_ip = bc->labels[label_index].end_ip;
    cur->active = 1;
    if (cur->ip > cur->end_ip || cur->end_ip > (rpyl_u32)bc->code_count) {
        cur->active = 0;
        return 0;
    }
    return 1;
}

int rpyl_vm_step_opcode(RpylVmCursor* cur, rpyl_u32* opcode_out, rpyl_u32* ip_out) {
    rpyl_u32 next;
    rpyl_u32 ip;
    if (!cur || !cur->active || !cur->bc) return 0;
    if (cur->ip >= cur->end_ip) {
        cur->active = 0;
        return 0;
    }
    ip = cur->ip;
    if (!rpyl_opcode_next_ip(cur->bc, ip, &next)) {
        cur->active = 0;
        return 0;
    }
    if (opcode_out) *opcode_out = cur->bc->code[ip];
    if (ip_out) *ip_out = ip;
    cur->ip = next;
    if (cur->ip >= cur->end_ip) cur->active = 0;
    return 1;
}

static int rpyl_vm_validate_operands(const RpylBytecode* bc, rpyl_u32 start_ip, rpyl_u32 end_ip) {
    rpyl_u32 ip;
    rpyl_u32 next;
    rpyl_u32 op;
    rpyl_u32 argc;
    rpyl_u32 i;
    rpyl_u32 skip_ip;

    ip = start_ip;
    while (ip < end_ip) {
        if (!rpyl_opcode_next_ip(bc, ip, &next)) return 0;
        if (next <= ip || next > end_ip) return 0;
        op = bc->code[ip];

        if (op == (rpyl_u32)RPYL_BC_OP_CMD) {
            if ((size_t)bc->code[ip + 1u] >= bc->string_count) return 0;
            argc = bc->code[ip + 2u];
            for (i = 0u; i < argc; i++) {
                if ((size_t)bc->code[ip + 3u + i] >= bc->string_count) return 0;
            }
        } else if (op == (rpyl_u32)RPYL_BC_OP_SET) {
            if ((bc->code[ip + 1u] & ~(rpyl_u32)RPYL_BC_SET_GLOBAL) != 0u) return 0;
            if ((size_t)bc->code[ip + 2u] >= bc->string_count) return 0;
            argc = bc->code[ip + 3u];
            for (i = 0u; i < argc; i++) {
                if ((size_t)bc->code[ip + 4u + i] >= bc->string_count) return 0;
            }
        } else if (op == (rpyl_u32)RPYL_BC_OP_CALL || op == (rpyl_u32)RPYL_BC_OP_JUMP) {
            argc = bc->code[ip + 1u];
            for (i = 0u; i < argc; i++) {
                if ((size_t)bc->code[ip + 2u + i] >= bc->string_count) return 0;
            }
        } else if (op == (rpyl_u32)RPYL_BC_OP_ONCE_CHECK) {
            skip_ip = bc->code[ip + 2u];
            if (skip_ip < next || skip_ip > end_ip) return 0;
        } else if (op == (rpyl_u32)RPYL_BC_OP_ON_ENTER_CHECK) {
            skip_ip = bc->code[ip + 1u];
            if (skip_ip < next || skip_ip > end_ip) return 0;
        }
        ip = next;
    }
    return ip == end_ip ? 1 : 0;
}

int rpyl_vm_validate(const RpylBytecode* bc) {
    size_t i;
    if (!bc) return 0;
    if (bc->version != (rpyl_u32)RPYL_BC_VERSION) return 0;
    if (!bc->code || !bc->strings || !bc->labels || !bc->defines) return 0;
    if (bc->code_count > bc->code_cap) return 0;
    if (bc->string_count > bc->string_cap) return 0;
    if (bc->label_count > bc->label_cap) return 0;
    if (bc->define_count > bc->define_cap) return 0;
    if (bc->code_count > (size_t)RPYL_BC_MAX_CODE) return 0;
    if (bc->string_count > (size_t)RPYL_BC_MAX_STRINGS) return 0;
    if (bc->label_count > (size_t)RPYL_BC_MAX_LABELS) return 0;
    if (bc->define_count > (size_t)RPYL_BC_MAX_DEFINES) return 0;
    for (i = 0u; i < bc->string_count; i++) {
        if (!bc->strings[i]) return 0;
    }
    for (i = 0u; i < bc->define_count; i++) {
        if ((size_t)bc->defines[i].name_sid >= bc->string_count) return 0;
        if ((size_t)bc->defines[i].value_sid >= bc->string_count) return 0;
    }
    for (i = 0u; i < bc->label_count; i++) {
        size_t j;
        rpyl_u32 bad;
        if ((size_t)bc->labels[i].name_sid >= bc->string_count) return 0;
        if (bc->labels[i].ip > bc->labels[i].end_ip) return 0;
        if ((size_t)bc->labels[i].end_ip > bc->code_count) return 0;
        if (!rpyl_opcode_validate_stream(bc, bc->labels[i].ip, bc->labels[i].end_ip, &bad)) return 0;
        if (!rpyl_vm_validate_operands(bc, bc->labels[i].ip, bc->labels[i].end_ip)) return 0;
        for (j = i + 1u; j < bc->label_count; j++) {
            if ((size_t)bc->labels[j].name_sid >= bc->string_count) return 0;
            if (rpyl_common_streq(bc->strings[bc->labels[i].name_sid], bc->strings[bc->labels[j].name_sid])) return 0;
        }
    }
    return 1;
}
