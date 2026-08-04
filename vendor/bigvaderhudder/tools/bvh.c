#include <stdio.h>
#include <string.h>

#include "../include/bvh.h"

static BVH_Context g_ctx;

static void usage(const char *argv0) {
    fprintf(stderr,
        "BigVaderHudder CLI\n"
        "\n"
        "Usage:\n"
        "  %s run <file.bhud>\n"
        "  %s compile <file.bhud> -o <out.bvbc>\n"
        "  %s transpile <file.bhud> -o <out.json> [--pretty]\n"
        "  %s tokens <file.bhud>\n"
        "  %s ast <file.bhud>\n"
        "  %s ir <file.bhud>\n"
        "  %s symbols <file.bhud>\n",
        argv0, argv0, argv0, argv0, argv0, argv0, argv0);
}

static int write_file_bin(const char *path, const unsigned char *data, unsigned long len) {
    FILE *f;
    unsigned long wr;
    f = fopen(path, "wb");
    if (!f) return 0;
    wr = (unsigned long)fwrite(data, 1, (size_t)len, f);
    fclose(f);
    return wr == len ? 1 : 0;
}

static const char *find_out_arg(int argc, char **argv, int start) {
    int i;
    for (i = start; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) return argv[i + 1];
    }
    return NULL;
}

static int has_arg(int argc, char **argv, const char *needle) {
    int i;
    for (i = 0; i < argc; i++) {
        if (strcmp(argv[i], needle) == 0) return 1;
    }
    return 0;
}

static int parse_or_report(const char *path, BVH_Error *err) {
    if (!bvh_parse_file(&g_ctx, path, err)) {
        fprintf(stderr, "Parse error: %s (line %d, col %d)\n", err->message, err->line, err->col);
        return 0;
    }
    return 1;
}

int main(int argc, char **argv) {
    const char *cmd;
    const char *inpath;
    const char *outpath;
    BVH_Error err;
    FILE *out;
    int pretty;

    if (argc < 3) {
        usage(argv[0]);
        return 2;
    }

    cmd = argv[1];
    inpath = argv[2];
    bvh_context_init(&g_ctx);

    if (strcmp(cmd, "tokens") == 0 || strcmp(cmd, "token") == 0) {
        if (!parse_or_report(inpath, &err)) return 1;
        bvh_dump_tokens(&g_ctx, stdout);
        return 0;
    }

    if (strcmp(cmd, "ast") == 0) {
        if (!parse_or_report(inpath, &err)) return 1;
        bvh_dump_ast(&g_ctx, stdout);
        return 0;
    }

    if (strcmp(cmd, "symbols") == 0) {
        if (!parse_or_report(inpath, &err)) return 1;
        bvh_dump_symbols(&g_ctx, stdout);
        return 0;
    }

    if (strcmp(cmd, "ir") == 0) {
        if (!parse_or_report(inpath, &err)) return 1;
        if (!bvh_build_ir(&g_ctx, &err)) {
            fprintf(stderr, "IR error: %s (line %d, col %d)\n", err.message, err.line, err.col);
            return 1;
        }
        bvh_dump_ir(&g_ctx, stdout);
        return 0;
    }

    if (strcmp(cmd, "run") == 0) {
        if (!bvh_run_file(&g_ctx, inpath, NULL, stdout, &err)) {
            fprintf(stderr, "Run error: %s (line %d, col %d)\n", err.message, err.line, err.col);
            return 1;
        }
        return 0;
    }

    if (strcmp(cmd, "compile") == 0) {
        outpath = find_out_arg(argc, argv, 3);
        if (!outpath) {
            fprintf(stderr, "missing -o <out.bvbc>\n");
            usage(argv[0]);
            return 2;
        }
        if (!parse_or_report(inpath, &err)) return 1;
        if (!bvh_compile_program(&g_ctx, &err)) {
            fprintf(stderr, "Compile error: %s (line %d, col %d)\n", err.message, err.line, err.col);
            return 1;
        }
        if (!write_file_bin(outpath, g_ctx.bytecode.data, g_ctx.bytecode.len)) {
            fprintf(stderr, "Could not write %s\n", outpath);
            return 1;
        }
        return 0;
    }

    if (strcmp(cmd, "transpile") == 0) {
        outpath = find_out_arg(argc, argv, 3);
        pretty = has_arg(argc, argv, "--pretty");
        if (!outpath) {
            fprintf(stderr, "missing -o <out.json>\n");
            usage(argv[0]);
            return 2;
        }
        if (!parse_or_report(inpath, &err)) return 1;
        out = fopen(outpath, "wb");
        if (!out) {
            fprintf(stderr, "Could not open %s\n", outpath);
            return 1;
        }
        if (!bvh_transpile_json(&g_ctx, out, pretty, &err)) {
            fclose(out);
            fprintf(stderr, "Transpile error: %s (line %d, col %d)\n", err.message, err.line, err.col);
            return 1;
        }
        fclose(out);
        return 0;
    }

    fprintf(stderr, "Unknown command: %s\n", cmd);
    usage(argv[0]);
    return 2;
}
