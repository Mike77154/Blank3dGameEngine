#include "projectilevisual2d89.h"

#include <string.h>

static long pv2d89_mul_q16(long a, long b)
{
    /* C89/fixed-point friendly: Q16 x Q16 using two Q8 halves. This avoids
       requiring a 64-bit intermediate on 32-bit targets. */
    return (a / 256L) * (b / 256L);
}

static long pv2d89_lerp_q16(long a, long b, long t_q16)
{
    long d;
    d = b - a;
    return a + pv2d89_mul_q16(d, t_q16);
}

static unsigned char pv2d89_lerp_u8(unsigned char a, unsigned char b,
                                    long t_q16)
{
    long av;
    long bv;
    long v;
    av = (long)a << 16;
    bv = (long)b << 16;
    v = pv2d89_lerp_q16(av, bv, t_q16) >> 16;
    if (v < 0L) v = 0L;
    if (v > 255L) v = 255L;
    return (unsigned char)v;
}

static PV2D89Color pv2d89_lerp_color(PV2D89Color a, PV2D89Color b,
                                     long t_q16)
{
    PV2D89Color c;
    c.r = pv2d89_lerp_u8(a.r, b.r, t_q16);
    c.g = pv2d89_lerp_u8(a.g, b.g, t_q16);
    c.b = pv2d89_lerp_u8(a.b, b.b, t_q16);
    c.a = pv2d89_lerp_u8(a.a, b.a, t_q16);
    return c;
}

static unsigned char pv2d89_scale_alpha(unsigned char alpha, long factor_q16)
{
    long v;
    v = pv2d89_mul_q16((long)alpha << 16, factor_q16) >> 16;
    if (v < 0L) v = 0L;
    if (v > 255L) v = 255L;
    return (unsigned char)v;
}

static long pv2d89_age_ratio(unsigned int age_ms, unsigned int life_ms)
{
    unsigned long num;
    if (life_ms == 0U) return 0L;
    if (age_ms >= life_ms) return PV2D89_Q16_ONE;
    num = (unsigned long)age_ms * 65536UL;
    return (long)(num / (unsigned long)life_ms);
}

static PV2D89Color pv2d89_main_color(const PV2D89Recipe *recipe,
                                     long age_q16)
{
    long t;
    if (age_q16 <= 16384L) {
        t = age_q16 * 4L;
        return pv2d89_lerp_color(recipe->main_start, recipe->main_mid, t);
    }
    if (age_q16 <= 43691L) {
        t = ((age_q16 - 16384L) * 65536L) / 27307L;
        return pv2d89_lerp_color(recipe->main_mid, recipe->main_end, t);
    }
    return recipe->main_end;
}

static long pv2d89_main_alpha_factor(long age_q16)
{
    if (age_q16 < 16384L) return 65536L;
    if (age_q16 < 43691L) return 60293L; /* 0.92 */
    return 52429L; /* 0.80 */
}

static long pv2d89_core_alpha_factor(long age_q16)
{
    if (age_q16 < 16384L) return 65536L;
    if (age_q16 < 43691L) return 54395L; /* 0.83 */
    return 39977L; /* 0.61 */
}

static long pv2d89_glow_alpha_factor(long age_q16)
{
    if (age_q16 < 16384L) return 65536L;
    if (age_q16 < 43691L) return 55050L; /* 0.84 */
    return 41288L; /* 0.63 */
}

static long pv2d89_pulse(const PV2D89Recipe *recipe,
                         unsigned int frame_index,
                         int projectile_slot)
{
    unsigned int slot;
    unsigned int p;
    unsigned int tri;
    long range;
    long t;
    if (!recipe) return PV2D89_Q16_ONE;
    if (recipe->pulse_min_q16 <= 0L ||
        recipe->pulse_max_q16 <= recipe->pulse_min_q16)
        return recipe->pulse_min_q16 > 0L ?
               recipe->pulse_min_q16 : PV2D89_Q16_ONE;
    slot = projectile_slot < 0 ? 0U : (unsigned int)projectile_slot;
    p = (frame_index * 5U + slot * 3U) & 31U;
    tri = p <= 16U ? p : 32U - p;
    t = (long)((tri * 65536UL) / 16UL);
    range = recipe->pulse_max_q16 - recipe->pulse_min_q16;
    return recipe->pulse_min_q16 + pv2d89_mul_q16(range, t);
}

