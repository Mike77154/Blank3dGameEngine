#include "rpyl_transpile.h"
#include "rpyl_config.h"

#if RPYL_ENABLE_FILE_IO
#include "rpyl_port.h"
#include <stdio.h>
#endif
#include <string.h>

#if RPYL_ENABLE_FILE_IO
static void c_escape_and_write(FILE* f, const char* s) {
    const unsigned char* p;
    unsigned char c;

    if (!f) return;
    if (!s) s = "";
    fputc('"', f);
    p = (const unsigned char*)s;
    while (*p) {
        c = *p++;
        if (c == '\\') fputs("\\\\", f);
        else if (c == '"') fputs("\\\"", f);
        else if (c == '\n') fputs("\\n", f);
        else if (c == '\r') fputs("\\r", f);
        else if (c == '\t') fputs("\\t", f);
        else if (c < 32u || c > 126u) {
            static const char hex[] = "0123456789ABCDEF";
            fputs("\\x", f);
            fputc(hex[(c >> 4) & 0x0Fu], f);
            fputc(hex[c & 0x0Fu], f);
        } else {
            fputc((int)c, f);
        }
    }
    fputc('"', f);
}

static void safe_ident(char* out, size_t out_sz, const char* in) {
    size_t i;
    size_t j;

    if (!out || out_sz == 0) return;
    if (!in || !in[0]) {
        rpyl_strcpy_trunc(out, out_sz, "rpyl_program");
        return;
    }

    j = 0;
    for (i = 0; in[i] && j + 1 < out_sz; i++) {
        char c;
        c = in[i];
        if (j == 0) {
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_') out[j++] = c;
            else if (c >= '0' && c <= '9') {
                if (j + 2 < out_sz) {
                    out[j++] = '_';
                    out[j++] = c;
                }
            } else {
                out[j++] = '_';
            }
        } else {
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_') out[j++] = c;
            else out[j++] = '_';
        }
    }
    out[j] = 0;
    if (out[0] == 0) rpyl_strcpy_trunc(out, out_sz, "rpyl_program");
}

