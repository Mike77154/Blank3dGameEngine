#include "invariantSpecialoperations_89_ddsl2.h"

#include <ctype.h>
#include <string.h>

static void iso89_d_copy(char *dst, unsigned int cap, const char *src)
{
    unsigned int i;
    if (!dst || cap == 0U) return;
    if (!src) src = "";
    i = 0U;
    while (src[i] != '\0' && i + 1U < cap) {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
}

static int iso89_d_ci_equal(const char *a, const char *b)
{
    unsigned char ca;
    unsigned char cb;
    if (!a || !b) return 0;
    while (*a && *b) {
        ca = (unsigned char)tolower((unsigned char)*a++);
        cb = (unsigned char)tolower((unsigned char)*b++);
        if (ca != cb) return 0;
    }
    return *a == '\0' && *b == '\0';
}

static int iso89_d_ident_char(unsigned char c)
{
    return isalnum(c) || c == '_' || c == '-';
}

static const char *iso89_d_skip_ws(const char *p, const char *end)
{
    while (p < end && (*p == ' ' || *p == '\t' || *p == '\r')) ++p;
    return p;
}

static int iso89_d_word(const char **io_p, const char *end,
                        char *out, unsigned int cap)
{
    const char *p;
    unsigned int n;
    if (!io_p || !*io_p || !out || cap < 2U) return 0;
    p = iso89_d_skip_ws(*io_p, end);
    if (p >= end || !iso89_d_ident_char((unsigned char)*p)) return 0;
    n = 0U;
    while (p < end && iso89_d_ident_char((unsigned char)*p)) {
        if (n + 1U >= cap) return 0;
        out[n++] = *p++;
    }
    out[n] = '\0';
    *io_p = p;
    return 1;
}

static int iso89_d_bool(const char *word, iso89_value *out)
{
    if (!word || !out) return 0;
    if (iso89_d_ci_equal(word, "true")) {
        *out = 1L;
        return 1;
    }
    if (iso89_d_ci_equal(word, "false")) {
        *out = 0L;
        return 1;
    }
    return 0;
}

void iso89_ddsl2_symbols_init(iso89_ddsl2_symbols *symbols)
{
    int i;
    if (!symbols) return;
    symbols->count = 0;
    for (i = 0; i < ISO89_DDSL2_MAX_SYMBOLS; ++i) {
        symbols->names[i][0] = '\0';
        symbols->name_ptrs[i] = symbols->names[i];
    }
}

iso89_subject iso89_ddsl2_find_subject(const iso89_ddsl2_symbols *symbols,
                                       const char *name)
{
    int i;
    if (!symbols || !name) return ISO89_SUBJECT_NONE;
    for (i = 0; i < symbols->count; ++i)
        if (strcmp(symbols->names[i], name) == 0)
            return (iso89_subject)(i + 1);
    return ISO89_SUBJECT_NONE;
}

const char *iso89_ddsl2_subject_name(const iso89_ddsl2_symbols *symbols,
                                     iso89_subject subject)
{
    unsigned long index;
    if (!symbols || subject == ISO89_SUBJECT_NONE) return 0;
    index = subject - 1UL;
    if (index >= (unsigned long)symbols->count) return 0;
    return symbols->names[index];
}

static iso89_subject iso89_d_intern(iso89_ddsl2_symbols *symbols,
                                    const char *name)
{
    iso89_subject found;
    if (!symbols || !name || !*name) return ISO89_SUBJECT_NONE;
    found = iso89_ddsl2_find_subject(symbols, name);
    if (found != ISO89_SUBJECT_NONE) return found;
    if (symbols->count >= ISO89_DDSL2_MAX_SYMBOLS) return ISO89_SUBJECT_NONE;
    if (strlen(name) + 1U > ISO89_DDSL2_SYMBOL_CAP) return ISO89_SUBJECT_NONE;
    strcpy(symbols->names[symbols->count], name);
    symbols->name_ptrs[symbols->count] = symbols->names[symbols->count];
    ++symbols->count;
    return (iso89_subject)symbols->count;
}

static int iso89_d_parse_relation(const char *line, const char *end,
                                  char *left, iso89_value *trigger,
                                  char *right, iso89_value *result)
{
    const char *p;
    char w0[32];
    char w1[ISO89_DDSL2_SYMBOL_CAP];
    char w2[32];
    char w3[32];
    char w4[ISO89_DDSL2_SYMBOL_CAP];
    char w5[32];
    p = line;
    if (!iso89_d_word(&p, end, w0, sizeof(w0)) ||
        !iso89_d_ci_equal(w0, "if")) return 0;
    if (!iso89_d_word(&p, end, w1, sizeof(w1))) return 0;
    if (!iso89_d_word(&p, end, w2, sizeof(w2)) ||
        !iso89_d_bool(w2, trigger)) return 0;
    if (!iso89_d_word(&p, end, w3, sizeof(w3)) ||
        !iso89_d_ci_equal(w3, "then")) return 0;
    if (!iso89_d_word(&p, end, w4, sizeof(w4))) return 0;
    if (!iso89_d_word(&p, end, w5, sizeof(w5)) ||
        !iso89_d_bool(w5, result)) return 0;
    p = iso89_d_skip_ws(p, end);
    if (p < end && *p != '#') return 0;
    iso89_d_copy(left, ISO89_DDSL2_SYMBOL_CAP, w1);
    iso89_d_copy(right, ISO89_DDSL2_SYMBOL_CAP, w4);
    return 1;
}

static int iso89_d_parse_specop(const char *line, const char *end,
                                iso89_specop *out)
{
    const char *p;
    char word[64];
    char value[64];
    p = line;
    if (!iso89_d_word(&p, end, word, sizeof(word)) ||
        !iso89_d_ci_equal(word, "apply")) return 0;
    if (!iso89_d_word(&p, end, word, sizeof(word)) ||
        !iso89_d_ci_equal(word, "specop")) return 0;
    p = iso89_d_skip_ws(p, end);
    if (p >= end || *p != '=') return 0;
    ++p;
    if (!iso89_d_word(&p, end, value, sizeof(value))) return 0;
    p = iso89_d_skip_ws(p, end);
    if (p < end && *p != '#') return 0;
    return iso89_specop_from_name(value, out);
}

static int iso89_d_append_line(char *out, unsigned int cap,
                               unsigned int *io_len,
                               const char *line, const char *end,
                               int blank)
{
    const char *p;
    unsigned int len;
    if (!out || !io_len) return 0;
    len = *io_len;
    if (!blank) {
        for (p = line; p < end; ++p) {
            if (len + 1U >= cap) return 0;
            out[len++] = *p;
        }
    }
    if (len + 1U >= cap) return 0;
    out[len++] = '\n';
    out[len] = '\0';
    *io_len = len;
    return 1;
}

static int iso89_d_flush_pending(iso89_context *ctx,
                                 iso89_ddsl2_symbols *symbols,
                                 int *pending,
                                 const char *left, iso89_value trigger,
                                 const char *right, iso89_value result,
                                 iso89_specop specop)
{
    iso89_subject a;
    iso89_subject b;
    if (!*pending) return 1;
    a = iso89_d_intern(symbols, left);
    b = iso89_d_intern(symbols, right);
    if (a == ISO89_SUBJECT_NONE || b == ISO89_SUBJECT_NONE) return 0;
    if (!iso89_add_relation(ctx, a, trigger, b, result, specop)) return 0;
    *pending = 0;
    return 1;
}

int iso89_ddsl2_preprocess(const char *source,
                           char *output,
                           unsigned int output_capacity,
                           iso89_context *ctx,
                           iso89_ddsl2_symbols *symbols,
                           char *error,
                           unsigned int error_capacity)
{
    const char *line;
    const char *end;
    const char *next;
    unsigned int out_len;
    char left[ISO89_DDSL2_SYMBOL_CAP];
    char right[ISO89_DDSL2_SYMBOL_CAP];
    char parsed_left[ISO89_DDSL2_SYMBOL_CAP];
    char parsed_right[ISO89_DDSL2_SYMBOL_CAP];
    iso89_value trigger;
    iso89_value result;
    iso89_value parsed_trigger;
    iso89_value parsed_result;
    iso89_specop specop;
    int pending;
    int relation_line;
    int specop_line;
    if (!source || !output || output_capacity < 2U || !ctx || !symbols)
        return 0;
    if (error && error_capacity > 0U) error[0] = '\0';
    output[0] = '\0';
    out_len = 0U;
    iso89_context_clear(ctx);
    iso89_ddsl2_symbols_init(symbols);
    pending = 0;
    left[0] = '\0';
    right[0] = '\0';
    trigger = 0L;
    result = 0L;
    parsed_left[0] = '\0';
    parsed_right[0] = '\0';
    parsed_trigger = 0L;
    parsed_result = 0L;
    specop = ISO89_SPECOP_NONE;

    line = source;
    while (*line) {
        end = line;
        while (*end && *end != '\n') ++end;
        next = *end == '\n' ? end + 1 : end;

        relation_line = iso89_d_parse_relation(line, end, parsed_left,
                                                &parsed_trigger, parsed_right,
                                                &parsed_result);
        specop_line = 0;
        if (!relation_line)
            specop_line = iso89_d_parse_specop(line, end, &specop);

        if (relation_line) {
            if (!iso89_d_flush_pending(ctx, symbols, &pending,
                                       left, trigger, right, result,
                                       ISO89_SPECOP_NONE)) {
                iso89_d_copy(error, error_capacity,
                             "invariant table full before relation");
                return 0;
            }
            iso89_d_copy(left, sizeof(left), parsed_left);
            iso89_d_copy(right, sizeof(right), parsed_right);
            trigger = parsed_trigger;
            result = parsed_result;
            pending = 1;
            specop = ISO89_SPECOP_NONE;
            if (!iso89_d_append_line(output, output_capacity, &out_len,
                                     line, end, 1)) {
                iso89_d_copy(error, error_capacity, "invariant output full");
                return 0;
            }
        } else if (specop_line) {
            if (!pending) {
                iso89_d_copy(error, error_capacity,
                             "Apply SpecOp requires a preceding invariant");
                return 0;
            }
            if (!iso89_d_flush_pending(ctx, symbols, &pending,
                                       left, trigger, right, result, specop)) {
                iso89_d_copy(error, error_capacity,
                             "could not apply invariant SpecOp");
                return 0;
            }
            if (!iso89_d_append_line(output, output_capacity, &out_len,
                                     line, end, 1)) {
                iso89_d_copy(error, error_capacity, "invariant output full");
                return 0;
            }
        } else {
            /* Blank/comment lines may sit between relation and Apply.  A real
             * DDSL statement finalizes a pending one-way relation. */
            const char *p;
            p = iso89_d_skip_ws(line, end);
            if (pending && p < end && *p != '#') {
                if (!iso89_d_flush_pending(ctx, symbols, &pending,
                                           left, trigger, right, result,
                                           ISO89_SPECOP_NONE)) {
                    iso89_d_copy(error, error_capacity,
                                 "could not finalize invariant");
                    return 0;
                }
            }
            if (!iso89_d_append_line(output, output_capacity, &out_len,
                                     line, end, 0)) {
                iso89_d_copy(error, error_capacity, "invariant output full");
                return 0;
            }
        }
        line = next;
    }

    if (pending && !iso89_d_flush_pending(ctx, symbols, &pending,
                                          left, trigger, right, result,
                                          ISO89_SPECOP_NONE)) {
        iso89_d_copy(error, error_capacity, "could not finalize invariant");
        return 0;
    }
    return 1;
}
