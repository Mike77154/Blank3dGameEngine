/* flags_state.c - parser/state for flags files (C89) */
#include "flags_state.h"

#include <string.h>
#include <ctype.h>
#include <stdlib.h>

/* ---------- small helpers ---------- */
static void _log(FlagsLogFn fn, void *user, const char *msg)
{
    if (fn) fn(user, msg);
}

static int _cstr_len(const char *s)
{
    int n;
    if (!s) return 0;
    n = 0;
    while (s[n] != '\0') n++;
    return n;
}

static int _cstr_eq(const char *a, const char *b)
{
    int i;
    if (a == b) return 1;
    if (!a || !b) return 0;
    i = 0;
    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) return 0;
        i++;
    }
    return (a[i] == '\0' && b[i] == '\0') ? 1 : 0;
}

static int _cstr_endswith(const char *s, const char *suffix)
{
    int ls, lf, i;
    if (!s || !suffix) return 0;
    ls = _cstr_len(s);
    lf = _cstr_len(suffix);
    if (lf <= 0 || ls < lf) return 0;
    for (i = 0; i < lf; ++i) {
        if (s[ls - lf + i] != suffix[i]) return 0;
    }
    return 1;
}

static int _cstr_startswith(const char *s, const char *prefix)
{
    int i;
    if (!prefix) return 1;
    if (!s) return 0;
    i = 0;
    while (prefix[i] != '\0') {
        if (s[i] == '\0') return 0;
        if (s[i] != prefix[i]) return 0;
        i++;
    }
    return 1;
}

static int _str_ieq_n(const char *a, int alen, const char *b)
{
    int blen, i;
    if (!a || !b) return 0;
    blen = _cstr_len(b);
    if (alen != blen) return 0;
    for (i = 0; i < alen; ++i) {
        unsigned char ca, cb;
        ca = (unsigned char)a[i];
        cb = (unsigned char)b[i];
        ca = (unsigned char)tolower((int)ca);
        cb = (unsigned char)tolower((int)cb);
        if (ca != cb) return 0;
    }
    return 1;
}

static void _trim_span(const char *s, int len, const char **out_start, int *out_len)
{
    int i, j;
    const char *p;

    if (!s || len <= 0) {
        *out_start = s;
        *out_len = 0;
        return;
    }

    p = s;
    i = 0;
    while (i < len) {
        char c;
        c = p[i];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            i++;
            continue;
        }
        break;
    }

    j = len - 1;
    while (j >= i) {
        char c;
        c = p[j];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            j--;
            continue;
        }
        break;
    }

    *out_start = p + i;
    *out_len = (j >= i) ? (j - i + 1) : 0;
}

static int _parse_int_strict(const char *s, int len, long *out_i)
{
    int i;
    int sign;
    long v;
    int saw_digit;

    if (!s || len <= 0 || !out_i) return 0;

    i = 0;
    sign = 1;
    v = 0;
    saw_digit = 0;

    if (i < len && (s[i] == '+' || s[i] == '-')) {
        if (s[i] == '-') sign = -1;
        i++;
    }

    while (i < len) {
        char c;
        c = s[i];
        if (c >= '0' && c <= '9') {
            saw_digit = 1;
            v = (v * 10L) + (long)(c - '0');
            i++;
            continue;
        }
        break;
    }
    if (!saw_digit) return 0;

    while (i < len) {
        char c;
        c = s[i];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            i++;
            continue;
        }
        return 0; /* invalid trailing char */
    }

    *out_i = (sign < 0) ? -v : v;
    return 1;
}

