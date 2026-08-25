#include <stdio.h>
#include <string.h>
#include "gcrosshair_base89.h"

int main(void)
{
    GC89_Style style;
    int count;

    gcb89_recipe_reset();
    if (!gcb89_recipe_load_root("recipes/gcrosshair.ini")) {
        fprintf(stderr, "recipe load failed: %s\n", gcb89_recipe_last_error());
        return 1;
    }
    if (!gcb89_recipe_is_loaded()) return 2;
    count = gcb89_preset_count();
    if (count != 192) return 3;
    if (strcmp(gcb89_preset_name(190), "retro_hex_neon_vector") != 0) return 4;
    if (!gcb89_make_preset(190, &style)) return 5;
    if (style.normal.shape_type != GC89_SHAPE_HEXAGON) return 6;
    if (style.fire.shape_radius_x_fx != GC89_FX_FROM_INT(20)) return 7;
    if (style.normal.outline_color_rgba != GC89_RGBA(0, 0, 0, 255)) return 8;
    if (strcmp(gcb89_asset_filename(1000), "assets/ring.tga") != 0) return 9;

    printf("gcrosshair_recipe89: OK (%d INI presets)\n", count);
    return 0;
}
