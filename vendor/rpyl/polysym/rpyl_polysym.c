#include "rpyl_polysym.h"

#include "rpyl_config.h"
#include "rpyl_port.h"

#if RPYL_ENABLE_FILE_IO
#include <stdio.h>
#endif
#include <string.h>

typedef struct SymbolEntry {
    char name[RPYL_POLYSYM_MAX_NAME];
    RpylSymbolFn fn;
    void* userdata;
} SymbolEntry;

struct RpylSymbolHost {
    int active;
    SymbolEntry symbols[RPYL_POLYSYM_MAX_SYMBOLS];
    int symbol_count;
    char dispatch_command[RPYL_POLYSYM_MAX_CMD];
};

typedef struct SymbolAttachment {
    RpylContext* ctx;
    RpylSymbolHost* host;
} SymbolAttachment;

static RpylSymbolHost g_symbol_hosts[RPYL_MAX_SYMBOLHOSTS];
static SymbolAttachment g_attachments[RPYL_POLYSYM_MAX_ATTACHMENTS];
static char g_sym_input[RPYL_POLYSYM_MAX_SCRIPT_BYTES];
static char g_sym_script[RPYL_POLYSYM_MAX_SCRIPT_BYTES];

static void safe_copy(char* dst, size_t dst_sz, const char* src) {
    if (!dst || dst_sz == 0u) return;
    rpyl_strcpy_trunc(dst, dst_sz, src ? src : "");
}

static RpylSymbolHost* attached_host_for(RpylContext* ctx) {
    int i;
    if (!ctx) return (RpylSymbolHost*)0;
    for (i = 0; i < RPYL_POLYSYM_MAX_ATTACHMENTS; i++) {
        if (g_attachments[i].ctx == ctx) return g_attachments[i].host;
    }
    return (RpylSymbolHost*)0;
}

static int attach_set(RpylContext* ctx, RpylSymbolHost* host) {
    int i;
    int empty_slot;
    if (!ctx || !host) return 0;
    empty_slot = -1;
    for (i = 0; i < RPYL_POLYSYM_MAX_ATTACHMENTS; i++) {
        if (g_attachments[i].ctx == ctx) {
            g_attachments[i].host = host;
            return 1;
        }
        if (empty_slot < 0 && !g_attachments[i].ctx) empty_slot = i;
    }
    if (empty_slot < 0) return 0;
    g_attachments[empty_slot].ctx = ctx;
    g_attachments[empty_slot].host = host;
    return 1;
}

static void attach_clear_ctx(RpylContext* ctx) {
    int i;
    if (!ctx) return;
    for (i = 0; i < RPYL_POLYSYM_MAX_ATTACHMENTS; i++) {
        if (g_attachments[i].ctx == ctx) {
            g_attachments[i].ctx = (RpylContext*)0;
            g_attachments[i].host = (RpylSymbolHost*)0;
        }
    }
}

static void attach_clear_host(RpylSymbolHost* host) {
    int i;
    if (!host) return;
    for (i = 0; i < RPYL_POLYSYM_MAX_ATTACHMENTS; i++) {
        if (g_attachments[i].host == host) {
            g_attachments[i].ctx = (RpylContext*)0;
            g_attachments[i].host = (RpylSymbolHost*)0;
        }
    }
}

static SymbolEntry* find_symbol(RpylSymbolHost* host, const char* name) {
    int i;
    if (!host || !name) return (SymbolEntry*)0;
    for (i = 0; i < host->symbol_count; i++) {
        if (strcmp(host->symbols[i].name, name) == 0) return &host->symbols[i];
    }
    return (SymbolEntry*)0;
}

static void cmd_dispatch_sym(RpylContext* ctx, const char** args, int argc) {
    RpylSymbolHost* host;
    SymbolEntry* e;
    if (!ctx || !args || argc < 1) return;
    host = attached_host_for(ctx);
    if (!host) return;
    e = find_symbol(host, args[0]);
    if (!e || !e->fn) return;
    (void)e->fn(ctx, e->userdata, args[0], args + 1, argc - 1);
}

RpylSymbolHost* rpyl_symbolhost_create(void) {
    int i;
    for (i = 0; i < RPYL_MAX_SYMBOLHOSTS; i++) {
        if (!g_symbol_hosts[i].active) {
            memset(&g_symbol_hosts[i], 0, sizeof(g_symbol_hosts[i]));
            g_symbol_hosts[i].active = 1;
            safe_copy(g_symbol_hosts[i].dispatch_command, sizeof(g_symbol_hosts[i].dispatch_command), "__sym");
            return &g_symbol_hosts[i];
        }
    }
    return (RpylSymbolHost*)0;
}

