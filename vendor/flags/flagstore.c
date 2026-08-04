/* flagstore.c - fixed-size key/value store - C89 */
#include "flagstore.h"

/* ---- small helpers ---- */
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

/* FNV-1a hash (works fine on C89 with unsigned long) */
static unsigned long _hash_key(const char *s)
{
    unsigned long h;
    unsigned char c;
    h = 2166136261UL;
    if (!s) return h;
    while (*s) {
        c = (unsigned char)*s++;
        h ^= (unsigned long)c;
        h *= 16777619UL;
    }
    return h;
}

static int _find_slot(const FlagStore *st, const char *key, unsigned long hash, int *out_index, int *out_found)
{
    int cap;
    int start;
    int i;

    if (!st || !st->entries || st->cap <= 0 || !key) return 0;

    cap = st->cap;
    start = (int)(hash % (unsigned long)cap);

    for (i = 0; i < cap; ++i) {
        int idx;
        FlagStoreEntry *e;
        idx = start + i;
        if (idx >= cap) idx -= cap;

        e = (FlagStoreEntry *)&st->entries[idx];
        if (!e->used) {
            *out_index = idx;
            *out_found = 0;
            return 1;
        }
        if (e->hash == hash && e->key && _cstr_eq(e->key, key)) {
            *out_index = idx;
            *out_found = 1;
            return 1;
        }
    }
    return 0; /* table full */
}

void flagstore_init(FlagStore *st,
                    FlagStoreEntry *entries, int entries_cap,
                    char *pool_buf, int pool_cap)
{
    int i;
    if (!st) return;
    st->entries = entries;
    st->cap = entries_cap;
    st->count = 0;
    flags_pool_init(&st->pool, pool_buf, pool_cap);

    if (st->entries && st->cap > 0) {
        for (i = 0; i < st->cap; ++i) {
            st->entries[i].used = 0;
            st->entries[i].hash = 0UL;
            st->entries[i].key = 0;
            st->entries[i].val = flags_value_none();
        }
    }
}

void flagstore_clear(FlagStore *st)
{
    int i;
    if (!st) return;
    if (st->entries && st->cap > 0) {
        for (i = 0; i < st->cap; ++i) {
            st->entries[i].used = 0;
            st->entries[i].hash = 0UL;
            st->entries[i].key = 0;
            st->entries[i].val = flags_value_none();
        }
    }
    st->count = 0;
    flags_pool_reset(&st->pool);
}

static int _store_value_dup_into_pool(FlagStore *st, const FlagsValue *in, FlagsValue *out)
{
    if (!st || !in || !out) return 0;
    *out = *in;

    if (in->type == FLAGS_VAL_STR) {
        const char *dup;
        int n;
        if (!in->as.s) {
            out->as.s = "";
            return 1;
        }
        n = _cstr_len(in->as.s);
        dup = flags_pool_dup(&st->pool, in->as.s, n);
        if (!dup) return 0;
        out->as.s = dup;
    }
    return 1;
}

int flagstore_set(FlagStore *st, const char *key, const FlagsValue *val)
{
    unsigned long h;
    int idx, found;
    FlagStoreEntry *e;
    const char *kdup;
    FlagsValue vdup;

    if (!st || !key || !val) return 0;
    if (!st->entries || st->cap <= 0) return 0;

    h = _hash_key(key);
    if (!_find_slot(st, key, h, &idx, &found)) return 0;

    e = &st->entries[idx];

    if (!found) {
        /* insert */
        kdup = flags_pool_dup_cstr(&st->pool, key);
        if (!kdup) return 0;

        e->used = 1;
        e->hash = h;
        e->key = kdup;
        st->count++;
    }

    if (!_store_value_dup_into_pool(st, val, &vdup)) return 0;
    e->val = vdup;
    return 1;
}

int flagstore_set_bool(FlagStore *st, const char *key, int b)
{
    FlagsValue v;
    v = flags_value_bool(b);
    return flagstore_set(st, key, &v);
}

