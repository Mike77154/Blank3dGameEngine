#include "rpyl_bytecode.h"

#if RPYL_ENABLE_FILE_IO
#include <stdio.h>
#endif
#include <string.h>

#define RPYL_BC_MAGIC_0 'R'
#define RPYL_BC_MAGIC_1 'P'
#define RPYL_BC_MAGIC_2 'B'
#define RPYL_BC_MAGIC_3 '1'

static RpylBytecode g_bytecodes[RPYL_MAX_BYTECODES];
static unsigned char g_bytecodes_used[RPYL_MAX_BYTECODES];

static void init_caps(RpylBytecode* bc) {
    if (!bc) return;
    bc->version = (rpyl_u32)RPYL_BC_VERSION;
    bc->code = bc->code_storage;
    bc->strings = bc->string_storage;
    bc->labels = bc->label_storage;
    bc->defines = bc->define_storage;
    bc->code_cap = (size_t)RPYL_BC_MAX_CODE;
    bc->string_cap = (size_t)RPYL_BC_MAX_STRINGS;
    bc->label_cap = (size_t)RPYL_BC_MAX_LABELS;
    bc->define_cap = (size_t)RPYL_BC_MAX_DEFINES;
    bc->next_once_id = 1;
}

static int ensure_code_cap(RpylBytecode* bc, size_t need) {
    if (!bc) return 0;
    if (need <= (size_t)RPYL_BC_MAX_CODE) return 1;
    bc->overflowed = 1;
    return 0;
}

static int ensure_string_cap(RpylBytecode* bc, size_t need) {
    if (!bc) return 0;
    if (need <= (size_t)RPYL_BC_MAX_STRINGS) return 1;
    bc->overflowed = 1;
    return 0;
}

static int ensure_label_cap(RpylBytecode* bc, size_t need) {
    if (!bc) return 0;
    if (need <= (size_t)RPYL_BC_MAX_LABELS) return 1;
    bc->overflowed = 1;
    return 0;
}

static int ensure_define_cap(RpylBytecode* bc, size_t need) {
    if (!bc) return 0;
    if (need <= (size_t)RPYL_BC_MAX_DEFINES) return 1;
    bc->overflowed = 1;
    return 0;
}

static int code_push(RpylBytecode* bc, rpyl_u32 w) {
    if (!ensure_code_cap(bc, bc->code_count + 1)) return 0;
    bc->code[bc->code_count++] = w;
    return 1;
}

static int find_string(const RpylBytecode* bc, const char* s) {
    size_t i;
    if (!bc || !s) return -1;
    for (i = 0; i < bc->string_count; i++) {
        if (bc->strings[i] && strcmp(bc->strings[i], s) == 0) return (int)i;
    }
    return -1;
}

static int add_string_raw(RpylBytecode* bc, const char* s) {
    size_t n;
    char* dst;
    int idx;

    if (!bc) return -1;
    if (!s) s = "";
    if (!ensure_string_cap(bc, bc->string_count + 1)) return -1;

    n = strlen(s);
    if (n + 1 > ((size_t)RPYL_BC_MAX_STRING_BYTES - bc->string_bytes)) {
        bc->overflowed = 1;
        return -1;
    }

    dst = bc->string_data + bc->string_bytes;
    memcpy(dst, s, n + 1);
    bc->string_bytes += n + 1;
    bc->strings[bc->string_count] = dst;
    idx = (int)bc->string_count;
    bc->string_count++;
    return idx;
}

static int intern_string(RpylBytecode* bc, const char* s) {
    int idx;
    if (!bc) return -1;
    if (!s) s = "";
    idx = find_string(bc, s);
    if (idx >= 0) return idx;
    return add_string_raw(bc, s);
}

static int define_find(RpylBytecode* bc, int name_sid) {
    size_t i;
    if (!bc) return -1;
    for (i = 0; i < bc->define_count; i++) {
        if ((int)bc->defines[i].name_sid == name_sid) return (int)i;
    }
    return -1;
}

