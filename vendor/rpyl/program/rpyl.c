#include "rpyl.h"

#include "rpyl_config.h"
#include "rpyl_arena.h"
#include "rpyl_ast.h"
#include "rpyl_bytecode.h"
#include "rpyl_compiler.h"
#include "rpyl_lexer.h"
#include "rpyl_parser.h"
#include "rpyl_port.h"
#include "rpyl_runtime.h"
#include "rpyl_transpile.h"
#include "rpyl_vm.h"

#if RPYL_ENABLE_STDIO
#include <stdio.h>
#else
#ifndef EOF
#define EOF (-1)
#endif
#endif
#include <string.h>

typedef struct UserCommand {
    char name[RPYL_CTX_MAX_CMD_NAME];
    RpylCommandFn fn;
} UserCommand;

struct RpylContext {
    int active;
    int pool_owned;
    RpylRuntime* rt;
    AstNode* root;
    char start_block[RPYL_CTX_MAX_START_BLOCK];
    char label_keyword[RPYL_LEXER_MAX_LABEL_KW];
    UserCommand cmds[RPYL_CTX_MAX_USER_COMMANDS];
    int cmd_count;
    RpylVfs vfs;
    int has_vfs;
    RpylErrorInfo last_error;
    RpylArena* arena;
    RpylBytecode* bc;
    int bc_valid;
};

static RpylContext g_context_pool[RPYL_MAX_CONTEXTS];
static char g_path_load_buffer[RPYL_FILE_MAX_BYTES];

void rpyl_set_error(RpylContext* ctx, const char* module, const char* filename, int line, int column, const char* message) {
    if (!ctx) return;
    rpyl_strcpy_trunc(ctx->last_error.module, sizeof(ctx->last_error.module), module ? module : "core");
    rpyl_strcpy_trunc(ctx->last_error.filename, sizeof(ctx->last_error.filename), filename ? filename : "");
    rpyl_strcpy_trunc(ctx->last_error.message, sizeof(ctx->last_error.message), message ? message : "");
    ctx->last_error.line = line > 0 ? line : 1;
    ctx->last_error.column = column > 0 ? column : 1;
}

void rpyl_clear_error(RpylContext* ctx) {
    if (!ctx) return;
    memset(&ctx->last_error, 0, sizeof(ctx->last_error));
}

const RpylErrorInfo* rpyl_get_last_error(RpylContext* ctx) {
    if (!ctx) return (const RpylErrorInfo*)0;
    return &ctx->last_error;
}

static void context_defaults(RpylContext* ctx) {
    if (!ctx) return;
    rpyl_strcpy_trunc(ctx->start_block, sizeof(ctx->start_block), "start");
    rpyl_strcpy_trunc(ctx->label_keyword, sizeof(ctx->label_keyword), "label");
    rpyl_clear_error(ctx);
}

static RpylCommandFn find_user_command(RpylContext* ctx, const char* name) {
    int i;
    if (!ctx || !name) return (RpylCommandFn)0;
    for (i = 0; i < ctx->cmd_count; i++) {
        if (strcmp(ctx->cmds[i].name, name) == 0) return ctx->cmds[i].fn;
    }
    return (RpylCommandFn)0;
}

static void dispatch_command(RpylRuntime* rt, const char* name, const char** args, int argc) {
    RpylContext* ctx;
    RpylCommandFn fn;
    ctx = (RpylContext*)rpyl_runtime_get_userdata(rt);
    fn = find_user_command(ctx, name);
    if (!fn) {
#if RPYL_ENABLE_LOG_STDIO
        fprintf(stderr, "[Rpyl] No handler bound for command: %s\n", name ? name : "(null)");
#else
        (void)name;
#endif
        return;
    }
    fn(ctx, args, argc);
}

typedef struct RpylVfsStream {
    RpylVfs vfs;
    void* handle;
    unsigned char buf[RPYL_IO_VFS_CHUNK];
    size_t pos;
    size_t len;
    int has_push;
    int push;
} RpylVfsStream;

