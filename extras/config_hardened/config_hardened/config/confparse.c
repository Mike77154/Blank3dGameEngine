#include "confparse.h"

#include <limits.h>

/* Utilidades minimas. */

static int conf_is_space(char c)
{
    return c == ' ' || c == '\t';
}

static int conf_is_newline(char c)
{
    return c == '\n' || c == '\r';
}

static int conf_is_comment(char c)
{
    return c == '#' || c == ';';
}

static int conf_is_digit(char c)
{
    return c >= '0' && c <= '9';
}

static char conf_tolower(char c)
{
    if (c >= 'A' && c <= 'Z') return (char)(c - 'A' + 'a');
    return c;
}

static void conf_zero_entry(conf_entry_t *o)
{
    o->section = 0;
    o->section_len = 0;
    o->key = 0;
    o->key_len = 0;
    o->value = 0;
    o->value_len = 0;
}

static void conf_consume_newline(conf_parser_t *p)
{
    if (p->pos >= p->length) return;

    if (p->data[p->pos] == '\r') {
        p->pos++;
        if (p->pos < p->length && p->data[p->pos] == '\n') {
            p->pos++;
        }
        return;
    }

    if (p->data[p->pos] == '\n') {
        p->pos++;
    }
}

static void conf_skip_to_eol(conf_parser_t *p)
{
    while (p->pos < p->length && !conf_is_newline(p->data[p->pos])) {
        p->pos++;
    }
    conf_consume_newline(p);
}

static void conf_trim_bounds(const char *d, int *start, int *end)
{
    while (*start < *end && conf_is_space(d[*start])) {
        (*start)++;
    }

    while (*end > *start && conf_is_space(d[*end - 1])) {
        (*end)--;
    }
}

static int conf_cut_inline_comment(const char *d, int start, int end)
{
    int i;

    for (i = start; i < end; ++i) {
        if (conf_is_comment(d[i])) {
            if (i == start || conf_is_space(d[i - 1])) {
                return i;
            }
        }
    }

    return end;
}

static void conf_skip_blank_and_comment_lines(conf_parser_t *p)
{
    int save;

    while (p->pos < p->length) {
        save = p->pos;

        while (p->pos < p->length && conf_is_space(p->data[p->pos])) {
            p->pos++;
        }

        if (p->pos >= p->length) return;

        if (conf_is_newline(p->data[p->pos])) {
            conf_consume_newline(p);
            continue;
        }

        if (conf_is_comment(p->data[p->pos])) {
            conf_skip_to_eol(p);
            continue;
        }

        if (save == p->pos) return;
        return;
    }
}

/* Deteccion de formato. Soporta comentarios iniciales. */

static conf_format_t detect_format(const char *d, int len)
{
    int i;
    int j;
    int first;
    int saw_colon;
    int saw_equal;

    i = 0;
    while (i < len) {
        while (i < len && conf_is_space(d[i])) i++;

        if (i >= len) break;

        if (conf_is_newline(d[i])) {
            i++;
            continue;
        }

        if (conf_is_comment(d[i])) {
            while (i < len && !conf_is_newline(d[i])) i++;
            continue;
        }

        first = i;
        if (d[first] == '[') return CONF_FMT_INI;

        saw_colon = 0;
        saw_equal = 0;
        j = first;
        while (j < len && !conf_is_newline(d[j])) {
            if (d[j] == ':' && !saw_equal) saw_colon = 1;
            if (d[j] == '=' && !saw_colon) saw_equal = 1;
            j++;
        }

        if (saw_equal) return CONF_FMT_INI;
        if (saw_colon) return CONF_FMT_YAML;

        /* Linea sin separador: ignorarla durante la deteccion. */
        i = j;
        continue;
    }

    return CONF_FMT_UNKNOWN;
}

void conf_init(conf_parser_t *p, const char *data, int length)
{
    if (!p) return;

    p->data = data;
    p->length = length;
    p->pos = 0;
    p->current_section = 0;
    p->current_section_len = 0;

    if (!data || length <= 0) {
        p->data = "";
        p->length = 0;
        p->format = CONF_FMT_UNKNOWN;
        return;
    }

    p->format = detect_format(data, length);
}

/* Lectura INI: iterativa, sin recursion. */

