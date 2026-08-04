/* conf_total.c - single-translation-unit library
   C89-only, no stdio.h, no stdlib.h, no string.h
*/
#include "conf_total.h"

/* ----------------------------------------
   Tiny helpers (no libc)
   ---------------------------------------- */

static int conf_is_space(char c) { return c == ' ' || c == '\t'; }
static int conf_is_digit(char c) { return c >= '0' && c <= '9'; }
static int conf_is_ident0(char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           (c == '_' || c == '-');
}
static int conf_is_ident(char c) {
    return conf_is_ident0(c) || conf_is_digit(c);
}

static unsigned int conf_align_up(unsigned int x, unsigned int a) {
    unsigned int r;
    if (a == 0u) return x;
    r = x % a;
    if (r == 0u) return x;
    return x + (a - r);
}

static void conf_set_err(conf_ctx_t *ctx, conf_err_t code, unsigned int pos, unsigned int line, unsigned int col) {
    if (!ctx) return;
    ctx->err.code = code;
    ctx->err.pos = pos;
    ctx->err.line = line ? line : 1u;
    ctx->err.col = col ? col : 1u;
}

const conf_error_t *conf_last_error(const conf_ctx_t *ctx) {
    if (!ctx) return 0;
    return &ctx->err;
}

static int conf_slice_eq(conf_slice_t a, conf_slice_t b) {
    unsigned int i;
    if (a.len != b.len) return 0;
    for (i = 0u; i < a.len; ++i) {
        if (a.ptr[i] != b.ptr[i]) return 0;
    }
    return 1;
}

static int conf_slice_eq_cstr_ci(conf_slice_t a, const char *b) {
    /* case-insensitive compare to NUL-terminated ascii word */
    unsigned int i = 0u;
    char cb;
    while (1) {
        if (i >= a.len) {
            return b[i] == '\0';
        }
        cb = b[i];
        if (cb == '\0') return 0;
        {
            char ca = a.ptr[i];
            /* tolower without ctype */
            if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
            if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
            if (ca != cb) return 0;
        }
        ++i;
    }
    /* unreachable */
    /* return 0; */
}

static conf_slice_t conf_slice_trim(conf_slice_t s) {
    unsigned int a = 0u;
    unsigned int b = s.len;
    while (a < b && conf_is_space(s.ptr[a])) a++;
    while (b > a && conf_is_space(s.ptr[b-1u])) b--;
    s.ptr += a;
    s.len = b - a;
    return s;
}

static conf_slice_t conf_slice_make(const char *p, unsigned int len) {
    conf_slice_t s;
    s.ptr = p;
    s.len = len;
    return s;
}

static conf_slice_t conf_slice_zero(void) {
    conf_slice_t s;
    s.ptr = 0;
    s.len = 0u;
    return s;
}


/* ----------------------------------------
   Arena allocator (no malloc)
   ---------------------------------------- */

static void *conf_arena_alloc(conf_ctx_t *ctx, unsigned int size, unsigned int align) {
    unsigned int off, off2;
    if (!ctx || !ctx->arena_mem) return 0;
    if (align == 0u) align = (unsigned int)sizeof(void*);
    off = conf_align_up(ctx->arena_off, align);
    off2 = off + size;
    if (off2 > ctx->arena_cap) {
        conf_set_err(ctx, CONF_ERR_OOM, 0u, 1u, 1u);
        return 0;
    }
    ctx->arena_off = off2;
    return (void*)(ctx->arena_mem + off);
}

static conf_slice_t conf_arena_copy(conf_ctx_t *ctx, const char *ptr, unsigned int len, int add_nul) {
    conf_slice_t out;
    unsigned int i;
    unsigned int extra = add_nul ? 1u : 0u;
    char *dst = (char*)conf_arena_alloc(ctx, len + extra, 1u);
    out.ptr = 0;
    out.len = 0u;
    if (!dst) return out;
    for (i = 0u; i < len; ++i) dst[i] = ptr[i];
    if (add_nul) dst[len] = '\0';
    out.ptr = dst;
    out.len = len;
    return out;
}

/* ----------------------------------------
   Canonical nodes builders
   ---------------------------------------- */

static conf_value_t *conf_new_value(conf_ctx_t *ctx, conf_type_t t) {
    conf_value_t *v = (conf_value_t*)conf_arena_alloc(ctx, (unsigned int)sizeof(conf_value_t), (unsigned int)sizeof(void*));
    if (!v) return 0;
    v->type = t;
    v->next = 0;
    /* clear union */
    v->as.t = 0;
    return v;
}

static conf_table_t *conf_new_table(conf_ctx_t *ctx) {
    conf_table_t *t = (conf_table_t*)conf_arena_alloc(ctx, (unsigned int)sizeof(conf_table_t), (unsigned int)sizeof(void*));
    if (!t) return 0;
    t->head = 0;
    return t;
}

static conf_array_t *conf_new_array(conf_ctx_t *ctx) {
    conf_array_t *a = (conf_array_t*)conf_arena_alloc(ctx, (unsigned int)sizeof(conf_array_t), (unsigned int)sizeof(void*));
    if (!a) return 0;
    a->head = 0;
    a->len = 0u;
    return a;
}

static conf_value_t *conf_make_table_value(conf_ctx_t *ctx) {
    conf_value_t *v = conf_new_value(ctx, CONF_TABLE);
    conf_table_t *t;
    if (!v) return 0;
    t = conf_new_table(ctx);
    if (!t) return 0;
    v->as.t = t;
    return v;
}

static conf_value_t *conf_make_array_value(conf_ctx_t *ctx) {
    conf_value_t *v = conf_new_value(ctx, CONF_ARRAY);
    conf_array_t *a;
    if (!v) return 0;
    a = conf_new_array(ctx);
    if (!a) return 0;
    v->as.a = a;
    return v;
}

static conf_pair_t *conf_table_find_pair(conf_table_t *t, conf_slice_t key) {
    conf_pair_t *p;
    for (p = t ? t->head : 0; p; p = p->next) {
        if (conf_slice_eq(p->key, key)) return p;
    }
    return 0;
}

static conf_err_t conf_table_put(conf_ctx_t *ctx, conf_table_t *t, conf_slice_t key, conf_value_t *val) {
    conf_pair_t *existing;
    conf_pair_t *p;

    if (!t) return CONF_ERR_SYNTAX;

    existing = conf_table_find_pair(t, key);
    if (existing) {
        if (ctx->load_flags & CONF_LOAD_OVERRIDE) {
            existing->value = val;
            return CONF_OK;
        }
        conf_set_err(ctx, CONF_ERR_DUPLICATE_KEY, 0u, 1u, 1u);
        return CONF_ERR_DUPLICATE_KEY;
    }

    p = (conf_pair_t*)conf_arena_alloc(ctx, (unsigned int)sizeof(conf_pair_t), (unsigned int)sizeof(void*));
    if (!p) return CONF_ERR_OOM;
    p->key = key;
    p->value = val;
    p->next = t->head;
    t->head = p;
    return CONF_OK;
}

static void conf_array_push(conf_ctx_t *ctx, conf_array_t *a, conf_value_t *elem) {
    (void)ctx;
    if (!a || !elem) return;
    if (!a->head) {
        a->head = elem;
    } else {
        conf_value_t *it = a->head;
        while (it->next) it = it->next;
        it->next = elem;
    }
    elem->next = 0;
    a->len += 1u;
}

/* ----------------------------------------
   Init/reset/fallback
   ---------------------------------------- */

void conf_ctx_init(conf_ctx_t *ctx, void *arena_mem, unsigned int arena_size) {
    if (!ctx) return;
    ctx->arena_mem = (unsigned char*)arena_mem;
    ctx->arena_cap = arena_size;
    ctx->arena_off = 0u;
    ctx->fallback = 0;
    ctx->err.code = CONF_OK;
    ctx->err.pos = 0u;
    ctx->err.line = 1u;
    ctx->err.col = 1u;
    ctx->load_flags = CONF_LOAD_COPY_SLICES; /* recommended default */
    ctx->root = conf_make_table_value(ctx);
    if (!ctx->root) {
        conf_set_err(ctx, CONF_ERR_OOM, 0u, 1u, 1u);
    }
}

void conf_ctx_reset(conf_ctx_t *ctx) {
    const conf_ctx_t *fb;
    unsigned int flags;
    if (!ctx) return;
    fb = ctx->fallback;
    flags = ctx->load_flags;
    ctx->arena_off = 0u;
    ctx->err.code = CONF_OK;
    ctx->err.pos = 0u;
    ctx->err.line = 1u;
    ctx->err.col = 1u;
    ctx->root = 0;
    ctx->fallback = fb;
    ctx->load_flags = flags;
    ctx->root = conf_make_table_value(ctx);
    if (!ctx->root) {
        conf_set_err(ctx, CONF_ERR_OOM, 0u, 1u, 1u);
    }
}

void conf_ctx_set_fallback(conf_ctx_t *ctx, const conf_ctx_t *fallback) {
    if (!ctx) return;
    ctx->fallback = fallback;
}

void conf_ctx_set_load_flags(conf_ctx_t *ctx, unsigned int flags) {
    if (!ctx) return;
    ctx->load_flags = flags;
    if ((ctx->load_flags & CONF_LOAD_COPY_SLICES) == 0u) {
        /* NOTE: we keep it allowed, but most callers want COPY_SLICES */
    }
}

const conf_value_t *conf_root(const conf_ctx_t *ctx) {
    return ctx ? ctx->root : 0;
}

/* ----------------------------------------
   Query: non-path helpers
   ---------------------------------------- */

unsigned int conf_array_len(const conf_value_t *v) {
    if (!v || v->type != CONF_ARRAY || !v->as.a) return 0u;
    return v->as.a->len;
}

const conf_value_t *conf_array_at(const conf_value_t *v, unsigned int idx) {
    unsigned int i;
    const conf_value_t *it;
    if (!v || v->type != CONF_ARRAY || !v->as.a) return 0;
    it = v->as.a->head;
    i = 0u;
    while (it) {
        if (i == idx) return it;
        it = it->next;
        ++i;
    }
    return 0;
}

const conf_value_t *conf_table_get_slice(const conf_value_t *v, conf_slice_t key) {
    const conf_pair_t *p;
    if (!v || v->type != CONF_TABLE || !v->as.t) return 0;
    for (p = v->as.t->head; p; p = p->next) {
        if (conf_slice_eq(p->key, key)) return p->value;
    }
    return 0;
}

/* ----------------------------------------
   Path resolver with fallback layering
   ---------------------------------------- */

static const conf_value_t *conf_resolve_in_table(const conf_value_t *table_v, conf_slice_t key) {
    return conf_table_get_slice(table_v, key);
}


static int conf_parse_uint_from_cstr(const char **pp, unsigned int *out) {
    const char *p = *pp;
    unsigned int v = 0u;
    int any = 0;
    while (*p >= '0' && *p <= '9') {
        any = 1;
        v = (unsigned int)(v * 10u + (unsigned int)(*p - '0'));
        ++p;
    }
    if (!any) return 0;
    *pp = p;
    *out = v;
    return 1;
}

static conf_slice_t conf_path_read_ident(const char **pp) {
    const char *p = *pp;
    const char *start = p;
    conf_slice_t s;
    while (conf_is_ident(*p)) ++p;
    s.ptr = start;
    s.len = (unsigned int)(p - start);
    *pp = p;
    return s;
}

