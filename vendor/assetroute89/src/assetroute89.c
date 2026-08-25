#include "assetroute89.h"
#include <string.h>

#define AR89_ERR_NONE 0
#define AR89_ERR_ARGUMENT 1
#define AR89_ERR_CAPACITY 2
#define AR89_ERR_PROVIDER 3

static void ar89_zero(void *p, unsigned int n)
{
    unsigned char *b;
    unsigned int i;
    b = (unsigned char *)p;
    for (i = 0U; i < n; ++i) b[i] = 0U;
}

static void ar89_copy(char *dst, ar89_u32 cap, const char *src)
{
    ar89_u32 i;
    if (!dst || cap == 0U) return;
    i = 0U;
    if (src) while (src[i] && i + 1U < cap) { dst[i] = src[i]; ++i; }
    dst[i] = '\0';
}

static int ar89_lower(int c)
{
    if (c >= 'A' && c <= 'Z') return c + ('a' - 'A');
    return c;
}

static int ar89_streq_ci(const char *a, const char *b)
{
    ar89_u32 i;
    if (!a || !b) return 0;
    i = 0U;
    while (a[i] && b[i]) {
        if (ar89_lower((unsigned char)a[i]) != ar89_lower((unsigned char)b[i])) return 0;
        ++i;
    }
    return a[i] == b[i];
}

static const char *ar89_basename(const char *path)
{
    const char *p;
    const char *last;
    if (!path) return 0;
    p = path;
    last = path;
    while (*p) {
        if (*p == '/' || *p == '\\') last = p + 1;
        ++p;
    }
    return last;
}

static const char *ar89_extension(const char *path)
{
    const char *b;
    const char *p;
    const char *dot;
    b = ar89_basename(path);
    if (!b) return 0;
    p = b;
    dot = 0;
    while (*p) { if (*p == '.') dot = p; ++p; }
    return dot;
}

static int ar89_ext_is(const char *path, const char *ext)
{
    const char *p;
    p = ar89_extension(path);
    return p && ar89_streq_ci(p, ext);
}

int ar89_is_supported_image_path(const char *path)
{
    return ar89_ext_is(path,".png") || ar89_ext_is(path,".apng") ||
           ar89_ext_is(path,".jpg") || ar89_ext_is(path,".jpeg") ||
           ar89_ext_is(path,".bmp") || ar89_ext_is(path,".tga") ||
           ar89_ext_is(path,".gif") || ar89_ext_is(path,".webp") ||
           ar89_ext_is(path,".tif") || ar89_ext_is(path,".tiff") ||
           ar89_ext_is(path,".dds") || ar89_ext_is(path,".pcx") ||
           ar89_ext_is(path,".psd") || ar89_ext_is(path,".qoi");
}

int ar89_is_supported_audio_path(const char *path)
{
    return ar89_ext_is(path,".wav") || ar89_ext_is(path,".mp2") ||
           ar89_ext_is(path,".mp3") || ar89_ext_is(path,".ogg") ||
           ar89_ext_is(path,".opus") || ar89_ext_is(path,".flac") ||
           ar89_ext_is(path,".mid") || ar89_ext_is(path,".midi");
}

int ar89_is_valid_audio_alias(const char *name)
{
    ar89_u32 i;
    int c;
    if (!name || !name[0]) return 0;
    c = (unsigned char)name[0];
    if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_')) return 0;
    i = 1U;
    while (name[i]) {
        c = (unsigned char)name[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
              (c >= '0' && c <= '9') || c == '_')) return 0;
        ++i;
    }
    return 1;
}

int ar89_make_renpy_name(int kind, const char *path, char *out_name, ar89_u32 out_cap)
{
    const char *b;
    const char *dot;
    ar89_u32 n;
    if (!path || !out_name || out_cap == 0U) return 0;
    b = ar89_basename(path);
    if (!b || !b[0]) return 0;
    dot = ar89_extension(b);
    n = 0U;
    while (b[n] && (!dot || b + n < dot) && n + 1U < out_cap) {
        out_name[n] = (char)ar89_lower((unsigned char)b[n]);
        ++n;
    }
    out_name[n] = '\0';
    if (!out_name[0]) return 0;
    if (kind == AR89_KIND_AUDIO && !ar89_is_valid_audio_alias(out_name)) return 0;
    return 1;
}

