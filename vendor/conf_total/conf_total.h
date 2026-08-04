#ifndef CONF_TOTAL_H
#define CONF_TOTAL_H

/* C89 only. No stdio. No stdlib.h. No string.h.
   All allocation is from a user-provided arena buffer (no malloc).
*/

/* -----------------------------
   Public slices and error model
   ----------------------------- */

typedef struct {
    const char *ptr;
    unsigned int len;
} conf_slice_t;

typedef enum {
    CONF_OK = 0,

    CONF_ERR_OOM,
    CONF_ERR_SYNTAX,
    CONF_ERR_BAD_NUMBER,
    CONF_ERR_BAD_ESCAPE,
    CONF_ERR_DUPLICATE_KEY,
    CONF_ERR_TYPE_MISMATCH,
    CONF_ERR_UNSUPPORTED
} conf_err_t;

typedef struct {
    conf_err_t code;
    unsigned int pos;   /* byte offset in the input (best-effort) */
    unsigned int line;  /* 1-based */
    unsigned int col;   /* 1-based */
} conf_error_t;

/* -----------------------------
   Canonical value model (AST-ish)
   ----------------------------- */

typedef long conf_fixed_t;

#define CONF_FIXED_SHIFT 16
#define CONF_FIXED_ONE   (1L << CONF_FIXED_SHIFT)
#define CONF_FIXED_FROM_INT(x) ((conf_fixed_t)((long)(x) << CONF_FIXED_SHIFT))
#define CONF_FIXED_TO_INT(x)   ((long)((x) >> CONF_FIXED_SHIFT))

typedef enum {
    CONF_NULL = 0,
    CONF_BOOL,
    CONF_INT,
    CONF_FIXED,
    CONF_STRING,     /* UTF-8 bytes, not necessarily NUL-terminated (but we often add a NUL) */
    CONF_DATETIME,   /* kept as string slice (TOML RFC3339-ish) */
    CONF_ARRAY,
    CONF_TABLE
} conf_type_t;

struct conf_value;
struct conf_pair;
struct conf_table;
struct conf_array;

typedef struct conf_value conf_value_t;
typedef struct conf_pair  conf_pair_t;
typedef struct conf_table conf_table_t;
typedef struct conf_array conf_array_t;

struct conf_array {
    conf_value_t *head;     /* linked list of elements */
    unsigned int  len;
};

struct conf_table {
    conf_pair_t *head;      /* linked list of key/value */
};

struct conf_value {
    conf_type_t type;

    union {
        int b;
        long i;
        conf_fixed_t fx;
        conf_slice_t s;
        conf_array_t *a;
        conf_table_t *t;
    } as;

    conf_value_t *next;     /* used for array element linkage */
};

struct conf_pair {
    conf_slice_t  key;
    conf_value_t *value;
    conf_pair_t  *next;
};

/* -----------------------------
   Context + arena
   ----------------------------- */

typedef struct conf_ctx conf_ctx_t;

typedef enum {
    CONF_LOAD_STRICT         = 0u,
    CONF_LOAD_OVERRIDE       = 1u << 0, /* allow later assignments to replace earlier ones */
    CONF_LOAD_COPY_SLICES    = 1u << 1  /* copy keys/strings into arena (recommended). On by default. */
} conf_load_flags_t;

struct conf_ctx {
    /* Arena */
    unsigned char *arena_mem;
    unsigned int   arena_cap;
    unsigned int   arena_off;

    /* Root is always a TABLE value */
    conf_value_t  *root;

    /* Optional fallback chain for defaults */
    const conf_ctx_t *fallback;

    /* Last error */
    conf_error_t err;

    /* Load flags */
    unsigned int load_flags;
};

/* -----------------------------
   Initialization
   ----------------------------- */

void conf_ctx_init(conf_ctx_t *ctx, void *arena_mem, unsigned int arena_size);

/* Reset context: clears root and rewinds arena (keeps flags/fallback). */
void conf_ctx_reset(conf_ctx_t *ctx);

/* Defaults / layering: lookup falls back when key not found at any level. */
void conf_ctx_set_fallback(conf_ctx_t *ctx, const conf_ctx_t *fallback);

/* Configure load behavior */
void conf_ctx_set_load_flags(conf_ctx_t *ctx, unsigned int flags);

/* Error access */
const conf_error_t *conf_last_error(const conf_ctx_t *ctx);

/* -----------------------------
   Loading from text (no file IO)
   ----------------------------- */

conf_err_t conf_load_auto(conf_ctx_t *ctx, const char *text, unsigned int len);
conf_err_t conf_load_toml(conf_ctx_t *ctx, const char *text, unsigned int len);
conf_err_t conf_load_yaml(conf_ctx_t *ctx, const char *text, unsigned int len); /* YAML 1.2 config-subset */
conf_err_t conf_load_ini (conf_ctx_t *ctx, const char *text, unsigned int len); /* pragmatic INI subset */

/* -----------------------------
   Queries (path-based)
   Path grammar (API-side):
     - segment: [A-Za-z0-9_-]+
     - separators: '.'
     - array index: [0], [1], ...
     - you may start with an index if root is an array (we keep root table, but arrays can appear anywhere)
   Examples:
     "video.fullscreen"
     "db.ports[0]"
     "servers[1].host"
   ----------------------------- */

const conf_value_t *conf_get(const conf_ctx_t *ctx, const char *path);

int   conf_get_flag (const conf_ctx_t *ctx, const char *path, int def);
/* For booleans: if value is NULL (present but null), it's treated as "flag present" => true */
int   conf_get_bool (const conf_ctx_t *ctx, const char *path, int def);
long  conf_get_int  (const conf_ctx_t *ctx, const char *path, long def);
conf_fixed_t conf_get_fixed(const conf_ctx_t *ctx, const char *path, conf_fixed_t def);
conf_slice_t conf_get_string(const conf_ctx_t *ctx, const char *path, conf_slice_t def);

/* Introspection helpers (non-path) */
const conf_value_t *conf_root(const conf_ctx_t *ctx);
unsigned int conf_array_len(const conf_value_t *v);
const conf_value_t *conf_array_at(const conf_value_t *v, unsigned int idx);
const conf_value_t *conf_table_get_slice(const conf_value_t *v, conf_slice_t key);

/* -----------------------------
   Overrides (write API)
   These create/replace values using dot-paths.
   They allocate from arena (still no malloc).
   ----------------------------- */

conf_err_t conf_override_bool(conf_ctx_t *ctx, const char *path, int v);
conf_err_t conf_override_int(conf_ctx_t *ctx, const char *path, long v);
conf_err_t conf_override_fixed(conf_ctx_t *ctx, const char *path, conf_fixed_t v);
conf_err_t conf_override_string(conf_ctx_t *ctx, const char *path, const char *bytes, unsigned int len);

/* Parse a TOML scalar (not a whole document) and override path with it.
   Example scalar strings:
     "true"
     "123"
     "3.14"
     "\"hello\""
     "[1,2,3]"
     "{ a = 1, b = 2 }"
*/
conf_err_t conf_override_scalar_toml(conf_ctx_t *ctx, const char *path, const char *scalar_text, unsigned int len);

#endif /* CONF_TOTAL_H */
