#include "bvh_runtime.h"

int bvh_run_string(BVH_Context *ctx, const char *src, const BVH_RuntimeCallbacks *callbacks, void *user, BVH_Error *err) {
    if (!bvh_parse_string(ctx, src, err)) return 0;
    if (!bvh_compile_program(ctx, err)) return 0;
    return bvh_exec_bytecode(&ctx->bytecode, callbacks, user, err);
}

int bvh_run_file(BVH_Context *ctx, const char *path, const BVH_RuntimeCallbacks *callbacks, void *user, BVH_Error *err) {
    if (!bvh_parse_file(ctx, path, err)) return 0;
    if (!bvh_compile_program(ctx, err)) return 0;
    return bvh_exec_bytecode(&ctx->bytecode, callbacks, user, err);
}
