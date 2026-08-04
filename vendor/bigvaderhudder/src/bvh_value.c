#include "bvh_value.h"

static void bvh_skip_sp(const char **io) {
    const char *p;
    p = *io;
    while (*p == ' ' || *p == '\t' || *p == '\r') p++;
    *io = p;
}

static int bvh_ascii_lower(int c) {
    if (c >= 'A' && c <= 'Z') return c + ('a' - 'A');
    return c;
}

static int bvh_eq_fold(const char *a, const char *b) {
    int ca;
    int cb;
    if (!a) a = "";
    if (!b) b = "";
    while (*a && *b) {
        ca = bvh_ascii_lower((unsigned char)*a);
        cb = bvh_ascii_lower((unsigned char)*b);
        if (ca != cb) return 0;
        a++;
        b++;
    }
    return (*a == '\0' && *b == '\0') ? 1 : 0;
}

static int bvh_key_is(const BVH_Context *ctx, int key_sym, const char *lit) {
    return bvh_eq_fold(bvh_symbol_text(ctx, key_sym), lit);
}

void bvh_value_empty(BVH_Value *v) {
    if (!v) return;
    v->kind = BVH_VALUE_EMPTY;
    v->a = 0L;
    v->b = 0L;
    v->c = 0L;
    v->u = 0UL;
    v->sym = BVH_NIL;
}

static int bvh_end_after_sp(const char *p) {
    bvh_skip_sp(&p);
    return *p == '\0' ? 1 : 0;
}

static int bvh_parse_long_at(const char **io, long *out) {
    const char *p;
    int neg;
    long v;
    int any;

    p = *io;
    bvh_skip_sp(&p);
    neg = 0;
    if (*p == '-' || *p == '+') {
        if (*p == '-') neg = 1;
        p++;
    }
    v = 0L;
    any = 0;
    while (bvh_is_digit_ch((unsigned char)*p)) {
        v = (v * 10L) + (long)(*p - '0');
        p++;
        any = 1;
    }
    if (!any) return 0;
    if (neg) v = -v;
    *io = p;
    if (out) *out = v;
    return 1;
}

int bvh_parse_long_text(const char *s, long *out) {
    const char *p;
    long v;
    p = s ? s : "";
    if (!bvh_parse_long_at(&p, &v)) return 0;
    if (!bvh_end_after_sp(p)) return 0;
    if (out) *out = v;
    return 1;
}

static int bvh_parse_fixed_at(const char **io, BVH_Fixed *out) {
    const char *p;
    int neg;
    long whole;
    long frac;
    long denom;
    int any;
    BVH_Fixed val;

    p = *io;
    bvh_skip_sp(&p);
    neg = 0;
    if (*p == '-' || *p == '+') {
        if (*p == '-') neg = 1;
        p++;
    }
    whole = 0L;
    any = 0;
    while (bvh_is_digit_ch((unsigned char)*p)) {
        whole = (whole * 10L) + (long)(*p - '0');
        p++;
        any = 1;
    }
    frac = 0L;
    denom = 1L;
    if (*p == '.') {
        p++;
        while (bvh_is_digit_ch((unsigned char)*p) && denom < 1000000L) {
            frac = (frac * 10L) + (long)(*p - '0');
            denom *= 10L;
            p++;
            any = 1;
        }
        while (bvh_is_digit_ch((unsigned char)*p)) p++;
    }
    if (!any) return 0;
    val = (BVH_Fixed)(whole << BVH_FIXED_SHIFT);
    if (denom > 1L) {
        val += (BVH_Fixed)(((frac * BVH_FIXED_ONE) + (denom / 2L)) / denom);
    }
    if (neg) val = -val;
    *io = p;
    if (out) *out = val;
    return 1;
}

int bvh_parse_fixed_text(const char *s, BVH_Fixed *out) {
    const char *p;
    BVH_Fixed v;
    p = s ? s : "";
    if (!bvh_parse_fixed_at(&p, &v)) return 0;
    if (!bvh_end_after_sp(p)) return 0;
    if (out) *out = v;
    return 1;
}

static int bvh_parse_vec2(const char *s, long *x, long *y) {
    const char *p;
    long a;
    long b;
    p = s ? s : "";
    if (!bvh_parse_long_at(&p, &a)) return 0;
    bvh_skip_sp(&p);
    if (*p != ',') return 0;
    p++;
    if (!bvh_parse_long_at(&p, &b)) return 0;
    if (!bvh_end_after_sp(p)) return 0;
    if (x) *x = a;
    if (y) *y = b;
    return 1;
}

static int bvh_parse_range(const char *s, long *a, long *b) {
    const char *p;
    long x;
    long y;
    p = s ? s : "";
    if (!bvh_parse_long_at(&p, &x)) return 0;
    bvh_skip_sp(&p);
    if (p[0] != '.' || p[1] != '.') return 0;
    p += 2;
    if (!bvh_parse_long_at(&p, &y)) return 0;
    if (!bvh_end_after_sp(p)) return 0;
    if (a) *a = x;
    if (b) *b = y;
    return 1;
}