static const conf_value_t *conf_get_impl(const conf_ctx_t *ctx, const char *path) {
    const char *p;
    const conf_value_t *cur_p;
    const conf_value_t *cur_f;

    if (!ctx || !ctx->root) return 0;
    p = path ? path : "";
    cur_p = ctx->root;
    cur_f = (ctx->fallback && ctx->fallback->root) ? ctx->fallback->root : 0;

    /* allow empty path => root */
    if (*p == '\0') return cur_p;

    while (*p) {
        /* leading dot is not allowed, but we tolerate */
        if (*p == '.') { ++p; continue; }

        /* If current is array, allow [index] segments directly */
        if (*p == '[') {
            unsigned int idx;
            const conf_value_t *vp = cur_p;
            const conf_value_t *vf = cur_f;
            if ((vp && vp->type != CONF_ARRAY) && (vf && vf->type != CONF_ARRAY)) return 0;
            ++p;
            if (!conf_parse_uint_from_cstr(&p, &idx)) return 0;
            if (*p != ']') return 0;
            ++p;
            /* pick element: primary first, else fallback */
            if (vp && vp->type == CONF_ARRAY) cur_p = conf_array_at(vp, idx);
            else cur_p = 0;
            if (vf && vf->type == CONF_ARRAY) cur_f = conf_array_at(vf, idx);
            else cur_f = 0;
            if (!cur_p && cur_f) { cur_p = cur_f; cur_f = 0; }
            continue;
        }

        /* must be table segment key */
        {
            conf_slice_t seg = conf_path_read_ident(&p);
            if (seg.len == 0u) return 0;

            if ((cur_p && cur_p->type != CONF_TABLE) && (cur_f && cur_f->type != CONF_TABLE)) return 0;

            /* fetch from both */
            {
                const conf_value_t *next_p = 0;
                const conf_value_t *next_f = 0;
                const conf_value_t *chosen;

                if (cur_p && cur_p->type == CONF_TABLE) next_p = conf_resolve_in_table(cur_p, seg);
                if (cur_f && cur_f->type == CONF_TABLE) next_f = conf_resolve_in_table(cur_f, seg);

                chosen = next_p ? next_p : next_f;
                if (!chosen) return 0;

                /* advance: if chosen from fallback, primary becomes null */
                if (next_p) {
                    cur_p = next_p;
                    cur_f = next_f;
                } else {
                    cur_p = next_f;
                    cur_f = 0;
                }
            }

            /* optional array index immediately after key */
            while (*p == '[') {
                unsigned int idx;
                const conf_value_t *vp = cur_p;
                const conf_value_t *vf = cur_f;
                ++p;
                if (!conf_parse_uint_from_cstr(&p, &idx)) return 0;
                if (*p != ']') return 0;
                ++p;
                if (vp && vp->type == CONF_ARRAY) cur_p = conf_array_at(vp, idx);
                else cur_p = 0;
                if (vf && vf->type == CONF_ARRAY) cur_f = conf_array_at(vf, idx);
                else cur_f = 0;
                if (!cur_p && cur_f) { cur_p = cur_f; cur_f = 0; }
            }
        }

        if (*p == '.') ++p;
    }

    return cur_p;
}

const conf_value_t *conf_get(const conf_ctx_t *ctx, const char *path) {
    return conf_get_impl(ctx, path);
}

int conf_get_flag(const conf_ctx_t *ctx, const char *path, int def) {
    const conf_value_t *v = conf_get_impl(ctx, path);
    if (!v) return def;
    if (v->type == CONF_NULL) return 1;
    if (v->type == CONF_BOOL) return v->as.b ? 1 : 0;
    return 1; /* presence => true */
}

int conf_get_bool(const conf_ctx_t *ctx, const char *path, int def) {
    const conf_value_t *v = conf_get_impl(ctx, path);
    if (!v) return def;
    if (v->type == CONF_NULL) return 1;
    if (v->type != CONF_BOOL) return def;
    return v->as.b ? 1 : 0;
}

long conf_get_int(const conf_ctx_t *ctx, const char *path, long def) {
    const conf_value_t *v = conf_get_impl(ctx, path);
    if (!v) return def;
    if (v->type != CONF_INT) return def;
    return v->as.i;
}

conf_fixed_t conf_get_fixed(const conf_ctx_t *ctx, const char *path, conf_fixed_t def) {
    const conf_value_t *v = conf_get_impl(ctx, path);
    if (!v) return def;
    if (v->type == CONF_FIXED) return v->as.fx;
    if (v->type == CONF_INT) return CONF_FIXED_FROM_INT(v->as.i);
    return def;
}

conf_slice_t conf_get_string(const conf_ctx_t *ctx, const char *path, conf_slice_t def) {
    const conf_value_t *v = conf_get_impl(ctx, path);
    if (!v) return def;
    if (v->type == CONF_STRING || v->type == CONF_DATETIME) return v->as.s;
    return def;
}

/* ----------------------------------------
   Override (write API): create tables along dot-path
   (arrays in path are allowed for indexing existing arrays-of-tables)
   ---------------------------------------- */


static conf_err_t conf_override_value(conf_ctx_t *ctx, const char *path, conf_value_t *value) {
    const char *p;
    conf_value_t *cur;
    if (!ctx || !ctx->root || !path) return CONF_ERR_SYNTAX;

    p = path;
    cur = ctx->root;
    while (*p) {
        if (*p == '.') { ++p; continue; }

        if (*p == '[') {
            /* array indexing requires current value to be array */
            unsigned int idx;
            ++p;
            if (!conf_parse_uint_from_cstr(&p, &idx)) return CONF_ERR_SYNTAX;
            if (*p != ']') return CONF_ERR_SYNTAX;
            ++p;
            if (!cur || cur->type != CONF_ARRAY) return CONF_ERR_TYPE_MISMATCH;
            cur = (conf_value_t*)conf_array_at(cur, idx);
            if (!cur) return CONF_ERR_SYNTAX;
            continue;
        }

        {
            conf_slice_t seg = conf_path_read_ident(&p);
            conf_value_t *next;
            if (seg.len == 0u) return CONF_ERR_SYNTAX;

            /* copy seg into arena for stable key */
            if (ctx->load_flags & CONF_LOAD_COPY_SLICES) {
                seg = conf_arena_copy(ctx, seg.ptr, seg.len, 1);
            }

            /* are we at final segment? final if no more . or [ after optional whitespace */
            {
                const char *save = p;
                while (*save == '.') { /* we'll treat dot as more segments */
                    break;
                }
                /* if end or dot or [ => not final decided yet */
            }

            /* if next char begins end-of-path => final segment */
            if (*p == '\0') {
                /* set/replace in cur table; override API is always replace-capable */
                conf_pair_t *existing;
                if (!cur || cur->type != CONF_TABLE) return CONF_ERR_TYPE_MISMATCH;
                existing = conf_table_find_pair(cur->as.t, seg);
                if (existing) {
                    existing->value = value;
                    return CONF_OK;
                }
                return conf_table_put(ctx, cur->as.t, seg, value);
            }

            /* if next char is '.' or '[' => still may be deeper; ensure table under this key */
            if (*p == '.' || *p == '[') {
                if (!cur || cur->type != CONF_TABLE) return CONF_ERR_TYPE_MISMATCH;

                next = (conf_value_t*)conf_table_get_slice(cur, seg);
                if (!next) {
                    conf_err_t e;
                    conf_value_t *tv = conf_make_table_value(ctx);
                    if (!tv) return CONF_ERR_OOM;
                    e = conf_table_put(ctx, cur->as.t, seg, tv);
                    if (e != CONF_OK) return e;
                    next = tv;
                } else {
                    if (next->type != CONF_TABLE && next->type != CONF_ARRAY) {
                        return CONF_ERR_TYPE_MISMATCH;
                    }
                }
                cur = next;

                /* handle immediate [index] after key for arrays-of-tables */
                while (*p == '[') {
                    unsigned int idx;
                    if (*p != '[') break;
                    ++p;
                    if (!conf_parse_uint_from_cstr(&p, &idx)) return CONF_ERR_SYNTAX;
                    if (*p != ']') return CONF_ERR_SYNTAX;
                    ++p;
                    if (!cur || cur->type != CONF_ARRAY) return CONF_ERR_TYPE_MISMATCH;
                    cur = (conf_value_t*)conf_array_at(cur, idx);
                    if (!cur) return CONF_ERR_SYNTAX;
                }

                continue;
            }

            /* otherwise unknown separator */
            return CONF_ERR_SYNTAX;
        }
    }
    return CONF_ERR_SYNTAX;
}

conf_err_t conf_override_bool(conf_ctx_t *ctx, const char *path, int v) {
    conf_value_t *val = conf_new_value(ctx, CONF_BOOL);
    if (!val) return CONF_ERR_OOM;
    val->as.b = v ? 1 : 0;
    return conf_override_value(ctx, path, val);
}

conf_err_t conf_override_int(conf_ctx_t *ctx, const char *path, long v) {
    conf_value_t *val = conf_new_value(ctx, CONF_INT);
    if (!val) return CONF_ERR_OOM;
    val->as.i = v;
    return conf_override_value(ctx, path, val);
}

conf_err_t conf_override_fixed(conf_ctx_t *ctx, const char *path, conf_fixed_t v) {
    conf_value_t *val = conf_new_value(ctx, CONF_FIXED);
    if (!val) return CONF_ERR_OOM;
    val->as.fx = v;
    return conf_override_value(ctx, path, val);
}

conf_err_t conf_override_string(conf_ctx_t *ctx, const char *path, const char *bytes, unsigned int len) {
    conf_value_t *val = conf_new_value(ctx, CONF_STRING);
    conf_slice_t s;
    if (!val) return CONF_ERR_OOM;
    s.ptr = bytes;
    s.len = len;
    if (ctx->load_flags & CONF_LOAD_COPY_SLICES) {
        s = conf_arena_copy(ctx, bytes, len, 1);
        if (!s.ptr) return CONF_ERR_OOM;
    }
    val->as.s = s;
    return conf_override_value(ctx, path, val);
}

/* ----------------------------------------
   TOML Lexer
   ---------------------------------------- */

typedef enum {
    TT_EOF = 0,
    TT_NEWLINE,

    TT_IDENT,      /* bare key */
    TT_STRING,     /* already unescaped? lexer keeps raw; parser unescapes */
    TT_NUMBER,     /* raw slice */
    TT_BOOL,       /* true/false raw slice */

    TT_EQ,         /* = */
    TT_DOT,        /* . */
    TT_COMMA,      /* , */
    TT_LBRACK,     /* [ */
    TT_RBRACK,     /* ] */
    TT_LBRACE,     /* { */
    TT_RBRACE      /* } */
} toml_tok_type_t;

typedef struct {
    toml_tok_type_t type;
    const char *ptr;
    unsigned int len;
    unsigned int pos;
    unsigned int line;
    unsigned int col;
    int str_kind; /* 0=none, 1=basic, 2=literal, 3=ml_basic, 4=ml_literal */
} toml_tok_t;

typedef struct {
    const char *src;
    unsigned int len;
    unsigned int pos;
    unsigned int line;
    unsigned int col;
} toml_lex_t;