static int vfs_fill(RpylVfsStream* s) {
    if (!s || !s->vfs.read) return 0;
    s->pos = 0;
    s->len = s->vfs.read(s->vfs.user, s->handle, s->buf, sizeof(s->buf));
    return (s->len > 0) ? 1 : 0;
}

static int vfs_getc(void* user) {
    RpylVfsStream* s;
    s = (RpylVfsStream*)user;
    if (!s) return EOF;
    if (s->has_push) {
        s->has_push = 0;
        return s->push;
    }
    if (s->pos >= s->len) {
        if (!vfs_fill(s)) return EOF;
    }
    return (int)s->buf[s->pos++];
}

static int vfs_ungetc(int c, void* user) {
    RpylVfsStream* s;
    s = (RpylVfsStream*)user;
    if (!s) return EOF;
    if (c == EOF) return EOF;
    if (s->has_push) return EOF;
    s->has_push = 1;
    s->push = c;
    return c;
}

static RpylContext* take_context(void) {
    int i;
    for (i = 0; i < RPYL_MAX_CONTEXTS; i++) {
        if (!g_context_pool[i].active) {
            memset(&g_context_pool[i], 0, sizeof(g_context_pool[i]));
            g_context_pool[i].active = 1;
            g_context_pool[i].pool_owned = 1;
            context_defaults(&g_context_pool[i]);
            return &g_context_pool[i];
        }
    }
    return (RpylContext*)0;
}

static int context_attach_parts(RpylContext* ctx, void* arena_buffer, size_t arena_capacity) {
    if (!ctx) return 0;
    ctx->rt = rpyl_runtime_create();
    if (!ctx->rt) return 0;
    if (arena_buffer && arena_capacity > 0) ctx->arena = rpyl_arena_create_with_buffer(arena_buffer, arena_capacity);
    else ctx->arena = rpyl_arena_create(0);
    if (!ctx->arena) {
        rpyl_runtime_destroy(ctx->rt);
        ctx->rt = (RpylRuntime*)0;
        return 0;
    }
    ctx->bc = rpyl_bytecode_create();
    if (!ctx->bc) {
        rpyl_arena_destroy(ctx->arena);
        ctx->arena = (RpylArena*)0;
        rpyl_runtime_destroy(ctx->rt);
        ctx->rt = (RpylRuntime*)0;
        return 0;
    }
    rpyl_runtime_set_userdata(ctx->rt, ctx);
    return 1;
}

static RpylContext* create_context_internal(void* arena_buffer, size_t arena_capacity) {
    RpylContext* ctx;
    ctx = take_context();
    if (!ctx) return (RpylContext*)0;
    if (!context_attach_parts(ctx, arena_buffer, arena_capacity)) {
        memset(ctx, 0, sizeof(*ctx));
        return (RpylContext*)0;
    }
    return ctx;
}

RpylContext* rpyl_create(void) {
    return create_context_internal((void*)0, 0);
}

RpylContext* rpyl_create_with_arena_buffer(void* buffer, size_t capacity) {
    return create_context_internal(buffer, capacity);
}

size_t rpyl_context_size(void) {
    return sizeof(RpylContext);
}

RpylContext* rpyl_init(void* context_storage, size_t context_storage_size, void* work_memory, size_t work_memory_size) {
    RpylContext* ctx;
    if (!context_storage || context_storage_size < sizeof(RpylContext)) return (RpylContext*)0;
    ctx = (RpylContext*)context_storage;
    memset(ctx, 0, sizeof(*ctx));
    ctx->active = 1;
    ctx->pool_owned = 0;
    context_defaults(ctx);
    if (!context_attach_parts(ctx, work_memory, work_memory_size)) {
        memset(ctx, 0, sizeof(*ctx));
        return (RpylContext*)0;
    }
    return ctx;
}

void rpyl_reset(RpylContext* ctx) {
    if (!ctx) return;
    if (ctx->arena) rpyl_arena_reset(ctx->arena);
    if (ctx->bc) rpyl_bytecode_clear(ctx->bc);
    if (ctx->rt) rpyl_runtime_reset_state(ctx->rt);
    ctx->root = (AstNode*)0;
    ctx->bc_valid = 0;
    ctx->cmd_count = 0;
    ctx->has_vfs = 0;
    memset(&ctx->vfs, 0, sizeof(ctx->vfs));
    context_defaults(ctx);
}

