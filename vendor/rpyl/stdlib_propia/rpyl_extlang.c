#include "rpyl_extlang.h"

#include "rpyl_config.h"
#include "rpyl_port.h"

#include <ctype.h>
#include <limits.h>
#if RPYL_ENABLE_FILE_IO
#include <stdio.h>
#endif
#include <string.h>

typedef struct LangEntry {
    char lang[RPYL_EXTLANG_MAX_LANG];
    RpylInitLangFn fn;
    void* userdata;
} LangEntry;

typedef struct InitBlock {
    char lang[RPYL_EXTLANG_MAX_LANG];
    int priority;
    int seq;
    int start_line;
    char code[RPYL_EXTLANG_MAX_INIT_CODE];
} InitBlock;

struct RpylLangHost {
    int active;
    LangEntry plugins[RPYL_EXTLANG_MAX_PLUGINS];
    int plugin_count;
    InitBlock inits[RPYL_EXTLANG_MAX_INITS];
    int init_count;
    char script[RPYL_EXTLANG_MAX_SCRIPT_BYTES];
    size_t script_len;
    char input[RPYL_EXTLANG_MAX_SCRIPT_BYTES];
    size_t input_len;
    char raw_code[RPYL_EXTLANG_MAX_INIT_CODE];
    size_t raw_len;
};

static RpylLangHost g_lang_hosts[RPYL_MAX_LANGHOSTS];

