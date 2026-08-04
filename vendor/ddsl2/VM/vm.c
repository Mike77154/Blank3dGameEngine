#include "VM/vm.h"

#include "common/strview.h"

#include <ctype.h>
#include <string.h>

static int sv_eq_ci(ddsl_strview sv, const char *cstr) {
    int i;
    int n;

    if (!cstr) cstr = "";
    n = (int)strlen(cstr);
    if (sv.len != n) return 0;

    for (i = 0; i < n; ++i) {
        char a = (char)tolower((unsigned char)sv.data[i]);
        char b = (char)tolower((unsigned char)cstr[i]);
        if (a != b) return 0;
    }
    return 1;
}

static int sv_to_cstr_trunc(ddsl_strview sv, char *dst, int dst_cap) {
    int n;
    if (!dst || dst_cap <= 0) return 0;
    n = sv.len;
    if (n < 0) n = 0;
    if (n > dst_cap - 1) n = dst_cap - 1;
    if (n > 0 && sv.data) memcpy(dst, sv.data, (size_t)n);
    dst[n] = '\0';
    return n;
}

static int ci_cmp_sv(ddsl_strview a, ddsl_strview b) {
    int i;
    int n;
    int na;
    int nb;

    na = a.len;
    nb = b.len;
    if (na < 0) na = 0;
    if (nb < 0) nb = 0;

    n = (na < nb) ? na : nb;
    for (i = 0; i < n; ++i) {
        char ca = (char)tolower((unsigned char)a.data[i]);
        char cb = (char)tolower((unsigned char)b.data[i]);
        if (ca < cb) return -1;
        if (ca > cb) return 1;
    }

    if (na < nb) return -1;
    if (na > nb) return 1;
    return 0;
}

static int store_get_value(ddsl_store *st, ddsl_strview ident, ddsl_value *out) {
    char keybuf[DDSL_MAX_KEY_LEN];
    const char *cur;

    if (!st || !out) return 0;

    /* bool literals */
    if (sv_eq_ci(ident, "true")) { *out = ddsl_v_bool(1); return 1; }
    if (sv_eq_ci(ident, "false")) { *out = ddsl_v_bool(0); return 1; }

    sv_to_cstr_trunc(ident, keybuf, (int)sizeof(keybuf));
    cur = ddsl_store_get(st, keybuf);
    if (cur) {
        ddsl_fixed d;
        if (ddsl_fixed_parse_cstr(cur, &d)) {
            *out = ddsl_v_num(d);
        } else {
            *out = ddsl_v_str(ddsl_sv_from_cstr(cur));
        }
        return 1;
    }

    return 0;
}

static int test_ident(ddsl_store *st, ddsl_strview ident) {
    ddsl_value v;
    if (!st) return 0;
    if (sv_eq_ci(ident, "true")) return 1;
    if (sv_eq_ci(ident, "false")) return 0;
    if (!store_get_value(st, ident, &v)) return 0;
    return ddsl_value_truthy(v);
}

static int vm_push(ddsl_vm *vm, ddsl_value v, ddsl_error *err) {
    if (!vm) return 0;
    if (vm->sp >= DDSL_VM_STACK_MAX) {
        if (err) ddsl_error_set(err, 0, 0, 0, "vm: stack overflow" );
        return 0;
    }
    vm->stack[vm->sp++] = v;
    return 1;
}

static int vm_pop(ddsl_vm *vm, ddsl_value *out, ddsl_error *err) {
    if (!vm || !out) return 0;
    if (vm->sp <= 0) {
        if (err) ddsl_error_set(err, 0, 0, 0, "vm: stack underflow" );
        return 0;
    }
    *out = vm->stack[--vm->sp];
    return 1;
}

static int do_emit(ddsl_vm *vm, const char *key_norm, ddsl_value v) {
    if (!vm || !vm->emit) return 1;
    return vm->emit(vm->emit_user, key_norm, v);
}

void ddsl_vm_init(ddsl_vm *vm, ddsl_store *store) {
    if (!vm) return;
    vm->store = store;
    vm->emit = NULL;
    vm->emit_user = NULL;
    vm->sp = 0;
}

void ddsl_vm_set_emit(ddsl_vm *vm, ddsl_emit_fn fn, void *user) {
    if (!vm) return;
    vm->emit = fn;
    vm->emit_user = user;
}

