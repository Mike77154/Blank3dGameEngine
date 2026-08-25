#include <stdio.h>
#include "gcrosshair_params89.h"

int main(void)
{
    GC89_Style style;
    GC89_InputState input;
    GC89_DrawSpec draw;

    gcp89_style_clear(&style);
    gcp89_variant_set_vector_px(&style.normal, 8, 12, 1, 1, 3,
                                GC89_ARM_ALL,
                                GC89_RGBA(255, 255, 255, 255));
    gcp89_variant_set_image_px(&style.normal, 12, 64, 64,
                              GC89_RGBA(255, 255, 255, 255), 1);
    input.flags = 0;
    input.spread_fx = GC89_FX_FROM_INT(3);
    gcp89_resolve(&style, &input, &draw);

    printf("mode=%d gap_fx=%ld image=%d\n",
           draw.draw_mode, draw.gap_fx, draw.image_id);
    return 0;
}