static FlagsValue _normalize_value(FlagsPool *pool, const char *s, int len)
{
    const char *t;
    int tlen;
    flags_fx_t fx;
    long iv;

    _trim_span(s, len, &t, &tlen);

    /* bool patterns (python's behavior: "1" => true, "0" => false) */
    if (_str_ieq_n(t, tlen, "true") || _str_ieq_n(t, tlen, "yes") || _str_ieq_n(t, tlen, "on") || _str_ieq_n(t, tlen, "1")) {
        return flags_value_bool(1);
    }
    if (_str_ieq_n(t, tlen, "false") || _str_ieq_n(t, tlen, "no") || _str_ieq_n(t, tlen, "off") || _str_ieq_n(t, tlen, "0")) {
        return flags_value_bool(0);
    }

    /* number? (python: if "." in l => float, else int) */
    {
        int i;
        int has_dot;
        has_dot = 0;
        for (i = 0; i < tlen; ++i) {
            if (t[i] == '.') { has_dot = 1; break; }
        }
        if (has_dot) {
            if (flags_fx_parse(t, tlen, &fx)) {
                return flags_value_fx(fx);
            }
        } else {
            if (_parse_int_strict(t, tlen, &iv)) {
                return flags_value_int(iv);
            }
        }
    }

    /* raw string */
    if (pool) {
        const char *dup;
        dup = flags_pool_dup(pool, t, tlen);
        if (dup) return flags_value_str(dup);
    }
    return flags_value_str("");
}

/* Adds an item to FlagState, duplicating strings into pool. */
static int _state_add_item(FlagState *st, const char *key, int key_len, const FlagsValue *val)
{
    const char *kdup;
    FlagsValue vdup;

    if (!st || !key || key_len <= 0 || !val) return 0;
    if (!st->items || st->items_cap <= 0) return 0;
    if (st->items_count >= st->items_cap) return 0;

    kdup = flags_pool_dup(&st->pool, key, key_len);
    if (!kdup) return 0;

    vdup = *val;
    if (val->type == FLAGS_VAL_STR) {
        const char *sdup;
        int slen;
        const char *src;
        src = val->as.s ? val->as.s : "";
        slen = _cstr_len(src);
        sdup = flags_pool_dup(&st->pool, src, slen);
        if (!sdup) return 0;
        vdup.as.s = sdup;
    }

    st->items[st->items_count].key = kdup;
    st->items[st->items_count].val = vdup;
    st->items_count++;
    return 1;
}

/* ---------- INI-like parsing ---------- */

static int _find_operator(const char *s, int len, int *out_pos, int *out_is_add)
{
    int i;
    /* look for "+=" first */
    for (i = 0; i < len - 1; ++i) {
        if (s[i] == '+' && s[i + 1] == '=') {
            *out_pos = i;
            *out_is_add = 1;
            return 1;
        }
    }
    for (i = 0; i < len; ++i) {
        if (s[i] == '=') {
            *out_pos = i;
            *out_is_add = 0;
            return 1;
        }
    }
    return 0;
}

static int _parse_ini_text(FlagState *st, const char *buf, int len)
{
    int i;
    int line_start;

    i = 0;
    line_start = 0;

    while (i <= len) {
        int is_eof;
        char c;
        int line_end;
        int line_len;
        const char *line;
        const char *t;
        int tlen;

        is_eof = (i == len) ? 1 : 0;
        c = is_eof ? '\n' : buf[i];

        if (c != '\n') {
            i++;
            continue;
        }

        line_end = i;
        line = buf + line_start;
        line_len = line_end - line_start;

        /* handle CRLF */
        if (line_len > 0 && line[line_len - 1] == '\r') line_len--;

        /* advance to next line */
        i++;
        line_start = i;

        /* truncate very long lines */
        if (line_len > FLAGS_MAX_LINE) line_len = FLAGS_MAX_LINE;

        _trim_span(line, line_len, &t, &tlen);
        if (tlen <= 0) continue;
        if (t[0] == '#') continue;

        /* split */
        {
            int pos, is_add;
            const char *k0, *v0;
            int klen, vlen;
            FlagsValue vnorm;

            if (!_find_operator(t, tlen, &pos, &is_add)) continue;

            k0 = t;
            klen = pos;
            v0 = t + pos + (is_add ? 2 : 1);
            vlen = tlen - (pos + (is_add ? 2 : 1));

            _trim_span(k0, klen, &k0, &klen);
            _trim_span(v0, vlen, &v0, &vlen);
            if (klen <= 0) continue;

            vnorm = _normalize_value(&st->pool, v0, vlen);

            if (is_add) {
                /* store as "key__add__" */
                char keybuf[FLAGS_MAX_KEY];
                int j;
                int suffix_len;
                suffix_len = (int)sizeof(FLAGS_ADD_SUFFIX) - 1; /* without NUL */
                if (klen + suffix_len >= FLAGS_MAX_KEY) {
                    /* truncate */
                    klen = FLAGS_MAX_KEY - suffix_len - 1;
                }
                for (j = 0; j < klen; ++j) keybuf[j] = k0[j];
                for (j = 0; j < suffix_len; ++j) keybuf[klen + j] = FLAGS_ADD_SUFFIX[j];
                keybuf[klen + suffix_len] = '\0';

                if (!_state_add_item(st, keybuf, klen + suffix_len, &vnorm)) return 0;
            } else {
                if (!_state_add_item(st, k0, klen, &vnorm)) return 0;
            }
        }
    }

    return 1;
}