static void pv2d89_corner(PV2D89Vec3 *out,
                          const PV2D89Vec3 *center,
                          const PV2D89Vec3 *right,
                          const PV2D89Vec3 *up,
                          long width_q16,
                          long height_q16,
                          int right_sign,
                          int up_sign)
{
    long hw;
    long hh;
    PV2D89Vec3 r;
    PV2D89Vec3 u;
    if (!out || !center || !right || !up) return;
    hw = width_q16 / 2L;
    hh = height_q16 / 2L;
    r.x = pv2d89_mul_q16(right->x, hw);
    r.y = pv2d89_mul_q16(right->y, hw);
    r.z = pv2d89_mul_q16(right->z, hw);
    u.x = pv2d89_mul_q16(up->x, hh);
    u.y = pv2d89_mul_q16(up->y, hh);
    u.z = pv2d89_mul_q16(up->z, hh);
    out->x = center->x + (right_sign > 0 ? r.x : -r.x) +
             (up_sign > 0 ? u.x : -u.x);
    out->y = center->y + (right_sign > 0 ? r.y : -r.y) +
             (up_sign > 0 ? u.y : -u.y);
    out->z = center->z + (right_sign > 0 ? r.z : -r.z) +
             (up_sign > 0 ? u.z : -u.z);
}

static void pv2d89_make_pass(PV2D89RenderPass *pass,
                             int layer,
                             const PV2D89Recipe *recipe,
                             const PV2D89ProjectileInput *input,
                             const PV2D89Frame *frame,
                             long width_q16,
                             long height_q16,
                             PV2D89Color tint,
                             int phase_flip_x)
{
    if (!pass || !recipe || !input || !frame) return;
    memset(pass, 0, sizeof(*pass));
    pass->enabled = 1;
    pass->layer = layer;
    pass->image_handle = frame->image_handle;
    pass->blend_mode = recipe->blend_mode;
    pass->source_enabled = (frame->flags & PV2D89_FRAME_SOURCE_RECT) ? 1 : 0;
    pass->source_x = frame->source_x;
    pass->source_y = frame->source_y;
    pass->source_w = frame->source_w;
    pass->source_h = frame->source_h;
    pass->flip_x = ((frame->flags & PV2D89_FRAME_FLIP_X) ? 1 : 0) ^
                   (phase_flip_x ? 1 : 0);
    pass->flip_y = (frame->flags & PV2D89_FRAME_FLIP_Y) ? 1 : 0;
    pass->tint = tint;
    pass->center_q16 = input->position_q16;
    pass->right_q16 = input->camera_right_q16;
    pass->up_q16 = input->camera_up_q16;
    pass->width_q16 = width_q16;
    pass->height_q16 = height_q16;
    pv2d89_corner(&pass->corners_q16[0], &pass->center_q16,
                  &pass->right_q16, &pass->up_q16,
                  width_q16, height_q16, -1, 1);
    pv2d89_corner(&pass->corners_q16[1], &pass->center_q16,
                  &pass->right_q16, &pass->up_q16,
                  width_q16, height_q16, 1, 1);
    pv2d89_corner(&pass->corners_q16[2], &pass->center_q16,
                  &pass->right_q16, &pass->up_q16,
                  width_q16, height_q16, 1, -1);
    pv2d89_corner(&pass->corners_q16[3], &pass->center_q16,
                  &pass->right_q16, &pass->up_q16,
                  width_q16, height_q16, -1, -1);
}

void pv2d89_recipe_init(PV2D89Recipe *recipe)
{
    if (!recipe) return;
    memset(recipe, 0, sizeof(*recipe));
    recipe->billboard_mode = PV2D89_BILLBOARD_VIEW;
    recipe->replace_mesh = 1;
    recipe->blend_mode = PV2D89_BLEND_ALPHA;
    recipe->width_scale_q16 = PV2D89_Q16_ONE;
    recipe->height_scale_q16 = PV2D89_Q16_ONE;
    recipe->pulse_min_q16 = PV2D89_Q16_ONE;
    recipe->pulse_max_q16 = PV2D89_Q16_ONE;
    recipe->main_start.r = 255U; recipe->main_start.g = 255U;
    recipe->main_start.b = 255U; recipe->main_start.a = 255U;
    recipe->main_mid = recipe->main_start;
    recipe->main_end = recipe->main_start;
    recipe->glow_scale_q16 = PV2D89_Q16_ONE;
    recipe->glow_color = recipe->main_start;
    recipe->core_scale_q16 = PV2D89_Q16_ONE;
    recipe->core_color = recipe->main_start;
    recipe->light_color = recipe->main_start;
}

