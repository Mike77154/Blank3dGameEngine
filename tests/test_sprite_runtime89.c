#include <stdio.h>
#include <string.h>

#include "blank3d_sprite_runtime89.h"

static int upload_calls;
static int draw_calls;
static int last_image;
static int last_x;
static int last_y;

static unsigned int fake_upload(void *user, const unsigned char *rgba,
                                unsigned int w, unsigned int h)
{
    (void)user;
    if (!rgba || w == 0U || h == 0U) return 0U;
    ++upload_calls;
    return 900U + (unsigned int)upload_calls;
}

static void fake_destroy(void *user, unsigned int token)
{
    (void)user;
    (void)token;
}

static int fake_draw(void *user, int image_id,
                     int sx, int sy, int sw, int sh,
                     int dx, int dy, unsigned int flags)
{
    (void)user;
    (void)sx;
    (void)sy;
    (void)flags;
    if (image_id <= 0 || sw <= 0 || sh <= 0) return 0;
    ++draw_calls;
    last_image = image_id;
    last_x = dx;
    last_y = dy;
    return 1;
}

int main(void)
{
    Blank3DImageAssets images;
    Blank3DImageBackend backend;
    Blank3DSpriteRuntime89 runtime;
    Blank3DSprite89RenderHost host;
    char path[AR89_PATH_CAP];
    sa89_id player;

    blank3d_image_assets_init(&images);
    memset(&backend, 0, sizeof(backend));
    backend.upload_rgba = fake_upload;
    backend.destroy_texture = fake_destroy;
    blank3d_image_assets_set_backend(&images, &backend);

    blank3d_sprite_runtime89_init(&runtime, &images);
    memset(&host, 0, sizeof(host));
    host.draw = fake_draw;
    blank3d_sprite_runtime89_set_render_host(&runtime, &host);

    if (!blank3d_sprite_runtime89_add_root(&runtime, AR89_KIND_IMAGE,
            "config/crosshair/assets", 1)) return 1;
    if (!blank3d_sprite_runtime89_scan(&runtime)) return 2;
    if (!blank3d_sprite_runtime89_resolve(&runtime, AR89_KIND_IMAGE,
            "ring", path, sizeof(path))) return 3;
    if (strcmp(path, "config/crosshair/assets/ring.tga") != 0) return 4;

    /* The logical name is enough: SpriteVerbs89 lazily asks AssetRoute89,
       which defines the SpriteAsset89 source only when first used. */
    if (!blank3d_sprite_runtime89_execute(&runtime,
            "hud_face", "sprite_show", "ring")) return 5;
    if (!blank3d_sprite_runtime89_execute(&runtime,
            "hud_face", "sprite_pos", "32 48")) return 6;
    if (!blank3d_sprite_runtime89_render(&runtime)) return 7;
    if (upload_calls != 1 || draw_calls != 1) return 8;
    if (last_image <= 0 || last_x != 32 || last_y != 48) return 9;

    player = sv89_target_player(&runtime.verbs, "hud_face");
    if (player == SA89_INVALID_ID) return 10;
    if (runtime.assets.players[player].x != 32 ||
        runtime.assets.players[player].y != 48) return 11;

    if (!blank3d_sprite_runtime89_execute_ddsl(&runtime,
            "image_speed", "hud_face 0.5")) return 12;
    if (runtime.assets.players[player].speed_q16 != 32768) return 13;

    blank3d_image_assets_shutdown(&images);
    puts("AssetRoute89 -> SpriteAsset89 -> SpriteVerbs89 -> Blank3D image registry: PASS");
    return 0;
}