void ar89_init(AssetRoute89 *ctx)
{
    if (!ctx) return;
    ar89_zero(ctx, (unsigned int)sizeof(*ctx));
}

void ar89_set_file_provider(AssetRoute89 *ctx, const AR89_FileProvider *provider)
{
    if (!ctx) return;
    if (provider) ctx->files = *provider;
    else ar89_zero(&ctx->files, (unsigned int)sizeof(ctx->files));
}

int ar89_add_root(AssetRoute89 *ctx, int kind, const char *path, int recursive)
{
    AR89_Root *r;
    if (!ctx || !path || !path[0]) return 0;
    if (ctx->root_count >= AR89_MAX_ROOTS) { ctx->last_error = AR89_ERR_CAPACITY; return 0; }
    r = &ctx->roots[ctx->root_count++];
    ar89_zero(r, (unsigned int)sizeof(*r));
    ar89_copy(r->path, AR89_PATH_CAP, path);
    r->kind = (unsigned char)kind;
    r->recursive = recursive ? 1U : 0U;
    r->used = 1U;
    return 1;
}

static int ar89_find(const AssetRoute89 *ctx, int kind, const char *name)
{
    ar89_id i;
    if (!ctx || !name) return -1;
    for (i = 0U; i < ctx->entry_count; ++i) {
        if (ctx->entries[i].used && (kind == AR89_KIND_ANY || ctx->entries[i].kind == kind) &&
            strcmp(ctx->entries[i].name, name) == 0) return (int)i;
    }
    return -1;
}

static int ar89_put(AssetRoute89 *ctx, int kind, const char *name, const char *path, int origin)
{
    int found;
    AR89_Entry *e;
    if (!ctx || !name || !path) return 0;
    found = ar89_find(ctx, kind, name);
    if (found >= 0) {
        e = &ctx->entries[found];
        if (e->origin == AR89_ORIGIN_EXPLICIT && origin == AR89_ORIGIN_AUTO) return 1;
        if (origin == AR89_ORIGIN_EXPLICIT || strcmp(path, e->path) < 0) {
            ar89_copy(e->path, AR89_PATH_CAP, path);
            e->origin = (unsigned char)origin;
        }
        return 1;
    }
    if (ctx->entry_count >= AR89_MAX_ENTRIES) { ctx->last_error = AR89_ERR_CAPACITY; return 0; }
    e = &ctx->entries[ctx->entry_count++];
    ar89_zero(e, (unsigned int)sizeof(*e));
    ar89_copy(e->name, AR89_NAME_CAP, name);
    ar89_copy(e->path, AR89_PATH_CAP, path);
    e->kind = (unsigned char)kind;
    e->origin = (unsigned char)origin;
    e->used = 1U;
    return 1;
}

int ar89_register_explicit(AssetRoute89 *ctx, int kind, const char *name, const char *path)
{
    char lowered[AR89_NAME_CAP];
    ar89_u32 i;
    if (!ctx || !name || !path) return 0;
    i = 0U;
    while (name[i] && i + 1U < AR89_NAME_CAP) {
        lowered[i] = (char)ar89_lower((unsigned char)name[i]);
        ++i;
    }
    lowered[i] = '\0';
    if (!lowered[0]) return 0;
    return ar89_put(ctx, kind, lowered, path, AR89_ORIGIN_EXPLICIT);
}

int ar89_discover(AssetRoute89 *ctx, int kind, const char *path)
{
    char name[AR89_NAME_CAP];
    if (!ctx || !path) return 0;
    if (kind == AR89_KIND_IMAGE && !ar89_is_supported_image_path(path)) return 1;
    if (kind == AR89_KIND_AUDIO && !ar89_is_supported_audio_path(path)) return 1;
    if (!ar89_make_renpy_name(kind, path, name, AR89_NAME_CAP)) return 1;
    return ar89_put(ctx, kind, name, path, AR89_ORIGIN_AUTO);
}

