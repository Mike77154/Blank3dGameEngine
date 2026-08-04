#ifndef RPYL_H
#define RPYL_H

#include <stddef.h> /* size_t */

#include "rpyl_alloc.h" /* bounded memory helpers */
#include "rpyl_fixed.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RpylContext RpylContext;

typedef struct RpylErrorInfo {
    char filename[128];
    char module[32];
    char message[160];
    int line;
    int column;
} RpylErrorInfo;

/* Optional compiled form (forward declared). */
struct RpylBytecode;

typedef void (*RpylCommandFn)(RpylContext* ctx, const char** args, int argc);

/* -----------------------------
   Virtual file system (optional)
   ----------------------------- */

typedef void*  (*RpylVfsOpenFn)(void* user, const char* path);
typedef size_t (*RpylVfsReadFn)(void* user, void* handle, void* dst, size_t bytes);
typedef int    (*RpylVfsCloseFn)(void* user, void* handle);

typedef struct RpylVfs {
    void* user;
    RpylVfsOpenFn open;
    RpylVfsReadFn read;
    RpylVfsCloseFn close;
} RpylVfs;

/* Install a VFS for rpyl_load_file(). If NULL, stdio fopen/fread is used. */
void rpyl_set_vfs(RpylContext* ctx, const RpylVfs* vfs);

/* -----------------------------
   Lifecycle
   ----------------------------- */

RpylContext* rpyl_create(void);

/* Create context whose AST arena uses an external caller-owned buffer. */
RpylContext* rpyl_create_with_arena_buffer(void* buffer, size_t capacity);

/* Placement-style initialization. context_storage must be at least
   rpyl_context_size() bytes. work_memory is used as the AST/work arena. */
size_t rpyl_context_size(void);
RpylContext* rpyl_init(void* context_storage, size_t context_storage_size, void* work_memory, size_t work_memory_size);
void rpyl_reset(RpylContext* ctx);

void rpyl_destroy(RpylContext* ctx);

/* -----------------------------
   Script loading
   ----------------------------- */

int rpyl_load_file(RpylContext* ctx, const char* path);
/* Preferred host/embedded path loader: read through VFS/file into a bounded buffer,
   then parse with rpyl_load_buffer(). This keeps reloadable asset packs and
   engine VFS flows on the same buffer route. */
int rpyl_load_path_buffered(RpylContext* ctx, const char* path);
int rpyl_load_buffer(RpylContext* ctx, const char* buffer, size_t len);

/* -----------------------------
   DSL config
   ----------------------------- */

void rpyl_set_label_keyword(RpylContext* ctx, const char* kw);
void rpyl_set_start_block(RpylContext* ctx, const char* block_name);

/* -----------------------------
   Commands
   ----------------------------- */

void rpyl_register_command(
    RpylContext* ctx,
    const char* name,
    RpylCommandFn fn
);

/* -----------------------------
   Execution (run-to-completion)
   ----------------------------- */

/* Return codes for rpyl_run / rpyl_run_compiled:
   - RPYL_EXEC_ERROR   (0)
   - RPYL_EXEC_DONE    (1)
   - RPYL_EXEC_YIELDED (2)
*/
#define RPYL_EXEC_ERROR   0
#define RPYL_EXEC_DONE    1
#define RPYL_EXEC_YIELDED 2

/* Execute a block (AST interpreter). */
int rpyl_run(RpylContext* ctx, const char* entry_block);

/* -----------------------------
   Coroutine-style execution
   ----------------------------- */

/* Begin execution of a block and keep state for stepping.
   If bytecode is available, stepping uses bytecode VM; otherwise AST.
   Returns 1 on success, 0 on failure.
*/
int rpyl_begin(RpylContext* ctx, const char* entry_block);

/* Execute until the script either finishes or yields.
   Returns one of RPYL_EXEC_*.
*/
int rpyl_step(RpylContext* ctx);

/* Query whether a coroutine is currently active (begun but not finished). */
int rpyl_is_running(RpylContext* ctx);

/* Generic yield requested from inside a command handler.
   - token is an engine-defined identifier.
   - On the next step boundary, rpyl_step() will return RPYL_EXEC_YIELDED.
*/
void rpyl_yield(RpylContext* ctx, unsigned long token);

/* If currently yielded, returns the yield token; otherwise 0. */
unsigned long rpyl_get_yield_token(RpylContext* ctx);

/* Signal that a token is ready; if it matches the current yield token,
   the next rpyl_step() will resume execution.
*/
void rpyl_signal(RpylContext* ctx, unsigned long token);

/* -----------------------------
   Utilities
   ----------------------------- */

const RpylErrorInfo* rpyl_get_last_error(RpylContext* ctx);
void rpyl_clear_error(RpylContext* ctx);

/* Host/module diagnostic hook. This lets extension modules report failures
   through the same error object returned by rpyl_get_last_error(). */
void rpyl_set_error(
    RpylContext* ctx,
    const char* module,
    const char* filename,
    int line,
    int column,
    const char* message
);

const char* rpyl_get_define(RpylContext* ctx, const char* name);

/* Variables */
const char* rpyl_get_var(RpylContext* ctx, const char* name);
void rpyl_set_var(RpylContext* ctx, const char* name, const char* value);

/* ================================ */
/* Bytecode (optional improvements) */
/* ================================ */

/* Compile the currently loaded script (AST) into bytecode.
   Returns non-zero on success. */
int rpyl_compile(RpylContext* ctx);

/* Execute the compiled bytecode (if available). If not compiled, returns 0.
   entry_block behaves like rpyl_run(). */
int rpyl_run_compiled(RpylContext* ctx, const char* entry_block);

/* Get compiled bytecode pointer (owned by ctx). */
const struct RpylBytecode* rpyl_get_bytecode(RpylContext* ctx);

/* Save/load bytecode to/from caller-owned memory.
   rpyl_save_bytecode_buffer(ctx, NULL, 0) returns the required byte count. */
size_t rpyl_save_bytecode_buffer(RpylContext* ctx, void* out_data, size_t out_capacity);
int rpyl_load_bytecode_buffer(RpylContext* ctx, const void* data, size_t len);

/* Save/load bytecode to/from a .rpb file. Host/toolchain convenience. */
int rpyl_save_bytecode(RpylContext* ctx, const char* out_path);
int rpyl_load_bytecode(RpylContext* ctx, const char* in_path);

/* Transpile compiled bytecode into a C89 source file embedding the program.
   symbol_prefix is used to name exported functions/objects. */
int rpyl_transpile_c(RpylContext* ctx, const char* out_c_path, const char* symbol_prefix);

/* Clone/install externally provided bytecode into ctx (useful for embedded/transpiled builds). */
int rpyl_use_bytecode(RpylContext* ctx, const struct RpylBytecode* bc);

#ifdef __cplusplus
}
#endif

#endif