static void toml_lex_init(toml_lex_t *lx, const char *src, unsigned int len) {
    lx->src = src;
    lx->len = len;
    lx->pos = 0u;
    lx->line = 1u;
    lx->col = 1u;
}

static char toml_peek(toml_lex_t *lx) {
    if (lx->pos >= lx->len) return '\0';
    return lx->src[lx->pos];
}

static char toml_peek2(toml_lex_t *lx, unsigned int ahead) {
    unsigned int p = lx->pos + ahead;
    if (p >= lx->len) return '\0';
    return lx->src[p];
}

static char toml_getc(toml_lex_t *lx) {
    char c;
    if (lx->pos >= lx->len) return '\0';
    c = lx->src[lx->pos++];
    if (c == '\n') {
        lx->line += 1u;
        lx->col = 1u;
    } else {
        lx->col += 1u;
    }
    return c;
}

static void toml_skip_ws_and_comments(toml_lex_t *lx) {
    while (1) {
        char c = toml_peek(lx);
        /* whitespace */
        while (c == ' ' || c == '\t') {
            toml_getc(lx);
            c = toml_peek(lx);
        }
        /* comment */
        if (c == '#') {
            while (c && c != '\n') {
                toml_getc(lx);
                c = toml_peek(lx);
            }
            /* newline will be handled by tokenization */
            continue;
        }
        break;
    }
}

static int toml_is_barekey_char(char c) {
    return conf_is_ident(c);
}

static toml_tok_t toml_make_tok(toml_tok_type_t t, const char *ptr, unsigned int len, unsigned int pos, unsigned int line, unsigned int col) {
    toml_tok_t tok;
    tok.type = t;
    tok.ptr = ptr;
    tok.len = len;
    tok.pos = pos;
    tok.line = line;
    tok.col = col;
    tok.str_kind = 0;
    return tok;
}

static toml_tok_t toml_next(conf_ctx_t *ctx, toml_lex_t *lx) {
    toml_tok_t tok;
    unsigned int pos0, line0, col0;
    const char *start;
    char c;

    toml_skip_ws_and_comments(lx);

    pos0 = lx->pos;
    line0 = lx->line;
    col0 = lx->col;

    c = toml_peek(lx);
    if (c == '\0') {
        tok = toml_make_tok(TT_EOF, lx->src + lx->pos, 0u, pos0, line0, col0);
        return tok;
    }

    if (c == '\r') {
        toml_getc(lx);
        if (toml_peek(lx) == '\n') toml_getc(lx);
        tok = toml_make_tok(TT_NEWLINE, lx->src + pos0, (unsigned int)(lx->pos - pos0), pos0, line0, col0);
        return tok;
    }
    if (c == '\n') {
        toml_getc(lx);
        tok = toml_make_tok(TT_NEWLINE, lx->src + pos0, 1u, pos0, line0, col0);
        return tok;
    }

    /* punctuation */
    if (c == '=') { toml_getc(lx); return toml_make_tok(TT_EQ, lx->src + pos0, 1u, pos0, line0, col0); }
    if (c == '.') { toml_getc(lx); return toml_make_tok(TT_DOT, lx->src + pos0, 1u, pos0, line0, col0); }
    if (c == ',') { toml_getc(lx); return toml_make_tok(TT_COMMA, lx->src + pos0, 1u, pos0, line0, col0); }
    if (c == '[') { toml_getc(lx); return toml_make_tok(TT_LBRACK, lx->src + pos0, 1u, pos0, line0, col0); }
    if (c == ']') { toml_getc(lx); return toml_make_tok(TT_RBRACK, lx->src + pos0, 1u, pos0, line0, col0); }
    if (c == '{') { toml_getc(lx); return toml_make_tok(TT_LBRACE, lx->src + pos0, 1u, pos0, line0, col0); }
    if (c == '}') { toml_getc(lx); return toml_make_tok(TT_RBRACE, lx->src + pos0, 1u, pos0, line0, col0); }

    /* strings: basic/literal and multiline */
    if (c == '"' || c == '\'') {
        char quote = c;
        int multiline = 0;
        int kind;
        /* detect triple quotes */
        if (toml_peek2(lx, 0u) == quote && toml_peek2(lx, 1u) == quote && toml_peek2(lx, 2u) == quote) {
            multiline = 1;
            toml_getc(lx); toml_getc(lx); toml_getc(lx);
        } else {
            toml_getc(lx);
        }
        kind = (quote == '"') ? (multiline ? 3 : 1) : (multiline ? 4 : 2);
        start = lx->src + lx->pos;

        while (1) {
            char d = toml_peek(lx);
            if (d == '\0') {
                conf_set_err(ctx, CONF_ERR_SYNTAX, pos0, line0, col0);
                return toml_make_tok(TT_EOF, lx->src + pos0, 0u, pos0, line0, col0);
            }
            if (!multiline) {
                if (d == quote) {
                    unsigned int len = (unsigned int)((lx->src + lx->pos) - start);
                    toml_getc(lx);
                    tok = toml_make_tok(TT_STRING, start, len, pos0, line0, col0);
                    tok.str_kind = kind;
                    return tok;
                }
                if (quote == '"' && d == '\\') {
                    toml_getc(lx);
                    /* skip escaped char (parser will validate) */
                    if (toml_peek(lx) != '\0') toml_getc(lx);
                    continue;
                }
                /* literal strings allow anything except newline and quote */
                if (d == '\n' || d == '\r') {
                    conf_set_err(ctx, CONF_ERR_SYNTAX, pos0, line0, col0);
                    return toml_make_tok(TT_EOF, lx->src + pos0, 0u, pos0, line0, col0);
                }
                toml_getc(lx);
            } else {
                /* multiline: ends at triple quote */
                if (d == quote && toml_peek2(lx, 1u) == quote && toml_peek2(lx, 2u) == quote) {
                    unsigned int len = (unsigned int)((lx->src + lx->pos) - start);
                    toml_getc(lx); toml_getc(lx); toml_getc(lx);
                    tok = toml_make_tok(TT_STRING, start, len, pos0, line0, col0);
                    tok.str_kind = kind;
                    return tok;
                }
                if (quote == '"' && d == '\\') {
                    toml_getc(lx);
                    if (toml_peek(lx) != '\0') toml_getc(lx);
                    continue;
                }
                toml_getc(lx);
            }
        }
    }

    /* numbers / date-times: read as raw NUMBER token, parser classifies.
       This must run before bare keys because digits are legal bare-key chars. */
    if (c == '+' || c == '-' || conf_is_digit(c)) {
        start = lx->src + lx->pos;
        toml_getc(lx);
        while (1) {
            char d = toml_peek(lx);
            /* stop on whitespace, comment, punctuation boundaries */
            if (d == '\0') break;
            if (d == ' ' || d == '\t' || d == '\n' || d == '\r') break;
            if (d == '#' || d == '=' || d == ',' || d == ']' || d == '}' ) break;
            /* allow digits/letters used by TOML numbers and datetimes */
            toml_getc(lx);
        }
        {
            unsigned int len = (unsigned int)((lx->src + lx->pos) - start);
            tok = toml_make_tok(TT_NUMBER, start, len, pos0, line0, col0);
            return tok;
        }
    }

    /* bare keys / identifiers / booleans */
    if (toml_is_barekey_char(c)) {
        start = lx->src + lx->pos;
        toml_getc(lx);
        while (toml_is_barekey_char(toml_peek(lx))) toml_getc(lx);
        {
            unsigned int len = (unsigned int)((lx->src + lx->pos) - start);
            tok = toml_make_tok(TT_IDENT, start, len, pos0, line0, col0);
            /* bool? */
            if (len == 4u && conf_slice_eq_cstr_ci(conf_slice_make(start, len), "true")) tok.type = TT_BOOL;
            if (len == 5u && conf_slice_eq_cstr_ci(conf_slice_make(start, len), "false")) tok.type = TT_BOOL;
            return tok;
        }
    }

    /* unknown */
    conf_set_err(ctx, CONF_ERR_SYNTAX, pos0, line0, col0);
    tok = toml_make_tok(TT_EOF, lx->src + lx->pos, 0u, pos0, line0, col0);
    return tok;
}

/* ----------------------------------------
   TOML value parsing helpers
   ---------------------------------------- */

static int conf_hex_val(char c, unsigned int *out) {
    if (c >= '0' && c <= '9') { *out = (unsigned int)(c - '0'); return 1; }
    if (c >= 'a' && c <= 'f') { *out = (unsigned int)(c - 'a' + 10); return 1; }
    if (c >= 'A' && c <= 'F') { *out = (unsigned int)(c - 'A' + 10); return 1; }
    return 0;
}

static int conf_utf8_encode(conf_ctx_t *ctx, unsigned int cp, conf_slice_t *out_bytes) {
    /* returns slice into arena with encoded bytes (no NUL), appends exactly 1-4 bytes */
    unsigned char tmp[4];
    unsigned int n = 0u;

    if (cp <= 0x7Fu) {
        tmp[0] = (unsigned char)cp; n = 1u;
    } else if (cp <= 0x7FFu) {
        tmp[0] = (unsigned char)(0xC0u | ((cp >> 6) & 0x1Fu));
        tmp[1] = (unsigned char)(0x80u | (cp & 0x3Fu));
        n = 2u;
    } else if (cp <= 0xFFFFu) {
        tmp[0] = (unsigned char)(0xE0u | ((cp >> 12) & 0x0Fu));
        tmp[1] = (unsigned char)(0x80u | ((cp >> 6) & 0x3Fu));
        tmp[2] = (unsigned char)(0x80u | (cp & 0x3Fu));
        n = 3u;
    } else if (cp <= 0x10FFFFu) {
        tmp[0] = (unsigned char)(0xF0u | ((cp >> 18) & 0x07u));
        tmp[1] = (unsigned char)(0x80u | ((cp >> 12) & 0x3Fu));
        tmp[2] = (unsigned char)(0x80u | ((cp >> 6) & 0x3Fu));
        tmp[3] = (unsigned char)(0x80u | (cp & 0x3Fu));
        n = 4u;
    } else {
        return 0;
    }

    out_bytes->ptr = (const char*)conf_arena_alloc(ctx, n, 1u);
    if (!out_bytes->ptr) return 0;
    {
        unsigned int i;
        for (i = 0u; i < n; ++i) ((char*)out_bytes->ptr)[i] = (char)tmp[i];
    }
    out_bytes->len = n;
    return 1;
}

static int conf_parse_uhex(const char *p, unsigned int nhex, unsigned int *out_cp) {
    unsigned int cp = 0u;
    unsigned int i;
    for (i = 0u; i < nhex; ++i) {
        unsigned int hv;
        if (!conf_hex_val(p[i], &hv)) return 0;
        cp = (cp << 4) | hv;
    }
    *out_cp = cp;
    return 1;
}

