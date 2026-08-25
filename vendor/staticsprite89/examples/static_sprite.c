#include "staticsprite89.h"

int main(void)
{
    SS89_StaticSprite sprite;
    SS89_Sample sample;
    ss89_init(&sprite);
    ss89_set_image(&sprite, "ui_health_icon");
    ss89_set_position_q16(&sprite, 32 * SS89_Q16_ONE, 24 * SS89_Q16_ONE, 0);
    if (ss89_sample(&sprite, &sample)) {
        /* A host renderer resolves sample.image_request and draws it. */
    }
    return 0;
}