void rpyl_symbolhost_destroy(RpylSymbolHost* host) {
    if (!host) return;
    attach_clear_host(host);
    memset(host, 0, sizeof(*host));
}

void rpyl_symbolhost_set_dispatch_command(RpylSymbolHost* host, const char* command_name) {
    if (!host || !command_name || !command_name[0]) return;
    safe_copy(host->dispatch_command, sizeof(host->dispatch_command), command_name);
}

const char* rpyl_symbolhost_get_dispatch_command(RpylSymbolHost* host) {
    if (!host) return "__sym";
    return host->dispatch_command;
}

int rpyl_symbolhost_export(RpylSymbolHost* host, const char* symbol, RpylSymbolFn fn, void* userdata) {
    SymbolEntry* e;
    if (!host || !symbol || !symbol[0] || !fn) return 0;
    e = find_symbol(host, symbol);
    if (e) {
        e->fn = fn;
        e->userdata = userdata;
        return 1;
    }
    if (host->symbol_count >= RPYL_POLYSYM_MAX_SYMBOLS) return 0;
    e = &host->symbols[host->symbol_count];
    memset(e, 0, sizeof(*e));
    safe_copy(e->name, sizeof(e->name), symbol);
    e->fn = fn;
    e->userdata = userdata;
    host->symbol_count++;
    return 1;
}

int rpyl_symbolhost_remove(RpylSymbolHost* host, const char* symbol) {
    int i;
    if (!host || !symbol) return 0;
    for (i = 0; i < host->symbol_count; i++) {
        if (strcmp(host->symbols[i].name, symbol) == 0) {
            int j;
            for (j = i; j + 1 < host->symbol_count; j++) {
                host->symbols[j] = host->symbols[j + 1];
            }
            host->symbol_count--;
            memset(&host->symbols[host->symbol_count], 0, sizeof(host->symbols[host->symbol_count]));
            return 1;
        }
    }
    return 0;
}

int rpyl_symbolhost_attach(RpylContext* ctx, RpylSymbolHost* host) {
    if (!ctx || !host) return 0;
    if (!attach_set(ctx, host)) return 0;
    rpyl_register_command(ctx, host->dispatch_command, cmd_dispatch_sym);
    return 1;
}

void rpyl_symbolhost_detach(RpylContext* ctx) {
    attach_clear_ctx(ctx);
}

RpylSymbolHost* rpyl_symbolhost_get(RpylContext* ctx) {
    return attached_host_for(ctx);
}

static int count_indent(const char* s) {
    int n;
    n = 0;
    if (!s) return 0;
    while (*s == ' ') {
        n++;
        s++;
    }
    return n;
}

static const char* lstrip(const char* s) {
    if (!s) return "";
    while (*s == ' ' || *s == '\t') s++;
    return s;
}