static conf_slice_t conf_toml_unescape_basic(conf_ctx_t *ctx, conf_slice_t raw, int multiline) {
    /* raw is inside quotes, may contain escapes.
       We decode to arena, and NUL-terminate for convenience.
       Returns empty slice on error.
    */
    conf_slice_t out;
    char *dst;
    unsigned int i = 0u;
    unsigned int out_cap = raw.len + 1u; /* safe upper bound (unicode escapes may expand but still <= raw.len in bytes? not always) */
    /* worst case: \uXXXX expands to up to 3 bytes, raw is 6 bytes, still <= raw.len */
    dst = (char*)conf_arena_alloc(ctx, out_cap, 1u);
    out.ptr = 0;
    out.len = 0u;
    if (!dst) return out;

    while (i < raw.len) {
        char c = raw.ptr[i++];

        if (c != '\\') {
            /* In multiline basic strings, a backslash at end-of-line can trim newline; we handle only when c=='\\' */
            dst[out.len++] = c;
            continue;
        }

        if (i >= raw.len) { conf_set_err(ctx, CONF_ERR_BAD_ESCAPE, 0u, 1u, 1u); return conf_slice_zero(); }
        c = raw.ptr[i++];

        if (multiline && c == '\n') {
            /* line ending backslash newline trims following whitespace/newline per TOML; we'll do minimal:
               consume immediate whitespace after newline.
            */
            while (i < raw.len && (raw.ptr[i] == ' ' || raw.ptr[i] == '\t' || raw.ptr[i] == '\r' || raw.ptr[i] == '\n')) {
                if (raw.ptr[i] == '\n') { i++; break; }
                i++;
            }
            continue;
        }

        switch (c) {
            case 'b': dst[out.len++] = '\b'; break;
            case 't': dst[out.len++] = '\t'; break;
            case 'n': dst[out.len++] = '\n'; break;
            case 'f': dst[out.len++] = '\f'; break;
            case 'r': dst[out.len++] = '\r'; break;
            case '"': dst[out.len++] = '"';  break;
            case '\\': dst[out.len++] = '\\'; break;
            case 'u':
            case 'U':
            {
                unsigned int cp;
                unsigned int nhex = (c == 'u') ? 4u : 8u;
                conf_slice_t enc;
                if (i + nhex > raw.len) { conf_set_err(ctx, CONF_ERR_BAD_ESCAPE, 0u, 1u, 1u); return conf_slice_zero(); }
                if (!conf_parse_uhex(raw.ptr + i, nhex, &cp)) { conf_set_err(ctx, CONF_ERR_BAD_ESCAPE, 0u, 1u, 1u); return conf_slice_zero(); }
                i += nhex;
                if (!conf_utf8_encode(ctx, cp, &enc)) { conf_set_err(ctx, CONF_ERR_BAD_ESCAPE, 0u, 1u, 1u); return conf_slice_zero(); }
                /* append bytes from enc */
                {
                    unsigned int j;
                    for (j = 0u; j < enc.len; ++j) {
                        dst[out.len++] = enc.ptr[j];
                    }
                }
                break;
            }
            default:
                conf_set_err(ctx, CONF_ERR_BAD_ESCAPE, 0u, 1u, 1u);
                return conf_slice_zero();
        }

        if (out.len + 5u >= out_cap) {
            /* out_cap should be enough; if not, we conservatively fail */
            conf_set_err(ctx, CONF_ERR_OOM, 0u, 1u, 1u);
            return conf_slice_zero();
        }
    }

    dst[out.len] = '\0';
    out.ptr = dst;
    return out;
}

static conf_slice_t conf_toml_handle_string(conf_ctx_t *ctx, toml_tok_t tok) {
    conf_slice_t raw;
    /* tok.ptr points inside quotes content already, lexer removed quotes */
    raw.ptr = tok.ptr;
    raw.len = tok.len;

    if (tok.str_kind == 1 || tok.str_kind == 3) {
        /* basic */
        return conf_toml_unescape_basic(ctx, raw, tok.str_kind == 3);
    }
    /* literal: no unescape; just copy (optionally) */
    if (ctx->load_flags & CONF_LOAD_COPY_SLICES) {
        return conf_arena_copy(ctx, raw.ptr, raw.len, 1);
    }
    return raw;
}

static int conf_parse_int_toml(conf_slice_t s, long *out) {
    /* TOML ints: decimal, hex 0x, octal 0o, binary 0b; underscores allowed.
       No libc; we parse into signed long (implementation-defined width).
    */
    unsigned int i = 0u;
    int neg = 0;
    int base = 10;
    unsigned long acc = 0ul;
    int any = 0;

    if (s.len == 0u) return 0;
    if (s.ptr[i] == '+' || s.ptr[i] == '-') {
        neg = (s.ptr[i] == '-') ? 1 : 0;
        i++;
    }
    if (i + 2u <= s.len && s.ptr[i] == '0') {
        char c1 = s.ptr[i+1u];
        if (c1 == 'x' || c1 == 'X') { base = 16; i += 2u; }
        else if (c1 == 'o' || c1 == 'O') { base = 8; i += 2u; }
        else if (c1 == 'b' || c1 == 'B') { base = 2; i += 2u; }
        else {
            /* could still be 0... decimal */
        }
    }

    for (; i < s.len; ++i) {
        char c = s.ptr[i];
        unsigned int v;
        if (c == '_') continue;
        if (base == 10) {
            if (!conf_is_digit(c)) return 0;
            v = (unsigned int)(c - '0');
        } else if (base == 16) {
            if (!conf_hex_val(c, &v)) return 0;
        } else if (base == 8) {
            if (c < '0' || c > '7') return 0;
            v = (unsigned int)(c - '0');
        } else {
            if (c != '0' && c != '1') return 0;
            v = (unsigned int)(c - '0');
        }
        any = 1;
        acc = acc * (unsigned long)base + (unsigned long)v;
    }

    if (!any) return 0;
    if (neg) {
        *out = -(long)acc;
    } else {
        *out = (long)acc;
    }
    return 1;
}

static int conf_slice_contains(conf_slice_t s, char needle) {
    unsigned int i;
    for (i = 0u; i < s.len; ++i) if (s.ptr[i] == needle) return 1;
    return 0;
}

static int conf_parse_fixed_toml(conf_slice_t s, conf_fixed_t *out) {
    /* Minimal 16.16 fixed-point parser:
       - sign
       - integer part digits/underscores
       - optional fractional .digits
       - optional exponent e/E[+-]digits
       Unsupported by design in fixed mode: inf/nan.
    */
    unsigned int i = 0u;
    int neg = 0;
    long whole = 0L;
    long frac_acc = 0L;
    long frac_scale = 1L;
    int any_whole = 0;
    int any_frac = 0;
    conf_fixed_t val;

    if (!out || s.len == 0u) return 0;

    if (s.ptr[i] == '+' || s.ptr[i] == '-') {
        neg = (s.ptr[i] == '-') ? 1 : 0;
        i++;
        if (i >= s.len) return 0;
    }

    while (i < s.len) {
        char c = s.ptr[i];
        if (c == '_') { i++; continue; }
        if (!conf_is_digit(c)) break;
        any_whole = 1;
        whole = whole * 10L + (long)(c - '0');
        i++;
    }

    if (i < s.len && s.ptr[i] == '.') {
        i++;
        while (i < s.len) {
            char c = s.ptr[i];
            if (c == '_') { i++; continue; }
            if (!conf_is_digit(c)) break;
            any_frac = 1;
            if (frac_scale < 10000L) {
                frac_acc = frac_acc * 10L + (long)(c - '0');
                frac_scale *= 10L;
            }
            i++;
        }
        if (!any_frac) return 0;
    }

    if (!any_whole && !any_frac) return 0;

    val = (conf_fixed_t)(whole << CONF_FIXED_SHIFT);
    if (frac_scale > 1L) {
        val += (conf_fixed_t)(((frac_acc * CONF_FIXED_ONE) + (frac_scale / 2L)) / frac_scale);
    }

    if (i < s.len && (s.ptr[i] == 'e' || s.ptr[i] == 'E')) {
        int exp_neg = 0;
        long exp = 0L;
        int exp_any = 0;
        i++;
        if (i < s.len && (s.ptr[i] == '+' || s.ptr[i] == '-')) {
            exp_neg = (s.ptr[i] == '-') ? 1 : 0;
            i++;
        }
        while (i < s.len) {
            char c = s.ptr[i];
            if (c == '_') { i++; continue; }
            if (!conf_is_digit(c)) break;
            exp_any = 1;
            exp = exp * 10L + (long)(c - '0');
            i++;
        }
        if (!exp_any) return 0;
        while (exp > 0L) {
            if (exp_neg) val = (conf_fixed_t)(val / 10L);
            else val = (conf_fixed_t)(val * 10L);
            exp--;
        }
    }

    while (i < s.len) {
        if (s.ptr[i] == '_') { i++; continue; }
        return 0;
    }

    if (neg) val = (conf_fixed_t)(-val);
    *out = val;
    return 1;
}

static int conf_is_datetime_like(conf_slice_t s) {
    /* TOML datetime tokens include '-' ':' and/or 'T' and are not quoted.
       We'll apply a conservative heuristic:
         - contains 'T' or ':' -> datetime
         - OR looks like YYYY-MM-DD (has '-' not at start, and digits around)
    */
    unsigned int i;
    int has_t = 0;
    int has_colon = 0;
    for (i = 0u; i < s.len; ++i) {
        char c = s.ptr[i];
        if (c == 'T' || c == 't') has_t = 1;
        if (c == ':') has_colon = 1;
    }
    if (has_t || has_colon) return 1;
    /* date-only: 4 digits '-' 2 digits '-' 2 digits ... */
    if (s.len >= 10u && conf_is_digit(s.ptr[0]) && conf_is_digit(s.ptr[1]) && conf_is_digit(s.ptr[2]) && conf_is_digit(s.ptr[3]) &&
        s.ptr[4] == '-' &&
        conf_is_digit(s.ptr[5]) && conf_is_digit(s.ptr[6]) &&
        s.ptr[7] == '-' &&
        conf_is_digit(s.ptr[8]) && conf_is_digit(s.ptr[9])) {
        return 1;
    }
    return 0;
}

/* ----------------------------------------
   TOML Parser
   ---------------------------------------- */

typedef struct {
    toml_lex_t lx;
    toml_tok_t tok;
    int has_tok;
} toml_parser_t;

static void toml_p_init(toml_parser_t *p, const char *src, unsigned int len) {
    toml_lex_init(&p->lx, src, len);
    p->has_tok = 0;
}

static toml_tok_t toml_p_peek(conf_ctx_t *ctx, toml_parser_t *p) {
    if (!p->has_tok) {
        p->tok = toml_next(ctx, &p->lx);
        p->has_tok = 1;
    }
    return p->tok;
}

static toml_tok_t toml_p_take(conf_ctx_t *ctx, toml_parser_t *p) {
    toml_tok_t t = toml_p_peek(ctx, p);
    p->has_tok = 0;
    return t;
}

static int toml_p_accept(conf_ctx_t *ctx, toml_parser_t *p, toml_tok_type_t tt) {
    toml_tok_t t = toml_p_peek(ctx, p);
    (void)ctx;
    if (t.type == tt) { toml_p_take(ctx, p); return 1; }
    return 0;
}

static int toml_p_expect(conf_ctx_t *ctx, toml_parser_t *p, toml_tok_type_t tt) {
    toml_tok_t t = toml_p_peek(ctx, p);
    if (t.type != tt) {
        conf_set_err(ctx, CONF_ERR_SYNTAX, t.pos, t.line, t.col);
        return 0;
    }
    toml_p_take(ctx, p);
    return 1;
}

