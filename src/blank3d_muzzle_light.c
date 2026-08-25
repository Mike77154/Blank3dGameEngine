#include "blank3d_muzzle_light.h"

#include <string.h>

#define B3D_Q16_ONE 65536L

static long b3d_q16_mul_safe(long a, long b)
{
    long ah;
    long bh;
    if (a <= 0L || b <= 0L) return 0L;
    ah = a / 256L;
    bh = b / 256L;
    if (ah > 8388607L / (bh > 0L ? bh : 1L)) return 2147483647L;
    return ah * bh;
}

void blank3d_muzzle_light_init(Blank3DMuzzleLight *light)
{
    if (!light) return;
    memset(light, 0, sizeof(*light));
}

int blank3d_muzzle_light_emit(Blank3DMuzzleLight *light,
                              const GWeaponModules89 *modules,
                              const GWP89_Event *event)
{
    unsigned short life;
    if (!light || !modules || !event || !modules->muzzle_light)
        return 0;
    life = modules->muzzle_light_life_ms;
    if (life == 0U) life = modules->muzzle_image_life_ms;
    if (life == 0U) life = 70U;
    light->active = 1;
    light->fresh = 1;
    light->life_ms = life;
    light->total_ms = life;
    light->x_q12 = event->muzzle_origin.x;
    light->y_q12 = event->muzzle_origin.y;
    light->z_q12 = event->muzzle_origin.z;
    light->peak_intensity_q16 = modules->muzzle_light_intensity_q16 > 0L
                              ? modules->muzzle_light_intensity_q16
                              : (2L * B3D_Q16_ONE);
    light->radius_q16 = modules->muzzle_light_radius_q16 > 0L
                      ? modules->muzzle_light_radius_q16
                      : (7L * B3D_Q16_ONE);
    light->r = modules->muzzle_light_r > 255U ? 255U : modules->muzzle_light_r;
    light->g = modules->muzzle_light_g > 255U ? 255U : modules->muzzle_light_g;
    light->b = modules->muzzle_light_b > 255U ? 255U : modules->muzzle_light_b;
    return 1;
}

void blank3d_muzzle_light_update(Blank3DMuzzleLight *light,
                                 unsigned int dt_ms)
{
    if (!light || !light->active) return;
    /* Preserve one complete render frame at peak energy after the event. */
    if (light->fresh) {
        light->fresh = 0;
        return;
    }
    if (dt_ms >= (unsigned int)light->life_ms) {
        light->life_ms = 0U;
        light->active = 0;
        return;
    }
    light->life_ms = (unsigned short)(light->life_ms - (unsigned short)dt_ms);
}

int blank3d_muzzle_light_sample(const Blank3DMuzzleLight *light,
                                Blank3DMuzzleLightSample *sample)
{
    long fraction_q16;
    long shaped_q16;
    if (!sample) return 0;
    memset(sample, 0, sizeof(*sample));
    if (!light || !light->active || light->life_ms == 0U ||
        light->total_ms == 0U) return 0;
    fraction_q16 = ((long)light->life_ms * B3D_Q16_ONE) /
                   (long)light->total_ms;
    if (fraction_q16 < 0L) fraction_q16 = 0L;
    if (fraction_q16 > B3D_Q16_ONE) fraction_q16 = B3D_Q16_ONE;
    /* A muzzle flash is an impulse, not a lamp: quadratic falloff gives a
       hard first frame and rapidly collapses the illumination thereafter. */
    shaped_q16 = b3d_q16_mul_safe(fraction_q16, fraction_q16);
    sample->active = 1;
    sample->x_q12 = light->x_q12;
    sample->y_q12 = light->y_q12;
    sample->z_q12 = light->z_q12;
    sample->intensity_q16 = b3d_q16_mul_safe(light->peak_intensity_q16,
                                             shaped_q16);
    sample->radius_q16 = light->radius_q16;
    sample->r = light->r;
    sample->g = light->g;
    sample->b = light->b;
    return 1;
}
