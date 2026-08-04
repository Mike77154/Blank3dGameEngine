#include "bvh_vm.h"

static unsigned long bvh_read_u16(const unsigned char *p) {
    unsigned long v;
    v = (unsigned long)p[0];
    v |= ((unsigned long)p[1]) << 8;
    return v;
}

static unsigned long bvh_read_u32(const unsigned char *p) {
    unsigned long v;
    v = (unsigned long)p[0];
    v |= ((unsigned long)p[1]) << 8;
    v |= ((unsigned long)p[2]) << 16;
    v |= ((unsigned long)p[3]) << 24;
    return v;
}

static long bvh_read_s32(const unsigned char *p) {
    unsigned long u;
    long v;
    u = bvh_read_u32(p);
    if (u & 0x80000000UL) {
        v = -(long)(((~u) + 1UL) & 0xFFFFFFFFUL);
    } else {
        v = (long)u;
    }
    return v;
}

static int bvh_take_u8(const unsigned char *data, unsigned long len, unsigned long *off, int *out) {
    if (*off + 1UL > len) return 0;
    *out = (int)data[*off];
    *off += 1UL;
    return 1;
}

static int bvh_take_s32(const unsigned char *data, unsigned long len, unsigned long *off, long *out) {
    if (*off + 4UL > len) return 0;
    *out = bvh_read_s32(data + *off);
    *off += 4UL;
    return 1;
}

static int bvh_take_u32(const unsigned char *data, unsigned long len, unsigned long *off, unsigned long *out) {
    if (*off + 4UL > len) return 0;
    *out = bvh_read_u32(data + *off);
    *off += 4UL;
    return 1;
}

static int bvh_take_str(const unsigned char *data, unsigned long len, unsigned long *off, const char **out) {
    unsigned long n;
    if (*off + 2UL > len) return 0;
    n = bvh_read_u16(data + *off);
    *off += 2UL;
    if (*off + n + 1UL > len) return 0;
    if (data[*off + n] != 0U) return 0;
    *out = (const char *)(data + *off);
    *off += n + 1UL;
    return 1;
}

static int bvh_take_value(const unsigned char *data, unsigned long len, unsigned long *off, BVH_Value *v) {
    int kind;
    long a;
    long b;
    long c;
    unsigned long u;
    int anchor;

    bvh_value_empty(v);
    if (!bvh_take_u8(data, len, off, &kind)) return 0;
    v->kind = (BVH_ValueKind)kind;
    switch (v->kind) {
        case BVH_VALUE_INT:
        case BVH_VALUE_FIXED:
            if (!bvh_take_s32(data, len, off, &a)) return 0;
            v->a = a;
            return 1;
        case BVH_VALUE_VEC2I:
        case BVH_VALUE_RANGE:
            if (!bvh_take_s32(data, len, off, &a)) return 0;
            if (!bvh_take_s32(data, len, off, &b)) return 0;
            v->a = a;
            v->b = b;
            return 1;
        case BVH_VALUE_COLOR:
            if (!bvh_take_u32(data, len, off, &u)) return 0;
            v->u = u;
            return 1;
        case BVH_VALUE_ANCHOR_POS:
            if (!bvh_take_u8(data, len, off, &anchor)) return 0;
            if (!bvh_take_s32(data, len, off, &b)) return 0;
            if (!bvh_take_s32(data, len, off, &c)) return 0;
            v->a = (long)anchor;
            v->b = b;
            v->c = c;
            return 1;
        case BVH_VALUE_EMPTY:
        case BVH_VALUE_RAW:
        case BVH_VALUE_SYMBOL:
            return 1;
        default:
            return 0;
    }
}

static void bvh_default_on_begin(void *user, int kind, const char *name) {
    FILE *out;
    out = (FILE *)user;
    if (!out) out = stdout;
    fprintf(out, "BEGIN %-9s name=%s\n", bvh_entity_kind_name(kind), name ? name : "");
}

static void bvh_default_on_end(void *user, int kind) {
    FILE *out;
    out = (FILE *)user;
    if (!out) out = stdout;
    fprintf(out, "END   %-9s\n", bvh_entity_kind_name(kind));
}

static void bvh_format_value_no_ctx(const BVH_Value *value, const char *raw, char *out, unsigned long cap) {
    char tmp[BVH_VALUE_TEXT_MAX];
    long whole;
    long part;
    unsigned long av;
    unsigned long rgba;
    int neg;

    if (!out || cap == 0UL) return;
    tmp[0] = '\0';
    if (!value) {
        bvh_copy_slice(out, cap, "", 0UL);
        return;
    }
    switch (value->kind) {
        case BVH_VALUE_INT:
            sprintf(tmp, "%ld", value->a);
            break;
        case BVH_VALUE_FIXED:
            neg = value->a < 0L ? 1 : 0;
            av = neg ? (unsigned long)(-value->a) : (unsigned long)value->a;
            whole = (long)(av >> BVH_FIXED_SHIFT);
            part = (long)((((av & 0xFFFFUL) * 1000UL) + 32768UL) >> BVH_FIXED_SHIFT);
            if (part >= 1000L) {
                whole++;
                part = 0L;
            }
            if (neg) sprintf(tmp, "-%ld.%03ld", whole, part);
            else sprintf(tmp, "%ld.%03ld", whole, part);
            break;
        case BVH_VALUE_VEC2I:
            sprintf(tmp, "%ld,%ld", value->a, value->b);
            break;
        case BVH_VALUE_COLOR:
            rgba = value->u;
            sprintf(tmp, "#%02lX%02lX%02lX%02lX", (rgba >> 24) & 255UL, (rgba >> 16) & 255UL, (rgba >> 8) & 255UL, rgba & 255UL);
            break;
        case BVH_VALUE_ANCHOR_POS:
            sprintf(tmp, "%s %+ld,%+ld", bvh_anchor_name((int)value->a), value->b, value->c);
            break;
        case BVH_VALUE_RANGE:
            sprintf(tmp, "%ld..%ld", value->a, value->b);
            break;
        default:
            bvh_copy_slice(tmp, (unsigned long)sizeof(tmp), raw ? raw : "", bvh_cstr_len(raw));
            break;
    }
    bvh_copy_slice(out, cap, tmp, bvh_cstr_len(tmp));
}