static conf_slice_t toml_parse_key_part(conf_ctx_t *ctx, toml_parser_t *p) {
    toml_tok_t t = toml_p_peek(ctx, p);
    conf_slice_t out = {0,0};
    if (t.type == TT_IDENT || t.type == TT_NUMBER) {
        t = toml_p_take(ctx, p);
        out.ptr = t.ptr;
        out.len = t.len;
        if (ctx->load_flags & CONF_LOAD_COPY_SLICES) out = conf_arena_copy(ctx, out.ptr, out.len, 1);
        return out;
    }
    if (t.type == TT_STRING) {
        t = toml_p_take(ctx, p);
        out = conf_toml_handle_string(ctx, t); /* unquoted + unescaped already */
        return out;
    }
    conf_set_err(ctx, CONF_ERR_SYNTAX, t.pos, t.line, t.col);
    return out;
}

static int toml_parse_key_path(conf_ctx_t *ctx, toml_parser_t *p, conf_slice_t *parts, unsigned int *nparts, unsigned int max_parts) {
    unsigned int n = 0u;
    conf_slice_t part;

    part = toml_parse_key_part(ctx, p);
    if (!part.ptr) return 0;
    parts[n++] = part;

    while (toml_p_accept(ctx, p, TT_DOT)) {
        if (n >= max_parts) {
            conf_set_err(ctx, CONF_ERR_UNSUPPORTED, 0u, 1u, 1u);
            return 0;
        }
        part = toml_parse_key_part(ctx, p);
        if (!part.ptr) return 0;
        parts[n++] = part;
    }

    *nparts = n;
    return 1;
}

static conf_value_t *toml_parse_value(conf_ctx_t *ctx, toml_parser_t *p);

static conf_value_t *toml_parse_array(conf_ctx_t *ctx, toml_parser_t *p) {
    conf_value_t *arrv = conf_make_array_value(ctx);
    if (!arrv) return 0;

    if (!toml_p_expect(ctx, p, TT_LBRACK)) return 0;

    /* allow newlines */
    while (toml_p_accept(ctx, p, TT_NEWLINE)) {}

    if (toml_p_accept(ctx, p, TT_RBRACK)) return arrv;

    while (1) {
        conf_value_t *elem = toml_parse_value(ctx, p);
        if (!elem) return 0;
        conf_array_push(ctx, arrv->as.a, elem);

        while (toml_p_accept(ctx, p, TT_NEWLINE)) {}

        if (toml_p_accept(ctx, p, TT_COMMA)) {
            while (toml_p_accept(ctx, p, TT_NEWLINE)) {}
            /* allow trailing comma before ] */
            if (toml_p_accept(ctx, p, TT_RBRACK)) return arrv;
            continue;
        }
        if (toml_p_accept(ctx, p, TT_RBRACK)) return arrv;

        {
            toml_tok_t t = toml_p_peek(ctx, p);
            conf_set_err(ctx, CONF_ERR_SYNTAX, t.pos, t.line, t.col);
            return 0;
        }
    }
}

static conf_value_t *toml_parse_inline_table(conf_ctx_t *ctx, toml_parser_t *p) {
    conf_value_t *tv = conf_make_table_value(ctx);
    if (!tv) return 0;

    if (!toml_p_expect(ctx, p, TT_LBRACE)) return 0;

    /* allow empty */
    while (toml_p_accept(ctx, p, TT_NEWLINE)) {}
    if (toml_p_accept(ctx, p, TT_RBRACE)) return tv;

    while (1) {
        conf_slice_t keyparts[32];
        unsigned int nparts = 0u;
        if (!toml_parse_key_path(ctx, p, keyparts, &nparts, 32u)) return 0;
        if (!toml_p_expect(ctx, p, TT_EQ)) return 0;

        {
            conf_value_t *val = toml_parse_value(ctx, p);
            if (!val) return 0;

            /* insert into tv nested by dotted key */
            {
                unsigned int i;
                conf_value_t *cur = tv;
                for (i = 0u; i + 1u < nparts; ++i) {
                    conf_value_t *next = (conf_value_t*)conf_table_get_slice(cur, keyparts[i]);
                    if (!next) {
                        conf_value_t *nt = conf_make_table_value(ctx);
                        if (!nt) return 0;
                        if (conf_table_put(ctx, cur->as.t, keyparts[i], nt) != CONF_OK) return 0;
                        next = nt;
                    }
                    if (next->type != CONF_TABLE) {
                        conf_set_err(ctx, CONF_ERR_TYPE_MISMATCH, 0u, 1u, 1u);
                        return 0;
                    }
                    cur = next;
                }
                if (conf_table_put(ctx, cur->as.t, keyparts[nparts-1u], val) != CONF_OK) return 0;
            }
        }

        while (toml_p_accept(ctx, p, TT_NEWLINE)) {}

        if (toml_p_accept(ctx, p, TT_COMMA)) {
            while (toml_p_accept(ctx, p, TT_NEWLINE)) {}
            if (toml_p_accept(ctx, p, TT_RBRACE)) return tv; /* tolerate trailing comma */
            continue;
        }
        if (toml_p_accept(ctx, p, TT_RBRACE)) return tv;

        {
            toml_tok_t t = toml_p_peek(ctx, p);
            conf_set_err(ctx, CONF_ERR_SYNTAX, t.pos, t.line, t.col);
            return 0;
        }
    }
}

static conf_value_t *toml_parse_value(conf_ctx_t *ctx, toml_parser_t *p) {
    toml_tok_t t = toml_p_peek(ctx, p);

    if (t.type == TT_STRING) {
        t = toml_p_take(ctx, p);
        {
            conf_value_t *v = conf_new_value(ctx, CONF_STRING);
            if (!v) return 0;
            v->as.s = conf_toml_handle_string(ctx, t);
            if (!v->as.s.ptr) return 0;
            return v;
        }
    }

    if (t.type == TT_BOOL) {
        t = toml_p_take(ctx, p);
        {
            conf_value_t *v = conf_new_value(ctx, CONF_BOOL);
            if (!v) return 0;
            v->as.b = conf_slice_eq_cstr_ci(conf_slice_make(t.ptr, t.len), "true") ? 1 : 0;
            return v;
        }
    }

    if (t.type == TT_NUMBER) {
        t = toml_p_take(ctx, p);
        {
            conf_slice_t raw; raw.ptr = t.ptr; raw.len = t.len;
            raw = conf_slice_trim(raw);

            /* classify: datetime? */
            if (conf_is_datetime_like(raw)) {
                conf_value_t *v = conf_new_value(ctx, CONF_DATETIME);
                if (!v) return 0;
                if (ctx->load_flags & CONF_LOAD_COPY_SLICES) raw = conf_arena_copy(ctx, raw.ptr, raw.len, 1);
                v->as.s = raw;
                return v;
            }

            /* fixed-point if contains '.' or 'e'/'E' */
            if (conf_slice_contains(raw, '.') || conf_slice_contains(raw, 'e') || conf_slice_contains(raw, 'E')) {
                conf_fixed_t fx;
                if (!conf_parse_fixed_toml(raw, &fx)) {
                    conf_set_err(ctx, CONF_ERR_BAD_NUMBER, t.pos, t.line, t.col);
                    return 0;
                }
                {
                    conf_value_t *v = conf_new_value(ctx, CONF_FIXED);
                    if (!v) return 0;
                    v->as.fx = fx;
                    return v;
                }
            } else {
                long iv;
                if (!conf_parse_int_toml(raw, &iv)) {
                    conf_set_err(ctx, CONF_ERR_BAD_NUMBER, t.pos, t.line, t.col);
                    return 0;
                }
                {
                    conf_value_t *v = conf_new_value(ctx, CONF_INT);
                    if (!v) return 0;
                    v->as.i = iv;
                    return v;
                }
            }
        }
    }

    if (t.type == TT_LBRACK) {
        return toml_parse_array(ctx, p);
    }

    if (t.type == TT_LBRACE) {
        return toml_parse_inline_table(ctx, p);
    }

    conf_set_err(ctx, CONF_ERR_SYNTAX, t.pos, t.line, t.col);
    return 0;
}

static conf_err_t toml_apply_keyval(conf_ctx_t *ctx, conf_value_t *root_table, conf_slice_t *parts, unsigned int nparts, conf_value_t *val) {
    unsigned int i;
    conf_value_t *cur = root_table;

    for (i = 0u; i + 1u < nparts; ++i) {
        conf_value_t *next = (conf_value_t*)conf_table_get_slice(cur, parts[i]);
        if (!next) {
            conf_value_t *nt = conf_make_table_value(ctx);
            conf_err_t e;
            if (!nt) return CONF_ERR_OOM;
            e = conf_table_put(ctx, cur->as.t, parts[i], nt);
            if (e != CONF_OK) return e;
            next = nt;
        }
        if (next->type != CONF_TABLE) {
            conf_set_err(ctx, CONF_ERR_TYPE_MISMATCH, 0u, 1u, 1u);
            return CONF_ERR_TYPE_MISMATCH;
        }
        cur = next;
    }

    return conf_table_put(ctx, cur->as.t, parts[nparts - 1u], val);
}

static conf_err_t toml_enter_table(conf_ctx_t *ctx, conf_value_t *root_table, conf_slice_t *parts, unsigned int nparts, int array_of_tables, conf_value_t **out_current_table) {
    unsigned int i;
    conf_value_t *cur = root_table;

    for (i = 0u; i < nparts; ++i) {
        conf_value_t *next = (conf_value_t*)conf_table_get_slice(cur, parts[i]);

        if (i == nparts - 1u && array_of_tables) {
            /* target is array-of-tables at this key */
            if (!next) {
                conf_value_t *arrv = conf_make_array_value(ctx);
                conf_value_t *newtab;
                conf_err_t e;
                if (!arrv) return CONF_ERR_OOM;
                e = conf_table_put(ctx, cur->as.t, parts[i], arrv);
                if (e != CONF_OK) return e;
                newtab = conf_make_table_value(ctx);
                if (!newtab) return CONF_ERR_OOM;
                conf_array_push(ctx, arrv->as.a, newtab);
                *out_current_table = newtab;
                return CONF_OK;
            } else {
                if (next->type != CONF_ARRAY) {
                    conf_set_err(ctx, CONF_ERR_TYPE_MISMATCH, 0u, 1u, 1u);
                    return CONF_ERR_TYPE_MISMATCH;
                }
                {
                    conf_value_t *newtab = conf_make_table_value(ctx);
                    if (!newtab) return CONF_ERR_OOM;
                    conf_array_push(ctx, next->as.a, newtab);
                    *out_current_table = newtab;
                    return CONF_OK;
                }
            }
        }

        /* normal table navigation */
        if (!next) {
            conf_value_t *nt = conf_make_table_value(ctx);
            conf_err_t e;
            if (!nt) return CONF_ERR_OOM;
            e = conf_table_put(ctx, cur->as.t, parts[i], nt);
            if (e != CONF_OK) return e;
            next = nt;
        } else {
            if (next->type != CONF_TABLE) {
                conf_set_err(ctx, CONF_ERR_TYPE_MISMATCH, 0u, 1u, 1u);
                return CONF_ERR_TYPE_MISMATCH;
            }
        }
        cur = next;
    }

    *out_current_table = cur;
    return CONF_OK;
}

