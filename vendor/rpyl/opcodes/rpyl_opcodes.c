#include "rpyl_opcodes.h"
#include "rpyl_common.h"

const char* rpyl_opcode_name(unsigned long opcode) {
    switch (opcode) {
    case RPYL_BC_OP_NOP: return "NOP";
    case RPYL_BC_OP_CMD: return "CMD";
    case RPYL_BC_OP_SET: return "SET";
    case RPYL_BC_OP_CALL: return "CALL";
    case RPYL_BC_OP_JUMP: return "JUMP";
    case RPYL_BC_OP_RETURN: return "RETURN";
    case RPYL_BC_OP_ONCE_CHECK: return "ONCE_CHECK";
    case RPYL_BC_OP_ON_ENTER_CHECK: return "ON_ENTER_CHECK";
    case RPYL_BC_OP_END: return "END";
    default: return "UNKNOWN";
    }
}

int rpyl_opcode_operand_count(unsigned long opcode) {
    switch (opcode) {
    case RPYL_BC_OP_NOP: return 0;
    case RPYL_BC_OP_RETURN: return 0;
    case RPYL_BC_OP_END: return 0;
    case RPYL_BC_OP_ONCE_CHECK: return 2;
    case RPYL_BC_OP_ON_ENTER_CHECK: return 1;
    default: return -1;
    }
}

int rpyl_opcode_is_control(unsigned long opcode) {
    if (opcode == (unsigned long)RPYL_BC_OP_CALL) return 1;
    if (opcode == (unsigned long)RPYL_BC_OP_JUMP) return 1;
    if (opcode == (unsigned long)RPYL_BC_OP_RETURN) return 1;
    if (opcode == (unsigned long)RPYL_BC_OP_END) return 1;
    return 0;
}

static int checked_add(rpyl_u32 a, rpyl_u32 b, rpyl_u32 limit, rpyl_u32* out) {
    rpyl_u32 r;
    r = a + b;
    if (r < a) return 0;
    if (r > limit) return 0;
    if (out) *out = r;
    return 1;
}

int rpyl_opcode_next_ip(const RpylBytecode* bc, rpyl_u32 ip, rpyl_u32* next_ip) {
    rpyl_u32 op;
    rpyl_u32 argc;
    rpyl_u32 limit;
    if (!bc || !next_ip) return 0;
    limit = (rpyl_u32)bc->code_count;
    if (ip >= limit) return 0;
    op = bc->code[ip++];
    if (op == (rpyl_u32)RPYL_BC_OP_NOP || op == (rpyl_u32)RPYL_BC_OP_RETURN || op == (rpyl_u32)RPYL_BC_OP_END) {
        *next_ip = ip;
        return 1;
    }
    if (op == (rpyl_u32)RPYL_BC_OP_ONCE_CHECK) return checked_add(ip, 2UL, limit, next_ip);
    if (op == (rpyl_u32)RPYL_BC_OP_ON_ENTER_CHECK) return checked_add(ip, 1UL, limit, next_ip);
    if (op == (rpyl_u32)RPYL_BC_OP_CMD) {
        if (!checked_add(ip, 2UL, limit, &ip)) return 0;
        argc = bc->code[ip - 1UL];
        return checked_add(ip, argc, limit, next_ip);
    }
    if (op == (rpyl_u32)RPYL_BC_OP_SET) {
        if (!checked_add(ip, 3UL, limit, &ip)) return 0;
        argc = bc->code[ip - 1UL];
        return checked_add(ip, argc, limit, next_ip);
    }
    if (op == (rpyl_u32)RPYL_BC_OP_CALL || op == (rpyl_u32)RPYL_BC_OP_JUMP) {
        if (ip >= limit) return 0;
        argc = bc->code[ip++];
        return checked_add(ip, argc, limit, next_ip);
    }
    return 0;
}

int rpyl_opcode_validate_stream(const RpylBytecode* bc, rpyl_u32 start_ip, rpyl_u32 end_ip, rpyl_u32* bad_ip) {
    rpyl_u32 ip;
    rpyl_u32 next;
    if (!bc) return 0;
    if (start_ip > end_ip || end_ip > (rpyl_u32)bc->code_count) {
        if (bad_ip) *bad_ip = start_ip;
        return 0;
    }
    ip = start_ip;
    while (ip < end_ip) {
        if (!rpyl_opcode_next_ip(bc, ip, &next)) {
            if (bad_ip) *bad_ip = ip;
            return 0;
        }
        if (next <= ip) {
            if (bad_ip) *bad_ip = ip;
            return 0;
        }
        ip = next;
    }
    return ip == end_ip ? 1 : 0;
}

static void append_num(char* out, size_t out_size, unsigned long v) {
    char tmp[32];
    char rev[32];
    size_t i;
    size_t n;
    if (v == 0UL) {
        (void)rpyl_common_append(out, out_size, "0");
        return;
    }
    n = 0u;
    while (v > 0UL && n < sizeof(rev)) {
        rev[n++] = (char)('0' + (int)(v % 10UL));
        v /= 10UL;
    }
    for (i = 0u; i < n; i++) tmp[i] = rev[n - 1u - i];
    tmp[n] = '\0';
    (void)rpyl_common_append(out, out_size, tmp);
}

int rpyl_opcode_format(const RpylBytecode* bc, rpyl_u32 ip, char* out, size_t out_size) {
    rpyl_u32 op;
    rpyl_u32 next;
    if (!out || out_size == 0u) return 0;
    out[0] = '\0';
    if (!bc || ip >= (rpyl_u32)bc->code_count) return 0;
    op = bc->code[ip];
    (void)rpyl_common_append(out, out_size, rpyl_opcode_name((unsigned long)op));
    (void)rpyl_common_append(out, out_size, " @");
    append_num(out, out_size, (unsigned long)ip);
    if (rpyl_opcode_next_ip(bc, ip, &next)) {
        (void)rpyl_common_append(out, out_size, " -> ");
        append_num(out, out_size, (unsigned long)next);
    }
    return 1;
}