typedef struct AR89_ScanState_s { AssetRoute89 *ctx; int kind; } AR89_ScanState;
static int ar89_scan_sink(void *user, const char *path)
{
    AR89_ScanState *s;
    s = (AR89_ScanState *)user;
    return ar89_discover(s->ctx, s->kind, path);
}

int ar89_scan_roots(AssetRoute89 *ctx)
{
    ar89_id i;
    AR89_ScanState state;
    int r;
    if (!ctx || !ctx->files.enumerate) return 0;
    for (i = 0U; i < ctx->root_count; ++i) {
        if (!ctx->roots[i].used) continue;
        state.ctx = ctx;
        state.kind = ctx->roots[i].kind;
        r = ctx->files.enumerate(ctx->files.user, ctx->roots[i].path,
                                 ctx->roots[i].recursive, ar89_scan_sink, &state);
        if (r == AR89_PROVIDER_ERROR) { ctx->last_error = AR89_ERR_PROVIDER; return 0; }
    }
    return 1;
}

int ar89_resolve_name(const AssetRoute89 *ctx, int kind, const char *name,
                      char *out_path, ar89_u32 out_cap)
{
    char lowered[AR89_NAME_CAP];
    ar89_u32 i;
    int found;
    if (!ctx || !name || !out_path || out_cap == 0U) return 0;
    i = 0U;
    while (name[i] && i + 1U < AR89_NAME_CAP) { lowered[i] = (char)ar89_lower((unsigned char)name[i]); ++i; }
    lowered[i] = '\0';
    found = ar89_find(ctx, kind, lowered);
    if (found < 0) return 0;
    ar89_copy(out_path, out_cap, ctx->entries[found].path);
    return 1;
}

static int ar89_has_sep_or_ext(const char *s)
{
    ar89_u32 i;
    if (!s) return 0;
    for (i = 0U; s[i]; ++i) if (s[i]=='/' || s[i]=='\\' || s[i]=='.') return 1;
    return 0;
}

static int ar89_join(char *out, ar89_u32 cap, const char *a, const char *b)
{
    ar89_u32 n;
    ar89_u32 i;
    if (!out || cap == 0U || !a || !b) return 0;
    n = 0U;
    for (i = 0U; a[i] && n + 1U < cap; ++i) out[n++] = a[i];
    if (n && out[n-1U] != '/' && out[n-1U] != '\\' && n + 1U < cap) out[n++] = '/';
    for (i = 0U; b[i] && n + 1U < cap; ++i) out[n++] = b[i];
    out[n] = '\0';
    return b[i] == '\0';
}

int ar89_resolve_request(const AssetRoute89 *ctx, int kind, const char *request,
                         char *out_path, ar89_u32 out_cap)
{
    ar89_id i;
    char candidate[AR89_PATH_CAP];
    if (!ctx || !request || !out_path || out_cap == 0U) return 0;
    if (!ar89_has_sep_or_ext(request)) {
        if (ar89_resolve_name(ctx, kind, request, out_path, out_cap)) return 1;
    }
    if (ctx->files.exists && ctx->files.exists(ctx->files.user, request) == AR89_PROVIDER_FOUND) {
        ar89_copy(out_path, out_cap, request);
        return 1;
    }
    for (i = 0U; i < ctx->root_count; ++i) {
        if (!ctx->roots[i].used || (kind != AR89_KIND_ANY && ctx->roots[i].kind != kind)) continue;
        if (!ar89_join(candidate, AR89_PATH_CAP, ctx->roots[i].path, request)) continue;
        if (ctx->files.exists && ctx->files.exists(ctx->files.user, candidate) == AR89_PROVIDER_FOUND) {
            ar89_copy(out_path, out_cap, candidate);
            return 1;
        }
    }
    return 0;
}