void rpyl_destroy(RpylContext* ctx) {
    int pool_owned;
    if (!ctx) return;
    pool_owned = ctx->pool_owned;
    if (ctx->bc) rpyl_bytecode_destroy(ctx->bc);
    if (ctx->arena) rpyl_arena_destroy(ctx->arena);
    if (ctx->rt) rpyl_runtime_destroy(ctx->rt);
    memset(ctx, 0, sizeof(*ctx));
    if (!pool_owned) {
        return;
    }
}

void rpyl_set_vfs(RpylContext* ctx, const RpylVfs* vfs) {
    if (!ctx) return;
    if (!vfs || !vfs->open || !vfs->read) {
        ctx->has_vfs = 0;
        memset(&ctx->vfs, 0, sizeof(ctx->vfs));
        return;
    }
    ctx->vfs = *vfs;
    ctx->has_vfs = 1;
}

void rpyl_set_start_block(RpylContext* ctx, const char* block_name) {
    if (!ctx || !block_name || !block_name[0]) return;
    rpyl_strcpy_trunc(ctx->start_block, sizeof(ctx->start_block), block_name);
}

void rpyl_set_label_keyword(RpylContext* ctx, const char* kw) {
    if (!ctx || !kw || !kw[0]) return;
    rpyl_strcpy_trunc(ctx->label_keyword, sizeof(ctx->label_keyword), kw);
}

static void clear_loaded_program(RpylContext* ctx) {
    if (!ctx) return;
    if (ctx->arena) rpyl_arena_reset(ctx->arena);
    ctx->root = (AstNode*)0;
    if (ctx->bc) rpyl_bytecode_clear(ctx->bc);
    ctx->bc_valid = 0;
    if (ctx->rt) {
        rpyl_runtime_reset_state(ctx->rt);
        rpyl_runtime_clear_defines(ctx->rt);
    }
}

int rpyl_load_file(RpylContext* ctx, const char* path) {
    AstNode* root;
    if (!ctx || !path) return 0;
    rpyl_clear_error(ctx);
    clear_loaded_program(ctx);
    if (ctx->has_vfs) {
        RpylVfsStream vfs_s;
        RpylStream s;
        void* h;
        h = ctx->vfs.open(ctx->vfs.user, path);
        if (!h) {
#if RPYL_ENABLE_LOG_STDIO
            fprintf(stderr, "[Rpyl] VFS open failed: %s\n", path);
#endif
            rpyl_set_error(ctx, "io", path, 1, 1, "VFS open failed");
            return 0;
        }
        memset(&vfs_s, 0, sizeof(vfs_s));
        vfs_s.vfs = ctx->vfs;
        vfs_s.handle = h;
        s.user = &vfs_s;
        s.getc_fn = vfs_getc;
        s.ungetc_fn = vfs_ungetc;
        root = rpyl_parse_stream_ex_with_label(&s, ctx->arena, ctx->label_keyword);
        if (ctx->vfs.close) (void)ctx->vfs.close(ctx->vfs.user, h);
        if (!root) {
#if RPYL_ENABLE_LOG_STDIO
            fprintf(stderr, "[Rpyl] Parse failed (VFS): %s\n", path);
#endif
            rpyl_set_error(ctx, "parser", path, 1, 1, "parse failed while reading VFS stream");
            return 0;
        }
    } else {
#if RPYL_ENABLE_FILE_IO
        FILE* f;
        f = fopen(path, "rb");
        if (!f) {
#if RPYL_ENABLE_LOG_STDIO
            fprintf(stderr, "[Rpyl] Failed to open script: %s\n", path);
#endif
            rpyl_set_error(ctx, "io", path, 1, 1, "failed to open script file");
            return 0;
        }
        root = rpyl_parse_ex_with_label(f, ctx->arena, ctx->label_keyword);
        fclose(f);
        if (!root) {
#if RPYL_ENABLE_LOG_STDIO
            fprintf(stderr, "[Rpyl] Parse failed: %s\n", path);
#endif
            rpyl_set_error(ctx, "parser", path, 1, 1, "parse failed while reading file");
            return 0;
        }
#else
        rpyl_set_error(ctx, "io", path, 1, 1, "file IO disabled; use buffer or VFS loading");
        (void)path;
        return 0;
#endif
    }
    ctx->root = root;
    rpyl_runtime_reset_state(ctx->rt);
    rpyl_runtime_load_defines(ctx->rt, ctx->root);
    return 1;
}