static int parse_ini(conf_parser_t *p, conf_entry_t *o)
{
    int start;
    int end;
    int kstart;
    int kend;
    int vstart;
    int vend;

    while (p->pos < p->length) {
        conf_skip_blank_and_comment_lines(p);
        if (p->pos >= p->length) return 0;

        if (p->data[p->pos] == '[') {
            p->pos++;
            start = p->pos;

            while (p->pos < p->length &&
                   p->data[p->pos] != ']' &&
                   !conf_is_newline(p->data[p->pos])) {
                p->pos++;
            }

            if (p->pos >= p->length || p->data[p->pos] != ']') {
                conf_skip_to_eol(p);
                continue;
            }

            end = p->pos;
            conf_trim_bounds(p->data, &start, &end);
            p->current_section = p->data + start;
            p->current_section_len = end - start;

            p->pos++; /* ']' */
            conf_skip_to_eol(p);
            continue;
        }

        kstart = p->pos;
        while (p->pos < p->length &&
               p->data[p->pos] != '=' &&
               !conf_is_newline(p->data[p->pos])) {
            p->pos++;
        }

        if (p->pos >= p->length || p->data[p->pos] != '=') {
            conf_skip_to_eol(p);
            continue;
        }

        kend = p->pos;
        conf_trim_bounds(p->data, &kstart, &kend);

        p->pos++; /* '=' */
        vstart = p->pos;

        while (p->pos < p->length && !conf_is_newline(p->data[p->pos])) {
            p->pos++;
        }

        vend = p->pos;
        vend = conf_cut_inline_comment(p->data, vstart, vend);
        conf_trim_bounds(p->data, &vstart, &vend);

        conf_consume_newline(p);

        if (kend <= kstart) {
            continue;
        }

        o->section = p->current_section;
        o->section_len = p->current_section_len;
        o->key = p->data + kstart;
        o->key_len = kend - kstart;
        o->value = p->data + vstart;
        o->value_len = vend - vstart;
        return 1;
    }

    return 0;
}

/* Lectura YAML minima: section: / key: value, iterativa y con bounds-check. */

static int parse_yaml(conf_parser_t *p, conf_entry_t *o)
{
    int start;
    int end;
    int vstart;
    int vend;
    int after_colon;

    while (p->pos < p->length) {
        conf_skip_blank_and_comment_lines(p);
        if (p->pos >= p->length) return 0;

        start = p->pos;
        while (p->pos < p->length &&
               p->data[p->pos] != ':' &&
               !conf_is_newline(p->data[p->pos])) {
            p->pos++;
        }

        if (p->pos >= p->length || p->data[p->pos] != ':') {
            conf_skip_to_eol(p);
            continue;
        }

        end = p->pos;
        conf_trim_bounds(p->data, &start, &end);

        p->pos++; /* ':' */
        after_colon = p->pos;

        while (p->pos < p->length && conf_is_space(p->data[p->pos])) {
            p->pos++;
        }

        if (p->pos >= p->length ||
            conf_is_newline(p->data[p->pos]) ||
            conf_is_comment(p->data[p->pos])) {
            p->current_section = p->data + start;
            p->current_section_len = end - start;
            conf_skip_to_eol(p);
            continue;
        }

        vstart = p->pos;
        while (p->pos < p->length && !conf_is_newline(p->data[p->pos])) {
            p->pos++;
        }

        vend = p->pos;
        vend = conf_cut_inline_comment(p->data, vstart, vend);
        conf_trim_bounds(p->data, &vstart, &vend);

        conf_consume_newline(p);

        if (end <= start) {
            p->pos = after_colon;
            conf_skip_to_eol(p);
            continue;
        }

        o->section = p->current_section;
        o->section_len = p->current_section_len;
        o->key = p->data + start;
        o->key_len = end - start;
        o->value = p->data + vstart;
        o->value_len = vend - vstart;
        return 1;
    }

    return 0;
}

int conf_next(conf_parser_t *p, conf_entry_t *out)
{
    if (!p || !out) return 0;

    conf_zero_entry(out);

    if (p->format == CONF_FMT_INI) {
        return parse_ini(p, out);
    }

    if (p->format == CONF_FMT_YAML) {
        return parse_yaml(p, out);
    }

    return 0;
}

/* Helpers de comparacion sin null-termination. */

int conf_slice_eq(const char *s, int len, const char *z)
{
    int i;

    if (!s || !z || len < 0) return 0;

    for (i = 0; i < len; ++i) {
        if (z[i] == '\0') return 0;
        if (s[i] != z[i]) return 0;
    }

    return z[len] == '\0';
}

int conf_slice_ieq(const char *s, int len, const char *z)
{
    int i;

    if (!s || !z || len < 0) return 0;

    for (i = 0; i < len; ++i) {
        if (z[i] == '\0') return 0;
        if (conf_tolower(s[i]) != conf_tolower(z[i])) return 0;
    }

    return z[len] == '\0';
}

int conf_entry_section_eq(const conf_entry_t *e, const char *section)
{
    if (!e) return 0;
    return conf_slice_eq(e->section, e->section_len, section);
}

int conf_entry_key_eq(const conf_entry_t *e, const char *key)
{
    if (!e) return 0;
    return conf_slice_eq(e->key, e->key_len, key);
}

int conf_entry_value_eq(const conf_entry_t *e, const char *value)
{
    if (!e) return 0;
    return conf_slice_eq(e->value, e->value_len, value);
}

int conf_entry_value_ieq(const conf_entry_t *e, const char *value)
{
    if (!e) return 0;
    return conf_slice_ieq(e->value, e->value_len, value);
}

/* Parseo de valores. */

