#include "blank3d_muzzle_image.h"

#include <string.h>

static long b3d_q12_to_q16(long value)
{
    if (value > 134217727L) return 2147483632L;
    if (value < -134217728L) return (-2147483647L - 1L);
    return value << 4;
}

int blank3d_muzzle_image_emit(Blank3DSpritePlaneWorld *planes,
                              const GWeaponModules89 *modules,
                              const GWP89_Event *event)
{
    Blank3DSpritePlaneSpec spec;
    if (!planes || !modules || !event || !modules->muzzle_image_path[0])
        return 0;
    memset(&spec, 0, sizeof(spec));
    spec.x_q16 = b3d_q12_to_q16(event->muzzle_origin.x);
    spec.y_q16 = b3d_q12_to_q16(event->muzzle_origin.y);
    spec.z_q16 = b3d_q12_to_q16(event->muzzle_origin.z);
    /* Tiny forward bias avoids coplanarity with first-person weapon geometry. */
    spec.x_q16 += b3d_q12_to_q16(event->direction.x) / 16L;
    spec.y_q16 += b3d_q12_to_q16(event->direction.y) / 16L;
    spec.z_q16 += b3d_q12_to_q16(event->direction.z) / 16L;
    spec.width_q16 = modules->muzzle_image_width_q16;
    spec.height_q16 = modules->muzzle_image_height_q16;
    if (modules->muzzle_image_billboard == GWM89_MUZZLE_IMAGE_BILLBOARD_FIXED)
        spec.billboard_mode = SP89_BILLBOARD_FIXED;
    else if (modules->muzzle_image_billboard == GWM89_MUZZLE_IMAGE_BILLBOARD_VIEW)
        spec.billboard_mode = SP89_BILLBOARD_VIEW_ALIGNED;
    else spec.billboard_mode = SP89_BILLBOARD_CAMERA_FACING;
    spec.blend_mode = modules->muzzle_image_blend == GWM89_MUZZLE_IMAGE_BLEND_ALPHA
                    ? SP89_BLEND_ALPHA : SP89_BLEND_ADDITIVE;
    spec.depth_test = 1;
    spec.life_ms = modules->muzzle_image_life_ms > 0
                 ? modules->muzzle_image_life_ms : 70U;
    spec.tint_rgba = 0xFFFFFFFFUL;
    spec.plane_axis = 0;
    if (blank3d_spriteplanes_spawn_path(planes,
                                        modules->muzzle_image_path,
                                        &spec) < 0)
        return 0;

    /* Optional bloom-like halo implemented as a second additive SpritePlane.
       Keeping it in runtime preserves the source image byte-for-byte and lets
       every weapon tune the effect from its INI without shaders or heap use. */
    if (modules->muzzle_image_glow &&
        modules->muzzle_image_glow_scale_q16 > 0L &&
        modules->muzzle_image_glow_alpha > 0U) {
        unsigned long alpha;
        long glow_scale;
        glow_scale = modules->muzzle_image_glow_scale_q16;
        spec.width_q16 = (long)(((spec.width_q16 >> 8) *
                                 (glow_scale >> 8)));
        spec.height_q16 = (long)(((spec.height_q16 >> 8) *
                                  (glow_scale >> 8)));
        spec.blend_mode = SP89_BLEND_ADDITIVE;
        alpha = (unsigned long)(modules->muzzle_image_glow_alpha & 255U);
        spec.tint_rgba = 0xFFFFFF00UL | alpha;
        (void)blank3d_spriteplanes_spawn_path(planes,
                                               modules->muzzle_image_path,
                                               &spec);
    }
    return 1;
}