int rpyl_load_path_buffered(RpylContext* ctx, const char* path) {
    size_t used;
    if (!ctx || !path) return 0;
    rpyl_clear_error(ctx);
    used = 0u;
    if (ctx->has_vfs) {
        void* h;
        h = ctx->vfs.open(ctx->vfs.user, path);
        if (!h) {
            rpyl_set_error(ctx, "io", path, 1, 1, "VFS open failed");
            return 0;
        }
        while (used + 1u < sizeof(g_path_load_buffer)) {
            size_t n;
            n = ctx->vfs.read(ctx->vfs.user, h, g_path_load_buffer + used, (sizeof(g_path_load_buffer) - 1u) - used);
            if (n == 0u) break;
            used += n;
        }
        if (ctx->vfs.close) (void)ctx->vfs.close(ctx->vfs.user, h);
        if (used + 1u >= sizeof(g_path_load_buffer)) {
            rpyl_set_error(ctx, "io", path, 1, 1, "script exceeds bounded load buffer");
            return 0;
        }
        g_path_load_buffer[used] = 0;
        return rpyl_load_buffer(ctx, g_path_load_buffer, used);
    }
#if RPYL_ENABLE_FILE_IO
    {
        FILE* f;
        size_t n;
        f = fopen(path, "rb");
        if (!f) {
            rpyl_set_error(ctx, "io", path, 1, 1, "failed to open script file");
            return 0;
        }
        n = fread(g_path_load_buffer, 1u, sizeof(g_path_load_buffer) - 1u, f);
        if (ferror(f)) {
            fclose(f);
            rpyl_set_error(ctx, "io", path, 1, 1, "failed while reading script file");
            return 0;
        }
        if (!feof(f)) {
            fclose(f);
            rpyl_set_error(ctx, "io", path, 1, 1, "script exceeds bounded load buffer");
            return 0;
        }
        fclose(f);
        g_path_load_buffer[n] = 0;
        return rpyl_load_buffer(ctx, g_path_load_buffer, n);
    }
#else
    rpyl_set_error(ctx, "io", path, 1, 1, "file IO disabled; install VFS or use buffer loading");
    return 0;
#endif
}

int rpyl_load_buffer(RpylContext* ctx, const char* buffer, size_t len) {
    AstNode* root;
    if (!ctx || (!buffer && len > 0)) return 0;
    rpyl_clear_error(ctx);
    if (buffer && len == 0) len = strlen(buffer);
    clear_loaded_program(ctx);
    root = rpyl_parse_buffer_ex_with_label(buffer ? buffer : "", len, ctx->arena, ctx->label_keyword);
    if (!root) {
#if RPYL_ENABLE_LOG_STDIO
        fprintf(stderr, "[Rpyl] Parse failed (buffer)\n");
#endif
        rpyl_set_error(ctx, "parser", "<buffer>", 1, 1, "parse failed while reading memory buffer");
        return 0;
    }
    ctx->root = root;
    rpyl_runtime_reset_state(ctx->rt);
    rpyl_runtime_load_defines(ctx->rt, ctx->root);
    return 1;
}

