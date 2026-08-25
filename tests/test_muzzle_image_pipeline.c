#include "blank3d_weapon_ini.h"
#include "blank3d_weapon_host_io.h"
#include "blank3d_weapon_modules.h"
#include "blank3d_image_assets.h"
#include "blank3d_spriteplanes.h"
#include "blank3d_muzzle_image.h"
#include "gweapon89.h"

#include <stdio.h>
#include <string.h>

static int fail(const char *text)
{
    fprintf(stderr, "FAIL: %s\n", text);
    return 1;
}

int main(void)
{
    GWP89_Manager manager;
    const GWeaponModules89 *modules;
    Blank3DImageAssets images;
    Blank3DSpritePlaneWorld planes;
    GWP89_Event event;
    char status[160];
    int loaded;
    int sprite_id;
    const Blank3DImageAsset *asset;
    sprpl89_sprite *sprite;

    gwp89_init(&manager);
    if (blank3d_weapon_host_io_bind(&manager) < 0)
        return fail("host IO bind");
    status[0] = '\0';
    loaded = blank3d_weapon_ini_load_manifest(
        &manager, "tests/data/muzzle_image/weapons.ini",
        status, sizeof(status));
    if (loaded != 1) return fail(status[0] ? status : "manifest load");
    modules = blank3d_weapon_modules_get(1);
    if (!modules) return fail("modules unavailable");
    if (strcmp(modules->muzzle_image_path,
               "config/crosshair/assets/ring.tga") != 0)
        return fail("muzzle path parser");
    if (modules->muzzle_image_width_q16 != 81920L ||
        modules->muzzle_image_height_q16 != 49152L ||
        modules->muzzle_image_life_ms != 83U ||
        modules->muzzle_image_blend != GWM89_MUZZLE_IMAGE_BLEND_ALPHA ||
        modules->muzzle_image_billboard != GWM89_MUZZLE_IMAGE_BILLBOARD_VIEW)
        return fail("muzzle image recipe parser");

    blank3d_image_assets_init(&images);
    blank3d_spriteplanes_init(&planes, &images);
    memset(&event, 0, sizeof(event));
    event.muzzle_origin.x = 2L * 4096L;
    event.muzzle_origin.y = 3L * 4096L;
    event.muzzle_origin.z = -4L * 4096L;
    event.direction.z = -4096L;
    if (!blank3d_muzzle_image_emit(&planes, modules, &event))
        return fail("muzzle image did not spawn SpritePlane89");
    if (planes.ctx.sprite_count != 1 || planes.ctx.frame_count != 1)
        return fail("muzzle SpritePlane89 counts");
    sprite_id = 0;
    sprite = sprpl89_get_sprite(&planes.ctx, sprite_id);
    if (!sprite || !sprite->active ||
        sprite->billboard_mode != SP89_BILLBOARD_VIEW_ALIGNED ||
        sprite->blend_mode != SP89_BLEND_ALPHA ||
        sprite->width != 81920L || sprite->height != 49152L)
        return fail("muzzle SpritePlane89 recipe bridge");
    asset = blank3d_image_assets_get(&images, (int)sprite->texture_page);
    if (!asset || strcmp(asset->path,
                         "config/crosshair/assets/ring.tga") != 0)
        return fail("muzzle image registry bridge");

    blank3d_spriteplanes_update(&planes, 82U);
    if (!sprite->active) return fail("muzzle lifetime ended too early");
    blank3d_spriteplanes_update(&planes, 1U);
    if (sprite->active) return fail("muzzle lifetime did not expire");

    puts("weapon muzzle image -> imgcc0 asset -> SpritePlane89: PASS");
    return 0;
}