conf_err_t conf_load_toml(conf_ctx_t *ctx, const char *text, unsigned int len) {
    toml_parser_t p;
    conf_value_t *current_table;

    if (!ctx || !ctx->root) return CONF_ERR_SYNTAX;

    toml_p_init(&p, text, len);
    current_table = ctx->root;

    while (1) {
        toml_tok_t t = toml_p_peek(ctx, &p);
        if (ctx->err.code != CONF_OK) return ctx->err.code;

        if (t.type == TT_EOF) break;

        /* skip blank lines */
        if (t.type == TT_NEWLINE) { toml_p_take(ctx, &p); continue; }

        /* table headers */
        if (t.type == TT_LBRACK) {
            int array_of_tables = 0;
            conf_slice_t keyparts[32];
            unsigned int nparts = 0u;

            toml_p_take(ctx, &p); /* [ */
            if (toml_p_accept(ctx, &p, TT_LBRACK)) array_of_tables = 1;

            /* allow whitespace/comments already skipped; parse key path */
            if (!toml_parse_key_path(ctx, &p, keyparts, &nparts, 32u)) return ctx->err.code ? ctx->err.code : CONF_ERR_SYNTAX;

            if (array_of_tables) {
                if (!toml_p_expect(ctx, &p, TT_RBRACK)) return ctx->err.code;
                if (!toml_p_expect(ctx, &p, TT_RBRACK)) return ctx->err.code;
            } else {
                if (!toml_p_expect(ctx, &p, TT_RBRACK)) return ctx->err.code;
            }

            /* consume optional newline(s) */
            while (toml_p_accept(ctx, &p, TT_NEWLINE)) {}

            {
                conf_err_t e = toml_enter_table(ctx, ctx->root, keyparts, nparts, array_of_tables, &current_table);
                if (e != CONF_OK) return e;
            }
            continue;
        }

        /* key = value */
        {
            conf_slice_t keyparts[32];
            unsigned int nparts = 0u;
            conf_value_t *val;
            conf_err_t e;

            if (!toml_parse_key_path(ctx, &p, keyparts, &nparts, 32u)) return ctx->err.code ? ctx->err.code : CONF_ERR_SYNTAX;
            if (!toml_p_expect(ctx, &p, TT_EQ)) return ctx->err.code;

            val = toml_parse_value(ctx, &p);
            if (!val) return ctx->err.code ? ctx->err.code : CONF_ERR_SYNTAX;

            /* consume until newline/EOF */
            while (1) {
                toml_tok_t n = toml_p_peek(ctx, &p);
                if (n.type == TT_NEWLINE) { toml_p_take(ctx, &p); break; }
                if (n.type == TT_EOF) break;
                /* allow trailing spaces/comments already handled; but any extra tokens is syntax error */
                conf_set_err(ctx, CONF_ERR_SYNTAX, n.pos, n.line, n.col);
                return ctx->err.code;
            }

            e = toml_apply_keyval(ctx, current_table, keyparts, nparts, val);
            if (e != CONF_OK) return e;
            continue;
        }
    }

    return CONF_OK;
}

/* ----------------------------------------
   TOML scalar override: parse a single value and assign to path
   ---------------------------------------- */

conf_err_t conf_override_scalar_toml(conf_ctx_t *ctx, const char *path, const char *scalar_text, unsigned int len) {
    toml_parser_t p;
    conf_value_t *val;
    if (!ctx || !ctx->root) return CONF_ERR_SYNTAX;

    toml_p_init(&p, scalar_text, len);

    /* accept optional newlines */
    while (toml_p_accept(ctx, &p, TT_NEWLINE)) {}

    val = toml_parse_value(ctx, &p);
    if (!val) return ctx->err.code ? ctx->err.code : CONF_ERR_SYNTAX;

    /* consume trailing whitespace/newlines; ensure EOF */
    while (toml_p_accept(ctx, &p, TT_NEWLINE)) {}
    if (toml_p_peek(ctx, &p).type != TT_EOF) {
        toml_tok_t t = toml_p_peek(ctx, &p);
        conf_set_err(ctx, CONF_ERR_SYNTAX, t.pos, t.line, t.col);
        return ctx->err.code;
    }

    return conf_override_value(ctx, path, val);
}

/* ----------------------------------------
   INI parser (pragmatic subset)
   - [section]
   - key = value (or key:value)
   - comments: ; or #
   - values parsed as TOML scalars when possible (bool/int/fixed/string)
   ---------------------------------------- */

static void ini_skip_ws(const char *src, unsigned int len, unsigned int *i) {
    while (*i < len && (src[*i] == ' ' || src[*i] == '\t' || src[*i] == '\r')) (*i)++;
}

static void ini_skip_line(const char *src, unsigned int len, unsigned int *i) {
    while (*i < len && src[*i] != '\n') (*i)++;
    if (*i < len && src[*i] == '\n') (*i)++;
}

static conf_slice_t ini_read_until(const char *src, unsigned int len, unsigned int *i, char stop1, char stop2) {
    unsigned int start = *i;
    while (*i < len && src[*i] != stop1 && src[*i] != stop2 && src[*i] != '\n') (*i)++;
    return conf_slice_make(src + start, (unsigned int)(*i - start));
}

static conf_value_t *conf_parse_scalar_like_toml(conf_ctx_t *ctx, conf_slice_t s) {
    /* Try bool/int/fixed, otherwise string (trimmed). Accept unquoted strings. */
    conf_slice_t t = conf_slice_trim(s);

    /* empty => NULL */
    if (t.len == 0u) {
        conf_value_t *v = conf_new_value(ctx, CONF_NULL);
        return v;
    }

    if (conf_slice_eq_cstr_ci(t, "true") || conf_slice_eq_cstr_ci(t, "false") ||
        conf_slice_eq_cstr_ci(t, "yes") || conf_slice_eq_cstr_ci(t, "no") ||
        conf_slice_eq_cstr_ci(t, "on") || conf_slice_eq_cstr_ci(t, "off")) {
        conf_value_t *v = conf_new_value(ctx, CONF_BOOL);
        if (!v) return 0;
        v->as.b = (conf_slice_eq_cstr_ci(t, "true") || conf_slice_eq_cstr_ci(t, "yes") || conf_slice_eq_cstr_ci(t, "on")) ? 1 : 0;
        return v;
    }

    /* quoted string? */
    if (t.len >= 2u && ((t.ptr[0] == '"' && t.ptr[t.len-1u] == '"') || (t.ptr[0] == '\'' && t.ptr[t.len-1u] == '\''))) {
        toml_tok_t fake;
        fake.type = TT_STRING;
        fake.ptr = t.ptr + 1;
        fake.len = t.len - 2u;
        fake.pos = 0u; fake.line = 1u; fake.col = 1u;
        fake.str_kind = (t.ptr[0] == '"') ? 1 : 2;
        {
            conf_value_t *v = conf_new_value(ctx, CONF_STRING);
            if (!v) return 0;
            v->as.s = conf_toml_handle_string(ctx, fake);
            if (!v->as.s.ptr) return 0;
            return v;
        }
    }

    /* datetime-ish? */
    if (conf_is_datetime_like(t)) {
        conf_value_t *v = conf_new_value(ctx, CONF_DATETIME);
        if (!v) return 0;
        if (ctx->load_flags & CONF_LOAD_COPY_SLICES) t = conf_arena_copy(ctx, t.ptr, t.len, 1);
        v->as.s = t;
        return v;
    }

    /* fixed-point? */
    if (conf_slice_contains(t, '.') || conf_slice_contains(t, 'e') || conf_slice_contains(t, 'E')) {
        conf_fixed_t fx;
        if (conf_parse_fixed_toml(t, &fx)) {
            conf_value_t *v = conf_new_value(ctx, CONF_FIXED);
            if (!v) return 0;
            v->as.fx = fx;
            return v;
        }
    }

    /* int? */
    {
        long iv;
        if (conf_parse_int_toml(t, &iv)) {
            conf_value_t *v = conf_new_value(ctx, CONF_INT);
            if (!v) return 0;
            v->as.i = iv;
            return v;
        }
    }

    /* fallback string */
    {
        conf_value_t *v = conf_new_value(ctx, CONF_STRING);
        if (!v) return 0;
        if (ctx->load_flags & CONF_LOAD_COPY_SLICES) t = conf_arena_copy(ctx, t.ptr, t.len, 1);
        v->as.s = t;
        return v;
    }
}

conf_err_t conf_load_ini(conf_ctx_t *ctx, const char *text, unsigned int len) {
    unsigned int i = 0u;
    conf_value_t *current_table = ctx->root;

    if (!ctx || !ctx->root) return CONF_ERR_SYNTAX;

    while (i < len) {
        ini_skip_ws(text, len, &i);
        if (i >= len) break;

        if (text[i] == '\n') { i++; continue; }

        /* comment line */
        if (text[i] == ';' || text[i] == '#') {
            ini_skip_line(text, len, &i);
            continue;
        }

        /* section */
        if (text[i] == '[') {
            conf_slice_t name;
            i++;
            name = ini_read_until(text, len, &i, ']', ']');
            if (i < len && text[i] == ']') i++;
            name = conf_slice_trim(name);

            if (ctx->load_flags & CONF_LOAD_COPY_SLICES) name = conf_arena_copy(ctx, name.ptr, name.len, 1);

            /* enter/create table at root[name] */
            {
                conf_slice_t parts[2];
                unsigned int np = 1u;
                conf_err_t e;
                parts[0] = name;
                e = toml_enter_table(ctx, ctx->root, parts, np, 0, &current_table);
                if (e != CONF_OK) return e;
            }

            ini_skip_line(text, len, &i);
            continue;
        }

        /* key = value */
        {
            conf_slice_t key, val;
            char sep = 0;
            unsigned int start_key = i;
            while (i < len && text[i] != '\n' && text[i] != '=' && text[i] != ':') i++;
            if (i < len && (text[i] == '=' || text[i] == ':')) { sep = text[i]; }
            key.ptr = text + start_key;
            key.len = (unsigned int)(i - start_key);
            key = conf_slice_trim(key);

            if (i >= len || sep == 0) {
                /* key alone => flag */
                conf_value_t *v = conf_new_value(ctx, CONF_NULL);
                if (!v) return CONF_ERR_OOM;
                if (ctx->load_flags & CONF_LOAD_COPY_SLICES) key = conf_arena_copy(ctx, key.ptr, key.len, 1);
                {
                    conf_err_t e = conf_table_put(ctx, current_table->as.t, key, v);
                    if (e != CONF_OK) return e;
                }
                ini_skip_line(text, len, &i);
                continue;
            }

            i++; /* consume sep */
            val.ptr = text + i;
            /* read to end-of-line, but stop before inline comment ; or # if preceded by space */
            while (i < len && text[i] != '\n') {
                if ((text[i] == ';' || text[i] == '#')) {
                    /* treat as comment start if previous is space */
                    if (i > 0u && conf_is_space(text[i-1u])) break;
                }
                i++;
            }
            val.len = (unsigned int)((text + i) - val.ptr);
            val = conf_slice_trim(val);

            if (ctx->load_flags & CONF_LOAD_COPY_SLICES) key = conf_arena_copy(ctx, key.ptr, key.len, 1);

            {
                conf_value_t *vv = conf_parse_scalar_like_toml(ctx, val);
                conf_err_t e;
                if (!vv) return ctx->err.code ? ctx->err.code : CONF_ERR_OOM;
                e = conf_table_put(ctx, current_table->as.t, key, vv);
                if (e != CONF_OK) return e;
            }

            ini_skip_line(text, len, &i);
            continue;
        }
    }

    return CONF_OK;
}