static int bvh_color_from_name(const char *s, unsigned long *rgba) {
    if (bvh_eq_fold(s, "white")) { *rgba = 0xFFFFFFFFUL; return 1; }
    if (bvh_eq_fold(s, "black")) { *rgba = 0x000000FFUL; return 1; }
    if (bvh_eq_fold(s, "red")) { *rgba = 0xFF0000FFUL; return 1; }
    if (bvh_eq_fold(s, "green")) { *rgba = 0x00FF00FFUL; return 1; }
    if (bvh_eq_fold(s, "blue")) { *rgba = 0x0000FFFFUL; return 1; }
    if (bvh_eq_fold(s, "yellow")) { *rgba = 0xFFFF00FFUL; return 1; }
    if (bvh_eq_fold(s, "cyan")) { *rgba = 0x00FFFFFFUL; return 1; }
    if (bvh_eq_fold(s, "magenta")) { *rgba = 0xFF00FFFFUL; return 1; }
    if (bvh_eq_fold(s, "transparent")) { *rgba = 0x00000000UL; return 1; }
    return 0;
}

static int bvh_parse_color(const char *s, unsigned long *rgba) {
    unsigned long n;
    unsigned long v;
    int i;
    const char *p;

    p = s ? s : "";
    bvh_skip_sp(&p);
    if (bvh_color_from_name(p, rgba)) return 1;
    if (*p != '#') return 0;
    p++;
    n = 0UL;
    while (bvh_is_hex_ch((unsigned char)p[n])) n++;
    if (!(n == 6UL || n == 8UL)) return 0;
    v = 0UL;
    for (i = 0; i < (int)n; i++) {
        v = (v << 4) | (unsigned long)bvh_hex_value((unsigned char)p[i]);
    }
    p += n;
    if (!bvh_end_after_sp(p)) return 0;
    if (n == 6UL) v = (v << 8) | 0xFFUL;
    if (rgba) *rgba = v;
    return 1;
}

static int bvh_word_is_symbol(const char *s) {
    const char *p;
    int any;
    p = s ? s : "";
    bvh_skip_sp(&p);
    any = 0;
    while (*p) {
        int c;
        c = (unsigned char)*p;
        if (!(isalnum((unsigned char)c) || c == '_' || c == '.' || c == '-')) break;
        p++;
        any = 1;
    }
    if (!any) return 0;
    return bvh_end_after_sp(p);
}

static int bvh_anchor_id(const char *s, unsigned long n) {
    if (bvh_slice_eq_lit(s, n, "top_left")) return BVH_ANCHOR_TOP_LEFT;
    if (bvh_slice_eq_lit(s, n, "top_center")) return BVH_ANCHOR_TOP_CENTER;
    if (bvh_slice_eq_lit(s, n, "top_right")) return BVH_ANCHOR_TOP_RIGHT;
    if (bvh_slice_eq_lit(s, n, "center")) return BVH_ANCHOR_CENTER;
    if (bvh_slice_eq_lit(s, n, "bottom_left")) return BVH_ANCHOR_BOTTOM_LEFT;
    if (bvh_slice_eq_lit(s, n, "bottom_center")) return BVH_ANCHOR_BOTTOM_CENTER;
    if (bvh_slice_eq_lit(s, n, "bottom_right")) return BVH_ANCHOR_BOTTOM_RIGHT;
    return BVH_ANCHOR_NONE;
}

static int bvh_parse_anchor_pos(const char *s, int *anchor, long *x, long *y) {
    const char *p;
    const char *w;
    unsigned long n;
    int a;
    int sign;
    long vx;
    long vy;

    p = s ? s : "";
    bvh_skip_sp(&p);
    w = p;
    while (*p && (isalnum((unsigned char)*p) || *p == '_')) p++;
    n = (unsigned long)(p - w);
    a = bvh_anchor_id(w, n);
    if (a == BVH_ANCHOR_NONE) return 0;
    bvh_skip_sp(&p);
    vx = 0L;
    vy = 0L;
    if (*p == '\0') {
        *anchor = a;
        *x = 0L;
        *y = 0L;
        return 1;
    }
    sign = 1;
    if (*p == '+') {
        sign = 1;
        p++;
    } else if (*p == '-') {
        sign = -1;
        p++;
    } else {
        return 0;
    }
    if (!bvh_parse_vec2(p, &vx, &vy)) return 0;
    *anchor = a;
    *x = vx * (long)sign;
    *y = vy * (long)sign;
    return 1;
}

