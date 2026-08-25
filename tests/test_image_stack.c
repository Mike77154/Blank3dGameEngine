#include <stdio.h>
#include <string.h>

#include "blank3d_image_assets.h"
#include "blank3d_spriteplanes.h"

static int upload_calls;
static unsigned int upload_w;
static unsigned int upload_h;

static unsigned int fake_upload(void *user, const unsigned char *rgba,
                                unsigned int w, unsigned int h)
{
    (void)user;
    if (!rgba || !w || !h) return 0U;
    ++upload_calls;
    upload_w = w;
    upload_h = h;
    return 77U;
}

static void fake_destroy(void *user, unsigned int token)
{
    (void)user;
    (void)token;
}

int main(void)
{
    Blank3DImageAssets images;
    Blank3DImageBackend backend;
    Blank3DSpritePlaneWorld planes;
    Blank3DSpritePlaneSpec spec;
    sprpl89_camera camera;
    sprpl89_emit emit;
    const Blank3DImageAsset *asset;
    int sprite_id;

    blank3d_image_assets_init(&images);
    memset(&backend, 0, sizeof(backend));
    backend.upload_rgba = fake_upload;
    backend.destroy_texture = fake_destroy;
    blank3d_image_assets_set_backend(&images, &backend);

    if (!blank3d_image_assets_register(&images, 1000,
            "config/crosshair/assets/ring.tga")) return 1;
    if (!blank3d_image_assets_load(&images, 1000)) {
        fprintf(stderr, "%s\n", blank3d_image_assets_error(&images));
        return 2;
    }
    asset = blank3d_image_assets_get(&images, 1000);
    if (!asset || !asset->loaded || asset->format != IMGCC0_FMT_TGA) return 3;
    if (asset->width != 64U || asset->height != 64U) return 4;
    if (upload_calls != 1 || upload_w != 64U || upload_h != 64U) return 5;
    if (!blank3d_image_assets_load(&images, 1000) || upload_calls != 1) return 6;

    blank3d_spriteplanes_init(&planes, &images);
    memset(&spec, 0, sizeof(spec));
    spec.image_id = 1000;
    spec.x_q16 = 0L;
    spec.y_q16 = 2L * SP89_FX_ONE;
    spec.z_q16 = -4L * SP89_FX_ONE;
    spec.width_q16 = 2L * SP89_FX_ONE;
    spec.height_q16 = 2L * SP89_FX_ONE;
    spec.billboard_mode = SP89_BILLBOARD_CAMERA_FACING;
    spec.blend_mode = SP89_BLEND_ALPHA;
    spec.depth_test = 1;
    spec.tint_rgba = 0xFFFFFFFFUL;
    sprite_id = blank3d_spriteplanes_spawn(&planes, &spec);
    if (sprite_id < 0) return 7;

    memset(&camera, 0, sizeof(camera));
    camera.pos = sprpl89_v3(0, 2L * SP89_FX_ONE, 0);
    camera.forward = sprpl89_v3(0, 0, -SP89_FX_ONE);
    camera.right = sprpl89_v3(SP89_FX_ONE, 0, 0);
    camera.up = sprpl89_v3(0, SP89_FX_ONE, 0);
    camera.world_up = camera.up;
    if (blank3d_spriteplanes_emit(&planes, &camera, &emit) != SP89_OK) return 8;
    if (emit.packet_count < 1 || emit.tri_count < 2 || emit.vert_count < 4) return 9;
    if (emit.packets[0].page_id != 1000U) return 10;

    /* Transient consumers such as muzzle flashes may spawn hundreds of the
       same image.  The Blank3D wrapper must reuse the atlas-frame descriptor
       instead of exhausting SpritePlane89's fixed frame table. */
    spec.life_ms = 1UL;
    {
        int n;
        for (n = 0; n < 200; ++n) {
            if (blank3d_spriteplanes_spawn(&planes, &spec) < 0) return 11;
            blank3d_spriteplanes_update(&planes, 1U);
        }
    }
    if (planes.ctx.frame_count != 1) return 12;

    blank3d_spriteplanes_update(&planes, 16U);
    blank3d_image_assets_shutdown(&images);
    puts("imgcc0 -> image registry -> spriteplane89: PASS");
    return 0;
}