static void conf_trim_slice(const char **s, int *len)
{
    const char *p;
    int n;

    p = *s;
    n = *len;

    while (n > 0 && conf_is_space(*p)) {
        p++;
        n--;
    }

    while (n > 0 && conf_is_space(p[n - 1])) {
        n--;
    }

    *s = p;
    *len = n;
}

int conf_entry_parse_int(const conf_entry_t *e, int *out_value)
{
    const char *s;
    int len;
    int i;
    int sign;
    int any;
    unsigned long acc;
    unsigned long lim;

    if (!e || !out_value || !e->value || e->value_len <= 0) return 0;

    s = e->value;
    len = e->value_len;
    conf_trim_slice(&s, &len);

    i = 0;
    sign = 1;
    any = 0;
    acc = 0UL;

    if (i < len && (s[i] == '-' || s[i] == '+')) {
        if (s[i] == '-') sign = -1;
        i++;
    }

    lim = (sign < 0) ? ((unsigned long)INT_MAX + 1UL) : (unsigned long)INT_MAX;

    while (i < len && conf_is_digit(s[i])) {
        int digit;
        digit = s[i] - '0';
        any = 1;
        if (acc > (lim - (unsigned long)digit) / 10UL) return 0;
        acc = acc * 10UL + (unsigned long)digit;
        i++;
    }

    if (!any || i != len) return 0;

    if (sign < 0) {
        if (acc == ((unsigned long)INT_MAX + 1UL)) {
            *out_value = INT_MIN;
        } else {
            *out_value = -(int)acc;
        }
    } else {
        *out_value = (int)acc;
    }

    return 1;
}

int conf_entry_parse_bool(const conf_entry_t *e, int *out_value)
{
    const char *s;
    int len;

    if (!e || !out_value || !e->value) return 0;

    s = e->value;
    len = e->value_len;
    conf_trim_slice(&s, &len);

    if (conf_slice_ieq(s, len, "true") ||
        conf_slice_ieq(s, len, "yes") ||
        conf_slice_ieq(s, len, "on") ||
        conf_slice_eq(s, len, "1")) {
        *out_value = 1;
        return 1;
    }

    if (conf_slice_ieq(s, len, "false") ||
        conf_slice_ieq(s, len, "no") ||
        conf_slice_ieq(s, len, "off") ||
        conf_slice_eq(s, len, "0")) {
        *out_value = 0;
        return 1;
    }

    return 0;
}

int conf_entry_parse_fixed(const conf_entry_t *e, int frac_bits, int *out_value)
{
    const char *s;
    int len;
    int i;
    int sign;
    int any;
    unsigned long int_part;
    unsigned long frac_part;
    unsigned long frac_fixed;
    unsigned long divisor;
    unsigned long scale;
    unsigned long magnitude;
    unsigned long max_magnitude;
    unsigned long max_int_part;

    if (!e || !out_value || !e->value || e->value_len <= 0) return 0;

    /* C89-safe 32-bit fixed parser. frac_bits > 16 would need wider
       intermediates on Win32, so this helper intentionally rejects it. */
    if (frac_bits < 0 || frac_bits > 16) return 0;

    s = e->value;
    len = e->value_len;
    conf_trim_slice(&s, &len);

    i = 0;
    sign = 1;
    any = 0;
    int_part = 0UL;
    frac_part = 0UL;
    frac_fixed = 0UL;
    divisor = 1UL;
    scale = 1UL << frac_bits;

    if (i < len && (s[i] == '-' || s[i] == '+')) {
        if (s[i] == '-') sign = -1;
        i++;
    }

    max_magnitude = (sign < 0) ? ((unsigned long)INT_MAX + 1UL) : (unsigned long)INT_MAX;
    max_int_part = max_magnitude / scale;

    while (i < len && conf_is_digit(s[i])) {
        int digit;
        digit = s[i] - '0';
        any = 1;
        if (int_part > (max_int_part - (unsigned long)digit) / 10UL) return 0;
        int_part = int_part * 10UL + (unsigned long)digit;
        i++;
    }

    if (i < len && s[i] == '.') {
        i++;
        while (i < len && conf_is_digit(s[i])) {
            any = 1;
            if (divisor < 10000UL) {
                frac_part = frac_part * 10UL + (unsigned long)(s[i] - '0');
                divisor = divisor * 10UL;
            }
            i++;
        }
    }

    if (!any || i != len) return 0;

    magnitude = int_part * scale;
    if (divisor > 1UL) {
        frac_fixed = (frac_part * scale) / divisor;
        if (frac_fixed > max_magnitude - magnitude) return 0;
        magnitude += frac_fixed;
    }

    if (magnitude > max_magnitude) return 0;

    if (sign < 0) {
        if (magnitude == ((unsigned long)INT_MAX + 1UL)) {
            *out_value = INT_MIN;
        } else {
            *out_value = -(int)magnitude;
        }
    } else {
        *out_value = (int)magnitude;
    }

    return 1;
}