int bvh_value_from_raw(BVH_Context *ctx, int key_sym, int raw_sym, BVH_Value *out) {
    const char *raw;
    long a;
    long b;
    BVH_Fixed fx;
    unsigned long color;
    int anchor;

    bvh_value_empty(out);
    raw = bvh_symbol_text(ctx, raw_sym);
    if (raw[0] == '\0') return 1;

    if (bvh_key_is(ctx, key_sym, "alpha") || bvh_key_is(ctx, key_sym, "opacity") || bvh_key_is(ctx, key_sym, "scale")) {
        if (bvh_parse_fixed_text(raw, &fx)) {
            out->kind = BVH_VALUE_FIXED;
            out->a = (long)fx;
            return 1;
        }
    }

    if (bvh_key_is(ctx, key_sym, "z") || bvh_key_is(ctx, key_sym, "thickness")) {
        if (bvh_parse_long_text(raw, &a)) {
            out->kind = BVH_VALUE_INT;
            out->a = a;
            return 1;
        }
    }

    if (bvh_key_is(ctx, key_sym, "size")) {
        if (bvh_parse_vec2(raw, &a, &b)) {
            out->kind = BVH_VALUE_VEC2I;
            out->a = a;
            out->b = b;
            return 1;
        }
    }

    if (bvh_key_is(ctx, key_sym, "frames")) {
        if (bvh_parse_range(raw, &a, &b)) {
            out->kind = BVH_VALUE_RANGE;
            out->a = a;
            out->b = b;
            return 1;
        }
    }

    if (bvh_key_is(ctx, key_sym, "color")) {
        if (bvh_parse_color(raw, &color)) {
            out->kind = BVH_VALUE_COLOR;
            out->u = color;
            return 1;
        }
    }

    if (bvh_key_is(ctx, key_sym, "pos")) {
        if (bvh_parse_anchor_pos(raw, &anchor, &a, &b)) {
            out->kind = BVH_VALUE_ANCHOR_POS;
            out->a = (long)anchor;
            out->b = a;
            out->c = b;
            return 1;
        }
    }

    if (bvh_word_is_symbol(raw)) {
        out->kind = BVH_VALUE_SYMBOL;
        out->sym = raw_sym;
        return 1;
    }

    out->kind = BVH_VALUE_RAW;
    out->sym = raw_sym;
    return 1;
}

const char *bvh_value_kind_name(BVH_ValueKind kind) {
    switch (kind) {
        case BVH_VALUE_EMPTY: return "empty";
        case BVH_VALUE_RAW: return "raw";
        case BVH_VALUE_INT: return "int";
        case BVH_VALUE_FIXED: return "fixed";
        case BVH_VALUE_VEC2I: return "vec2i";
        case BVH_VALUE_COLOR: return "color";
        case BVH_VALUE_ANCHOR_POS: return "anchor_pos";
        case BVH_VALUE_SYMBOL: return "symbol";
        case BVH_VALUE_RANGE: return "range";
        default: return "unknown";
    }
}

const char *bvh_anchor_name(int anchor) {
    switch (anchor) {
        case BVH_ANCHOR_TOP_LEFT: return "top_left";
        case BVH_ANCHOR_TOP_CENTER: return "top_center";
        case BVH_ANCHOR_TOP_RIGHT: return "top_right";
        case BVH_ANCHOR_CENTER: return "center";
        case BVH_ANCHOR_BOTTOM_LEFT: return "bottom_left";
        case BVH_ANCHOR_BOTTOM_CENTER: return "bottom_center";
        case BVH_ANCHOR_BOTTOM_RIGHT: return "bottom_right";
        default: return "none";
    }
}

static void bvh_copy_cstr(char *out, unsigned long cap, const char *s) {
    bvh_copy_slice(out, cap, s ? s : "", bvh_cstr_len(s));
}

int bvh_value_format(const BVH_Context *ctx, const BVH_Value *value, char *out, unsigned long cap) {
    char tmp[BVH_VALUE_TEXT_MAX];
    long whole;
    long part;
    unsigned long av;
    unsigned long rgba;
    int neg;

    if (!out || cap == 0UL) return 0;
    if (!value) {
        out[0] = '\0';
        return 0;
    }

    tmp[0] = '\0';
    switch (value->kind) {
        case BVH_VALUE_EMPTY:
            bvh_copy_cstr(out, cap, "");
            return 1;
        case BVH_VALUE_INT:
            sprintf(tmp, "%ld", value->a);
            bvh_copy_cstr(out, cap, tmp);
            return 1;
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
            bvh_copy_cstr(out, cap, tmp);
            return 1;
        case BVH_VALUE_VEC2I:
            sprintf(tmp, "%ld,%ld", value->a, value->b);
            bvh_copy_cstr(out, cap, tmp);
            return 1;
        case BVH_VALUE_COLOR:
            rgba = value->u;
            sprintf(tmp, "#%02lX%02lX%02lX%02lX", (rgba >> 24) & 255UL, (rgba >> 16) & 255UL, (rgba >> 8) & 255UL, rgba & 255UL);
            bvh_copy_cstr(out, cap, tmp);
            return 1;
        case BVH_VALUE_ANCHOR_POS:
            sprintf(tmp, "%s %+ld,%+ld", bvh_anchor_name((int)value->a), value->b, value->c);
            bvh_copy_cstr(out, cap, tmp);
            return 1;
        case BVH_VALUE_SYMBOL:
        case BVH_VALUE_RAW:
            bvh_copy_cstr(out, cap, bvh_symbol_text(ctx, value->sym));
            return 1;
        case BVH_VALUE_RANGE:
            sprintf(tmp, "%ld..%ld", value->a, value->b);
            bvh_copy_cstr(out, cap, tmp);
            return 1;
        default:
            bvh_copy_cstr(out, cap, "?");
            return 0;
    }
}