/* ---------- JSON parsing + flatten ---------- */

typedef struct _JsonCtx {
    const char *s;
    int len;
    int i;
    FlagState *st;
} _JsonCtx;

static void _json_skip_ws(_JsonCtx *ctx)
{
    while (ctx->i < ctx->len) {
        char c;
        c = ctx->s[ctx->i];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            ctx->i++;
            continue;
        }
        break;
    }
}

static int _json_peek(_JsonCtx *ctx)
{
    if (ctx->i >= ctx->len) return 0;
    return (int)(unsigned char)ctx->s[ctx->i];
}

static int _json_consume(_JsonCtx *ctx, char expected)
{
    _json_skip_ws(ctx);
    if (ctx->i >= ctx->len) return 0;
    if (ctx->s[ctx->i] != expected) return 0;
    ctx->i++;
    return 1;
}

static int _json_parse_string(_JsonCtx *ctx, char *out, int out_cap, int *out_len)
{
    int n;

    _json_skip_ws(ctx);
    if (ctx->i >= ctx->len || ctx->s[ctx->i] != '\"') return 0;
    ctx->i++; /* skip " */

    n = 0;
    while (ctx->i < ctx->len) {
        char c;
        c = ctx->s[ctx->i++];
        if (c == '\"') {
            if (n < out_cap) out[n] = '\0';
            if (out_len) *out_len = n;
            return 1;
        }
        if (c == '\\') {
            if (ctx->i >= ctx->len) return 0;
            c = ctx->s[ctx->i++];
            if (c == '\"' || c == '\\' || c == '/') {
                /* keep c */
            } else if (c == 'b') c = '\b';
            else if (c == 'f') c = '\f';
            else if (c == 'n') c = '\n';
            else if (c == 'r') c = '\r';
            else if (c == 't') c = '\t';
            else if (c == 'u') {
                /* \uXXXX - we don't decode properly here; emit '?' and skip 4 hex digits */
                int k;
                c = '?';
                for (k = 0; k < 4; ++k) {
                    if (ctx->i < ctx->len) ctx->i++;
                }
            } else {
                /* unknown escape */
                return 0;
            }
        }
        if (n < out_cap - 1) {
            out[n++] = c;
        } else {
            /* truncate */
            n++;
        }
    }
    return 0;
}

static int _json_match_literal(_JsonCtx *ctx, const char *lit)
{
    int j;
    int l;
    _json_skip_ws(ctx);
    l = _cstr_len(lit);
    if (ctx->i + l > ctx->len) return 0;
    for (j = 0; j < l; ++j) {
        if (ctx->s[ctx->i + j] != lit[j]) return 0;
    }
    ctx->i += l;
    return 1;
}

static int _json_parse_number_value(_JsonCtx *ctx, FlagsValue *out_val)
{
    int start;
    int end;
    int has_dot;
    int has_exp;
    flags_fx_t fx;
    long iv;

    _json_skip_ws(ctx);
    start = ctx->i;
    if (start >= ctx->len) return 0;

    /* number chars */
    while (ctx->i < ctx->len) {
        char c;
        c = ctx->s[ctx->i];
        if ((c >= '0' && c <= '9') || c == '+' || c == '-' || c == '.' || c == 'e' || c == 'E') {
            ctx->i++;
            continue;
        }
        break;
    }
    end = ctx->i;
    if (end <= start) return 0;

    has_dot = 0;
    has_exp = 0;
    {
        int k;
        for (k = start; k < end; ++k) {
            if (ctx->s[k] == '.') has_dot = 1;
            if (ctx->s[k] == 'e' || ctx->s[k] == 'E') has_exp = 1;
        }
    }

    if (has_exp) {
        /* exponent not supported in fixed parser => store as string */
        /* NOTE: preserve the raw numeric token. */
        {
            const char *dup;
            dup = flags_pool_dup(&ctx->st->pool, ctx->s + start, end - start);
            if (!dup) return 0;
            *out_val = flags_value_str(dup);
            return 1;
        }
    }

    if (has_dot) {
        if (flags_fx_parse(ctx->s + start, end - start, &fx)) {
            *out_val = flags_value_fx(fx);
            return 1;
        }
    } else {
        if (_parse_int_strict(ctx->s + start, end - start, &iv)) {
            *out_val = flags_value_int(iv);
            return 1;
        }
    }

    /* fallback: store raw */
    {
        const char *dup;
        dup = flags_pool_dup(&ctx->st->pool, ctx->s + start, end - start);
        if (!dup) return 0;
        *out_val = flags_value_str(dup);
        return 1;
    }
}