void pv2d89_sample_clear(PV2D89Sample *sample)
{
    if (!sample) return;
    memset(sample, 0, sizeof(*sample));
}

int pv2d89_wants_billboard(const PV2D89Recipe *recipe)
{
    return recipe && recipe->enabled &&
           recipe->billboard_mode != PV2D89_BILLBOARD_NONE;
}

int pv2d89_build(const PV2D89Recipe *recipe,
                  const PV2D89ProjectileInput *input,
                  const PV2D89FrameProvider *provider,
                  PV2D89Sample *out_sample)
{
    PV2D89Frame frame;
    PV2D89Color main_color;
    PV2D89Color glow_color;
    PV2D89Color core_color;
    long mesh_scale;
    long pulse;
    long width;
    long height;
    long frame_sx;
    long frame_sy;
    long age_q16;
    long fade;
    int phase_flip;
    int pass_index;
    int rc;
    if (!out_sample) return PV2D89_ERR_ARGUMENT;
    pv2d89_sample_clear(out_sample);
    if (!recipe || !input || !provider || !provider->sample_frame)
        return PV2D89_ERR_ARGUMENT;
    if (!pv2d89_wants_billboard(recipe)) return PV2D89_NO_VISUAL;

    memset(&frame, 0, sizeof(frame));
    rc = provider->sample_frame(provider->user, recipe->weapon_id,
                                input->age_ms, input->projectile_slot, &frame);
    if (!rc) return PV2D89_ERR_PROVIDER;
    if (!frame.valid || frame.image_handle <= 0) return PV2D89_ERR_FRAME;

    mesh_scale = input->mesh_scale_q16 > 0L ?
                 input->mesh_scale_q16 : PV2D89_Q16_ONE;
    frame_sx = frame.scale_x_q16 > 0L ? frame.scale_x_q16 : PV2D89_Q16_ONE;
    frame_sy = frame.scale_y_q16 > 0L ? frame.scale_y_q16 : PV2D89_Q16_ONE;
    pulse = pv2d89_pulse(recipe, frame.frame_index, input->projectile_slot);
    width = pv2d89_mul_q16(mesh_scale, recipe->width_scale_q16);
    width = pv2d89_mul_q16(width, frame_sx);
    width = pv2d89_mul_q16(width, pulse);
    height = pv2d89_mul_q16(mesh_scale, recipe->height_scale_q16);
    height = pv2d89_mul_q16(height, frame_sy);
    height = pv2d89_mul_q16(height, pulse);
    if (width <= 0L || height <= 0L) return PV2D89_ERR_FRAME;

    age_q16 = pv2d89_age_ratio(input->age_ms, input->life_ms);
    main_color = pv2d89_main_color(recipe, age_q16);
    glow_color = recipe->glow_color;
    core_color = recipe->core_color;
    if (recipe->age_fade) {
        fade = pv2d89_main_alpha_factor(age_q16);
        main_color.a = pv2d89_scale_alpha(main_color.a, fade);
        glow_color.a = pv2d89_scale_alpha(glow_color.a,
                                           pv2d89_glow_alpha_factor(age_q16));
        core_color.a = pv2d89_scale_alpha(core_color.a,
                                           pv2d89_core_alpha_factor(age_q16));
    }

    phase_flip = 0;
    if (recipe->phase_flip_x) {
        unsigned int slot;
        slot = input->projectile_slot < 0 ? 0U :
               (unsigned int)input->projectile_slot;
        phase_flip = (int)((slot ^ frame.frame_index) & 1U);
    }

    pass_index = 0;
    if (recipe->glow_enabled && glow_color.a > 0U &&
        pass_index < PV2D89_MAX_PASSES) {
        pv2d89_make_pass(&out_sample->passes[pass_index], PV2D89_PASS_GLOW,
                         recipe, input, &frame,
                         pv2d89_mul_q16(width, recipe->glow_scale_q16),
                         pv2d89_mul_q16(height, recipe->glow_scale_q16),
                         glow_color, phase_flip);
        ++pass_index;
    }
    if (pass_index < PV2D89_MAX_PASSES) {
        pv2d89_make_pass(&out_sample->passes[pass_index], PV2D89_PASS_MAIN,
                         recipe, input, &frame, width, height,
                         main_color, phase_flip);
        ++pass_index;
    }
    if (recipe->core_enabled && core_color.a > 0U &&
        pass_index < PV2D89_MAX_PASSES) {
        pv2d89_make_pass(&out_sample->passes[pass_index], PV2D89_PASS_CORE,
                         recipe, input, &frame,
                         pv2d89_mul_q16(width, recipe->core_scale_q16),
                         pv2d89_mul_q16(height, recipe->core_scale_q16),
                         core_color, phase_flip);
        ++pass_index;
    }

    out_sample->valid = 1;
    out_sample->replace_mesh = recipe->replace_mesh ? 1 : 0;
    out_sample->frame_index = frame.frame_index;
    out_sample->pass_count = pass_index;
    if (recipe->light_enabled) {
        out_sample->light.enabled = 1;
        out_sample->light.group_id = recipe->weapon_id;
        out_sample->light.position_q16 = input->position_q16;
        out_sample->light.intensity_q16 = recipe->light_intensity_q16;
        out_sample->light.radius_q16 = recipe->light_radius_q16;
        out_sample->light.color = recipe->light_color;
    }
    return PV2D89_OK;
}