static int eval_cmp(ddsl_bc_op op, ddsl_value a, ddsl_value b) {
    int ok1;
    int ok2;
    ddsl_fixed n1;
    ddsl_fixed n2;

    ok1 = 0;
    ok2 = 0;
    n1 = ddsl_value_to_num(a, &ok1);
    n2 = ddsl_value_to_num(b, &ok2);

    if (op == DDSL_BC_CMP_EQ || op == DDSL_BC_CMP_NEQ) {
        if (ok1 && ok2) {
            int eq = (n1 == n2) ? 1 : 0;
            return (op == DDSL_BC_CMP_EQ) ? eq : (!eq);
        }

        /* string compare (case-insensitive) */
        {
            ddsl_strview sa;
            ddsl_strview sb;
            char ba[64];
            char bb[64];

            if (a.kind == DDSL_VAL_STR) sa = a.str;
            else {
                ddsl_value_to_cstr(a, ba, (int)sizeof(ba));
                sa = ddsl_sv_from_cstr(ba);
            }

            if (b.kind == DDSL_VAL_STR) sb = b.str;
            else {
                ddsl_value_to_cstr(b, bb, (int)sizeof(bb));
                sb = ddsl_sv_from_cstr(bb);
            }

            {
                int eq2 = (ci_cmp_sv(sa, sb) == 0) ? 1 : 0;
                return (op == DDSL_BC_CMP_EQ) ? eq2 : (!eq2);
            }
        }
    }

    /* orden: solo num */
    if (!(ok1 && ok2)) return 0;

    if (op == DDSL_BC_CMP_LT) return (n1 < n2) ? 1 : 0;
    if (op == DDSL_BC_CMP_LTE) return (n1 <= n2) ? 1 : 0;
    if (op == DDSL_BC_CMP_GT) return (n1 > n2) ? 1 : 0;
    if (op == DDSL_BC_CMP_GTE) return (n1 >= n2) ? 1 : 0;

    return 0;
}