void rpyl_register_command(RpylContext* ctx, const char* name, RpylCommandFn fn) {
    int i;
    if (!ctx || !name || !fn) return;
    for (i = 0; i < ctx->cmd_count; i++) {
        if (strcmp(ctx->cmds[i].name, name) == 0) {
            ctx->cmds[i].fn = fn;
            (void)rpyl_runtime_register(ctx->rt, name, dispatch_command);
            return;
        }
    }
    if (ctx->cmd_count >= RPYL_CTX_MAX_USER_COMMANDS) {
#if RPYL_ENABLE_LOG_STDIO
        fprintf(stderr, "[Rpyl] Too many commands registered (max %d)\n", (int)RPYL_CTX_MAX_USER_COMMANDS);
#endif
        return;
    }
    rpyl_strcpy_trunc(ctx->cmds[ctx->cmd_count].name, sizeof(ctx->cmds[ctx->cmd_count].name), name);
    ctx->cmds[ctx->cmd_count].fn = fn;
    ctx->cmd_count++;
    (void)rpyl_runtime_register(ctx->rt, name, dispatch_command);
}

int rpyl_run(RpylContext* ctx, const char* entry_block) {
    const char* entry;
    int rc;
    if (!ctx || !ctx->root) return RPYL_EXEC_ERROR;
    rpyl_clear_error(ctx);
    entry = entry_block ? entry_block : ctx->start_block;
    rc = rpyl_runtime_execute(ctx->rt, ctx->root, entry);
    if (rc == RPYL_EXEC_ERROR) rpyl_set_error(ctx, "runtime", "", 1, 1, "execution failed; block may be missing or runtime returned error");
    return rc;
}

int rpyl_begin(RpylContext* ctx, const char* entry_block) {
    const char* entry;
    if (!ctx) return 0;
    entry = entry_block ? entry_block : ctx->start_block;
    if (ctx->bc_valid) return rpyl_runtime_begin_bytecode(ctx->rt, ctx->bc, entry);
    if (!ctx->root) return 0;
    return rpyl_runtime_begin_ast(ctx->rt, ctx->root, entry);
}

int rpyl_step(RpylContext* ctx) {
    if (!ctx) return RPYL_EXEC_ERROR;
    return rpyl_runtime_step(ctx->rt);
}

int rpyl_is_running(RpylContext* ctx) {
    if (!ctx) return 0;
    return rpyl_runtime_is_running(ctx->rt);
}

void rpyl_yield(RpylContext* ctx, unsigned long token) {
    if (!ctx) return;
    rpyl_runtime_request_yield(ctx->rt, token);
}

unsigned long rpyl_get_yield_token(RpylContext* ctx) {
    if (!ctx) return 0;
    return (unsigned long)rpyl_runtime_get_yield_token(ctx->rt);
}

void rpyl_signal(RpylContext* ctx, unsigned long token) {
    if (!ctx) return;
    rpyl_runtime_signal(ctx->rt, token);
}

const char* rpyl_get_define(RpylContext* ctx, const char* name) {
    if (!ctx || !name) return (const char*)0;
    return rpyl_runtime_get_define(ctx->rt, name);
}

const char* rpyl_get_var(RpylContext* ctx, const char* name) {
    if (!ctx || !name) return (const char*)0;
    return rpyl_runtime_get_var(ctx->rt, name);
}

void rpyl_set_var(RpylContext* ctx, const char* name, const char* value) {
    if (!ctx || !name) return;
    rpyl_runtime_set_var(ctx->rt, name, value ? value : "");
}

int rpyl_compile(RpylContext* ctx) {
    int ok;
    if (!ctx || !ctx->root || !ctx->bc) return 0;
    rpyl_clear_error(ctx);
    rpyl_bytecode_clear(ctx->bc);
    ok = rpyl_compiler_compile_ast(ctx->bc, ctx->root);
    if (ok) ok = rpyl_vm_validate(ctx->bc);
    ctx->bc_valid = ok ? 1 : 0;
    if (!ok) {
        rpyl_bytecode_clear(ctx->bc);
        rpyl_set_error(ctx, "compiler", "", 1, 1, "semantic or bytecode validation failed");
        return 0;
    }
    rpyl_runtime_load_defines(ctx->rt, ctx->root);
    return 1;
}

int rpyl_run_compiled(RpylContext* ctx, const char* entry_block) {
    const char* entry;
    if (!ctx || !ctx->bc || !ctx->bc_valid) return RPYL_EXEC_ERROR;
    entry = entry_block ? entry_block : ctx->start_block;
    return rpyl_runtime_execute_bytecode(ctx->rt, ctx->bc, entry);
}