static int define_add_or_set(RpylBytecode* bc, int name_sid, int value_sid) {
    int idx;
    if (!bc) return 0;
    idx = define_find(bc, name_sid);
    if (idx >= 0) {
        bc->defines[idx].value_sid = (rpyl_u32)value_sid;
        return 1;
    }
    if (!ensure_define_cap(bc, bc->define_count + 1)) return 0;
    bc->defines[bc->define_count].name_sid = (rpyl_u32)name_sid;
    bc->defines[bc->define_count].value_sid = (rpyl_u32)value_sid;
    bc->define_count++;
    return 1;
}

static int label_add(RpylBytecode* bc, int name_sid, rpyl_u32 ip, rpyl_u32 end_ip) {
    if (!bc) return 0;
    if (!ensure_label_cap(bc, bc->label_count + 1)) return 0;
    bc->labels[bc->label_count].name_sid = (rpyl_u32)name_sid;
    bc->labels[bc->label_count].ip = ip;
    bc->labels[bc->label_count].end_ip = end_ip;
    bc->label_count++;
    return 1;
}

static int compile_node(RpylBytecode* bc, AstNode* node);

static int compile_call(RpylBytecode* bc, AstNode* node) {
    const char* name;
    if (!bc || !node) return 0;
    name = node->name;

    if (name && strcmp(name, "once") == 0) {
        if (node->arg_count >= 1) {
            int once_id;
            size_t skip_word_index;
            int inner_name_sid;
            int argc;
            int i;

            once_id = bc->next_once_id++;
            if (!code_push(bc, (rpyl_u32)RPYL_BC_OP_ONCE_CHECK)) return 0;
            if (!code_push(bc, (rpyl_u32)once_id)) return 0;
            skip_word_index = bc->code_count;
            if (!code_push(bc, 0)) return 0;

            inner_name_sid = intern_string(bc, node->args[0]);
            if (inner_name_sid < 0) return 0;
            if (!code_push(bc, (rpyl_u32)RPYL_BC_OP_CMD)) return 0;
            if (!code_push(bc, (rpyl_u32)inner_name_sid)) return 0;

            argc = node->arg_count - 1;
            if (argc < 0) argc = 0;
            if (!code_push(bc, (rpyl_u32)argc)) return 0;
            for (i = 0; i < argc; i++) {
                int sid;
                sid = intern_string(bc, node->args[i + 1]);
                if (sid < 0) return 0;
                if (!code_push(bc, (rpyl_u32)sid)) return 0;
            }
            bc->code[skip_word_index] = (rpyl_u32)bc->code_count;
            return 1;
        }
        return 1;
    }

    if (name && strcmp(name, "on_enter") == 0) {
        if (node->arg_count >= 1) {
            size_t skip_word_index;
            int inner_name_sid;
            int argc;
            int i;

            if (!code_push(bc, (rpyl_u32)RPYL_BC_OP_ON_ENTER_CHECK)) return 0;
            skip_word_index = bc->code_count;
            if (!code_push(bc, 0)) return 0;

            inner_name_sid = intern_string(bc, node->args[0]);
            if (inner_name_sid < 0) return 0;
            if (!code_push(bc, (rpyl_u32)RPYL_BC_OP_CMD)) return 0;
            if (!code_push(bc, (rpyl_u32)inner_name_sid)) return 0;

            argc = node->arg_count - 1;
            if (argc < 0) argc = 0;
            if (!code_push(bc, (rpyl_u32)argc)) return 0;
            for (i = 0; i < argc; i++) {
                int sid;
                sid = intern_string(bc, node->args[i + 1]);
                if (sid < 0) return 0;
                if (!code_push(bc, (rpyl_u32)sid)) return 0;
            }
            bc->code[skip_word_index] = (rpyl_u32)bc->code_count;
            return 1;
        }
        return 1;
    }

    if (name && (strcmp(name, "set") == 0 || strcmp(name, "let") == 0 || strcmp(name, "setg") == 0 || strcmp(name, "global") == 0)) {
        int flags;
        int var_sid;
        int argc;
        int i;

        if (node->arg_count < 2) return 1;
        flags = 0;
        if (strcmp(name, "setg") == 0 || strcmp(name, "global") == 0) flags |= RPYL_BC_SET_GLOBAL;
        var_sid = intern_string(bc, node->args[0]);
        if (var_sid < 0) return 0;
        argc = node->arg_count - 1;
        if (argc < 0) argc = 0;

        if (!code_push(bc, (rpyl_u32)RPYL_BC_OP_SET)) return 0;
        if (!code_push(bc, (rpyl_u32)flags)) return 0;
        if (!code_push(bc, (rpyl_u32)var_sid)) return 0;
        if (!code_push(bc, (rpyl_u32)argc)) return 0;

        for (i = 0; i < argc; i++) {
            int sid;
            sid = intern_string(bc, node->args[i + 1]);
            if (sid < 0) return 0;
            if (!code_push(bc, (rpyl_u32)sid)) return 0;
        }
        return 1;
    }

    if (name && strcmp(name, "call") == 0) {
        int argc;
        int i;
        if (node->arg_count < 1) return 1;
        argc = node->arg_count;
        if (!code_push(bc, (rpyl_u32)RPYL_BC_OP_CALL)) return 0;
        if (!code_push(bc, (rpyl_u32)argc)) return 0;
        for (i = 0; i < argc; i++) {
            int sid;
            sid = intern_string(bc, node->args[i]);
            if (sid < 0) return 0;
            if (!code_push(bc, (rpyl_u32)sid)) return 0;
        }
        return 1;
    }

    if (name && (strcmp(name, "jump") == 0 || strcmp(name, "goto") == 0)) {
        int argc;
        int i;
        if (node->arg_count < 1) return 1;
        argc = node->arg_count;
        if (!code_push(bc, (rpyl_u32)RPYL_BC_OP_JUMP)) return 0;
        if (!code_push(bc, (rpyl_u32)argc)) return 0;
        for (i = 0; i < argc; i++) {
            int sid;
            sid = intern_string(bc, node->args[i]);
            if (sid < 0) return 0;
            if (!code_push(bc, (rpyl_u32)sid)) return 0;
        }
        return 1;
    }

    if (name && strcmp(name, "return") == 0) {
        if (!code_push(bc, (rpyl_u32)RPYL_BC_OP_RETURN)) return 0;
        return 1;
    }

    {
        int name_sid;
        int argc;
        int i;

        name_sid = intern_string(bc, name);
        if (name_sid < 0) return 0;
        argc = node->arg_count;
        if (!code_push(bc, (rpyl_u32)RPYL_BC_OP_CMD)) return 0;
        if (!code_push(bc, (rpyl_u32)name_sid)) return 0;
        if (!code_push(bc, (rpyl_u32)argc)) return 0;
        for (i = 0; i < argc; i++) {
            int sid;
            sid = intern_string(bc, node->args[i]);
            if (sid < 0) return 0;
            if (!code_push(bc, (rpyl_u32)sid)) return 0;
        }
    }

    return 1;
}