/* Capture raw JSON array into a string slice "[...]" (including brackets). */
static int _json_capture_array_raw(_JsonCtx *ctx, const char **out_start, int *out_len)
{
    int start;
    int depth;
    int in_str;

    _json_skip_ws(ctx);
    if (_json_peek(ctx) != (int)'[') return 0;

    start = ctx->i;
    depth = 0;
    in_str = 0;

    while (ctx->i < ctx->len) {
        char c;
        c = ctx->s[ctx->i++];

        if (in_str) {
            if (c == '\\') {
                /* skip escaped char */
                if (ctx->i < ctx->len) ctx->i++;
                continue;
            }
            if (c == '\"') in_str = 0;
            continue;
        } else {
            if (c == '\"') { in_str = 1; continue; }
            if (c == '[') { depth++; continue; }
            if (c == ']') {
                depth--;
                if (depth == 0) {
                    int end;
                    end = ctx->i;
                    *out_start = ctx->s + start;
                    *out_len = end - start;
                    return 1;
                }
            }
        }
    }
    return 0;
}

static int _json_parse_value_scalar(_JsonCtx *ctx, FlagsValue *out_val)
{
    int c;
    _json_skip_ws(ctx);
    c = _json_peek(ctx);
    if (c == 0) return 0;

    if (c == '\"') {
        char tmp[FLAGS_MAX_JSON_STRING];
        int tlen;
        const char *dup;
        if (!_json_parse_string(ctx, tmp, (int)sizeof(tmp), &tlen)) return 0;
        /* store in pool */
        dup = flags_pool_dup(&ctx->st->pool, tmp, (tlen < (int)sizeof(tmp) ? tlen : (int)sizeof(tmp) - 1));
        if (!dup) return 0;
        *out_val = flags_value_str(dup);
        return 1;
    }
    if (c == 't') { if (_json_match_literal(ctx, "true"))  { *out_val = flags_value_bool(1); return 1; } }
    if (c == 'f') { if (_json_match_literal(ctx, "false")) { *out_val = flags_value_bool(0); return 1; } }
    if (c == 'n') { if (_json_match_literal(ctx, "null"))  { *out_val = flags_value_none(); return 1; } }
    if (c == '[') {
        const char *a0;
        int alen;
        const char *dup;
        if (!_json_capture_array_raw(ctx, &a0, &alen)) return 0;
        dup = flags_pool_dup(&ctx->st->pool, a0, alen);
        if (!dup) return 0;
        *out_val = flags_value_str(dup);
        return 1;
    }

    /* number */
    if (c == '-' || c == '+' || (c >= '0' && c <= '9')) {
        return _json_parse_number_value(ctx, out_val);
    }

    return 0;
}

