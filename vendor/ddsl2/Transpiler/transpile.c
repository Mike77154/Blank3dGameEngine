#include "Transpiler/transpile.h"

#include "common/strview.h"

#include <string.h>
#include <stdio.h> /* sprintf */

typedef struct sbuf {
    char *out;
    int cap;
    int len;
    int overflow;
} sbuf;

static void sb_init(sbuf *b, char *out, int cap) {
    if (!b) return;
    b->out = out;
    b->cap = cap;
    b->len = 0;
    b->overflow = 0;
    if (b->out && b->cap > 0) b->out[0] = '\0';
}

static void sb_putc(sbuf *b, char c) {
    if (!b || !b->out || b->cap <= 0) return;
    if (b->len + 1 >= b->cap) { b->overflow = 1; return; }
    b->out[b->len++] = c;
    b->out[b->len] = '\0';
}

static void sb_puts(sbuf *b, const char *s) {
    int i;
    if (!b || !s) return;
    for (i = 0; s[i]; ++i) {
        sb_putc(b, s[i]);
        if (b->overflow) return;
    }
}

static void sb_put_int(sbuf *b, int v) {
    char tmp[64];
    sprintf(tmp, "%d", v);
    sb_puts(b, tmp);
}

static void sb_put_long(sbuf *b, long v) {
    char tmp[64];
    sprintf(tmp, "%ld", v);
    sb_puts(b, tmp);
}

static void sb_put_fixed_raw(sbuf *b, ddsl_fixed v) {
    sb_puts(b, "((ddsl_fixed)");
    sb_put_long(b, (long)v);
    sb_puts(b, "L)");
}

static void sb_put_cstr_lit_from_sv(sbuf *b, ddsl_strview sv) {
    int i;
    sb_putc(b, '"');
    for (i = 0; i < sv.len; ++i) {
        unsigned char ch;
        ch = (unsigned char)sv.data[i];
        if (ch == '"') { sb_puts(b, "\\\""); continue; }
        if (ch == '\\') { sb_puts(b, "\\\\"); continue; }
        if (ch == '\n') { sb_puts(b, "\\n"); continue; }
        if (ch == '\r') { sb_puts(b, "\\r"); continue; }
        if (ch == '\t') { sb_puts(b, "\\t"); continue; }
        if (ch < 32 || ch > 126) {
            char tmp[8];
            /* Tres dígitos octales exactos: el siguiente carácter nunca
             * puede extender accidentalmente el escape, a diferencia de \xNN.
             */
            sprintf(tmp, "\\%03o", (unsigned int)ch);
            sb_puts(b, tmp);
            continue;
        }
        sb_putc(b, (char)ch);
        if (b->overflow) break;
    }
    sb_putc(b, '"');
}

static void sb_put_cstr_lit(sbuf *b, const char *s) {
    ddsl_strview sv;
    if (!s) s = "";
    sv = ddsl_sv_from_cstr(s);
    sb_put_cstr_lit_from_sv(b, sv);
}

static void emit_ins(sbuf *b, const ddsl_bc_ins *in) {
    /* ddsl_bc_ins: { op, a, num, sv, key } */
    sb_puts(b, "    { ");
    sb_puts(b, ddsl_bc_op_name(in->op));
    sb_puts(b, ", ");
    sb_put_int(b, in->a);
    sb_puts(b, ", ");
    sb_put_fixed_raw(b, in->num);
    sb_puts(b, ", { ");
    if (in->sv.data && in->sv.len > 0) {
        sb_put_cstr_lit_from_sv(b, in->sv);
        sb_puts(b, ", ");
        sb_put_int(b, in->sv.len);
    } else {
        sb_puts(b, "0, 0");
    }
    sb_puts(b, " }, ");
    if (in->key) sb_put_cstr_lit(b, in->key);
    else sb_puts(b, "0");
    sb_puts(b, " },\n");
}

int ddsl_transpile_bytecode_to_c(const ddsl_bc_program *bc,
                                 const ddsl_transpile_opts *opts,
                                 char *out,
                                 int out_cap,
                                 ddsl_error *err) {
    sbuf b;
    const char *func_name;
    const char *prog_name;
    int i;

    if (err) ddsl_error_clear(err);

    if (!bc || !bc->code || bc->count <= 0 || !out || out_cap <= 0) {
        if (err) ddsl_error_set(err, 0, 0, 0, "transpile: argumentos inválidos" );
        return 0;
    }

    func_name = (opts && opts->func_name) ? opts->func_name : "ddsl2_script_run";
    prog_name = (opts && opts->prog_name) ? opts->prog_name : "ddsl2_script_prog";

    sb_init(&b, out, out_cap);

    sb_puts(&b, "/* Auto-generado por ddsl_transpile_bytecode_to_c() */\n");
    sb_puts(&b, "#include \"VM/vm.h\"\n\n");
    sb_puts(&b, "static const ddsl_bc_ins ddsl2_code[] = {\n");

    for (i = 0; i < bc->count; ++i) {
        emit_ins(&b, &bc->code[i]);
        if (b.overflow) break;
    }

    sb_puts(&b, "};\n\n");
    sb_puts(&b, "static const ddsl_bc_program ");
    sb_puts(&b, prog_name);
    sb_puts(&b, " = { (ddsl_bc_ins*)ddsl2_code, ");
    sb_put_int(&b, bc->count);
    sb_puts(&b, " };\n\n");

    sb_puts(&b, "int ");
    sb_puts(&b, func_name);
    sb_puts(&b, "(ddsl_vm *vm, ddsl_error *err) {\n");
    sb_puts(&b, "    return ddsl_vm_run(vm, &");
    sb_puts(&b, prog_name);
    sb_puts(&b, ", err);\n");
    sb_puts(&b, "}\n");

    if (b.overflow) {
        if (err) ddsl_error_set(err, 0, 0, 0, "transpile: buffer de salida insuficiente" );
        return 0;
    }

    return 1;
}