static int compile_block(RpylBytecode* bc, AstNode* node) {
    const char* name;
    AstNode* child;
    if (!bc || !node) return 0;
    name = node->name;

    if (name && strcmp(name, "once") == 0) {
        int once_id;
        size_t skip_word_index;
        once_id = bc->next_once_id++;
        if (!code_push(bc, (rpyl_u32)RPYL_BC_OP_ONCE_CHECK)) return 0;
        if (!code_push(bc, (rpyl_u32)once_id)) return 0;
        skip_word_index = bc->code_count;
        if (!code_push(bc, 0)) return 0;
        child = node->children;
        while (child) {
            if (!compile_node(bc, child)) return 0;
            child = child->next;
        }
        bc->code[skip_word_index] = (rpyl_u32)bc->code_count;
        return 1;
    }

    if (name && strcmp(name, "on_enter") == 0) {
        size_t skip_word_index;
        if (!code_push(bc, (rpyl_u32)RPYL_BC_OP_ON_ENTER_CHECK)) return 0;
        skip_word_index = bc->code_count;
        if (!code_push(bc, 0)) return 0;
        child = node->children;
        while (child) {
            if (!compile_node(bc, child)) return 0;
            child = child->next;
        }
        bc->code[skip_word_index] = (rpyl_u32)bc->code_count;
        return 1;
    }

    child = node->children;
    while (child) {
        if (!compile_node(bc, child)) return 0;
        child = child->next;
    }
    return 1;
}