static int _json_parse_object_flat(_JsonCtx *ctx, char *prefix, int prefix_len, int depth)
{
    int c;
    int first;

    if (depth > FLAGS_MAX_JSON_DEPTH) return 0;

    if (!_json_consume(ctx, '{')) return 0;

    _json_skip_ws(ctx);
    c = _json_peek(ctx);
    if (c == (int)'}') {
        ctx->i++;
        return 1;
    }

    first = 1;
    while (1) {
        char keytmp[FLAGS_MAX_JSON_STRING];
        int keylen;
        int base_len;
        int new_len;

        (void)first;

        /* key */
        if (!_json_parse_string(ctx, keytmp, (int)sizeof(keytmp), &keylen)) return 0;
        if (!_json_consume(ctx, ':')) return 0;

        /* build dotted key into prefix buffer */
        base_len = prefix_len;
        new_len = prefix_len;

        if (base_len > 0) {
            if (base_len + 1 >= FLAGS_MAX_KEY) return 0;
            prefix[base_len] = '.';
            new_len = base_len + 1;
        }

        /* append keytmp */
        {
            int k;
            int maxcopy;
            maxcopy = FLAGS_MAX_KEY - new_len - 1;
            if (maxcopy < 0) return 0;
            if (keylen > maxcopy) keylen = maxcopy;

            for (k = 0; k < keylen; ++k) prefix[new_len + k] = keytmp[k];
            prefix[new_len + keylen] = '\0';
            new_len = new_len + keylen;
        }

        _json_skip_ws(ctx);
        c = _json_peek(ctx);

        if (c == (int)'{') {
            /* recurse */
            if (!_json_parse_object_flat(ctx, prefix, new_len, depth + 1)) return 0;
        } else {
            /* scalar => store */
            FlagsValue v;
            if (!_json_parse_value_scalar(ctx, &v)) return 0;

            if (!_state_add_item(ctx->st, prefix, new_len, &v)) return 0;
        }

        /* restore prefix */
        prefix[prefix_len] = '\0';

        _json_skip_ws(ctx);
        c = _json_peek(ctx);
        if (c == (int)',') {
            ctx->i++;
            _json_skip_ws(ctx);
            continue;
        }
        if (c == (int)'}') {
            ctx->i++;
            break;
        }
        return 0;
    }

    return 1;
}

static int _parse_json_text(FlagState *st, const char *buf, int len)
{
    _JsonCtx ctx;
    char prefix[FLAGS_MAX_KEY];

    ctx.s = buf;
    ctx.len = len;
    ctx.i = 0;
    ctx.st = st;

    prefix[0] = '\0';

    _json_skip_ws(&ctx);
    if (_json_peek(&ctx) != (int)'{') return 0;
    if (!_json_parse_object_flat(&ctx, prefix, 0, 0)) return 0;

    _json_skip_ws(&ctx);
    /* allow trailing whitespace */
    return 1;
}

/* ---------- public API ---------- */

void flagstate_init(FlagState *st,
                    const char *path,
                    FlagsKV *items, int items_cap,
                    char *pool_buf, int pool_cap)
{
    if (!st) return;
    st->path = path;
    st->mtime = 0;
    st->items = items;
    st->items_cap = items_cap;
    st->items_count = 0;
    flags_pool_init(&st->pool, pool_buf, pool_cap);
}

int flagstate_load(FlagState *st,
                   const FlagsIO *io,
                   char *file_buf, int file_buf_cap,
                   FlagsLogFn log_fn, void *log_user)
{
    long mtime;
    int file_len;
    int ok;
    const char *p;
    int plen;
    const char *t;
    int tlen;

    if (!st || !io || !io->read_all || !io->get_mtime) return -1;
    if (!st->path) return -1;

    if (!io->get_mtime(io->user, st->path, &mtime)) {
        return 0; /* missing */
    }
    if (mtime == st->mtime) {
        return 0; /* unchanged */
    }

    if (!file_buf || file_buf_cap <= 0) return -1;

    file_len = 0;
    if (!io->read_all(io->user, st->path, file_buf, file_buf_cap, &file_len)) {
        return -1;
    }
    if (file_len < 0) file_len = 0;
    if (file_len >= file_buf_cap) file_len = file_buf_cap - 1;
    file_buf[file_len] = '\0';

    /* reset state and parse */
    st->items_count = 0;
    flags_pool_reset(&st->pool);

    p = file_buf;
    plen = file_len;

    _trim_span(p, plen, &t, &tlen);

    ok = 0;
    if (tlen > 0 && t[0] == '{') {
        ok = _parse_json_text(st, p, plen);
    } else {
        ok = _parse_ini_text(st, p, plen);
    }

    if (!ok) {
        _log(log_fn, log_user, "[flags] parse error");
        /* do NOT update mtime so we keep retrying */
        return -1;
    }

    st->mtime = mtime;
    return 1;
}

