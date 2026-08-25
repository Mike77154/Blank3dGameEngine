#include "blank3d_flamethrower_billboard89.h"

#include <limits.h>
#include <string.h>

static g3d_fix b3d_billboard_mul_q12(g3d_fix a, g3d_fix b)
{
    long product;
    product = (long)a * (long)b;
    product /= (long)G3D_FIX_ONE;
    if (product > (long)INT_MAX) return (g3d_fix)INT_MAX;
    if (product < (long)INT_MIN) return (g3d_fix)INT_MIN;
    return (g3d_fix)product;
}

static unsigned char b3d_billboard_scaled_alpha(unsigned short base,
                                                 unsigned int numerator,
                                                 unsigned int denominator)
{
    unsigned long value;
    if (denominator == 0U) denominator = 1U;
    value = ((unsigned long)base * (unsigned long)numerator) /
            (unsigned long)denominator;
    if (value > 255UL) value = 255UL;
    return (unsigned char)value;
}

void blank3d_projectile_billboard_sample(
    const Blank3DWeaponModules *modules,
    unsigned int age_ms,
    unsigned int life_ms,
    int projectile_slot,
    g3d_fix mesh_scale_q12,
    Blank3DFlameBillboardSample *out_sample)
{
    unsigned int slot_phase;
    unsigned int frame_ms;
    unsigned int frame_count;
    unsigned int columns;
    unsigned int rows;
    unsigned int phase_step;
    unsigned int frame;
    unsigned int p;
    unsigned int tri;
    g3d_fix pulse;
    g3d_fix width_factor;
    g3d_fix height_factor;
    unsigned int alpha_num;
    unsigned int hot_num;
    unsigned int glow_num;
    if (!out_sample) return;
    memset(out_sample, 0, sizeof(*out_sample));
    if (!modules || !modules->projectile_visual_image[0]) return;
    if (mesh_scale_q12 <= 0) mesh_scale_q12 = G3D_FIX_ONE;
    if (life_ms == 0U) life_ms = 1U;
    if (age_ms > life_ms) age_ms = life_ms;

    frame_ms = modules->projectile_visual_frame_ms;
    if (frame_ms == 0U) frame_ms = 1U;
    frame_count = modules->projectile_visual_frame_count;
    if (frame_count == 0U) frame_count = 1U;
    columns = modules->projectile_visual_columns;
    if (columns == 0U) columns = 1U;
    rows = modules->projectile_visual_rows;
    if (rows == 0U) rows = 1U;
    if (frame_count > columns * rows) frame_count = columns * rows;
    if (frame_count == 0U) return;
    phase_step = modules->projectile_visual_phase_step;

    slot_phase = projectile_slot < 0 ? 0U : (unsigned int)projectile_slot;
    frame = ((age_ms / frame_ms) + slot_phase * phase_step) % frame_count;
    out_sample->frame_index = (unsigned short)frame;
    out_sample->atlas_column = (unsigned short)(frame % columns);
    out_sample->atlas_row = (unsigned short)(frame / columns);
    out_sample->flip_x = (unsigned char)((slot_phase ^ frame) & 1U);

    /* Deterministic 0.90..1.10 size flicker remains behavior, while the
       authored base width/height now come entirely from the weapon recipe. */
    p = (frame * 5U + slot_phase * 3U) & 31U;
    tri = p <= 16U ? p : 32U - p;
    pulse = (g3d_fix)(3686L + (long)tri * 51L);

    width_factor = (g3d_fix)(modules->projectile_visual_width_scale_q16 / 16L);
    height_factor = (g3d_fix)(modules->projectile_visual_height_scale_q16 / 16L);
    if (width_factor <= 0) width_factor = G3D_FIX_ONE;
    if (height_factor <= 0) height_factor = G3D_FIX_ONE;
    out_sample->width_q12 = b3d_billboard_mul_q12(
        b3d_billboard_mul_q12(mesh_scale_q12, width_factor), pulse);
    out_sample->height_q12 = b3d_billboard_mul_q12(
        b3d_billboard_mul_q12(mesh_scale_q12, height_factor), pulse);
    if (out_sample->width_q12 < 128) out_sample->width_q12 = 128;
    if (out_sample->height_q12 < 128) out_sample->height_q12 = 128;

    out_sample->hot_scale_q12 =
        (g3d_fix)(modules->projectile_visual_core_scale_q16 / 16L);
    out_sample->glow_scale_q12 =
        (g3d_fix)(modules->projectile_visual_glow_scale_q16 / 16L);
    if (out_sample->hot_scale_q12 <= 0) out_sample->hot_scale_q12 = G3D_FIX_ONE;
    if (out_sample->glow_scale_q12 <= 0) out_sample->glow_scale_q12 = G3D_FIX_ONE;

    /* Fire-like heat falloff is presentation behavior, but all amplitudes,
       sizes, cadence and lighting values are data-driven. */
    if (age_ms < life_ms / 4U) {
        out_sample->core_r = 255U; out_sample->core_g = 238U; out_sample->core_b = 196U;
        alpha_num = 100U; hot_num = 100U; glow_num = 100U;
    } else if (age_ms < (life_ms * 2U) / 3U) {
        out_sample->core_r = 255U; out_sample->core_g = 205U; out_sample->core_b = 132U;
        alpha_num = 92U; hot_num = 83U; glow_num = 84U;
    } else {
        out_sample->core_r = 255U; out_sample->core_g = 154U; out_sample->core_b = 72U;
        alpha_num = 80U; hot_num = 61U; glow_num = 63U;
    }
    out_sample->core_a = b3d_billboard_scaled_alpha(215U, alpha_num, 100U);
    out_sample->hot_a = modules->projectile_visual_core
        ? b3d_billboard_scaled_alpha(modules->projectile_visual_core_alpha, hot_num, 100U) : 0U;
    out_sample->glow_a = modules->projectile_visual_glow
        ? b3d_billboard_scaled_alpha(modules->projectile_visual_glow_alpha, glow_num, 100U) : 0U;
}