static int compile_node(RpylBytecode* bc, AstNode* node) {
    if (!bc || !node) return 0;
    if (node->type == AST_CALL) return compile_call(bc, node);
    if (node->type == AST_BLOCK) return compile_block(bc, node);
    return 1;
}

static int bytecode_is_pool_object(const RpylBytecode* bc) {
    int i;
    if (!bc) return 0;
    for (i = 0; i < RPYL_MAX_BYTECODES; i++) {
        if (&g_bytecodes[i] == bc) return 1;
    }
    return 0;
}

RpylBytecode* rpyl_bytecode_create(void) {
    int i;
    for (i = 0; i < RPYL_MAX_BYTECODES; i++) {
        if (!g_bytecodes_used[i]) {
            g_bytecodes_used[i] = 1;
            memset(&g_bytecodes[i], 0, sizeof(g_bytecodes[i]));
            g_bytecodes[i].from_pool = 1;
            init_caps(&g_bytecodes[i]);
            return &g_bytecodes[i];
        }
    }
    return (RpylBytecode*)0;
}

void rpyl_bytecode_clear(RpylBytecode* bc) {
    int from_pool;
    if (!bc) return;
    from_pool = bytecode_is_pool_object(bc);
    memset(bc, 0, sizeof(*bc));
    bc->from_pool = from_pool;
    init_caps(bc);
}

void rpyl_bytecode_destroy(RpylBytecode* bc) {
    int i;
    if (!bc) return;
    for (i = 0; i < RPYL_MAX_BYTECODES; i++) {
        if (&g_bytecodes[i] == bc) {
            rpyl_bytecode_clear(bc);
            g_bytecodes_used[i] = 0;
            return;
        }
    }
    rpyl_bytecode_clear(bc);
}

int rpyl_bytecode_copy_into(RpylBytecode* out, const RpylBytecode* in) {
    size_t i;
    if (!out || !in) return 0;
    rpyl_bytecode_clear(out);
    out->version = in->version;
    for (i = 0; i < in->string_count; i++) {
        if (add_string_raw(out, in->strings[i] ? in->strings[i] : "") < 0) return 0;
    }
    if (in->code_count > (size_t)RPYL_BC_MAX_CODE) return 0;
    for (i = 0; i < in->code_count; i++) out->code[i] = in->code[i];
    out->code_count = in->code_count;
    if (in->label_count > (size_t)RPYL_BC_MAX_LABELS) return 0;
    for (i = 0; i < in->label_count; i++) out->labels[i] = in->labels[i];
    out->label_count = in->label_count;
    if (in->define_count > (size_t)RPYL_BC_MAX_DEFINES) return 0;
    for (i = 0; i < in->define_count; i++) out->defines[i] = in->defines[i];
    out->define_count = in->define_count;
    out->next_once_id = in->next_once_id;
    return 1;
}

int rpyl_bytecode_copy(RpylBytecode* out, const RpylBytecode* in) {
    return rpyl_bytecode_copy_into(out, in);
}

int rpyl_bytecode_compile_ast(RpylBytecode* out, AstNode* root) {
    AstNode* n;
    if (!out || !root) return 0;
    rpyl_bytecode_clear(out);
    out->version = (rpyl_u32)RPYL_BC_VERSION;

    n = root->children;
    while (n) {
        if (n->type == AST_DEFINE) {
            int ns;
            int vs;
            ns = intern_string(out, n->name);
            vs = intern_string(out, n->value);
            if (ns < 0 || vs < 0) return 0;
            if (!define_add_or_set(out, ns, vs)) return 0;
        }
        n = n->next;
    }

    n = root->children;
    while (n) {
        if (n->type == AST_BLOCK) {
            int name_sid;
            rpyl_u32 start_ip;
            rpyl_u32 end_ip;
            AstNode* child;
            name_sid = intern_string(out, n->name);
            if (name_sid < 0) return 0;
            start_ip = (rpyl_u32)out->code_count;
            child = n->children;
            while (child) {
                if (!compile_node(out, child)) return 0;
                child = child->next;
            }
            if (!code_push(out, (rpyl_u32)RPYL_BC_OP_END)) return 0;
            end_ip = (rpyl_u32)out->code_count;
            if (!label_add(out, name_sid, start_ip, end_ip)) return 0;
        }
        n = n->next;
    }
    return 1;
}