int flagstore_set_int(FlagStore *st, const char *key, long i)
{
    FlagsValue v;
    v = flags_value_int(i);
    return flagstore_set(st, key, &v);
}

int flagstore_set_fx(FlagStore *st, const char *key, flags_fx_t fx)
{
    FlagsValue v;
    v = flags_value_fx(fx);
    return flagstore_set(st, key, &v);
}

int flagstore_set_str(FlagStore *st, const char *key, const char *s)
{
    FlagsValue v;
    v = flags_value_str(s);
    return flagstore_set(st, key, &v);
}

int flagstore_get(const FlagStore *st, const char *key, FlagsValue *out_val)
{
    unsigned long h;
    int idx, found;
    const FlagStoreEntry *e;

    if (!st || !key || !out_val) return 0;
    if (!st->entries || st->cap <= 0) return 0;

    h = _hash_key(key);
    if (!_find_slot(st, key, h, &idx, &found)) return 0;
    if (!found) return 0;

    e = &st->entries[idx];
    *out_val = e->val;
    return 1;
}

int flagstore_toggle(FlagStore *st, const char *key, int *out_new)
{
    FlagsValue v;
    int cur;
    int has;

    if (!st || !key) return 0;

    has = flagstore_get(st, key, &v);
    cur = 0;
    if (has && v.type == FLAGS_VAL_BOOL) {
        cur = (v.as.i != 0) ? 1 : 0;
    } else {
        cur = 0;
    }
    cur = (cur == 0) ? 1 : 0;
    if (!flagstore_set_bool(st, key, cur)) return 0;
    if (out_new) *out_new = cur;
    return 1;
}

/* ---- JSON serialization helpers (no stdio) ---- */
static int _out_ch(char *out, int out_cap, int *io, char c)
{
    if (!out || out_cap <= 0 || !io) return 0;
    if (*io >= out_cap - 1) return 0; /* reserve NUL */
    out[*io] = c;
    (*io)++;
    out[*io] = '\0';
    return 1;
}

static int _out_str(char *out, int out_cap, int *io, const char *s)
{
    int i;
    if (!s) s = "";
    i = 0;
    while (s[i] != '\0') {
        if (!_out_ch(out, out_cap, io, s[i])) return 0;
        i++;
    }
    return 1;
}

static int _out_uint(char *out, int out_cap, int *io, unsigned long v)
{
    char tmp[32];
    int n;
    n = 0;
    if (v == 0UL) {
        tmp[n++] = '0';
    } else {
        while (v > 0UL && n < (int)sizeof(tmp)) {
            tmp[n++] = (char)('0' + (char)(v % 10UL));
            v /= 10UL;
        }
    }
    while (n > 0) {
        n--;
        if (!_out_ch(out, out_cap, io, tmp[n])) return 0;
    }
    return 1;
}

static int _out_int(char *out, int out_cap, int *io, long v)
{
    unsigned long u;
    if (v < 0) {
        if (!_out_ch(out, out_cap, io, '-')) return 0;
        u = (unsigned long)(-v);
    } else {
        u = (unsigned long)v;
    }
    return _out_uint(out, out_cap, io, u);
}

static int _out_json_escaped(char *out, int out_cap, int *io, const char *s)
{
    int i;
    unsigned char c;
    if (!s) s = "";
    i = 0;
    while (s[i] != '\0') {
        c = (unsigned char)s[i];
        if (c == '\"' || c == '\\') {
            if (!_out_ch(out, out_cap, io, '\\')) return 0;
            if (!_out_ch(out, out_cap, io, (char)c)) return 0;
        } else if (c == '\n') {
            if (!_out_str(out, out_cap, io, "\\n")) return 0;
        } else if (c == '\r') {
            if (!_out_str(out, out_cap, io, "\\r")) return 0;
        } else if (c == '\t') {
            if (!_out_str(out, out_cap, io, "\\t")) return 0;
        } else if (c < 0x20) {
            /* control chars: emit as '?' to keep it simple */
            if (!_out_ch(out, out_cap, io, '?')) return 0;
        } else {
            if (!_out_ch(out, out_cap, io, (char)c)) return 0;
        }
        i++;
    }
    return 1;
}