/* ----------------------------------------
   YAML 1.2 config-subset parser
   Supports:
     - block mappings: key: value
     - block sequences: - item
     - nesting by indentation (spaces)
     - comments with '#'
     - scalars: plain, 'single-quoted', "double-quoted" (double uses basic escapes like \n, \t, \", \\)
   Not supported (by design, for engine configs):
     - anchors & aliases
     - complex keys
     - flow collections [a, b], {k: v}
     - tags, directives, multi-document streams
   ---------------------------------------- */

typedef enum {
    YT_EOF = 0,
    YT_NEWLINE,
    YT_INDENT,
    YT_DEDENT,
    YT_DASH,
    YT_COLON,
    YT_SCALAR
} ytok_type_t;

typedef struct {
    ytok_type_t type;
    conf_slice_t text; /* for scalar */
    unsigned int pos, line, col;
    int quoted; /* 0 plain, 1 single, 2 double */
} ytok_t;

typedef struct {
    const char *src;
    unsigned int len;
    unsigned int pos;
    unsigned int line;
    unsigned int col;

    unsigned int indent_stack[64];
    unsigned int indent_top;
    int bol; /* beginning-of-line */
    int pending_dedents; /* how many dedents to emit */
    int emitted_eof;
} ylex_t;

static void ylex_init(ylex_t *lx, const char *src, unsigned int len) {
    lx->src = src;
    lx->len = len;
    lx->pos = 0u;
    lx->line = 1u;
    lx->col = 1u;
    lx->indent_stack[0] = 0u;
    lx->indent_top = 0u;
    lx->bol = 1;
    lx->pending_dedents = 0;
    lx->emitted_eof = 0;
}

static char ypeek(ylex_t *lx) {
    if (lx->pos >= lx->len) return '\0';
    return lx->src[lx->pos];
}

static char ygetc(ylex_t *lx) {
    char c;
    if (lx->pos >= lx->len) return '\0';
    c = lx->src[lx->pos++];
    if (c == '\n') {
        lx->line += 1u;
        lx->col = 1u;
        lx->bol = 1;
    } else {
        lx->col += 1u;
        lx->bol = 0;
    }
    return c;
}

static void yskip_to_eol(ylex_t *lx) {
    while (1) {
        char c = ypeek(lx);
        if (c == '\0' || c == '\n') break;
        ygetc(lx);
    }
}

static void yskip_inline_ws(ylex_t *lx) {
    while (ypeek(lx) == ' ' || ypeek(lx) == '\t' || ypeek(lx) == '\r') ygetc(lx);
}

static ytok_t ytok_make(ytok_type_t t, unsigned int pos, unsigned int line, unsigned int col) {
    ytok_t tok;
    tok.type = t;
    tok.text.ptr = 0;
    tok.text.len = 0u;
    tok.pos = pos;
    tok.line = line;
    tok.col = col;
    tok.quoted = 0;
    return tok;
}

static conf_slice_t yaml_unescape_double(conf_ctx_t *ctx, conf_slice_t raw) {
    /* YAML double quoted escapes (subset).
       We reuse the TOML basic-string unescape implementation.
    */
    return conf_toml_unescape_basic(ctx, raw, 0);
}

static ytok_t ylex_next(conf_ctx_t *ctx, ylex_t *lx) {
    unsigned int pos0, line0, col0;

    /* emit pending dedents first */
    if (lx->pending_dedents > 0) {
        lx->pending_dedents--;
        return ytok_make(YT_DEDENT, lx->pos, lx->line, lx->col);
    }

    pos0 = lx->pos;
    line0 = lx->line;
    col0 = lx->col;

    if (lx->emitted_eof) {
        return ytok_make(YT_EOF, lx->pos, lx->line, lx->col);
    }

    /* handle indentation at BOL */
    if (lx->bol) {
        unsigned int spaces = 0u;
        /* count spaces (tabs forbidden for indentation) */
        while (ypeek(lx) == ' ') { ygetc(lx); spaces++; }
        if (ypeek(lx) == '\t') {
            conf_set_err(ctx, CONF_ERR_SYNTAX, pos0, line0, col0);
            return ytok_make(YT_EOF, pos0, line0, col0);
        }

        /* skip empty lines and comment-only lines, but keep NEWLINE tokens */
        if (ypeek(lx) == '\n' || ypeek(lx) == '\0' || ypeek(lx) == '#') {
            /* restore bol spaces are already consumed; treat line as blank/comment */
            if (ypeek(lx) == '#') yskip_to_eol(lx);
            if (ypeek(lx) == '\n') { ygetc(lx); return ytok_make(YT_NEWLINE, pos0, line0, col0); }
            /* EOF */
            /* emit dedents down to 0 */
            if (lx->indent_top > 0u) {
                lx->pending_dedents = (int)lx->indent_top;
                lx->indent_top = 0u;
                lx->emitted_eof = 1;
                return ytok_make(YT_DEDENT, lx->pos, lx->line, lx->col);
            }
            lx->emitted_eof = 1;
            return ytok_make(YT_EOF, lx->pos, lx->line, lx->col);
        }

        /* compute indent transitions */
        if (spaces > lx->indent_stack[lx->indent_top]) {
            if (lx->indent_top + 1u >= 64u) {
                conf_set_err(ctx, CONF_ERR_UNSUPPORTED, pos0, line0, col0);
                return ytok_make(YT_EOF, pos0, line0, col0);
            }
            lx->indent_top++;
            lx->indent_stack[lx->indent_top] = spaces;
            return ytok_make(YT_INDENT, pos0, line0, col0);
        }
        if (spaces < lx->indent_stack[lx->indent_top]) {
            /* pop until match */
            while (lx->indent_top > 0u && spaces < lx->indent_stack[lx->indent_top]) {
                lx->indent_top--;
                lx->pending_dedents++;
            }
            if (spaces != lx->indent_stack[lx->indent_top]) {
                conf_set_err(ctx, CONF_ERR_SYNTAX, pos0, line0, col0);
                return ytok_make(YT_EOF, pos0, line0, col0);
            }
            /* emit one dedent now */
            if (lx->pending_dedents > 0) {
                lx->pending_dedents--;
                return ytok_make(YT_DEDENT, pos0, line0, col0);
            }
        }
    }

    /* inline whitespace */
    yskip_inline_ws(lx);
    pos0 = lx->pos; line0 = lx->line; col0 = lx->col;

    /* newline */
    if (ypeek(lx) == '\r') {
        ygetc(lx);
        if (ypeek(lx) == '\n') ygetc(lx);
        return ytok_make(YT_NEWLINE, pos0, line0, col0);
    }
    if (ypeek(lx) == '\n') {
        ygetc(lx);
        return ytok_make(YT_NEWLINE, pos0, line0, col0);
    }
    if (ypeek(lx) == '\0') {
        /* emit dedents */
        if (lx->indent_top > 0u) {
            lx->pending_dedents = (int)lx->indent_top;
            lx->indent_top = 0u;
            lx->emitted_eof = 1;
            return ytok_make(YT_DEDENT, pos0, line0, col0);
        }
        lx->emitted_eof = 1;
        return ytok_make(YT_EOF, pos0, line0, col0);
    }

    /* comment */
    if (ypeek(lx) == '#') {
        yskip_to_eol(lx);
        return ytok_make(YT_NEWLINE, pos0, line0, col0);
    }

    /* dash for sequences: must be "- " or "-\n" */
    if (ypeek(lx) == '-') {
        /* peek next */
        char n1;
        ygetc(lx);
        n1 = ypeek(lx);
        if (n1 == ' ' || n1 == '\n' || n1 == '\r') {
            return ytok_make(YT_DASH, pos0, line0, col0);
        }
        /* otherwise it's part of scalar, rewind by pretending scalar started at '-' */
        lx->pos = pos0;
        lx->line = line0;
        lx->col = col0;
        lx->bol = 0;
    }

    /* colon */
    if (ypeek(lx) == ':') {
        ygetc(lx);
        return ytok_make(YT_COLON, pos0, line0, col0);
    }

    /* scalar: quoted or plain until newline or # comment */
    {
        ytok_t tok = ytok_make(YT_SCALAR, pos0, line0, col0);
        char c = ypeek(lx);
        if (c == '"' || c == '\'') {
            char q = c;
            unsigned int start;
            ygetc(lx); /* consume quote */
            start = lx->pos;
            tok.quoted = (q == '"') ? 2 : 1;
            while (1) {
                char d = ypeek(lx);
                if (d == '\0' || d == '\n' || d == '\r') {
                    conf_set_err(ctx, CONF_ERR_SYNTAX, pos0, line0, col0);
                    return ytok_make(YT_EOF, pos0, line0, col0);
                }
                if (d == q) {
                    unsigned int end = lx->pos;
                    ygetc(lx);
                    tok.text.ptr = lx->src + start;
                    tok.text.len = end - start;
                    return tok;
                }
                if (q == '"' && d == '\\') {
                    ygetc(lx);
                    if (ypeek(lx) != '\0') ygetc(lx);
                    continue;
                }
                ygetc(lx);
            }
        } else {
            unsigned int start = lx->pos;
            while (1) {
                char d = ypeek(lx);
                if (d == '\0' || d == '\n' || d == '\r') break;
                if (d == '#') break;
                /* stop before ':' only if it acts as key separator? we leave it to parser by tokenizing ':' separately */
                if (d == ':') break;
                ygetc(lx);
            }
            tok.text.ptr = lx->src + start;
            tok.text.len = (unsigned int)(lx->pos - start);
            tok.text = conf_slice_trim(tok.text);
            return tok;
        }
    }
}

typedef struct {
    ylex_t lx;
    ytok_t buf[2];
    int nbuf;
} yparser_t;

static void yp_init(yparser_t *p, const char *src, unsigned int len) {
    ylex_init(&p->lx, src, len);
    p->nbuf = 0;
}

static void yp_fill(conf_ctx_t *ctx, yparser_t *p, int need) {
    while (p->nbuf < need && p->nbuf < 2) {
        p->buf[p->nbuf] = ylex_next(ctx, &p->lx);
        p->nbuf++;
    }
}

static ytok_t yp_peek(conf_ctx_t *ctx, yparser_t *p) {
    yp_fill(ctx, p, 1);
    return p->buf[0];
}

static ytok_t yp_peek2(conf_ctx_t *ctx, yparser_t *p) {
    yp_fill(ctx, p, 2);
    if (p->nbuf < 2) return p->buf[0];
    return p->buf[1];
}

static ytok_t yp_take(conf_ctx_t *ctx, yparser_t *p) {
    ytok_t t;
    yp_fill(ctx, p, 1);
    t = p->buf[0];
    if (p->nbuf > 1) p->buf[0] = p->buf[1];
    if (p->nbuf > 0) p->nbuf--;
    return t;
}

static int yp_accept(conf_ctx_t *ctx, yparser_t *p, ytok_type_t tt) {
    ytok_t t = yp_peek(ctx, p);
    (void)ctx;
    if (t.type == tt) { yp_take(ctx, p); return 1; }
    return 0;
}

static int yp_expect(conf_ctx_t *ctx, yparser_t *p, ytok_type_t tt) {
    ytok_t t = yp_peek(ctx, p);
    if (t.type != tt) {
        conf_set_err(ctx, CONF_ERR_SYNTAX, t.pos, t.line, t.col);
        return 0;
    }
    yp_take(ctx, p);
    return 1;
}

