#include "blank3d_sprite_runtime89.h"
#include "spriteasset89_renlist89.h"
#include "spriteverbs89_aseprite.h"
#include "assetroute89_posix.h"
#include "assetroute89_win32.h"

#include <stdio.h>
#include <string.h>

static char b3d_sprite89_import_buffer[B3D_SPRITE89_IMPORT_CAP];

static void b3d_sprite89_copy(char *dst, unsigned int cap, const char *src)
{
    unsigned int i;
    if (!dst || cap == 0U) return;
    if (!src) src = "";
    i = 0U;
    while (src[i] && i + 1U < cap) {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
}

static void b3d_sprite89_status(Blank3DSpriteRuntime89 *runtime,
                                const char *text)
{
    if (!runtime) return;
    b3d_sprite89_copy(runtime->status,
                      (unsigned int)sizeof(runtime->status), text);
}

static int b3d_sprite89_read_file(const char *path,
                                  char *buffer, unsigned int cap,
                                  unsigned int *out_size)
{
    FILE *file;
    size_t count;
    int extra;
    if (out_size) *out_size = 0U;
    if (!path || !buffer || cap < 2U) return 0;
    file = fopen(path, "rb");
    if (!file) return 0;
    count = fread(buffer, 1U, (size_t)(cap - 1U), file);
    extra = fgetc(file);
    fclose(file);
    if (extra != EOF) return 0;
    buffer[count] = '\0';
    if (out_size) *out_size = (unsigned int)count;
    return 1;
}

static int b3d_sprite89_try_default_root(Blank3DSpriteRuntime89 *runtime,
                                         int kind, const char *path)
{
    if (!runtime || !path || !runtime->file_provider.exists) return 0;
    if (runtime->file_provider.exists(runtime->file_provider.user, path)
        != AR89_PROVIDER_FOUND) return 0;
    return ar89_add_root(&runtime->routes, kind, path, 1);
}

static int b3d_sprite89_image_acquire(void *user, const char *path,
                                      sa89_u32 *out_handle,
                                      sa89_u32 *out_width,
                                      sa89_u32 *out_height,
                                      sa89_u32 *out_frames)
{
    Blank3DSpriteRuntime89 *runtime;
    const Blank3DImageAsset *slot;
    int image_id;
    runtime = (Blank3DSpriteRuntime89 *)user;
    if (!runtime || !runtime->images || !path || !out_handle ||
        !out_width || !out_height || !out_frames)
        return SA89_PROVIDER_ERROR;
    image_id = blank3d_image_assets_resolve_request(runtime->images, path);
    if (image_id <= 0 || !blank3d_image_assets_load(runtime->images, image_id)) {
        b3d_sprite89_status(runtime,
                            blank3d_image_assets_error(runtime->images));
        return SA89_PROVIDER_ERROR;
    }
    slot = blank3d_image_assets_get(runtime->images, image_id);
    if (!slot || !slot->loaded) return SA89_PROVIDER_ERROR;
    *out_handle = (sa89_u32)image_id;
    *out_width = (sa89_u32)slot->width;
    *out_height = (sa89_u32)slot->height;
    /* Blank3D's GPU image registry exposes frame zero. Sprite sheets and
       RenList sequences are represented by SpriteAsset89 frame metadata. */
    *out_frames = 1U;
    return SA89_PROVIDER_HANDLED;
}

static int b3d_sprite89_image_get_frame(void *user, sa89_u32 handle,
                                        sa89_u32 frame_index,
                                        SA89_ImageView *out_view)
{
    Blank3DSpriteRuntime89 *runtime;
    const Blank3DImageAsset *slot;
    runtime = (Blank3DSpriteRuntime89 *)user;
    if (!runtime || !runtime->images || !out_view || frame_index != 0U)
        return SA89_PROVIDER_ERROR;
    slot = blank3d_image_assets_get(runtime->images, (int)handle);
    if (!slot || !slot->loaded) return SA89_PROVIDER_ERROR;
    runtime->current_image_id = (int)handle;
    out_view->pixels = 0;
    out_view->width = (sa89_u32)slot->width;
    out_view->height = (sa89_u32)slot->height;
    out_view->stride = (sa89_u32)slot->width * 4U;
    return SA89_PROVIDER_HANDLED;
}

static void b3d_sprite89_image_release(void *user, sa89_u32 handle)
{
    /* Texture ownership stays in Blank3DImageAssets. Reset/shutdown there
       performs destruction exactly once. */
    (void)user;
    (void)handle;
}

static int b3d_sprite89_draw(void *user, const SA89_ImageView *view,
                             sa89_s32 src_x, sa89_s32 src_y,
                             sa89_s32 src_w, sa89_s32 src_h,
                             sa89_s32 dst_x, sa89_s32 dst_y,
                             sa89_u32 flags)
{
    Blank3DSpriteRuntime89 *runtime;
    (void)view;
    runtime = (Blank3DSpriteRuntime89 *)user;
    if (!runtime || runtime->current_image_id <= 0 ||
        !runtime->render_host.draw)
        return SA89_PROVIDER_ERROR;
    if (!runtime->render_host.draw(runtime->render_host.user,
            runtime->current_image_id,
            (int)src_x, (int)src_y, (int)src_w, (int)src_h,
            (int)dst_x, (int)dst_y, (unsigned int)flags))
        return SA89_PROVIDER_ERROR;
    return SA89_PROVIDER_HANDLED;
}

void blank3d_sprite_runtime89_init(Blank3DSpriteRuntime89 *runtime,
                                    Blank3DImageAssets *images)
{
    SA89_ImageProvider image_provider;
    SA89_RenderProvider render_provider;
    if (!runtime) return;
    memset(runtime, 0, sizeof(*runtime));
    runtime->images = images;
    ar89_init(&runtime->routes);
#ifdef _WIN32
    ar89_win32_make_provider(&runtime->file_provider);
#else
    ar89_posix_make_provider(&runtime->file_provider);
#endif
    ar89_set_file_provider(&runtime->routes, &runtime->file_provider);

    (void)b3d_sprite89_try_default_root(runtime, AR89_KIND_IMAGE, "images");
    (void)b3d_sprite89_try_default_root(runtime, AR89_KIND_IMAGE,
                                         "assets/images");
    (void)b3d_sprite89_try_default_root(runtime, AR89_KIND_IMAGE,
                                         "game/images");
    (void)b3d_sprite89_try_default_root(runtime, AR89_KIND_IMAGE,
                                         "config/images");
    (void)b3d_sprite89_try_default_root(runtime, AR89_KIND_IMAGE,
                                         "config/weapons/assets");
    (void)b3d_sprite89_try_default_root(runtime, AR89_KIND_IMAGE,
                                         "config/skybox/assets");
    (void)b3d_sprite89_try_default_root(runtime, AR89_KIND_DATA,
                                         "config/weapons/visuals");
    (void)b3d_sprite89_try_default_root(runtime, AR89_KIND_DATA,
                                         "config/sprites");
    (void)b3d_sprite89_try_default_root(runtime, AR89_KIND_DATA,
                                         "assets/sprites");
    (void)b3d_sprite89_try_default_root(runtime, AR89_KIND_DATA,
                                         "game/sprites");
    (void)b3d_sprite89_try_default_root(runtime, AR89_KIND_AUDIO, "audio");
    (void)b3d_sprite89_try_default_root(runtime, AR89_KIND_AUDIO,
                                         "assets/audio");
    (void)b3d_sprite89_try_default_root(runtime, AR89_KIND_AUDIO,
                                         "game/audio");
    (void)b3d_sprite89_try_default_root(runtime, AR89_KIND_AUDIO,
                                         "config/audio");
    if (runtime->routes.root_count > 0U)
        (void)ar89_scan_roots(&runtime->routes);

    if (images) blank3d_image_assets_set_routes(images, &runtime->routes);

    sa89_init(&runtime->assets);
    memset(&image_provider, 0, sizeof(image_provider));
    image_provider.acquire = b3d_sprite89_image_acquire;
    image_provider.get_frame = b3d_sprite89_image_get_frame;
    image_provider.release = b3d_sprite89_image_release;
    image_provider.user = runtime;
    sa89_set_image_provider(&runtime->assets, &image_provider);

    memset(&render_provider, 0, sizeof(render_provider));
    render_provider.draw = b3d_sprite89_draw;
    render_provider.user = runtime;
    sa89_set_render_provider(&runtime->assets, &render_provider);

    sv89_init(&runtime->verbs, &runtime->assets);
    runtime->lazy_route.routes = &runtime->routes;
    runtime->lazy_route.static_duration_ms = 1000U;
    sv89_set_lazy_asset_provider(&runtime->verbs,
                                 sv89_assetroute_lazy,
                                 &runtime->lazy_route);
    runtime->current_image_id = 0;
    runtime->initialized = 1;
    b3d_sprite89_status(runtime,
        "SpriteAsset89 + SpriteVerbs89 + AssetRoute89 initialized");
}

void blank3d_sprite_runtime89_set_render_host(
    Blank3DSpriteRuntime89 *runtime,
    const Blank3DSprite89RenderHost *host)
{
    if (!runtime) return;
    if (host) runtime->render_host = *host;
    else memset(&runtime->render_host, 0, sizeof(runtime->render_host));
}

int blank3d_sprite_runtime89_add_root(Blank3DSpriteRuntime89 *runtime,
                                      int kind, const char *path,
                                      int recursive)
{
    if (!runtime || !runtime->initialized) return 0;
    if (!ar89_add_root(&runtime->routes, kind, path, recursive)) {
        b3d_sprite89_status(runtime, "AssetRoute89 root capacity/error");
        return 0;
    }
    return 1;
}

int blank3d_sprite_runtime89_scan(Blank3DSpriteRuntime89 *runtime)
{
    if (!runtime || !runtime->initialized) return 0;
    if (!ar89_scan_roots(&runtime->routes)) {
        b3d_sprite89_status(runtime, "AssetRoute89 scan failed");
        return 0;
    }
    b3d_sprite89_status(runtime, "AssetRoute89 scan complete");
    return 1;
}

int blank3d_sprite_runtime89_alias(Blank3DSpriteRuntime89 *runtime,
                                   int kind, const char *name,
                                   const char *path)
{
    if (!runtime || !runtime->initialized) return 0;
    if (!ar89_register_explicit(&runtime->routes, kind, name, path)) {
        b3d_sprite89_status(runtime, "AssetRoute89 alias failed");
        return 0;
    }
    return 1;
}

int blank3d_sprite_runtime89_resolve(Blank3DSpriteRuntime89 *runtime,
                                     int kind, const char *request,
                                     char *out_path, unsigned int out_cap)
{
    if (!runtime || !runtime->initialized || !out_path || out_cap == 0U)
        return 0;
    return ar89_resolve_request(&runtime->routes, kind, request,
                                out_path, (ar89_u32)out_cap);
}

static const char *b3d_sprite89_resolve_image_request(
    Blank3DSpriteRuntime89 *runtime, const char *request,
    char *resolved, unsigned int cap)
{
    if (!runtime || !request || !resolved || cap == 0U) return 0;
    if (ar89_resolve_request(&runtime->routes, AR89_KIND_IMAGE, request,
                             resolved, (ar89_u32)cap))
        return resolved;
    /* Keep provider-agnostic authoring useful: explicit raw paths may be
       registered before the file exists (e.g. generated content). */
    b3d_sprite89_copy(resolved, cap, request);
    return resolved;
}

int blank3d_sprite_runtime89_define_static(Blank3DSpriteRuntime89 *runtime,
                                           const char *asset_name,
                                           const char *request,
                                           unsigned int duration_ms)
{
    char path[AR89_PATH_CAP];
    if (!runtime || !runtime->initialized || !asset_name || !request)
        return 0;
    if (!b3d_sprite89_resolve_image_request(runtime, request,
                                             path, sizeof(path))) return 0;
    if (!sa89_define_static(&runtime->assets, asset_name, path,
                            duration_ms ? duration_ms : 1000U)) {
        b3d_sprite89_status(runtime,
                            sa89_error_string(runtime->assets.last_error));
        return 0;
    }
    b3d_sprite89_status(runtime, "SpriteAsset89 static asset defined");
    return 1;
}

int blank3d_sprite_runtime89_define_grid(Blank3DSpriteRuntime89 *runtime,
                                         const char *asset_name,
                                         const char *clip_name,
                                         const char *request,
                                         int frame_w, int frame_h,
                                         unsigned int frame_count,
                                         unsigned int columns,
                                         unsigned int duration_ms,
                                         int loop_mode)
{
    char path[AR89_PATH_CAP];
    if (!runtime || !runtime->initialized || !asset_name || !clip_name ||
        !request || frame_w <= 0 || frame_h <= 0 || frame_count == 0U ||
        columns == 0U || frame_count > 65534U || columns > 65534U)
        return 0;
    if (!b3d_sprite89_resolve_image_request(runtime, request,
                                             path, sizeof(path))) return 0;
    if (!sa89_define_grid(&runtime->assets, asset_name, clip_name, path,
                          (sa89_s32)frame_w, (sa89_s32)frame_h,
                          (sa89_id)frame_count, (sa89_id)columns,
                          duration_ms ? duration_ms : 100U, loop_mode)) {
        b3d_sprite89_status(runtime,
                            sa89_error_string(runtime->assets.last_error));
        return 0;
    }
    b3d_sprite89_status(runtime, "SpriteAsset89 grid clip defined");
    return 1;
}

int blank3d_sprite_runtime89_import_renlist_file(
    Blank3DSpriteRuntime89 *runtime, const char *path,
    sa89_id *out_asset_id)
{
    unsigned int size;
    if (!runtime || !runtime->initialized || !path) return 0;
    if (!b3d_sprite89_read_file(path, b3d_sprite89_import_buffer,
                                B3D_SPRITE89_IMPORT_CAP, &size)) {
        b3d_sprite89_status(runtime, "SpriteAsset89 RenList read failed");
        return 0;
    }
    if (!sa89_renlist_import(&runtime->assets,
                              b3d_sprite89_import_buffer, size,
                              out_asset_id)) {
        b3d_sprite89_status(runtime, "SpriteAsset89 RenList import failed");
        return 0;
    }
    b3d_sprite89_status(runtime, "SpriteAsset89 RenList imported");
    return 1;
}

int blank3d_sprite_runtime89_import_aseprite_file(
    Blank3DSpriteRuntime89 *runtime, const char *asset_name,
    const char *json_path, const char *sheet_override,
    sa89_id *out_asset_id)
{
    unsigned int size;
    if (!runtime || !runtime->initialized || !asset_name || !json_path)
        return 0;
    if (!b3d_sprite89_read_file(json_path, b3d_sprite89_import_buffer,
                                B3D_SPRITE89_IMPORT_CAP, &size)) {
        b3d_sprite89_status(runtime, "SpriteVerbs89 Aseprite JSON read failed");
        return 0;
    }
    if (!sv89_aseprite_import_json(&runtime->assets, asset_name,
                                    sheet_override,
                                    b3d_sprite89_import_buffer, size,
                                    out_asset_id)) {
        b3d_sprite89_status(runtime, "SpriteVerbs89 Aseprite import failed");
        return 0;
    }
    b3d_sprite89_status(runtime, "SpriteVerbs89 Aseprite import complete");
    return 1;
}

int blank3d_sprite_runtime89_execute(Blank3DSpriteRuntime89 *runtime,
                                     const char *target,
                                     const char *verb,
                                     const char *argument)
{
    if (!runtime || !runtime->initialized) return 0;
    if (!sv89_execute(&runtime->verbs, target, verb, argument)) {
        b3d_sprite89_status(runtime, "SpriteVerbs89 command failed");
        return 0;
    }
    return 1;
}

int blank3d_sprite_runtime89_execute_ddsl(Blank3DSpriteRuntime89 *runtime,
                                          const char *verb,
                                          const char *value_text)
{
    char target[SV89_TARGET_CAP];
    char argument[SV89_ARG_CAP];
    unsigned int i;
    unsigned int j;
    if (!runtime || !verb || !value_text) return 0;
    i = 0U;
    while (value_text[i] == ' ' || value_text[i] == '\t') ++i;
    j = 0U;
    while (value_text[i] && value_text[i] != ' ' && value_text[i] != '\t') {
        if (j + 1U < (unsigned int)sizeof(target)) target[j++] = value_text[i];
        ++i;
    }
    target[j] = '\0';
    while (value_text[i] == ' ' || value_text[i] == '\t') ++i;
    b3d_sprite89_copy(argument, (unsigned int)sizeof(argument), value_text + i);
    if (!target[0]) return 0;
    return blank3d_sprite_runtime89_execute(runtime, target, verb,
                                            argument[0] ? argument : 0);
}

void blank3d_sprite_runtime89_step(Blank3DSpriteRuntime89 *runtime,
                                   unsigned int delta_ms)
{
    sa89_id i;
    if (!runtime || !runtime->initialized) return;
    for (i = 0U; i < SA89_MAX_PLAYERS; ++i)
        if (runtime->assets.players[i].used)
            sa89_player_step(&runtime->assets, i, (sa89_u32)delta_ms);
}

int blank3d_sprite_runtime89_render(Blank3DSpriteRuntime89 *runtime)
{
    sa89_id i;
    int ok;
    if (!runtime || !runtime->initialized) return 0;
    ok = 1;
    for (i = 0U; i < SA89_MAX_PLAYERS; ++i) {
        if (!runtime->assets.players[i].used ||
            !runtime->assets.players[i].visible) continue;
        if (!sa89_player_render(&runtime->assets, i)) ok = 0;
    }
    if (!ok)
        b3d_sprite89_status(runtime,
                            sa89_error_string(runtime->assets.last_error));
    return ok;
}

int blank3d_sprite_runtime89_loop_mode(const char *text)
{
    if (!text || !text[0]) return SA89_LOOP_FORWARD;
    if (strcmp(text, "none") == 0 || strcmp(text, "once") == 0)
        return SA89_LOOP_NONE;
    if (strcmp(text, "reverse") == 0) return SA89_LOOP_REVERSE;
    if (strcmp(text, "pingpong") == 0 || strcmp(text, "ping-pong") == 0)
        return SA89_LOOP_PINGPONG;
    return SA89_LOOP_FORWARD;
}

int blank3d_sprite_runtime89_kind(const char *text)
{
    if (!text) return AR89_KIND_ANY;
    if (strcmp(text, "image") == 0 || strcmp(text, "images") == 0 ||
        strcmp(text, "sprite") == 0 || strcmp(text, "sprites") == 0)
        return AR89_KIND_IMAGE;
    if (strcmp(text, "audio") == 0 || strcmp(text, "sound") == 0 ||
        strcmp(text, "music") == 0)
        return AR89_KIND_AUDIO;
    if (strcmp(text, "data") == 0) return AR89_KIND_DATA;
    return AR89_KIND_ANY;
}

const char *blank3d_sprite_runtime89_status(
    const Blank3DSpriteRuntime89 *runtime)
{
    return runtime ? runtime->status : "sprite runtime unavailable";
}
