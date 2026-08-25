#ifndef ASSETROUTE89_H
#define ASSETROUTE89_H

#include <limits.h>
#if UINT_MAX != 0xFFFFFFFFU
#error AssetRoute89 requires a 32-bit unsigned int target
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define AR89_VERSION_MAJOR 0
#define AR89_VERSION_MINOR 1
#define AR89_VERSION_PATCH 0

#ifndef AR89_MAX_ENTRIES
#define AR89_MAX_ENTRIES 1024
#endif
#ifndef AR89_MAX_ROOTS
#define AR89_MAX_ROOTS 16
#endif
#ifndef AR89_NAME_CAP
#define AR89_NAME_CAP 96
#endif
#ifndef AR89_PATH_CAP
#define AR89_PATH_CAP 320
#endif

#define AR89_KIND_ANY   0
#define AR89_KIND_IMAGE 1
#define AR89_KIND_AUDIO 2
#define AR89_KIND_DATA  3

#define AR89_ORIGIN_AUTO     0
#define AR89_ORIGIN_EXPLICIT 1

#define AR89_PROVIDER_ERROR   (-1)
#define AR89_PROVIDER_MISSING 0
#define AR89_PROVIDER_FOUND   1

typedef unsigned int ar89_u32;
typedef unsigned short ar89_id;

typedef int (*ar89_exists_fn)(void *user, const char *path);
typedef int (*ar89_enum_sink_fn)(void *sink_user, const char *path);
typedef int (*ar89_enumerate_fn)(void *user, const char *root, int recursive,
                                 ar89_enum_sink_fn sink, void *sink_user);

typedef struct AR89_FileProvider_s {
    ar89_exists_fn exists;
    ar89_enumerate_fn enumerate;
    void *user;
} AR89_FileProvider;

typedef struct AR89_Entry_s {
    char name[AR89_NAME_CAP];
    char path[AR89_PATH_CAP];
    unsigned char kind;
    unsigned char origin;
    unsigned char used;
} AR89_Entry;

typedef struct AR89_Root_s {
    char path[AR89_PATH_CAP];
    unsigned char kind;
    unsigned char recursive;
    unsigned char used;
} AR89_Root;

typedef struct AssetRoute89_s {
    AR89_Entry entries[AR89_MAX_ENTRIES];
    AR89_Root roots[AR89_MAX_ROOTS];
    ar89_id entry_count;
    ar89_id root_count;
    AR89_FileProvider files;
    int last_error;
} AssetRoute89;

void ar89_init(AssetRoute89 *ctx);
void ar89_set_file_provider(AssetRoute89 *ctx, const AR89_FileProvider *provider);
int ar89_add_root(AssetRoute89 *ctx, int kind, const char *path, int recursive);
int ar89_register_explicit(AssetRoute89 *ctx, int kind, const char *name, const char *path);
int ar89_discover(AssetRoute89 *ctx, int kind, const char *path);
int ar89_scan_roots(AssetRoute89 *ctx);
int ar89_resolve_name(const AssetRoute89 *ctx, int kind, const char *name,
                      char *out_path, ar89_u32 out_cap);
int ar89_resolve_request(const AssetRoute89 *ctx, int kind, const char *request,
                         char *out_path, ar89_u32 out_cap);
int ar89_make_renpy_name(int kind, const char *path, char *out_name, ar89_u32 out_cap);
int ar89_is_supported_image_path(const char *path);
int ar89_is_supported_audio_path(const char *path);
int ar89_is_valid_audio_alias(const char *name);

#ifdef __cplusplus
}
#endif
#endif