static int line_is_blank(const char* s) {
    const char* p;
    p = s ? s : "";
    while (*p) {
        if (!(*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) return 0;
        p++;
    }
    return 1;
}

static int is_init_header_line(const char* line) {
    const char* p;
    int seen_colon;
    if (count_indent(line) != 0) return 0;
    p = lstrip(line);
    if (strncmp(p, "init", 4) != 0) return 0;
    p += 4;
    if (!(*p == ' ' || *p == '\t')) return 0;
    seen_colon = 0;
    while (*p && *p != '\n' && *p != '\r') {
        if (*p == ':') seen_colon = 1;
        p++;
    }
    return seen_colon;
}

static int append_text(char* dst, size_t dst_sz, size_t* used, const char* text) {
    size_t n;
    if (!dst || !used || !text) return 0;
    n = strlen(text);
    if (*used + n + 1u > dst_sz) return 0;
    memcpy(dst + *used, text, n);
    *used += n;
    dst[*used] = 0;
    return 1;
}

static int next_line_from_buffer(const char* src, size_t len, size_t* pos, char* out, size_t out_sz) {
    size_t i;
    char c;
    if (!src || !pos || !out || out_sz == 0u) return 0;
    if (*pos >= len) return 0;
    i = 0u;
    while (*pos < len) {
        c = src[*pos];
        (*pos)++;
        if (i + 1u < out_sz) out[i++] = c;
        if (c == '\n') break;
    }
    out[i] = 0;
    return 1;
}

#if RPYL_ENABLE_FILE_IO
static int read_file_limited(const char* path, char* out, size_t out_sz, size_t* out_len) {
    FILE* f;
    size_t n;
    if (!path || !out || out_sz == 0u || !out_len) return 0;
    f = fopen(path, "rb");
    if (!f) return 0;
    n = fread(out, 1u, out_sz - 1u, f);
    if (ferror(f)) {
        fclose(f);
        return 0;
    }
    if (!feof(f)) {
        fclose(f);
        return 0;
    }
    fclose(f);
    out[n] = 0;
    *out_len = n;
    return 1;
}

#else
static int read_file_limited(const char* path, char* out, size_t out_sz, size_t* out_len) {
    (void)path;
    if (out && out_sz > 0u) out[0] = 0;
    if (out_len) *out_len = 0u;
    return 0;
}
#endif

static const char* skip_ws(const char* p) {
    if (!p) return "";
    while (*p == ' ' || *p == '\t') p++;
    return p;
}

static const char* parse_ident(const char* p, char* out, size_t out_sz) {
    size_t i;
    if (!p || !out || out_sz == 0u) return (const char*)0;
    if (!((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z') || *p == '_')) return (const char*)0;
    i = 0u;
    while ((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9') || *p == '_' || *p == '.') {
        if (i + 1u < out_sz) out[i++] = *p;
        p++;
    }
    out[i] = 0;
    return p;
}

static const char* parse_quoted(const char* p, char* out, size_t out_sz) {
    char q;
    size_t i;
    if (!p || !out || out_sz == 0u) return (const char*)0;
    q = *p;
    if (!(q == '"' || q == '\'' || q == '`')) return (const char*)0;
    p++;
    i = 0u;
    while (*p && *p != q) {
        char c;
        c = *p++;
        if (c == '\\' && *p) {
            c = *p++;
            if (c == 'n') c = '\n';
            else if (c == 't') c = '\t';
            else if (c == 'r') c = '\r';
        }
        if (i + 1u < out_sz) out[i++] = c;
    }
    if (*p != q) return (const char*)0;
    p++;
    out[i] = 0;
    return p;
}

static const char* parse_bare_token(const char* p, char* out, size_t out_sz) {
    size_t i;
    if (!p || !out || out_sz == 0u) return (const char*)0;
    i = 0u;
    while (*p && *p != ',' && *p != ')' && *p != '\n' && *p != '\r' && *p != ' ' && *p != '\t') {
        if (i + 1u < out_sz) out[i++] = *p;
        p++;
    }
    out[i] = 0;
    return p;
}

static int quote_token_for_script(char* out, size_t out_sz, const char* token) {
    size_t used;
    const char* p;
    if (!out || out_sz == 0u || !token) return 0;
    used = strlen(out);
    if (used + 3u >= out_sz) return 0;
    out[used++] = ' ';
    out[used++] = '"';
    p = token;
    while (*p) {
        if (used + 3u >= out_sz) return 0;
        if (*p == '"' || *p == '\\') out[used++] = '\\';
        if (*p == '\n') {
            out[used++] = '\\';
            out[used++] = 'n';
            p++;
            continue;
        }
        out[used++] = *p++;
    }
    out[used++] = '"';
    out[used] = 0;
    return 1;
}

static int buffer_has_non_space(const char* out) {
    const char* p;
    if (!out) return 0;
    p = out;
    while (*p) {
        if (*p != ' ' && *p != '\t') return 1;
        p++;
    }
    return 0;
}

static int append_plain_arg(char* out, size_t out_sz, const char* token) {
    size_t used;
    size_t n;
    int need_space;

    if (!out || !token) return 0;
    used = strlen(out);
    n = strlen(token);
    need_space = buffer_has_non_space(out);
    if (used + n + (need_space ? 2u : 1u) > out_sz) return 0;
    if (need_space) out[used++] = ' ';
    memcpy(out + used, token, n);
    used += n;
    out[used] = 0;
    return 1;
}

static int translate_dollar_line(const char* line, const char* dispatch_cmd, char* out, size_t out_sz) {
    const char* p;
    char symbol[RPYL_POLYSYM_MAX_NAME];
    int indent;
    int i;
    if (!line || !dispatch_cmd || !out || out_sz == 0u) return 0;
    p = lstrip(line);
    if (*p != '$') return 0;
    out[0] = 0;
    indent = count_indent(line);
    for (i = 0; i < indent && i + 1 < (int)out_sz; i++) out[i] = ' ';
    out[i] = 0;
    p++;
    p = skip_ws(p);
    p = parse_ident(p, symbol, sizeof(symbol));
    if (!p || !symbol[0]) return -1;
    if (!append_plain_arg(out, out_sz, dispatch_cmd)) return -1;
    if (!append_plain_arg(out, out_sz, symbol)) return -1;
    p = skip_ws(p);
    if (*p == '(') {
        p++;
        while (1) {
            char tok[RPYL_POLYSYM_MAX_TOKEN];
            int quoted;
            p = skip_ws(p);
            if (*p == ')') {
                p++;
                break;
            }
            if (*p == 0 || *p == '\n' || *p == '\r') return -1;
            quoted = 0;
            if (*p == '"' || *p == '\'' || *p == '`') {
                p = parse_quoted(p, tok, sizeof(tok));
                quoted = 1;
            } else {
                p = parse_bare_token(p, tok, sizeof(tok));
            }
            if (!p || !tok[0]) return -1;
            if (quoted) {
                if (!quote_token_for_script(out, out_sz, tok)) return -1;
            } else {
                if (!append_plain_arg(out, out_sz, tok)) return -1;
            }
            p = skip_ws(p);
            if (*p == ',') {
                p++;
                continue;
            }
            if (*p == ')') {
                p++;
                break;
            }
            return -1;
        }
    } else {
        while (*p && *p != '\n' && *p != '\r') {
            char tok2[RPYL_POLYSYM_MAX_TOKEN];
            p = skip_ws(p);
            if (!*p || *p == '\n' || *p == '\r') break;
            p = parse_bare_token(p, tok2, sizeof(tok2));
            if (!p || !tok2[0]) return -1;
            if (!append_plain_arg(out, out_sz, tok2)) return -1;
        }
    }
    {
        size_t used;
        used = strlen(out);
        if (used + 2u > out_sz) return -1;
        out[used++] = '\n';
        out[used] = 0;
    }
    return 1;
}

int rpyl_load_buffer_extlang_symbols(
    RpylContext* ctx,
    RpylLangHost* lang_host,
    RpylSymbolHost* sym_host,
    const char* buffer,
    size_t len,
    const char* filename
) {
    size_t pos;
    size_t out_used;
    char line[RPYL_POLYSYM_MAX_LINE];
    char out_line[RPYL_POLYSYM_MAX_OUT_LINE];
    int in_init;
    int init_indent;
    int has_re_line;
    char re_line[RPYL_POLYSYM_MAX_LINE];

    if (!ctx || !lang_host || !sym_host || (!buffer && len > 0)) return 0;
    if (buffer && len == 0) len = strlen(buffer);
    if (!rpyl_symbolhost_attach(ctx, sym_host)) return 0;

    pos = 0;
    out_used = 0;
    g_sym_script[0] = 0;
    in_init = 0;
    init_indent = 0;
    has_re_line = 0;
    re_line[0] = 0;

    while (1) {
        if (has_re_line) {
            safe_copy(line, sizeof(line), re_line);
            has_re_line = 0;
        } else {
            if (!next_line_from_buffer(buffer ? buffer : "", len, &pos, line, sizeof(line))) break;
        }
        if (!in_init) {
            if (is_init_header_line(line)) {
                in_init = 1;
                init_indent = count_indent(line);
                if (!append_text(g_sym_script, sizeof(g_sym_script), &out_used, line)) return 0;
                continue;
            }
            if (*lstrip(line) == '$') {
                int t;
                t = translate_dollar_line(line, rpyl_symbolhost_get_dispatch_command(sym_host), out_line, sizeof(out_line));
                if (t < 0) return 0;
                if (t == 1) {
                    if (!append_text(g_sym_script, sizeof(g_sym_script), &out_used, out_line)) return 0;
                } else {
                    if (!append_text(g_sym_script, sizeof(g_sym_script), &out_used, line)) return 0;
                }
            } else {
                if (!append_text(g_sym_script, sizeof(g_sym_script), &out_used, line)) return 0;
            }
            continue;
        }
        if (!line_is_blank(line) && count_indent(line) <= init_indent) {
            in_init = 0;
            has_re_line = 1;
            safe_copy(re_line, sizeof(re_line), line);
            continue;
        }
        if (!append_text(g_sym_script, sizeof(g_sym_script), &out_used, line)) return 0;
    }

    return rpyl_load_buffer_extlang(ctx, lang_host, g_sym_script, strlen(g_sym_script), filename ? filename : "");
}

int rpyl_load_file_extlang_symbols(
    RpylContext* ctx,
    RpylLangHost* lang_host,
    RpylSymbolHost* sym_host,
    const char* path
) {
    size_t len;

    if (!ctx || !lang_host || !sym_host || !path) return 0;
    len = 0;
    if (!read_file_limited(path, g_sym_input, sizeof(g_sym_input), &len)) return 0;
    return rpyl_load_buffer_extlang_symbols(ctx, lang_host, sym_host, g_sym_input, len, path);
}