static int is_space_char(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static int is_ident_start(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
}

static int is_ident_char(char c) {
    return is_ident_start(c) || (c >= '0' && c <= '9') || c == '-';
}

static const char* lstrip(const char* s) {
    const char* p;
    p = s ? s : "";
    while (*p == ' ' || *p == '\t') p++;
    return p;
}

static int line_is_blank(const char* s) {
    const char* p;
    p = s ? s : "";
    while (*p) {
        if (!is_space_char(*p)) return 0;
        p++;
    }
    return 1;
}

static int count_indent(const char* s) {
    int n;
    const char* p;
    n = 0;
    p = s ? s : "";
    while (*p == ' ') {
        n++;
        p++;
    }
    return n;
}

static int indentation_has_tab(const char* s) {
    const char* p;
    p = s ? s : "";
    while (*p == ' ' || *p == '\t') {
        if (*p == '\t') return 1;
        p++;
    }
    return 0;
}

static void safe_copy(char* dst, size_t dst_size, const char* src) {
    rpyl_strcpy_trunc(dst, dst_size, src ? src : "");
}

static int safe_append(char* dst, size_t dst_size, const char* src) {
    size_t used;
    size_t add;

    if (!dst || dst_size == 0u || !src) return 0;
    used = strlen(dst);
    add = strlen(src);
    if (used + add + 1u > dst_size) return 0;
    if (add > 0u) memcpy(dst + used, src, add);
    dst[used + add] = 0;
    return 1;
}

static void set_extlang_error(RpylContext* ctx, const char* filename, int line, const char* message) {
    rpyl_set_error(ctx, "extlang", filename ? filename : "", line, 1, message ? message : "extlang failure");
}

static void set_lang_error(RpylContext* ctx, const char* filename, int line, const char* prefix, const char* lang) {
    char message[160];

    message[0] = 0;
    safe_copy(message, sizeof(message), prefix ? prefix : "extlang failure");
    if (lang && lang[0]) {
        (void)safe_append(message, sizeof(message), ": ");
        (void)safe_append(message, sizeof(message), lang);
    }
    set_extlang_error(ctx, filename, line, message);
}

static void to_lower_ascii(char* s) {
    size_t i;
    if (!s) return;
    for (i = 0u; s[i]; i++) {
        s[i] = (char)tolower((unsigned char)s[i]);
    }
}

static int valid_lang_name(const char* lang) {
    size_t i;

    if (!lang || !lang[0]) return 0;
    if (!is_ident_start(lang[0])) return 0;
    for (i = 1u; lang[i]; i++) {
        if (!is_ident_char(lang[i])) return 0;
    }
    return 1;
}

static LangEntry* find_plugin(RpylLangHost* host, const char* lang) {
    int i;
    if (!host || !lang) return (LangEntry*)0;
    for (i = 0; i < host->plugin_count; i++) {
        if (strcmp(host->plugins[i].lang, lang) == 0) return &host->plugins[i];
    }
    return (LangEntry*)0;
}

static const char* parse_int_token(const char* p, int* out_value, int* had_value, int* overflowed) {
    int sign;
    unsigned long value;
    unsigned long limit;
    unsigned long digit;

    if (out_value) *out_value = 0;
    if (had_value) *had_value = 0;
    if (overflowed) *overflowed = 0;
    if (!p) return p;

    while (*p == ' ' || *p == '\t') p++;
    sign = 1;
    if (*p == '-') {
        sign = -1;
        p++;
    } else if (*p == '+') {
        p++;
    }

    if (!isdigit((unsigned char)*p)) return p;
    if (had_value) *had_value = 1;
    value = 0UL;
    limit = (sign < 0) ? ((unsigned long)INT_MAX + 1UL) : (unsigned long)INT_MAX;

    while (isdigit((unsigned char)*p)) {
        digit = (unsigned long)(*p - '0');
        if (value > (limit - digit) / 10UL) {
            if (overflowed) *overflowed = 1;
        } else if (!overflowed || !*overflowed) {
            value = value * 10UL + digit;
        }
        p++;
    }

    if (overflowed && *overflowed) return p;
    if (out_value) {
        if (sign < 0 && value == (unsigned long)INT_MAX + 1UL) *out_value = INT_MIN;
        else if (sign < 0) *out_value = -(int)value;
        else *out_value = (int)value;
    }
    return p;
}

/* Return values: 0 = not an init header, 1 = valid, negative = malformed. */
static int parse_init_header(const char* line_trimmed, char* out_lang, size_t out_lang_sz, int* out_priority) {
    const char* p;
    int pri;
    int had_pri;
    int overflowed;
    size_t i;
    int lang_too_long;

    if (!line_trimmed || !out_lang || out_lang_sz == 0u || !out_priority) return -1;
    out_lang[0] = 0;
    *out_priority = 0;

    p = line_trimmed;
    if (strncmp(p, "init", 4u) != 0) return 0;
    p += 4;
    if (!(*p == ' ' || *p == '\t')) return 0;

    pri = 0;
    had_pri = 0;
    overflowed = 0;
    p = parse_int_token(p, &pri, &had_pri, &overflowed);
    if (overflowed) return -2;
    if (had_pri) *out_priority = pri;

    while (*p == ' ' || *p == '\t') p++;
    if (!is_ident_start(*p)) return -1;

    i = 0u;
    lang_too_long = 0;
    while (is_ident_char(*p)) {
        if (i + 1u < out_lang_sz) out_lang[i++] = (char)tolower((unsigned char)*p);
        else lang_too_long = 1;
        p++;
    }
    out_lang[i] = 0;
    if (lang_too_long) return -3;

    while (*p == ' ' || *p == '\t') p++;
    if (*p != ':') return -1;
    return 1;
}

static int append_buf(char* dst, size_t dst_sz, size_t* used, const char* text) {
    size_t n;

    if (!dst || !used || !text) return 0;
    n = strlen(text);
    if (*used + n + 1u > dst_sz) return 0;
    if (n > 0u) memcpy(dst + *used, text, n);
    *used += n;
    dst[*used] = 0;
    return 1;
}

static int next_line_from_buffer(const char* src, size_t len, size_t* pos, char* out, size_t out_sz, int* out_truncated) {
    size_t i;
    int truncated;

    if (out_truncated) *out_truncated = 0;
    if (!src || !pos || !out || out_sz == 0u) return 0;
    if (*pos >= len) return 0;

    i = 0u;
    truncated = 0;
    while (*pos < len) {
        char c;
        c = src[*pos];
        (*pos)++;
        if (i + 1u < out_sz) out[i++] = c;
        else truncated = 1;
        if (c == '\n') break;
    }
    out[i] = 0;
    if (out_truncated) *out_truncated = truncated;
    return 1;
}

static int append_stripped_line(char* dst, size_t dst_sz, size_t* used, const char* line, int strip_cols) {
    const char* p;
    int n;

    if (!dst || !used || !line) return 0;
    if (line_is_blank(line)) return append_buf(dst, dst_sz, used, "\n");

    p = line;
    n = 0;
    while (n < strip_cols && *p == ' ') {
        p++;
        n++;
    }
    return append_buf(dst, dst_sz, used, p);
}

static int finish_init(RpylContext* ctx, RpylLangHost* host, const char* filename, const char* lang, int priority, int seq, int start_line, int strip_cols) {
    InitBlock* block;
    char line[RPYL_EXTLANG_MAX_LINE];
    size_t pos;
    size_t used;
    int truncated;

    if (!host || !lang) return 0;
    if (host->init_count >= RPYL_EXTLANG_MAX_INITS) {
        set_extlang_error(ctx, filename, start_line, "too many init blocks; increase RPYL_EXTLANG_MAX_INITS");
        return 0;
    }

    block = &host->inits[host->init_count];
    memset(block, 0, sizeof(*block));
    safe_copy(block->lang, sizeof(block->lang), lang);
    block->priority = priority;
    block->seq = seq;
    block->start_line = start_line;

    pos = 0u;
    used = 0u;
    block->code[0] = 0;
    truncated = 0;
    while (next_line_from_buffer(host->raw_code, host->raw_len, &pos, line, sizeof(line), &truncated)) {
        if (truncated) {
            set_extlang_error(ctx, filename, start_line, "internal init line truncation detected");
            return 0;
        }
        if (!append_stripped_line(block->code, sizeof(block->code), &used, line, strip_cols)) {
            set_extlang_error(ctx, filename, start_line, "init block exceeds RPYL_EXTLANG_MAX_INIT_CODE");
            return 0;
        }
    }

    host->init_count++;
    host->raw_len = 0u;
    host->raw_code[0] = 0;
    return 1;
}

static void sort_inits(InitBlock* blocks, int count) {
    int i;

    for (i = 1; i < count; i++) {
        InitBlock key;
        int j;

        key = blocks[i];
        j = i - 1;
        while (j >= 0) {
            int move;
            move = 0;
            if (blocks[j].priority > key.priority) move = 1;
            else if (blocks[j].priority == key.priority && blocks[j].seq > key.seq) move = 1;
            if (!move) break;
            blocks[j + 1] = blocks[j];
            j--;
        }
        blocks[j + 1] = key;
    }
}

#if RPYL_ENABLE_FILE_IO
/* 1 success, -1 open, -2 read, -3 too large, 0 bad arguments. */
static int read_file_limited(const char* path, char* out, size_t out_sz, size_t* out_len) {
    FILE* f;
    size_t n;

    if (!path || !out || out_sz == 0u || !out_len) return 0;
    f = fopen(path, "rb");
    if (!f) return -1;
    n = fread(out, 1u, out_sz - 1u, f);
    if (ferror(f)) {
        fclose(f);
        return -2;
    }
    if (!feof(f)) {
        fclose(f);
        return -3;
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
    return -4;
}
#endif

RpylLangHost* rpyl_langhost_create(void) {
    int i;

    for (i = 0; i < RPYL_MAX_LANGHOSTS; i++) {
        if (!g_lang_hosts[i].active) {
            memset(&g_lang_hosts[i], 0, sizeof(g_lang_hosts[i]));
            g_lang_hosts[i].active = 1;
            return &g_lang_hosts[i];
        }
    }
    return (RpylLangHost*)0;
}

void rpyl_langhost_destroy(RpylLangHost* host) {
    if (!host) return;
    memset(host, 0, sizeof(*host));
}

int rpyl_langhost_register(RpylLangHost* host, const char* lang, RpylInitLangFn fn, void* userdata) {
    char norm[RPYL_EXTLANG_MAX_LANG];
    int i;

    if (!host || !lang || !lang[0] || !fn) return 0;
    if (strlen(lang) >= sizeof(norm)) return 0;
    if (!valid_lang_name(lang)) return 0;
    safe_copy(norm, sizeof(norm), lang);
    to_lower_ascii(norm);

    for (i = 0; i < host->plugin_count; i++) {
        if (strcmp(host->plugins[i].lang, norm) == 0) {
            host->plugins[i].fn = fn;
            host->plugins[i].userdata = userdata;
            return 1;
        }
    }

    if (host->plugin_count >= RPYL_EXTLANG_MAX_PLUGINS) return 0;
    safe_copy(host->plugins[host->plugin_count].lang, sizeof(host->plugins[host->plugin_count].lang), norm);
    host->plugins[host->plugin_count].fn = fn;
    host->plugins[host->plugin_count].userdata = userdata;
    host->plugin_count++;
    return 1;
}

int rpyl_load_buffer_extlang(RpylContext* ctx, RpylLangHost* host, const char* buffer, size_t len, const char* filename) {
    char line[RPYL_EXTLANG_MAX_LINE];
    char re_line[RPYL_EXTLANG_MAX_LINE];
    char init_lang[RPYL_EXTLANG_MAX_LANG];
    size_t pos;
    int has_re_line;
    int line_no;
    int in_init;
    int init_indent;
    int init_priority;
    int init_start_line;
    int init_seq;
    int strip_cols;
    int i;
    int truncated;
    const char* source_name;

    source_name = filename ? filename : "<buffer>";
    if (!ctx) return 0;
    rpyl_clear_error(ctx);
    if (!host) {
        set_extlang_error(ctx, source_name, 1, "null language host");
        return 0;
    }
    if (!buffer && len > 0u) {
        set_extlang_error(ctx, source_name, 1, "null input buffer with non-zero length");
        return 0;
    }
    if (buffer && len == 0u) len = strlen(buffer);

    host->init_count = 0;
    host->script_len = 0u;
    host->input_len = 0u;
    host->raw_len = 0u;
    host->script[0] = 0;
    host->input[0] = 0;
    host->raw_code[0] = 0;

    pos = 0u;
    has_re_line = 0;
    line_no = 0;
    in_init = 0;
    init_indent = 0;
    init_priority = 0;
    init_start_line = 0;
    init_seq = 0;
    strip_cols = 0;
    init_lang[0] = 0;
    re_line[0] = 0;

    while (1) {
        const char* trimmed;

        truncated = 0;
        if (has_re_line) {
            safe_copy(line, sizeof(line), re_line);
            has_re_line = 0;
        } else {
            if (!next_line_from_buffer(buffer ? buffer : "", len, &pos, line, sizeof(line), &truncated)) break;
            line_no++;
            if (truncated) {
                set_extlang_error(ctx, source_name, line_no, "line exceeds RPYL_EXTLANG_MAX_LINE; input was not truncated");
                return 0;
            }
        }

        trimmed = lstrip(line);
        if (!in_init) {
            char lang_buf[RPYL_EXTLANG_MAX_LANG];
            int pri_buf;
            int header_status;

            pri_buf = 0;
            header_status = parse_init_header(trimmed, lang_buf, sizeof(lang_buf), &pri_buf);
            if (header_status < 0 && count_indent(line) == 0) {
                if (header_status == -2) set_extlang_error(ctx, source_name, line_no, "init priority is outside the C int range");
                else if (header_status == -3) set_extlang_error(ctx, source_name, line_no, "init language name exceeds RPYL_EXTLANG_MAX_LANG");
                else set_extlang_error(ctx, source_name, line_no, "malformed init header; expected: init [priority] language:");
                return 0;
            }
            if (header_status == 1) {
                if (indentation_has_tab(line)) {
                    set_extlang_error(ctx, source_name, line_no, "tabs are not allowed in init indentation; use spaces");
                    return 0;
                }
                if (count_indent(line) != 0) {
                    set_extlang_error(ctx, source_name, line_no, "init header must start at column 1");
                    return 0;
                }
                in_init = 1;
                init_indent = 0;
                init_priority = pri_buf;
                init_start_line = line_no;
                init_seq = host->init_count;
                strip_cols = RPYL_EXTLANG_BODY_INDENT;
                safe_copy(init_lang, sizeof(init_lang), lang_buf);
                host->raw_len = 0u;
                host->raw_code[0] = 0;
                continue;
            }
            if (!append_buf(host->script, sizeof(host->script), &host->script_len, line)) {
                set_extlang_error(ctx, source_name, line_no, "script outside init blocks exceeds RPYL_EXTLANG_MAX_SCRIPT_BYTES");
                return 0;
            }
            continue;
        }

        if (!line_is_blank(line)) {
            int indent;
            indent = count_indent(line);
            if (indentation_has_tab(line)) {
                set_extlang_error(ctx, source_name, line_no, "tabs are not allowed in init indentation; use spaces");
                return 0;
            }
            if (indent <= init_indent) {
                if (!finish_init(ctx, host, source_name, init_lang, init_priority, init_seq, init_start_line, strip_cols)) return 0;
                in_init = 0;
                has_re_line = 1;
                safe_copy(re_line, sizeof(re_line), line);
                continue;
            }
            if (indent < strip_cols) {
                set_extlang_error(ctx, source_name, line_no, "init body must be indented by at least RPYL_EXTLANG_BODY_INDENT spaces");
                return 0;
            }
        }

        if (!append_buf(host->raw_code, sizeof(host->raw_code), &host->raw_len, line)) {
            set_extlang_error(ctx, source_name, line_no, "init block exceeds RPYL_EXTLANG_MAX_INIT_CODE");
            return 0;
        }
    }

    if (in_init) {
        if (!finish_init(ctx, host, source_name, init_lang, init_priority, init_seq, init_start_line, strip_cols)) return 0;
    }

    sort_inits(host->inits, host->init_count);
    for (i = 0; i < host->init_count; i++) {
        LangEntry* e;
        const RpylErrorInfo* plugin_error;

        e = find_plugin(host, host->inits[i].lang);
        if (!e || !e->fn) {
            set_lang_error(ctx, source_name, host->inits[i].start_line, "no plugin registered for init language", host->inits[i].lang);
            return 0;
        }
        rpyl_clear_error(ctx);
        if (!e->fn(ctx, e->userdata, host->inits[i].lang, host->inits[i].priority,
                   host->inits[i].code, source_name, host->inits[i].start_line)) {
            plugin_error = rpyl_get_last_error(ctx);
            if (!plugin_error || !plugin_error->message[0]) {
                set_lang_error(ctx, source_name, host->inits[i].start_line, "init language callback failed", host->inits[i].lang);
            }
            return 0;
        }
    }

    if (!rpyl_load_buffer(ctx, host->script, host->script_len)) {
        const RpylErrorInfo* core_error;
        char module[32];
        char message[160];
        int error_line;
        int error_column;

        core_error = rpyl_get_last_error(ctx);
        safe_copy(module, sizeof(module), core_error && core_error->module[0] ? core_error->module : "parser");
        safe_copy(message, sizeof(message), core_error && core_error->message[0] ? core_error->message : "failed to parse script after removing init blocks");
        error_line = core_error ? core_error->line : 1;
        error_column = core_error ? core_error->column : 1;
        rpyl_set_error(ctx, module, source_name, error_line, error_column, message);
        return 0;
    }
    return 1;
}

int rpyl_load_file_extlang(RpylContext* ctx, RpylLangHost* host, const char* path) {
    size_t len;
    int status;

    if (!ctx) return 0;
    rpyl_clear_error(ctx);
    if (!host || !path) {
        set_extlang_error(ctx, path ? path : "", 1, "invalid arguments to rpyl_load_file_extlang");
        return 0;
    }
    len = 0u;
    status = read_file_limited(path, host->input, sizeof(host->input), &len);
    if (status != 1) {
        if (status == -1) set_extlang_error(ctx, path, 1, "failed to open extlang script file");
        else if (status == -2) set_extlang_error(ctx, path, 1, "failed while reading extlang script file");
        else if (status == -3) set_extlang_error(ctx, path, 1, "extlang script file exceeds RPYL_EXTLANG_MAX_SCRIPT_BYTES");
        else if (status == -4) set_extlang_error(ctx, path, 1, "file IO disabled; use rpyl_load_buffer_extlang");
        else set_extlang_error(ctx, path, 1, "invalid extlang file-load arguments");
        return 0;
    }
    host->input_len = len;
    return rpyl_load_buffer_extlang(ctx, host, host->input, len, path);
}