int ddsl_vm_run(ddsl_vm *vm, const ddsl_bc_program *prog, ddsl_error *err) {
    int ip;
    int guard;
    ddsl_bc_ins in;

    if (err) ddsl_error_clear(err);
    if (!vm || !vm->store || !prog || !prog->code || prog->count <= 0) {
        if (err) ddsl_error_set(err, 0, 0, 0, "vm: argumentos inválidos" );
        return 0;
    }

    vm->sp = 0;
    ip = 0;
    guard = 0;

    while (ip >= 0 && ip < prog->count) {
        guard++;
        if (guard > 4096) {
            if (err) ddsl_error_set(err, 0, 0, 0, "vm: instruccion limite, posible loop" );
            return 0;
        }
        in = prog->code[ip];

        switch (in.op) {
            case DDSL_BC_NOP:
                ip++;
                break;

            case DDSL_BC_PUSH_NUM:
                if (!vm_push(vm, ddsl_v_num(in.num), err)) return 0;
                ip++;
                break;

            case DDSL_BC_PUSH_STR:
                if (!vm_push(vm, ddsl_v_str(in.sv), err)) return 0;
                ip++;
                break;

            case DDSL_BC_PUSH_BOOL:
                if (!vm_push(vm, ddsl_v_bool(in.a ? 1 : 0), err)) return 0;
                ip++;
                break;

            case DDSL_BC_LOAD_IDENT: {
                ddsl_value v;
                if (store_get_value(vm->store, in.sv, &v)) {
                    if (!vm_push(vm, v, err)) return 0;
                } else {
                    /* si no existe => string constante con el mismo nombre */
                    if (!vm_push(vm, ddsl_v_str(in.sv), err)) return 0;
                }
                ip++;
            } break;

            case DDSL_BC_TEST_IDENT: {
                int b;
                b = test_ident(vm->store, in.sv);
                if (!vm_push(vm, ddsl_v_bool(b), err)) return 0;
                ip++;
            } break;

            case DDSL_BC_NEG: {
                ddsl_value rhs;
                int ok;
                ddsl_fixed n;
                if (!vm_pop(vm, &rhs, err)) return 0;
                ok = 0;
                n = ddsl_value_to_num(rhs, &ok);
                if (!ok) n = DDSL_FIXED_ZERO;
                if (!vm_push(vm, ddsl_v_num(ddsl_fixed_neg(n)), err)) return 0;
                ip++;
            } break;

            case DDSL_BC_ADD:
            case DDSL_BC_SUB:
            case DDSL_BC_MUL:
            case DDSL_BC_DIV: {
                ddsl_value a;
                ddsl_value b;
                int ok1;
                int ok2;
                ddsl_fixed n1;
                ddsl_fixed n2;
                ddsl_fixed r;

                if (!vm_pop(vm, &b, err)) return 0;
                if (!vm_pop(vm, &a, err)) return 0;

                ok1 = 0;
                ok2 = 0;
                n1 = ddsl_value_to_num(a, &ok1);
                n2 = ddsl_value_to_num(b, &ok2);
                if (!ok1) n1 = DDSL_FIXED_ZERO;
                if (!ok2) n2 = DDSL_FIXED_ZERO;

                r = DDSL_FIXED_ZERO;
                if (in.op == DDSL_BC_ADD) r = ddsl_fixed_add(n1, n2);
                else if (in.op == DDSL_BC_SUB) r = ddsl_fixed_sub(n1, n2);
                else if (in.op == DDSL_BC_MUL) r = ddsl_fixed_mul(n1, n2);
                else if (in.op == DDSL_BC_DIV) {
                    if (n2 == DDSL_FIXED_ZERO) r = DDSL_FIXED_ZERO;
                    else r = ddsl_fixed_div(n1, n2);
                }

                if (!vm_push(vm, ddsl_v_num(r), err)) return 0;
                ip++;
            } break;

            case DDSL_BC_CMP_EQ:
            case DDSL_BC_CMP_NEQ:
            case DDSL_BC_CMP_LT:
            case DDSL_BC_CMP_LTE:
            case DDSL_BC_CMP_GT:
            case DDSL_BC_CMP_GTE: {
                ddsl_value a;
                ddsl_value b;
                int r;
                if (!vm_pop(vm, &b, err)) return 0;
                if (!vm_pop(vm, &a, err)) return 0;
                r = eval_cmp(in.op, a, b);
                if (!vm_push(vm, ddsl_v_bool(r), err)) return 0;
                ip++;
            } break;

            case DDSL_BC_TRUTHY: {
                ddsl_value v;
                int b;
                if (!vm_pop(vm, &v, err)) return 0;
                b = ddsl_value_truthy(v);
                if (!vm_push(vm, ddsl_v_bool(b), err)) return 0;
                ip++;
            } break;

            case DDSL_BC_POP: {
                ddsl_value v;
                if (!vm_pop(vm, &v, err)) return 0;
                ip++;
            } break;

            case DDSL_BC_JMP:
                ip = in.a;
                break;

            case DDSL_BC_JMP_IF_FALSE: {
                ddsl_value v;
                int b;
                if (!vm_pop(vm, &v, err)) return 0;
                b = ddsl_value_truthy(v);
                if (!b) ip = in.a;
                else ip++;
            } break;

            case DDSL_BC_JMP_IF_TRUE: {
                ddsl_value v;
                int b;
                if (!vm_pop(vm, &v, err)) return 0;
                b = ddsl_value_truthy(v);
                if (b) ip = in.a;
                else ip++;
            } break;

            case DDSL_BC_STORE_TRUE: {
                const char *key;
                key = in.key ? in.key : "";
                if (!ddsl_store_set(vm->store, key, "true")) {
                    if (err) ddsl_error_set(err, 0, 0, 0, "vm: store lleno o escritura inválida");
                    return 0;
                }
                if (!do_emit(vm, key, ddsl_v_bool(1))) {
                    if (err) ddsl_error_set(err, 0, 0, 0, "vm: abortado por callback" );
                    return 0;
                }
                ip++;
            } break;

            case DDSL_BC_STORE_SET: {
                const char *key;
                ddsl_value v;
                char valbuf[DDSL_MAX_VALUE_LEN];

                key = in.key ? in.key : "";
                if (!vm_pop(vm, &v, err)) return 0;
                ddsl_value_to_cstr(v, valbuf, (int)sizeof(valbuf));
                if (!ddsl_store_set(vm->store, key, valbuf)) {
                    if (err) ddsl_error_set(err, 0, 0, 0, "vm: store lleno o escritura inválida");
                    return 0;
                }
                if (!do_emit(vm, key, v)) {
                    if (err) ddsl_error_set(err, 0, 0, 0, "vm: abortado por callback" );
                    return 0;
                }
                ip++;
            } break;

            case DDSL_BC_END:
                return 1;

            default:
                if (err) ddsl_error_set(err, 0, 0, 0, "vm: opcode desconocido" );
                return 0;
        }
    }

    return 1;
}

int ddsl_vm_exec_source(ddsl_vm *vm, ddsl_arena *arena, const char *source, ddsl_error *err) {
    ddsl_bc_program *bc;
    size_t mark;
    int ok;

    if (err) ddsl_error_clear(err);
    if (!vm || !arena || !source) {
        if (err) ddsl_error_set(err, 0, 0, 0, "vm_exec: argumentos inválidos" );
        return 0;
    }

    mark = ddsl_arena_mark(arena);

    bc = NULL;
    if (!ddsl_compile_source_to_bytecode(arena, source, &bc, err)) {
        ddsl_arena_rewind(arena, mark);
        return 0;
    }

    ok = ddsl_vm_run(vm, bc, err);
    ddsl_arena_rewind(arena, mark);
    return ok;
}
