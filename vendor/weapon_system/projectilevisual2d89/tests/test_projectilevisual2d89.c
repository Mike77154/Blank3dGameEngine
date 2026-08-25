#include <stdio.h>
#include <string.h>
#include "projectilevisual2d89.h"

static int fake_frame(void *user, int weapon_id, unsigned int age_ms,
                      int projectile_slot, PV2D89Frame *out)
{
    (void)user;
    (void)projectile_slot;
    if (!out || weapon_id != 14) return 0;
    memset(out, 0, sizeof(*out));
    out->valid = 1;
    out->image_handle = 77;
    out->flags = PV2D89_FRAME_SOURCE_RECT;
    out->source_x = (int)((age_ms / 33U) * 128U);
    out->source_y = 0;
    out->source_w = 128;
    out->source_h = 128;
    out->scale_x_q16 = PV2D89_Q16_ONE;
    out->scale_y_q16 = PV2D89_Q16_ONE;
    out->frame_index = age_ms / 33U;
    return 1;
}

int main(void)
{
    PV2D89Recipe r;
    PV2D89ProjectileInput in;
    PV2D89FrameProvider provider;
    PV2D89Sample s;
    PV2D89LightAccumulator accum;
    PV2D89LightSample light;
    int rc;

    pv2d89_recipe_init(&r);
    r.enabled = 1;
    r.weapon_id = 14;
    r.billboard_mode = PV2D89_BILLBOARD_VIEW;
    r.replace_mesh = 1;
    r.blend_mode = PV2D89_BLEND_ADDITIVE;
    r.width_scale_q16 = 98304L;
    r.height_scale_q16 = 131072L;
    r.pulse_min_q16 = 58982L;
    r.pulse_max_q16 = 72090L;
    r.phase_flip_x = 1;
    r.age_fade = 1;
    r.main_start.r = 255U; r.main_start.g = 238U; r.main_start.b = 196U; r.main_start.a = 215U;
    r.main_mid.r = 255U; r.main_mid.g = 205U; r.main_mid.b = 132U; r.main_mid.a = 215U;
    r.main_end.r = 255U; r.main_end.g = 154U; r.main_end.b = 72U; r.main_end.a = 215U;
    r.glow_enabled = 1;
    r.glow_scale_q16 = 85197L;
    r.glow_color.r = 255U; r.glow_color.g = 104U; r.glow_color.b = 28U; r.glow_color.a = 105U;
    r.core_enabled = 1;
    r.core_scale_q16 = 47186L;
    r.core_color.r = 255U; r.core_color.g = 248U; r.core_color.b = 214U; r.core_color.a = 150U;
    r.light_enabled = 1;
    r.light_intensity_q16 = 78643L;
    r.light_radius_q16 = 262144L;
    r.light_color.r = 255U; r.light_color.g = 112U; r.light_color.b = 32U; r.light_color.a = 255U;

    memset(&in, 0, sizeof(in));
    in.position_q16.x = 65536L;
    in.position_q16.y = 131072L;
    in.position_q16.z = -196608L;
    in.camera_right_q16.x = 65536L;
    in.camera_up_q16.y = 65536L;
    in.mesh_scale_q16 = 65536L;
    in.age_ms = 33U;
    in.life_ms = 1000U;
    in.projectile_slot = 1;

    provider.user = 0;
    provider.sample_frame = fake_frame;
    rc = pv2d89_build(&r, &in, &provider, &s);
    if (rc != PV2D89_OK || !s.valid || !s.replace_mesh ||
        s.pass_count != 3 || s.frame_index != 1U ||
        s.passes[0].layer != PV2D89_PASS_GLOW ||
        s.passes[1].layer != PV2D89_PASS_MAIN ||
        s.passes[2].layer != PV2D89_PASS_CORE ||
        s.passes[1].image_handle != 77 ||
        s.passes[1].source_x != 128 ||
        s.passes[1].corners_q16[0].x >= s.passes[1].corners_q16[1].x ||
        !s.light.enabled) {
        puts("projectilevisual2d89 billboard build failed");
        return 1;
    }

    pv2d89_light_accumulator_init(&accum);
    if (!pv2d89_light_accumulator_add(&accum, &s.light)) return 2;
    s.light.position_q16.x += 65536L;
    if (!pv2d89_light_accumulator_add(&accum, &s.light)) return 3;
    if (!pv2d89_light_accumulator_finish(&accum, 7UL, &light) ||
        !light.enabled || light.position_q16.x <= 65536L ||
        light.intensity_q16 <= 0L || light.radius_q16 <= 0L) {
        puts("projectilevisual2d89 light aggregation failed");
        return 4;
    }

    r.enabled = 0;
    if (pv2d89_build(&r, &in, &provider, &s) != PV2D89_NO_VISUAL) return 5;
    puts("projectilevisual2d89: PASS");
    return 0;
}