static int _out_fx(char *out, int out_cap, int *io, flags_fx_t fx)
{
    long ip;
    long frac;
    unsigned long ufrac;
    int neg;
    flags_fx_t afx;

    neg = 0;
    afx = fx;
    if (fx < 0) { neg = 1; afx = -fx; }

    ip = (long)(afx >> FLAGS_FX_FRAC_BITS);

    /* 4 digits fractional: frac = round((afx & 0xFFFF) * 10000 / 65536) */
    frac = (long)(( (afx & (FLAGS_FX_ONE - 1L)) * 10000L ) >> FLAGS_FX_FRAC_BITS);
    if (frac < 0) frac = 0;
    if (frac > 9999) frac = 9999;
    ufrac = (unsigned long)frac;

    if (neg) {
        if (!_out_ch(out, out_cap, io, '-')) return 0;
    }
    if (!_out_int(out, out_cap, io, ip)) return 0;
    if (!_out_ch(out, out_cap, io, '.')) return 0;

    /* pad to 4 digits */
    if (ufrac < 1000UL) { if (!_out_ch(out, out_cap, io, '0')) return 0; }
    if (ufrac < 100UL)  { if (!_out_ch(out, out_cap, io, '0')) return 0; }
    if (ufrac < 10UL)   { if (!_out_ch(out, out_cap, io, '0')) return 0; }

    if (!_out_uint(out, out_cap, io, ufrac)) return 0;
    return 1;
}

int flagstore_to_json(const FlagStore *st, char *out, int out_cap, int pretty)
{
    int i;
    int io;
    int first;

    if (!st || !out || out_cap <= 0) return 0;
    out[0] = '\0';
    io = 0;

    if (!_out_ch(out, out_cap, &io, '{')) return 0;
    if (pretty) {
        if (!_out_ch(out, out_cap, &io, '\n')) return 0;
    }

    first = 1;
    for (i = 0; i < st->cap; ++i) {
        const FlagStoreEntry *e;
        e = &st->entries[i];
        if (!e->used || !e->key) continue;

        if (!first) {
            if (!_out_ch(out, out_cap, &io, ',')) return 0;
            if (pretty) {
                if (!_out_ch(out, out_cap, &io, '\n')) return 0;
            }
        }
        first = 0;

        if (pretty) {
            if (!_out_str(out, out_cap, &io, "  ")) return 0;
        }

        if (!_out_ch(out, out_cap, &io, '\"')) return 0;
        if (!_out_json_escaped(out, out_cap, &io, e->key)) return 0;
        if (!_out_ch(out, out_cap, &io, '\"')) return 0;

        if (!_out_ch(out, out_cap, &io, ':')) return 0;
        if (pretty) {
            if (!_out_ch(out, out_cap, &io, ' ')) return 0;
        }

        /* value */
        if (e->val.type == FLAGS_VAL_BOOL) {
            if (e->val.as.i) {
                if (!_out_str(out, out_cap, &io, "true")) return 0;
            } else {
                if (!_out_str(out, out_cap, &io, "false")) return 0;
            }
        } else if (e->val.type == FLAGS_VAL_INT) {
            if (!_out_int(out, out_cap, &io, e->val.as.i)) return 0;
        } else if (e->val.type == FLAGS_VAL_FX) {
            if (!_out_fx(out, out_cap, &io, e->val.as.fx)) return 0;
        } else if (e->val.type == FLAGS_VAL_STR) {
            if (!_out_ch(out, out_cap, &io, '\"')) return 0;
            if (!_out_json_escaped(out, out_cap, &io, e->val.as.s)) return 0;
            if (!_out_ch(out, out_cap, &io, '\"')) return 0;
        } else {
            if (!_out_str(out, out_cap, &io, "null")) return 0;
        }
    }

    if (pretty) {
        if (!_out_ch(out, out_cap, &io, '\n')) return 0;
    }
    if (!_out_ch(out, out_cap, &io, '}')) return 0;

    return io;
}