static void bvh_default_on_prop(void *user, const char *key, const char *raw, const BVH_Value *value) {
    FILE *out;
    char val[BVH_VALUE_TEXT_MAX];
    out = (FILE *)user;
    if (!out) out = stdout;
    bvh_format_value_no_ctx(value, raw, val, (unsigned long)sizeof(val));
    fprintf(out, "  PROP %-12s raw=%s kind=%s value=%s\n", key ? key : "", raw ? raw : "", value ? bvh_value_kind_name(value->kind) : "none", val);
}

static int bvh_header_ok(const unsigned char *data, unsigned long len) {
    if (len < 8UL) return 0;
    if (data[0] != (unsigned char)'B') return 0;
    if (data[1] != (unsigned char)'V') return 0;
    if (data[2] != (unsigned char)'H') return 0;
    if (data[3] != (unsigned char)'B') return 0;
    if (data[4] != (unsigned char)BVH_VERSION_MAJOR) return 0;
    return 1;
}

int bvh_vm_exec(const BVH_Bytecode *bc, const BVH_RuntimeCallbacks *callbacks, void *user, BVH_Error *err) {
    unsigned long off;
    const unsigned char *data;
    unsigned long len;
    BVH_RuntimeCallbacks cb;
    int stack[BVH_MAX_VM_STACK];
    int sp;

    if (err) {
        err->line = 0;
        err->col = 0;
        err->message[0] = '\0';
    }
    if (!bc) {
        bvh_error_set(err, 0, 0, "null bytecode");
        return 0;
    }
    data = bc->data;
    len = bc->len;
    if (!bvh_header_ok(data, len)) {
        bvh_error_set(err, 0, 0, "bad bytecode header");
        return 0;
    }

    if (callbacks) cb = *callbacks;
    else {
        cb.on_begin = bvh_default_on_begin;
        cb.on_end = bvh_default_on_end;
        cb.on_prop = bvh_default_on_prop;
    }

    off = 8UL;
    sp = 0;
    while (off < len) {
        int op;
        if (!bvh_take_u8(data, len, &off, &op)) {
            bvh_error_set(err, 0, 0, "truncated opcode");
            return 0;
        }

        if (op == BVH_OP_BEGIN) {
            int kind;
            const char *name;
            if (!bvh_take_u8(data, len, &off, &kind)) {
                bvh_error_set(err, 0, 0, "truncated begin");
                return 0;
            }
            if (!bvh_take_str(data, len, &off, &name)) {
                bvh_error_set(err, 0, 0, "bad begin string");
                return 0;
            }
            if (sp >= BVH_MAX_VM_STACK) {
                bvh_error_set(err, 0, 0, "VM stack limit reached");
                return 0;
            }
            stack[sp] = kind;
            sp++;
            if (cb.on_begin) cb.on_begin(user, kind, name);
            continue;
        }

        if (op == BVH_OP_END) {
            int kind2;
            if (!bvh_take_u8(data, len, &off, &kind2)) {
                bvh_error_set(err, 0, 0, "truncated end");
                return 0;
            }
            if (sp <= 0) {
                bvh_error_set(err, 0, 0, "VM stack underflow");
                return 0;
            }
            sp--;
            if (stack[sp] != kind2) {
                bvh_error_set(err, 0, 0, "VM end kind mismatch");
                return 0;
            }
            if (cb.on_end) cb.on_end(user, kind2);
            continue;
        }

        if (op == BVH_OP_PROP) {
            const char *key;
            const char *raw;
            BVH_Value value;
            if (!bvh_take_str(data, len, &off, &key)) {
                bvh_error_set(err, 0, 0, "bad property key string");
                return 0;
            }
            if (!bvh_take_str(data, len, &off, &raw)) {
                bvh_error_set(err, 0, 0, "bad property raw string");
                return 0;
            }
            if (!bvh_take_value(data, len, &off, &value)) {
                bvh_error_set(err, 0, 0, "bad property value");
                return 0;
            }
            if (cb.on_prop) cb.on_prop(user, key, raw, &value);
            continue;
        }

        bvh_error_set(err, 0, 0, "unknown opcode");
        return 0;
    }

    if (sp != 0) {
        bvh_error_set(err, 0, 0, "VM stack not empty");
        return 0;
    }
    return 1;
}

int bvh_exec_bytecode(const BVH_Bytecode *bc, const BVH_RuntimeCallbacks *callbacks, void *user, BVH_Error *err) {
    return bvh_vm_exec(bc, callbacks, user, err);
}
