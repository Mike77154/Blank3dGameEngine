#include <stdio.h>
#include <string.h>
#include "gcrosshair_runtime89.h"

#define W 320
#define H 200
static unsigned char rgba[W * H * 4];

int main(void)
{
    GC89R_Runtime runtime;
    GC89P_SoftwareSurface surface;
    GC89_InputState input;

    memset(&input, 0, sizeof(input));
    if (!gc89r_init(&runtime, "../gcrosshair_base89/recipes/gcrosshair.ini"))
        return 1;
    if (!gc89r_set_preset(&runtime, 144)) return 1;

    gc89p_surface_init(&surface, rgba, W, H, W * 4);
    gc89p_surface_clear(&surface, GC89_RGBA(16, 16, 20, 255));
    gc89r_draw_rgba(&runtime, &input, &surface, 0);

    printf("rendered preset %d: %s\n",
           gc89r_preset_id(&runtime), gc89r_preset_name(&runtime));
    return 0;
}
