#include "bvh_transpile.h"

static void bvh_indent(FILE *out, int pretty, int depth) {
    int i;
    if (!pretty) return;
    for (i = 0; i < depth; i++) fputs("  ", out);
}

static void bvh_nl(FILE *out, int pretty) {
    if (pretty) fputc('\n', out);
}

static void bvh_json_escape(FILE *out, const char *s) {
    const unsigned char *p;
    unsigned char c;
    p = (const unsigned char *)(s ? s : "");
    while (*p) {
        c = *p;
        if (c == '\\' || c == '"') {
            fputc('\\', out);
            fputc((int)c, out);
        } else if (c == '\n') {
            fputs("\\n", out);
        } else if (c == '\r') {
            fputs("\\r", out);
        } else if (c == '\t') {
            fputs("\\t", out);
        } else {
            fputc((int)c, out);
        }
        p++;
    }
}

static void bvh_json_str(FILE *out, const char *s) {
    fputc('"', out);
    bvh_json_escape(out, s);
    fputc('"', out);
}

static void bvh_emit_json_value(const BVH_Context *ctx, FILE *out, int pretty, int depth, const BVH_Value *value) {
    char text[BVH_VALUE_TEXT_MAX];
    BVH_UNUSED(pretty);
    BVH_UNUSED(depth);
    bvh_value_format(ctx, value, text, (unsigned long)sizeof(text));
    fputs("{", out);
    fputs("\"kind\":", out);
    bvh_json_str(out, bvh_value_kind_name(value->kind));
    fputs(",\"text\":", out);
    bvh_json_str(out, text);
    switch (value->kind) {
        case BVH_VALUE_INT:
            fprintf(out, ",\"i\":%ld", value->a);
            break;
        case BVH_VALUE_FIXED:
            fprintf(out, ",\"q16\":%ld", value->a);
            break;
        case BVH_VALUE_VEC2I:
            fprintf(out, ",\"x\":%ld,\"y\":%ld", value->a, value->b);
            break;
        case BVH_VALUE_COLOR:
            fprintf(out, ",\"rgba\":%lu", value->u);
            break;
        case BVH_VALUE_ANCHOR_POS:
            fputs(",\"anchor\":", out);
            bvh_json_str(out, bvh_anchor_name((int)value->a));
            fprintf(out, ",\"x\":%ld,\"y\":%ld", value->b, value->c);
            break;
        case BVH_VALUE_RANGE:
            fprintf(out, ",\"from\":%ld,\"to\":%ld", value->a, value->b);
            break;
        default:
            break;
    }
    fputs("}", out);
}

static void bvh_emit_props(const BVH_Context *ctx, FILE *out, int pretty, int depth, int first_prop) {
    int p;
    int first;
    p = first_prop;
    first = 1;
    fputs("[", out);
    if (pretty && p != BVH_NIL) fputc('\n', out);
    while (p != BVH_NIL) {
        if (!first) {
            fputc(',', out);
            bvh_nl(out, pretty);
        }
        bvh_indent(out, pretty, depth + 1);
        fputs("{", out);
        fputs("\"key\":", out);
        bvh_json_str(out, bvh_symbol_text(ctx, ctx->props[p].key_sym));
        fputs(",\"raw\":", out);
        bvh_json_str(out, bvh_symbol_text(ctx, ctx->props[p].raw_sym));
        fputs(",\"value\":", out);
        bvh_emit_json_value(ctx, out, pretty, depth + 1, &ctx->props[p].value);
        fputs("}", out);
        first = 0;
        p = ctx->props[p].next;
    }
    if (pretty && !first) {
        fputc('\n', out);
        bvh_indent(out, pretty, depth);
    }
    fputs("]", out);
}

static void bvh_emit_item(const BVH_Context *ctx, FILE *out, int pretty, int depth, int idx) {
    const BVH_AstItem *item;
    int ch;
    int first;

    item = &ctx->items[idx];
    fputs("{", out);
    bvh_nl(out, pretty);

    bvh_indent(out, pretty, depth + 1);
    fputs("\"kind\":", out);
    bvh_json_str(out, bvh_entity_kind_name((int)item->kind));
    fputc(',', out);
    bvh_nl(out, pretty);

    bvh_indent(out, pretty, depth + 1);
    fputs("\"name\":", out);
    bvh_json_str(out, bvh_symbol_text(ctx, item->name_sym));
    fputc(',', out);
    bvh_nl(out, pretty);

    bvh_indent(out, pretty, depth + 1);
    fputs("\"props\":", out);
    if (pretty) fputc(' ', out);
    bvh_emit_props(ctx, out, pretty, depth + 1, item->first_prop);
    fputc(',', out);
    bvh_nl(out, pretty);

    bvh_indent(out, pretty, depth + 1);
    fputs("\"children\":", out);
    if (pretty) fputc(' ', out);
    fputs("[", out);
    ch = item->first_child;
    first = 1;
    if (pretty && ch != BVH_NIL) fputc('\n', out);
    while (ch != BVH_NIL) {
        if (!first) {
            fputc(',', out);
            bvh_nl(out, pretty);
        }
        bvh_indent(out, pretty, depth + 2);
        bvh_emit_item(ctx, out, pretty, depth + 2, ch);
        first = 0;
        ch = ctx->items[ch].next;
    }
    if (pretty && !first) {
        fputc('\n', out);
        bvh_indent(out, pretty, depth + 1);
    }
    fputs("]", out);

    bvh_nl(out, pretty);
    bvh_indent(out, pretty, depth);
    fputs("}", out);
}

int bvh_transpile_json(const BVH_Context *ctx, FILE *out, int pretty, BVH_Error *err) {
    int idx;
    int first;
    if (err) {
        err->line = 0;
        err->col = 0;
        err->message[0] = '\0';
    }
    if (!ctx || !out) {
        bvh_error_set(err, 0, 0, "invalid transpile argument");
        return 0;
    }
    fputs("{", out);
    bvh_nl(out, pretty);
    bvh_indent(out, pretty, 1);
    fputs("\"version\":", out);
    fprintf(out, "%d", BVH_VERSION_MAJOR);
    fputc(',', out);
    bvh_nl(out, pretty);
    bvh_indent(out, pretty, 1);
    fputs("\"items\":", out);
    if (pretty) fputc(' ', out);
    fputs("[", out);
    idx = ctx->program.first_item;
    first = 1;
    if (pretty && idx != BVH_NIL) fputc('\n', out);
    while (idx != BVH_NIL) {
        if (!first) {
            fputc(',', out);
            bvh_nl(out, pretty);
        }
        bvh_indent(out, pretty, 2);
        bvh_emit_item(ctx, out, pretty, 2, idx);
        first = 0;
        idx = ctx->items[idx].next;
    }
    if (pretty && !first) {
        fputc('\n', out);
        bvh_indent(out, pretty, 1);
    }
    fputs("]", out);
    bvh_nl(out, pretty);
    fputs("}\n", out);
    return 1;
}