typedef struct RpylBcBufferWriter {
    unsigned char* data;
    size_t capacity;
    size_t pos;
    int overflowed;
} RpylBcBufferWriter;

typedef struct RpylBcBufferReader {
    const unsigned char* data;
    size_t len;
    size_t pos;
} RpylBcBufferReader;

static int bc_buf_write(RpylBcBufferWriter* w, const void* src, size_t n) {
    const unsigned char* p;
    size_t i;
    if (!w) return 0;
    if (w->pos + n < w->pos) {
        w->overflowed = 1;
        return 0;
    }
    if (w->data) {
        if (w->pos + n > w->capacity) {
            w->overflowed = 1;
            return 0;
        }
        p = (const unsigned char*)src;
        for (i = 0; i < n; i++) w->data[w->pos + i] = p ? p[i] : 0u;
    }
    w->pos += n;
    return 1;
}

static int bc_buf_write_u32(RpylBcBufferWriter* w, rpyl_u32 v) {
    unsigned char b[4];
    b[0] = (unsigned char)(v & 0xFFu);
    b[1] = (unsigned char)((v >> 8) & 0xFFu);
    b[2] = (unsigned char)((v >> 16) & 0xFFu);
    b[3] = (unsigned char)((v >> 24) & 0xFFu);
    return bc_buf_write(w, b, 4u);
}

static int bc_buf_read(RpylBcBufferReader* r, void* dst, size_t n) {
    unsigned char* d;
    size_t i;
    if (!r || !dst) return 0;
    if (r->pos + n < r->pos || r->pos + n > r->len) return 0;
    d = (unsigned char*)dst;
    for (i = 0; i < n; i++) d[i] = r->data[r->pos + i];
    r->pos += n;
    return 1;
}

static int bc_buf_read_u32(RpylBcBufferReader* r, rpyl_u32* out) {
    unsigned char b[4];
    rpyl_u32 v;
    if (!out) return 0;
    if (!bc_buf_read(r, b, 4u)) return 0;
    v = 0;
    v |= (rpyl_u32)b[0];
    v |= ((rpyl_u32)b[1]) << 8;
    v |= ((rpyl_u32)b[2]) << 16;
    v |= ((rpyl_u32)b[3]) << 24;
    *out = v;
    return 1;
}

size_t rpyl_bytecode_save_buffer(const RpylBytecode* bc, void* out_data, size_t out_capacity) {
    RpylBcBufferWriter w;
    rpyl_u32 i;
    unsigned char m[4];

    if (!bc) return 0u;
    w.data = (unsigned char*)out_data;
    w.capacity = out_capacity;
    w.pos = 0u;
    w.overflowed = 0;

    m[0] = (unsigned char)RPYL_BC_MAGIC_0;
    m[1] = (unsigned char)RPYL_BC_MAGIC_1;
    m[2] = (unsigned char)RPYL_BC_MAGIC_2;
    m[3] = (unsigned char)RPYL_BC_MAGIC_3;
    if (!bc_buf_write(&w, m, 4u)) return 0u;

    if (!bc_buf_write_u32(&w, (rpyl_u32)bc->version)) return 0u;
    if (!bc_buf_write_u32(&w, (rpyl_u32)bc->string_count)) return 0u;
    if (!bc_buf_write_u32(&w, (rpyl_u32)bc->code_count)) return 0u;
    if (!bc_buf_write_u32(&w, (rpyl_u32)bc->label_count)) return 0u;
    if (!bc_buf_write_u32(&w, (rpyl_u32)bc->define_count)) return 0u;

    for (i = 0; i < (rpyl_u32)bc->string_count; i++) {
        const char* str;
        rpyl_u32 len;
        str = bc->strings[i] ? bc->strings[i] : "";
        len = (rpyl_u32)strlen(str);
        if (!bc_buf_write_u32(&w, len)) return 0u;
        if (len > 0u) {
            if (!bc_buf_write(&w, str, (size_t)len)) return 0u;
        }
    }

    for (i = 0; i < (rpyl_u32)bc->code_count; i++) {
        if (!bc_buf_write_u32(&w, bc->code[i])) return 0u;
    }

    for (i = 0; i < (rpyl_u32)bc->label_count; i++) {
        if (!bc_buf_write_u32(&w, bc->labels[i].name_sid)) return 0u;
        if (!bc_buf_write_u32(&w, bc->labels[i].ip)) return 0u;
        if (!bc_buf_write_u32(&w, bc->labels[i].end_ip)) return 0u;
    }

    for (i = 0; i < (rpyl_u32)bc->define_count; i++) {
        if (!bc_buf_write_u32(&w, bc->defines[i].name_sid)) return 0u;
        if (!bc_buf_write_u32(&w, bc->defines[i].value_sid)) return 0u;
    }

    return w.overflowed ? 0u : w.pos;
}