static conf_value_t *yaml_parse_scalar_value(conf_ctx_t *ctx, ytok_t tok) {
    /* convert scalar token to typed value */
    conf_slice_t s = tok.text;

    if (tok.quoted == 2) {
        /* double quoted */
        conf_slice_t u = yaml_unescape_double(ctx, s);
        conf_value_t *v = conf_new_value(ctx, CONF_STRING);
        if (!v) return 0;
        v->as.s = u;
        return v;
    }
    if (tok.quoted == 1) {
        /* single quoted literal */
        conf_value_t *v = conf_new_value(ctx, CONF_STRING);
        if (!v) return 0;
        if (ctx->load_flags & CONF_LOAD_COPY_SLICES) s = conf_arena_copy(ctx, s.ptr, s.len, 1);
        v->as.s = s;
        return v;
    }

    /* plain scalars: try bool/int/fixed/datetime, else string */
    return conf_parse_scalar_like_toml(ctx, s);
}

static conf_value_t *yaml_parse_block(conf_ctx_t *ctx, yparser_t *p, int *out_is_table);

static conf_err_t yaml_put_kv(conf_ctx_t *ctx, conf_value_t *tablev, conf_slice_t key, conf_value_t *val) {
    if (ctx->load_flags & CONF_LOAD_COPY_SLICES) key = conf_arena_copy(ctx, key.ptr, key.len, 1);
    return conf_table_put(ctx, tablev->as.t, key, val);
}

static conf_value_t *yaml_parse_mapping(conf_ctx_t *ctx, yparser_t *p) {
    conf_value_t *tablev = conf_make_table_value(ctx);
    if (!tablev) return 0;

    while (1) {
        ytok_t t = yp_peek(ctx, p);
        if (t.type == YT_EOF || t.type == YT_DEDENT) break;
        if (t.type == YT_NEWLINE) { yp_take(ctx, p); continue; }

        /* key must be scalar (plain or quoted) */
        if (t.type != YT_SCALAR) {
            conf_set_err(ctx, CONF_ERR_SYNTAX, t.pos, t.line, t.col);
            return 0;
        }
        t = yp_take(ctx, p);
        {
            conf_slice_t key = t.text;
            conf_value_t *val = 0;

            if (!yp_expect(ctx, p, YT_COLON)) return 0;

            /* value can be scalar on same line, or newline then indented block, or empty => null */
            {
                ytok_t n = yp_peek(ctx, p);
                if (n.type == YT_SCALAR) {
                    n = yp_take(ctx, p);
                    val = yaml_parse_scalar_value(ctx, n);
                    if (!val) return 0;
                    /* consume rest of line */
                    while (yp_accept(ctx, p, YT_SCALAR)) {}
                    /* optional newline */
                    yp_accept(ctx, p, YT_NEWLINE);
                } else if (n.type == YT_NEWLINE) {
                    yp_take(ctx, p);
                    /* nested block? */
                    if (yp_accept(ctx, p, YT_INDENT)) {
                        int is_table = 0;
                        val = yaml_parse_block(ctx, p, &is_table);
                        if (!val) return 0;
                        if (!yp_expect(ctx, p, YT_DEDENT)) return 0;
                    } else {
                        /* key: (empty) => NULL */
                        val = conf_new_value(ctx, CONF_NULL);
                        if (!val) return 0;
                    }
                } else if (n.type == YT_INDENT) {
                    /* unlikely: treat as nested */
                    yp_take(ctx, p);
                    {
                        int is_table = 0;
                        val = yaml_parse_block(ctx, p, &is_table);
                        if (!val) return 0;
                        if (!yp_expect(ctx, p, YT_DEDENT)) return 0;
                    }
                } else {
                    /* key: <nothing> */
                    val = conf_new_value(ctx, CONF_NULL);
                    if (!val) return 0;
                }
            }

            if (yaml_put_kv(ctx, tablev, key, val) != CONF_OK) return 0;
        }
    }

    return tablev;
}

static conf_value_t *yaml_parse_sequence(conf_ctx_t *ctx, yparser_t *p) {
    conf_value_t *arrv = conf_make_array_value(ctx);
    if (!arrv) return 0;

    while (1) {
        ytok_t t = yp_peek(ctx, p);
        if (t.type == YT_EOF || t.type == YT_DEDENT) break;
        if (t.type == YT_NEWLINE) { yp_take(ctx, p); continue; }

        if (t.type != YT_DASH) {
            /* end of sequence if we encounter mapping-looking token? In YAML, block can mix but we keep strict. */
            break;
        }
        yp_take(ctx, p); /* dash */

        /* value after dash can be scalar on same line, or newline then indent block, or empty => null */
        {
            ytok_t n = yp_peek(ctx, p);
            conf_value_t *val = 0;

            if (n.type == YT_SCALAR) {
                n = yp_take(ctx, p);
                val = yaml_parse_scalar_value(ctx, n);
                if (!val) return 0;
                /* eat rest of line */
                while (yp_accept(ctx, p, YT_SCALAR)) {}
                yp_accept(ctx, p, YT_NEWLINE);
            } else if (n.type == YT_NEWLINE) {
                yp_take(ctx, p);
                if (yp_accept(ctx, p, YT_INDENT)) {
                    int is_table = 0;
                    val = yaml_parse_block(ctx, p, &is_table);
                    if (!val) return 0;
                    if (!yp_expect(ctx, p, YT_DEDENT)) return 0;
                } else {
                    val = conf_new_value(ctx, CONF_NULL);
                    if (!val) return 0;
                }
            } else if (n.type == YT_INDENT) {
                yp_take(ctx, p);
                {
                    int is_table = 0;
                    val = yaml_parse_block(ctx, p, &is_table);
                    if (!val) return 0;
                    if (!yp_expect(ctx, p, YT_DEDENT)) return 0;
                }
            } else {
                val = conf_new_value(ctx, CONF_NULL);
                if (!val) return 0;
            }

            conf_array_push(ctx, arrv->as.a, val);
        }
    }

    return arrv;
}

static conf_value_t *yaml_parse_block(conf_ctx_t *ctx, yparser_t *p, int *out_is_table) {
    ytok_t t;

    /* decide based on next token: dash => sequence, scalar+colon => mapping */
    while (yp_accept(ctx, p, YT_NEWLINE)) {}

    t = yp_peek(ctx, p);

    if (t.type == YT_DASH) {
        if (out_is_table) *out_is_table = 0;
        return yaml_parse_sequence(ctx, p);
    }

    if (t.type == YT_SCALAR) {
        /* Non-destructive lookahead: scalar followed by ':' means mapping. */
        ytok_t t2 = yp_peek2(ctx, p);
        if (t2.type != YT_COLON) {
            conf_set_err(ctx, CONF_ERR_UNSUPPORTED, t.pos, t.line, t.col);
            return 0;
        }

        if (out_is_table) *out_is_table = 1;
        return yaml_parse_mapping(ctx, p);
    }

    /* empty block => empty table */
    if (out_is_table) *out_is_table = 1;
    return conf_make_table_value(ctx);
}

conf_err_t conf_load_yaml(conf_ctx_t *ctx, const char *text, unsigned int len) {
    yparser_t p;
    conf_value_t *rootv;

    if (!ctx || !ctx->root) return CONF_ERR_SYNTAX;

    yp_init(&p, text, len);

    /* parse top-level as mapping or sequence.
       Our ctx->root is a table; if YAML produces a sequence at root, we store it under key "$".
    */
    {
        int is_table = 1;
        rootv = yaml_parse_block(ctx, &p, &is_table);
        if (!rootv) return ctx->err.code ? ctx->err.code : CONF_ERR_SYNTAX;

        if (is_table) {
            /* merge into ctx->root (rootv is table) */
            conf_pair_t *it = rootv->as.t->head;
            while (it) {
                conf_err_t e = conf_table_put(ctx, ctx->root->as.t, it->key, it->value);
                if (e != CONF_OK) return e;
                it = it->next;
            }
        } else {
            /* store under "$" */
            conf_slice_t k = { "$", 1u };
            if (ctx->load_flags & CONF_LOAD_COPY_SLICES) k = conf_arena_copy(ctx, k.ptr, k.len, 1);
            {
                conf_err_t e = conf_table_put(ctx, ctx->root->as.t, k, rootv);
                if (e != CONF_OK) return e;
            }
        }
    }

    return CONF_OK;
}

/* ----------------------------------------
   Auto-detect: TOML vs YAML vs INI
   Heuristic (good enough for configs):
     - if we see '=' outside quotes early => TOML/INI
     - if we see '[' at line start => TOML/INI
     - if we see ':' outside quotes early => YAML
     - if ';' comment lines are present => INI
   ---------------------------------------- */

static int scan_outside_quotes_has(const char *s, unsigned int len, char needle) {
    unsigned int i = 0u;
    char q = 0;
    while (i < len) {
        char c = s[i++];
        if (q) {
            if (c == '\\' && q == '"' && i < len) { i++; continue; }
            if (c == q) q = 0;
            continue;
        }
        if (c == '"' || c == '\'') { q = c; continue; }
        if (c == needle) return 1;
        if (c == '\n') break; /* just first line scan */
    }
    return 0;
}

static int first_nonspace_is(const char *s, unsigned int len, char ch) {
    unsigned int i;
    for (i = 0u; i < len; ++i) {
        char c = s[i];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') continue;
        if (c == '#') { /* skip comment line */
            while (i < len && s[i] != '\n') i++;
            continue;
        }
        return c == ch;
    }
    return 0;
}

static int has_ini_semicolon_comment(const char *s, unsigned int len) {
    unsigned int i = 0u;
    while (i < len) {
        /* start of line */
        while (i < len && (s[i] == '\r' || s[i] == '\n')) i++;
        while (i < len && (s[i] == ' ' || s[i] == '\t')) i++;
        if (i < len && s[i] == ';') return 1;
        while (i < len && s[i] != '\n') i++;
    }
    return 0;
}

conf_err_t conf_load_auto(conf_ctx_t *ctx, const char *text, unsigned int len) {
    int has_eq, has_colon, starts_brack, ini_semicolon;

    if (!ctx || !ctx->root) return CONF_ERR_SYNTAX;

    ini_semicolon = has_ini_semicolon_comment(text, len);
    starts_brack = first_nonspace_is(text, len, '[');

    has_eq = scan_outside_quotes_has(text, len, '=');
    has_colon = scan_outside_quotes_has(text, len, ':');

    if (ini_semicolon) {
        return conf_load_ini(ctx, text, len);
    }

    if (has_eq || starts_brack) {
        /* could be TOML or INI; try TOML first, fallback to INI on syntax */
        {
            conf_err_t e = conf_load_toml(ctx, text, len);
            if (e == CONF_OK) return e;
            /* reset and try INI */
            conf_ctx_reset(ctx);
            return conf_load_ini(ctx, text, len);
        }
    }

    if (has_colon) {
        return conf_load_yaml(ctx, text, len);
    }

    /* last resort: try TOML then YAML then INI */
    {
        conf_err_t e = conf_load_toml(ctx, text, len);
        if (e == CONF_OK) return e;
        conf_ctx_reset(ctx);
        e = conf_load_yaml(ctx, text, len);
        if (e == CONF_OK) return e;
        conf_ctx_reset(ctx);
        return conf_load_ini(ctx, text, len);
    }
}