const RpylBytecode* rpyl_get_bytecode(RpylContext* ctx) {
    if (!ctx || !ctx->bc_valid) return (const RpylBytecode*)0;
    return (const RpylBytecode*)ctx->bc;
}

size_t rpyl_save_bytecode_buffer(RpylContext* ctx, void* out_data, size_t out_capacity) {
    if (!ctx) return 0u;
    if (!ctx->bc_valid) {
        if (!rpyl_compile(ctx)) return 0u;
    }
    return rpyl_bytecode_save_buffer(ctx->bc, out_data, out_capacity);
}

int rpyl_load_bytecode_buffer(RpylContext* ctx, const void* data, size_t len) {
    int ok;
    if (!ctx || (!data && len > 0u) || !ctx->bc) return 0;
    rpyl_clear_error(ctx);
    rpyl_bytecode_clear(ctx->bc);
    ok = rpyl_bytecode_load_buffer(ctx->bc, data, len);
    if (ok) ok = rpyl_vm_validate(ctx->bc);
    ctx->bc_valid = ok ? 1 : 0;
    if (!ok) {
        rpyl_bytecode_clear(ctx->bc);
        rpyl_set_error(ctx, "bytecode", "", 1, 1, "invalid bytecode buffer");
        return 0;
    }
    if (ctx->arena) rpyl_arena_reset(ctx->arena);
    ctx->root = (AstNode*)0;
    rpyl_runtime_reset_state(ctx->rt);
    rpyl_runtime_load_defines_from_bytecode(ctx->rt, ctx->bc);
    return 1;
}

int rpyl_save_bytecode(RpylContext* ctx, const char* out_path) {
    if (!ctx || !out_path) return 0;
    if (!ctx->bc_valid) {
        if (!rpyl_compile(ctx)) return 0;
    }
    return rpyl_bytecode_save(ctx->bc, out_path);
}

int rpyl_load_bytecode(RpylContext* ctx, const char* in_path) {
    int ok;
    if (!ctx || !in_path || !ctx->bc) return 0;
    rpyl_clear_error(ctx);
    rpyl_bytecode_clear(ctx->bc);
    ok = rpyl_bytecode_load_into(ctx->bc, in_path);
    if (ok) ok = rpyl_vm_validate(ctx->bc);
    ctx->bc_valid = ok ? 1 : 0;
    if (!ok) {
        rpyl_bytecode_clear(ctx->bc);
        rpyl_set_error(ctx, "bytecode", in_path, 1, 1, "invalid bytecode file");
        return 0;
    }
    if (ctx->arena) rpyl_arena_reset(ctx->arena);
    ctx->root = (AstNode*)0;
    rpyl_runtime_reset_state(ctx->rt);
    rpyl_runtime_load_defines_from_bytecode(ctx->rt, ctx->bc);
    return 1;
}

int rpyl_transpile_c(RpylContext* ctx, const char* out_c_path, const char* symbol_prefix) {
    if (!ctx || !out_c_path) return 0;
    if (!ctx->bc_valid) {
        if (!rpyl_compile(ctx)) return 0;
    }
    return rpyl_transpile_bytecode_to_c(ctx->bc, out_c_path, symbol_prefix);
}

int rpyl_use_bytecode(RpylContext* ctx, const RpylBytecode* bc) {
    if (!ctx || !bc || !ctx->bc) return 0;
    rpyl_clear_error(ctx);
    if (!rpyl_vm_validate(bc) || !rpyl_bytecode_copy_into(ctx->bc, bc) || !rpyl_vm_validate(ctx->bc)) {
        rpyl_bytecode_clear(ctx->bc);
        ctx->bc_valid = 0;
        rpyl_set_error(ctx, "bytecode", "", 1, 1, "external bytecode validation failed");
        return 0;
    }
    ctx->bc_valid = 1;
    if (ctx->arena) rpyl_arena_reset(ctx->arena);
    ctx->root = (AstNode*)0;
    rpyl_runtime_reset_state(ctx->rt);
    rpyl_runtime_load_defines_from_bytecode(ctx->rt, ctx->bc);
    return 1;
}