int rpyl_bytecode_load_buffer(RpylBytecode* bc, const void* data, size_t len) {
    RpylBcBufferReader r;
    unsigned char m[4];
    rpyl_u32 version;
    rpyl_u32 sc;
    rpyl_u32 cc;
    rpyl_u32 lc;
    rpyl_u32 dc;
    rpyl_u32 i;

    if (!bc || (!data && len > 0u)) return 0;
    r.data = (const unsigned char*)data;
    r.len = len;
    r.pos = 0u;

    if (!bc_buf_read(&r, m, 4u)) return 0;
    if (m[0] != (unsigned char)RPYL_BC_MAGIC_0 || m[1] != (unsigned char)RPYL_BC_MAGIC_1 || m[2] != (unsigned char)RPYL_BC_MAGIC_2 || m[3] != (unsigned char)RPYL_BC_MAGIC_3) return 0;

    if (!bc_buf_read_u32(&r, &version)) return 0;
    if (version != (rpyl_u32)RPYL_BC_VERSION) return 0;
    if (!bc_buf_read_u32(&r, &sc)) return 0;
    if (!bc_buf_read_u32(&r, &cc)) return 0;
    if (!bc_buf_read_u32(&r, &lc)) return 0;
    if (!bc_buf_read_u32(&r, &dc)) return 0;

    if (sc > (rpyl_u32)RPYL_BC_MAX_STRINGS || cc > (rpyl_u32)RPYL_BC_MAX_CODE || lc > (rpyl_u32)RPYL_BC_MAX_LABELS || dc > (rpyl_u32)RPYL_BC_MAX_DEFINES) return 0;

    rpyl_bytecode_clear(bc);
    bc->version = version;

    for (i = 0; i < sc; i++) {
        rpyl_u32 slen;
        char* dst;
        if (!bc_buf_read_u32(&r, &slen)) { rpyl_bytecode_clear(bc); return 0; }
        if ((size_t)slen + 1u > ((size_t)RPYL_BC_MAX_STRING_BYTES - bc->string_bytes)) { rpyl_bytecode_clear(bc); return 0; }
        dst = bc->string_data + bc->string_bytes;
        if (slen > 0u) {
            if (!bc_buf_read(&r, dst, (size_t)slen)) { rpyl_bytecode_clear(bc); return 0; }
        }
        dst[slen] = 0;
        bc->strings[bc->string_count++] = dst;
        bc->string_bytes += (size_t)slen + 1u;
    }

    for (i = 0; i < cc; i++) {
        rpyl_u32 wv;
        if (!bc_buf_read_u32(&r, &wv)) { rpyl_bytecode_clear(bc); return 0; }
        bc->code[bc->code_count++] = wv;
    }

    for (i = 0; i < lc; i++) {
        RpylBcLabel L;
        if (!bc_buf_read_u32(&r, &L.name_sid)) { rpyl_bytecode_clear(bc); return 0; }
        if (!bc_buf_read_u32(&r, &L.ip)) { rpyl_bytecode_clear(bc); return 0; }
        if (!bc_buf_read_u32(&r, &L.end_ip)) { rpyl_bytecode_clear(bc); return 0; }
        bc->labels[bc->label_count++] = L;
    }

    for (i = 0; i < dc; i++) {
        RpylBcDefine D;
        if (!bc_buf_read_u32(&r, &D.name_sid)) { rpyl_bytecode_clear(bc); return 0; }
        if (!bc_buf_read_u32(&r, &D.value_sid)) { rpyl_bytecode_clear(bc); return 0; }
        bc->defines[bc->define_count++] = D;
    }

    return 1;
}

