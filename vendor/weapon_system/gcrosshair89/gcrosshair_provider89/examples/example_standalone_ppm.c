#include <stdio.h>
#include <string.h>
#include "gcrosshair_provider89.h"

#define W 256
#define H 256

static unsigned char rgba[W * H * 4];

int main(void)
{
    FILE *fp;
    int x;
    int y;
    GC89_Core core;
    GC89_DrawSpec spec;
    GC89P_SoftwareSurface surface;

    gc89_core_init(&core);
    memset(&spec, 0, sizeof(spec));
    spec.visible = 1;
    spec.draw_mode = GC89_DRAW_VECTOR;
    spec.dot_enabled = 1;
    spec.dot_size_fx = GC89_FX_FROM_INT(3);
    spec.thickness_fx = GC89_FX_FROM_INT(2);
    spec.color_rgba = GC89_RGBA(80, 255, 160, 255);
    spec.shape_type = GC89_SHAPE_DIAMOND;
    spec.shape_segment_mask = GC89_SEGMENT_0 | GC89_SEGMENT_1 |
                              GC89_SEGMENT_2 | GC89_SEGMENT_3;
    spec.shape_direction_mask = GC89_DIRECTION_ALL;
    spec.shape_radius_x_fx = GC89_FX_FROM_INT(36);
    spec.shape_radius_y_fx = GC89_FX_FROM_INT(36);
    spec.outline_enabled = 1;
    spec.outline_width_fx = GC89_FX_FROM_INT(1);
    spec.outline_color_rgba = GC89_RGBA(0, 0, 0, 255);

    gc89p_surface_init(&surface, rgba, W, H, W * 4);
    gc89p_surface_clear(&surface, GC89_RGBA(24, 24, 24, 255));
    gc89p_draw_rgba(&core, &spec, &surface, 0);

    fp = fopen("provider_standalone.ppm", "wb");
    if (!fp) return 1;
    fprintf(fp, "P6\n%d %d\n255\n", W, H);
    for (y = 0; y < H; ++y) {
        for (x = 0; x < W; ++x) {
            unsigned char *p;
            p = rgba + (y * W + x) * 4;
            fwrite(p, 1, 3, fp);
        }
    }
    fclose(fp);
    puts("wrote provider_standalone.ppm");
    return 0;
}
