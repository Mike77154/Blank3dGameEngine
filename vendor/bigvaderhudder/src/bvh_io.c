#include "bvh_parser.h"

static int bvh_copy_source(BVH_Context *ctx, const char *src, BVH_Error *err) {
    unsigned long n;
    if (!ctx) return 0;
    if (!src) src = "";
    n = bvh_cstr_len(src);
    if (n > BVH_MAX_SOURCE_SIZE) {
        bvh_error_set(err, 0, 0, "source buffer limit reached");
        return 0;
    }
    if (n) memcpy(ctx->source, src, (size_t)n);
    ctx->source[n] = '\0';
    ctx->source_len = n;
    return 1;
}

static int bvh_read_source_file(BVH_Context *ctx, const char *path, BVH_Error *err) {
    FILE *f;
    size_t got;
    unsigned long pos;

    if (!ctx || !path) {
        bvh_error_set(err, 0, 0, "invalid file argument");
        return 0;
    }

    f = fopen(path, "rb");
    if (!f) {
        bvh_error_set(err, 0, 0, "could not open input file");
        return 0;
    }

    pos = 0UL;
    for (;;) {
        if (pos >= BVH_MAX_SOURCE_SIZE) {
            fclose(f);
            bvh_error_set(err, 0, 0, "input file exceeds source buffer");
            return 0;
        }
        got = fread(ctx->source + pos, 1, (size_t)(BVH_MAX_SOURCE_SIZE - pos), f);
        pos += (unsigned long)got;
        if (got == 0U) break;
    }

    if (ferror(f)) {
        fclose(f);
        bvh_error_set(err, 0, 0, "could not read input file");
        return 0;
    }
    fclose(f);
    ctx->source[pos] = '\0';
    ctx->source_len = pos;
    return 1;
}

int bvh_parse_string(BVH_Context *ctx, const char *src, BVH_Error *err) {
    if (!ctx) {
        bvh_error_set(err, 0, 0, "null context");
        return 0;
    }
    bvh_context_init(ctx);
    if (!bvh_copy_source(ctx, src, err)) return 0;
    return bvh_parse_loaded(ctx, err);
}

int bvh_parse_file(BVH_Context *ctx, const char *path, BVH_Error *err) {
    if (!ctx) {
        bvh_error_set(err, 0, 0, "null context");
        return 0;
    }
    bvh_context_init(ctx);
    if (!bvh_read_source_file(ctx, path, err)) return 0;
    return bvh_parse_loaded(ctx, err);
}