void pv2d89_light_accumulator_init(PV2D89LightAccumulator *accum)
{
    if (!accum) return;
    memset(accum, 0, sizeof(*accum));
}

int pv2d89_light_accumulator_add(PV2D89LightAccumulator *accum,
                                 const PV2D89LightSample *sample)
{
    PV2D89Vec3 delta;
    int n;
    if (!accum || !sample || !sample->enabled) return 0;
    if (!accum->used) {
        accum->used = 1;
        accum->group_id = sample->group_id;
        accum->count = 1;
        accum->center_q16 = sample->position_q16;
        accum->base_intensity_q16 = sample->intensity_q16;
        accum->base_radius_q16 = sample->radius_q16;
        accum->color = sample->color;
        return 1;
    }
    if (accum->group_id != sample->group_id) return 0;
    n = accum->count + 1;
    delta.x = sample->position_q16.x - accum->center_q16.x;
    delta.y = sample->position_q16.y - accum->center_q16.y;
    delta.z = sample->position_q16.z - accum->center_q16.z;
    accum->center_q16.x += delta.x / (long)n;
    accum->center_q16.y += delta.y / (long)n;
    accum->center_q16.z += delta.z / (long)n;
    accum->count = n;
    return 1;
}

int pv2d89_light_accumulator_finish(const PV2D89LightAccumulator *accum,
                                    unsigned long frame_stamp,
                                    PV2D89LightSample *out_light)
{
    int energy_count;
    unsigned long phase;
    unsigned long tri;
    long intensity;
    long radius;
    long flicker_q16;
    if (!out_light) return 0;
    memset(out_light, 0, sizeof(*out_light));
    if (!accum || !accum->used || accum->count <= 0) return 0;
    energy_count = accum->count;
    if (energy_count > 12) energy_count = 12;
    intensity = accum->base_intensity_q16;
    radius = accum->base_radius_q16;
    intensity += (long)(energy_count - 1) * (intensity / 14L);
    radius += (long)(energy_count - 1) * (radius / 32L);
    phase = (frame_stamp * 5UL + (unsigned long)accum->count * 3UL) & 15UL;
    tri = phase <= 8UL ? phase : 16UL - phase;
    flicker_q16 = 57672L + (long)tri * 1966L;
    intensity = pv2d89_mul_q16(intensity, flicker_q16);
    out_light->enabled = 1;
    out_light->group_id = accum->group_id;
    out_light->position_q16 = accum->center_q16;
    out_light->intensity_q16 = intensity;
    out_light->radius_q16 = radius;
    out_light->color = accum->color;
    return 1;
}