#if RPYL_ENABLE_FILE_IO
static int write_u32(FILE* f, rpyl_u32 v) {
    unsigned char b[4];
    b[0] = (unsigned char)(v & 0xFFu);
    b[1] = (unsigned char)((v >> 8) & 0xFFu);
    b[2] = (unsigned char)((v >> 16) & 0xFFu);
    b[3] = (unsigned char)((v >> 24) & 0xFFu);
    return (fwrite(b, 1, 4, f) == 4) ? 1 : 0;
}

static int read_u32(FILE* f, rpyl_u32* out) {
    unsigned char b[4];
    size_t n;
    rpyl_u32 v;
    if (!out) return 0;
    n = fread(b, 1, 4, f);
    if (n != 4) return 0;
    v = 0;
    v |= (rpyl_u32)b[0];
    v |= ((rpyl_u32)b[1]) << 8;
    v |= ((rpyl_u32)b[2]) << 16;
    v |= ((rpyl_u32)b[3]) << 24;
    *out = v;
    return 1;
}

int rpyl_bytecode_save(const RpylBytecode* bc, const char* path) {
    FILE* f;
    rpyl_u32 i;
    unsigned char m[4];

    if (!bc || !path) return 0;
    f = fopen(path, "wb");
    if (!f) return 0;

    m[0] = (unsigned char)RPYL_BC_MAGIC_0;
    m[1] = (unsigned char)RPYL_BC_MAGIC_1;
    m[2] = (unsigned char)RPYL_BC_MAGIC_2;
    m[3] = (unsigned char)RPYL_BC_MAGIC_3;
    if (fwrite(m, 1, 4, f) != 4) { fclose(f); return 0; }

    if (!write_u32(f, (rpyl_u32)bc->version)) { fclose(f); return 0; }
    if (!write_u32(f, (rpyl_u32)bc->string_count)) { fclose(f); return 0; }
    if (!write_u32(f, (rpyl_u32)bc->code_count)) { fclose(f); return 0; }
    if (!write_u32(f, (rpyl_u32)bc->label_count)) { fclose(f); return 0; }
    if (!write_u32(f, (rpyl_u32)bc->define_count)) { fclose(f); return 0; }

    for (i = 0; i < (rpyl_u32)bc->string_count; i++) {
        const char* s;
        rpyl_u32 len;
        s = bc->strings[i] ? bc->strings[i] : "";
        len = (rpyl_u32)strlen(s);
        if (!write_u32(f, len)) { fclose(f); return 0; }
        if (len > 0) {
            if (fwrite(s, 1, (size_t)len, f) != (size_t)len) { fclose(f); return 0; }
        }
    }

    for (i = 0; i < (rpyl_u32)bc->code_count; i++) {
        if (!write_u32(f, bc->code[i])) { fclose(f); return 0; }
    }

    for (i = 0; i < (rpyl_u32)bc->label_count; i++) {
        if (!write_u32(f, bc->labels[i].name_sid)) { fclose(f); return 0; }
        if (!write_u32(f, bc->labels[i].ip)) { fclose(f); return 0; }
        if (!write_u32(f, bc->labels[i].end_ip)) { fclose(f); return 0; }
    }

    for (i = 0; i < (rpyl_u32)bc->define_count; i++) {
        if (!write_u32(f, bc->defines[i].name_sid)) { fclose(f); return 0; }
        if (!write_u32(f, bc->defines[i].value_sid)) { fclose(f); return 0; }
    }

    fclose(f);
    return 1;
}

