#include "blank3d_weapon_ini.h"
#include "blank3d_weapon_host_io.h"
#include "blank3d_weapon_modules.h"
#include "blank3d_image_assets.h"
#include "blank3d_spriteplanes.h"
#include "blank3d_muzzle_image.h"
#include "blank3d_muzzle_light.h"
#include "gweapon89.h"

#include <stdio.h>
#include <string.h>

static unsigned int upload_calls;
static unsigned int upload_w;
static unsigned int upload_h;
static int upload_corner_ok;
static int upload_center_ok;

static unsigned int fake_upload(void *user, const unsigned char *rgba,
                                unsigned int w, unsigned int h)
{
    (void)user;
    if (!rgba || !w || !h) return 0U;
    ++upload_calls;
    upload_w = w;
    upload_h = h;
    if (w == 512U && h == 512U) {
        const unsigned char *corner = rgba;
        const unsigned char *center = rgba + (((256U * w) + 256U) * 4U);
        upload_corner_ok = (corner[0] <= 8U && corner[1] <= 8U &&
                            corner[2] <= 8U && corner[3] == 255U);
        upload_center_ok = (center[0] >= 248U && center[1] >= 248U &&
                            center[2] >= 248U && center[3] == 255U);
    }
    return 31337U;
}

static void fake_destroy(void *user, unsigned int token)
{
    (void)user;
    (void)token;
}

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
    Blank3DImageBackend backend;
    Blank3DSpritePlaneWorld planes;
    GWP89_Event event;
    const Blank3DImageAsset *asset;
    sprpl89_sprite *core;
    sprpl89_sprite *glow;
    sprpl89_camera camera;
    sprpl89_emit emitted;
    Blank3DMuzzleLight light;
    Blank3DMuzzleLightSample light_sample;
    char status[192];
    int loaded;
    int image_id;

    gwp89_init(&manager);
    if (blank3d_weapon_host_io_bind(&manager) < 0)
        return fail("host IO bind");

    status[0] = '\0';
    loaded = blank3d_weapon_ini_load_manifest(
        &manager, "config/weapons/weapons.ini", status, sizeof(status));
    if (loaded < 1) return fail(status[0] ? status : "weapon manifest load");

    modules = blank3d_weapon_modules_get(B3D_WEAPON_ID_PISTOL);
    if (!modules) return fail("pistol modules unavailable");
    if (strcmp(modules->muzzle_image_path,
               "config/weapons/assets/pistol_muzzle_flash.jpg") != 0)
        return fail("pistol muzzle image path");
    if (modules->muzzle_image_blend != GWM89_MUZZLE_IMAGE_BLEND_ADDITIVE ||
        modules->muzzle_image_billboard != GWM89_MUZZLE_IMAGE_BILLBOARD_VIEW ||
        !modules->muzzle_image_glow ||
        modules->muzzle_image_glow_scale_q16 <= 65536L ||
        modules->muzzle_image_glow_alpha != 160U ||
        !modules->muzzle_light ||
        modules->muzzle_light_life_ms != 72U ||
        modules->muzzle_light_intensity_q16 <= (2L * 65536L) ||
        modules->muzzle_light_radius_q16 < (8L * 65536L))
        return fail("pistol glow/light recipe parser");

    blank3d_image_assets_init(&images);
    memset(&backend, 0, sizeof(backend));
    backend.upload_rgba = fake_upload;
    backend.destroy_texture = fake_destroy;
    blank3d_image_assets_set_backend(&images, &backend);
    blank3d_spriteplanes_init(&planes, &images);

    memset(&event, 0, sizeof(event));
    event.muzzle_origin.x = 4096L;
    event.muzzle_origin.y = 8192L;
    event.muzzle_origin.z = -12288L;
    event.direction.z = -4096L;

    if (!blank3d_muzzle_image_emit(&planes, modules, &event))
        return fail("pistol muzzle emit");
    if (planes.ctx.sprite_count != 2 || planes.ctx.frame_count != 1)
        return fail("core + glow SpritePlane count");

    core = sprpl89_get_sprite(&planes.ctx, 0);
    glow = sprpl89_get_sprite(&planes.ctx, 1);
    if (!core || !glow || !core->active || !glow->active)
        return fail("pistol muzzle sprites active");
    if (core->blend_mode != SP89_BLEND_ADDITIVE ||
        glow->blend_mode != SP89_BLEND_ADDITIVE)
        return fail("additive brightness mode");
    if (glow->width <= core->width || glow->height <= core->height)
        return fail("glow pass not enlarged");
    if (glow->color.a != 160U || core->color.a != 255U)
        return fail("glow alpha recipe");

    blank3d_muzzle_light_init(&light);
    if (!blank3d_muzzle_light_emit(&light, modules, &event))
        return fail("muzzle illumination emit");
    if (!blank3d_muzzle_light_sample(&light, &light_sample) ||
        light_sample.intensity_q16 != modules->muzzle_light_intensity_q16 ||
        light_sample.radius_q16 != modules->muzzle_light_radius_q16)
        return fail("muzzle illumination peak sample");

    image_id = (int)core->texture_page;
    if (image_id <= 0) return fail("muzzle image registration");
    if (!blank3d_image_assets_load(&images, image_id)) {
        fprintf(stderr, "decode error: %s\n", blank3d_image_assets_error(&images));
        return 1;
    }
    asset = blank3d_image_assets_get(&images, image_id);
    if (!asset || !asset->loaded || asset->format != IMGCC0_FMT_JPEG)
        return fail("native progressive muzzle JPEG decode");
    if (asset->width != 512U || asset->height != 512U ||
        upload_calls != 1U || upload_w != 512U || upload_h != 512U)
        return fail("native progressive muzzle JPEG dimensions");
    if (!upload_corner_ok || !upload_center_ok)
        return fail("native progressive muzzle JPEG pixel sanity");

    memset(&camera, 0, sizeof(camera));
    camera.pos = sprpl89_v3(0, 0, 0);
    camera.forward = sprpl89_v3(0, 0, -SP89_FX_ONE);
    camera.right = sprpl89_v3(SP89_FX_ONE, 0, 0);
    camera.up = sprpl89_v3(0, SP89_FX_ONE, 0);
    camera.world_up = camera.up;
    memset(&emitted, 0, sizeof(emitted));
    if (blank3d_spriteplanes_emit(&planes, &camera, &emitted) != SP89_OK ||
        emitted.packet_count != 2)
        return fail("muzzle emit packet count");

    blank3d_spriteplanes_update(&planes, 57U);
    if (!core->active || !glow->active)
        return fail("muzzle flash died early");
    blank3d_spriteplanes_update(&planes, 1U);
    if (core->active || glow->active)
        return fail("muzzle flash lifetime");
    if (blank3d_spriteplanes_emit(&planes, &camera, &emitted) != SP89_OK ||
        emitted.packet_count != 0 || emitted.tri_count != 0 ||
        emitted.vert_count != 0)
        return fail("dead muzzle retained stale render packets");

    blank3d_muzzle_light_update(&light, 16U); /* fresh frame is held */
    if (!blank3d_muzzle_light_sample(&light, &light_sample) ||
        light_sample.intensity_q16 != modules->muzzle_light_intensity_q16)
        return fail("muzzle light peak frame");
    blank3d_muzzle_light_update(&light, 36U);
    if (!blank3d_muzzle_light_sample(&light, &light_sample) ||
        light_sample.intensity_q16 >= modules->muzzle_light_intensity_q16 ||
        light_sample.intensity_q16 <= 0L)
        return fail("muzzle light impulse decay");
    blank3d_muzzle_light_update(&light, 36U);
    if (blank3d_muzzle_light_sample(&light, &light_sample))
        return fail("muzzle light lifetime");

    blank3d_image_assets_shutdown(&images);
    puts("pistol progressive JPEG muzzle + finite glow + illumination pulse: PASS");
    return 0;
}