int flagstate_get(const FlagState *st, const char *key, FlagsValue *out_val)
{
    int i;
    if (!st || !key || !out_val) return 0;
    for (i = 0; i < st->items_count; ++i) {
        const FlagsKV *kv;
        kv = &st->items[i];
        if (kv->key && _cstr_eq(kv->key, key)) {
            *out_val = kv->val;
            return 1;
        }
    }
    return 0;
}

int flagstate_get_bool(const FlagState *st, const char *key, int default_val)
{
    FlagsValue v;
    if (!st || !key) return default_val;
    if (!flagstate_get(st, key, &v)) return default_val;

    if (v.type == FLAGS_VAL_BOOL) return (v.as.i != 0) ? 1 : 0;

    if (v.type == FLAGS_VAL_STR) {
        const char *s;
        int n;
        s = v.as.s ? v.as.s : "";
        n = _cstr_len(s);
        if (_str_ieq_n(s, n, "true") || _str_ieq_n(s, n, "yes") || _str_ieq_n(s, n, "on") || _str_ieq_n(s, n, "1")) return 1;
        if (_str_ieq_n(s, n, "false") || _str_ieq_n(s, n, "no") || _str_ieq_n(s, n, "off") || _str_ieq_n(s, n, "0")) return 0;
    }

    return default_val;
}

const FlagsKV *flagstate_items(const FlagState *st, int *out_count)
{
    if (out_count) *out_count = st ? st->items_count : 0;
    if (!st) return 0;
    return (const FlagsKV *)st->items;
}

static const char *_map_dst(const FlagsKeyMap *m, int n, const char *src)
{
    int i;
    if (!m || n <= 0 || !src) return 0;
    for (i = 0; i < n; ++i) {
        if (m[i].src && _cstr_eq(m[i].src, src)) return m[i].dst;
    }
    return 0;
}

void flagstate_apply_adds(const FlagState *st,
                          FlagStore *mem,
                          const char *prefix,
                          int has_lo, flags_fx_t lo,
                          int has_hi, flags_fx_t hi,
                          const FlagsKeyMap *target_map, int target_map_count)
{
    int i;
    int suffix_len;

    if (!st || !mem) return;
    suffix_len = (int)sizeof(FLAGS_ADD_SUFFIX) - 1;

    for (i = 0; i < st->items_count; ++i) {
        const FlagsKV *kv;
        const char *k;
        int klen;
        const char *base_key;
        char base_buf[FLAGS_MAX_KEY];
        const char *dst;
        FlagsValue cur_v;
        flags_fx_t cur_fx;
        flags_fx_t delta_fx;
        int have_cur;
        int have_delta;

        kv = &st->items[i];
        k = kv->key;
        if (!k) continue;
        if (!_cstr_endswith(k, FLAGS_ADD_SUFFIX)) continue;

        klen = _cstr_len(k);
        if (klen < suffix_len) continue;

        /* base_key = key without suffix */
        if (klen - suffix_len >= FLAGS_MAX_KEY) continue;
        {
            int j;
            for (j = 0; j < klen - suffix_len; ++j) base_buf[j] = k[j];
            base_buf[klen - suffix_len] = '\0';
        }
        base_key = base_buf;

        if (prefix && !_cstr_startswith(base_key, prefix)) continue;

        dst = _map_dst(target_map, target_map_count, base_key);
        if (!dst) dst = base_key;

        /* current value */
        have_cur = flagstore_get(mem, dst, &cur_v);
        if (!have_cur || !flags_value_to_fx(&cur_v, &cur_fx)) {
            cur_fx = FLAGS_FX_FROM_INT(0);
        }

        /* delta */
        have_delta = flags_value_to_fx(&kv->val, &delta_fx);
        if (!have_delta) {
            /* try parse string numeric (python would float(v) and fail => skip) */
            if (kv->val.type == FLAGS_VAL_STR && kv->val.as.s) {
                int n;
                n = _cstr_len(kv->val.as.s);
                if (flags_fx_parse(kv->val.as.s, n, &delta_fx)) {
                    have_delta = 1;
                }
            }
        }
        if (!have_delta) continue;

        cur_fx = cur_fx + delta_fx;
        cur_fx = flags_fx_clamp(cur_fx, has_lo, lo, has_hi, hi);

        (void)flagstore_set_fx(mem, dst, cur_fx);
    }
}