int rpyl_bytecode_load_into(RpylBytecode* bc, const char* path) {
    FILE* f;
    unsigned char m[4];
    rpyl_u32 version;
    rpyl_u32 sc;
    rpyl_u32 cc;
    rpyl_u32 lc;
    rpyl_u32 dc;
    rpyl_u32 i;

    if (!bc || !path) return 0;
    f = fopen(path, "rb");
    if (!f) return 0;

    if (fread(m, 1, 4, f) != 4) { fclose(f); return 0; }
    if (m[0] != (unsigned char)RPYL_BC_MAGIC_0 || m[1] != (unsigned char)RPYL_BC_MAGIC_1 || m[2] != (unsigned char)RPYL_BC_MAGIC_2 || m[3] != (unsigned char)RPYL_BC_MAGIC_3) {
        fclose(f);
        return 0;
    }

    if (!read_u32(f, &version)) { fclose(f); return 0; }
    if (version != (rpyl_u32)RPYL_BC_VERSION) { fclose(f); return 0; }
    if (!read_u32(f, &sc)) { fclose(f); return 0; }
    if (!read_u32(f, &cc)) { fclose(f); return 0; }
    if (!read_u32(f, &lc)) { fclose(f); return 0; }
    if (!read_u32(f, &dc)) { fclose(f); return 0; }

    if (sc > (rpyl_u32)RPYL_BC_MAX_STRINGS || cc > (rpyl_u32)RPYL_BC_MAX_CODE || lc > (rpyl_u32)RPYL_BC_MAX_LABELS || dc > (rpyl_u32)RPYL_BC_MAX_DEFINES) {
        fclose(f);
        return 0;
    }

    rpyl_bytecode_clear(bc);
    bc->version = version;

    for (i = 0; i < sc; i++) {
        rpyl_u32 len;
        char* dst;
        if (!read_u32(f, &len)) { fclose(f); rpyl_bytecode_clear(bc); return 0; }
        if ((size_t)len + 1 > ((size_t)RPYL_BC_MAX_STRING_BYTES - bc->string_bytes)) { fclose(f); rpyl_bytecode_clear(bc); return 0; }
        dst = bc->string_data + bc->string_bytes;
        if (len > 0) {
            if (fread(dst, 1, (size_t)len, f) != (size_t)len) { fclose(f); rpyl_bytecode_clear(bc); return 0; }
        }
        dst[len] = 0;
        bc->strings[bc->string_count++] = dst;
        bc->string_bytes += (size_t)len + 1;
    }

    for (i = 0; i < cc; i++) {
        rpyl_u32 w;
        if (!read_u32(f, &w)) { fclose(f); rpyl_bytecode_clear(bc); return 0; }
        bc->code[bc->code_count++] = w;
    }

    for (i = 0; i < lc; i++) {
        RpylBcLabel L;
        if (!read_u32(f, &L.name_sid)) { fclose(f); rpyl_bytecode_clear(bc); return 0; }
        if (!read_u32(f, &L.ip)) { fclose(f); rpyl_bytecode_clear(bc); return 0; }
        if (!read_u32(f, &L.end_ip)) { fclose(f); rpyl_bytecode_clear(bc); return 0; }
        bc->labels[bc->label_count++] = L;
    }

    for (i = 0; i < dc; i++) {
        RpylBcDefine D;
        if (!read_u32(f, &D.name_sid)) { fclose(f); rpyl_bytecode_clear(bc); return 0; }
        if (!read_u32(f, &D.value_sid)) { fclose(f); rpyl_bytecode_clear(bc); return 0; }
        bc->defines[bc->define_count++] = D;
    }

    fclose(f);
    return 1;
}
#else
int rpyl_bytecode_save(const RpylBytecode* bc, const char* path) {
    (void)bc;
    (void)path;
    return 0;
}

int rpyl_bytecode_load_into(RpylBytecode* bc, const char* path) {
    (void)bc;
    (void)path;
    return 0;
}
#endif