int rpyl_transpile_bytecode_to_c(const RpylBytecode* bc, const char* out_c_path, const char* symbol_prefix) {
    FILE* f;
    char pref[RPYL_TRANSPILE_MAX_PREFIX];
    size_t i;
    size_t sc;
    size_t cc;
    size_t lc;
    size_t dc;

    if (!bc || !out_c_path) return 0;
    safe_ident(pref, sizeof(pref), symbol_prefix);

    sc = bc->string_count ? bc->string_count : 1;
    cc = bc->code_count ? bc->code_count : 1;
    lc = bc->label_count ? bc->label_count : 1;
    dc = bc->define_count ? bc->define_count : 1;

    f = fopen(out_c_path, "wb");
    if (!f) return 0;

    fprintf(f, "/* Auto-generated RPYL program, C89. */\n");
    fprintf(f, "#include \"rpyl_bytecode.h\"\n");
    fprintf(f, "#include <string.h>\n\n");

    if (bc->string_count > 0) {
        fprintf(f, "static const char* %s_strings[%lu] = {\n", pref, (unsigned long)sc);
        for (i = 0; i < bc->string_count; i++) {
            fprintf(f, "    ");
            c_escape_and_write(f, bc->strings[i]);
            if (i + 1 < bc->string_count) fprintf(f, ",");
            fprintf(f, "\n");
        }
        fprintf(f, "};\n\n");
    }

    if (bc->code_count > 0) {
        fprintf(f, "static const rpyl_u32 %s_code[%lu] = {\n", pref, (unsigned long)cc);
        for (i = 0; i < bc->code_count; i++) {
            fprintf(f, "    %lu", (unsigned long)bc->code[i]);
            if (i + 1 < bc->code_count) fprintf(f, ",");
            fprintf(f, "\n");
        }
        fprintf(f, "};\n\n");
    }

    if (bc->label_count > 0) {
        fprintf(f, "static const RpylBcLabel %s_labels[%lu] = {\n", pref, (unsigned long)lc);
        for (i = 0; i < bc->label_count; i++) {
            fprintf(f, "    { %lu, %lu, %lu }", (unsigned long)bc->labels[i].name_sid, (unsigned long)bc->labels[i].ip, (unsigned long)bc->labels[i].end_ip);
            if (i + 1 < bc->label_count) fprintf(f, ",");
            fprintf(f, "\n");
        }
        fprintf(f, "};\n\n");
    }

    if (bc->define_count > 0) {
        fprintf(f, "static const RpylBcDefine %s_defines[%lu] = {\n", pref, (unsigned long)dc);
        for (i = 0; i < bc->define_count; i++) {
            fprintf(f, "    { %lu, %lu }", (unsigned long)bc->defines[i].name_sid, (unsigned long)bc->defines[i].value_sid);
            if (i + 1 < bc->define_count) fprintf(f, ",");
            fprintf(f, "\n");
        }
        fprintf(f, "};\n\n");
    }

    fprintf(f, "static RpylBytecode %s_bc;\n", pref);
    fprintf(f, "static int %s_ready = 0;\n\n", pref);

    fprintf(f, "static int %s_add_string(const char* s) {\n", pref);
    fprintf(f, "    size_t n;\n");
    fprintf(f, "    char* dst;\n");
    fprintf(f, "    if (!s) s = \"\";\n");
    fprintf(f, "    n = strlen(s);\n");
    fprintf(f, "    if (%s_bc.string_count >= (size_t)RPYL_BC_MAX_STRINGS) return 0;\n", pref);
    fprintf(f, "    if (n + 1u > (size_t)RPYL_BC_MAX_STRING_BYTES - %s_bc.string_bytes) return 0;\n", pref);
    fprintf(f, "    dst = %s_bc.string_data + %s_bc.string_bytes;\n", pref, pref);
    fprintf(f, "    if (n > 0u) memcpy(dst, s, n);\n");
    fprintf(f, "    dst[n] = 0;\n");
    fprintf(f, "    %s_bc.strings[%s_bc.string_count++] = dst;\n", pref, pref);
    fprintf(f, "    %s_bc.string_bytes += n + 1u;\n", pref);
    fprintf(f, "    return 1;\n");
    fprintf(f, "}\n\n");

    fprintf(f, "const RpylBytecode* %s_get_bytecode(void) {\n", pref);
    fprintf(f, "    size_t i;\n");
    fprintf(f, "    if (!%s_ready) {\n", pref);
    fprintf(f, "        rpyl_bytecode_clear(&%s_bc);\n", pref);
    fprintf(f, "        %s_bc.version = (rpyl_u32)RPYL_BC_VERSION;\n", pref);
    if (bc->string_count > 0) {
        fprintf(f, "        for (i = 0; i < (size_t)%lu; i++) {\n", (unsigned long)bc->string_count);
        fprintf(f, "            if (!%s_add_string(%s_strings[i])) return (const RpylBytecode*)0;\n", pref, pref);
        fprintf(f, "        }\n");
    }
    if (bc->code_count > 0) {
        fprintf(f, "        for (i = 0; i < (size_t)%lu; i++) %s_bc.code[i] = %s_code[i];\n", (unsigned long)bc->code_count, pref, pref);
    }
    fprintf(f, "        %s_bc.code_count = (size_t)%lu;\n", pref, (unsigned long)bc->code_count);
    if (bc->label_count > 0) {
        fprintf(f, "        for (i = 0; i < (size_t)%lu; i++) %s_bc.labels[i] = %s_labels[i];\n", (unsigned long)bc->label_count, pref, pref);
    }
    fprintf(f, "        %s_bc.label_count = (size_t)%lu;\n", pref, (unsigned long)bc->label_count);
    if (bc->define_count > 0) {
        fprintf(f, "        for (i = 0; i < (size_t)%lu; i++) %s_bc.defines[i] = %s_defines[i];\n", (unsigned long)bc->define_count, pref, pref);
    }
    fprintf(f, "        %s_bc.define_count = (size_t)%lu;\n", pref, (unsigned long)bc->define_count);
    fprintf(f, "        %s_bc.next_once_id = %d;\n", pref, bc->next_once_id);
    fprintf(f, "        %s_ready = 1;\n", pref);
    fprintf(f, "    }\n");
    fprintf(f, "    return &%s_bc;\n", pref);
    fprintf(f, "}\n");

    fclose(f);
    return 1;
}
#else
int rpyl_transpile_bytecode_to_c(const RpylBytecode* bc, const char* out_c_path, const char* symbol_prefix) {
    (void)bc;
    (void)out_c_path;
    (void)symbol_prefix;
    return 0;
}
#endif
