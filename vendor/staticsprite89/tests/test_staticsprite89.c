#include "staticsprite89.h"
#include <stdio.h>
#include <string.h>

int main(void)
{
    SS89_StaticSprite s;
    SS89_Sample sample;
    ss89_init(&s);
    if (!ss89_set_image(&s, "fire_idle")) return 2;
    ss89_set_source_rect(&s, 16, 32, 64, 64);
    ss89_set_position_q16(&s, 2 * SS89_Q16_ONE, 3 * SS89_Q16_ONE, -4 * SS89_Q16_ONE);
    ss89_set_scale_q16(&s, SS89_Q16_ONE / 2, SS89_Q16_ONE * 2);
    ss89_set_tint_rgba8(&s, 255U, 128U, 64U, 200U);
    ss89_set_flags(&s, SS89_FLAG_FLIP_X);
    ss89_set_user_tag(&s, 77U);
    if (!ss89_sample(&s, &sample)) return 3;
    if (strcmp(sample.image_request, "fire_idle") != 0) return 4;
    if (!sample.source_enabled || sample.source.x != 16 || sample.source.y != 32) return 5;
    if (sample.position_q16[2] != -4 * SS89_Q16_ONE) return 6;
    if (sample.scale_q16[0] != SS89_Q16_ONE / 2) return 7;
    if (sample.tint_rgba[1] != 128U || sample.tint_rgba[3] != 200U) return 8;
    if (sample.flags != SS89_FLAG_FLIP_X || sample.user_tag != 77U) return 9;
    ss89_set_visible(&s, 0);
    if (!ss89_sample(&s, &sample) || sample.visible != 0U) return 10;
    printf("StaticSprite89 PASS request=%s tag=%u\n", sample.image_request, (unsigned)sample.user_tag);
    return 0;
}
