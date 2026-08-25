#include <stdio.h>
#include <string.h>
#include "gcrosshair_base89.h"

int main(void)
{
    GC89_Style style;
    GC89_InputState input;
    GCB89_AnimationPreset anim;
    GC89_Fixed spread;
    int i;
    int image_count;
    int vector_count;
    int abi2_shape_count;
    const char *asset;

    gcb89_make_blank3d_original_style(&style);
    if (style.normal.gap_fx != GC89_FX_FROM_INT(9)) return 1;
    if (style.normal.arm_length_fx != GC89_FX_FROM_INT(13)) return 2;
    if (style.normal.dot_size_fx != GC89_FX_FROM_INT(3)) return 3;
    if (style.aim.gap_fx != GC89_FX_FROM_INT(5)) return 4;
    if (style.aim.arm_length_fx != GC89_FX_FROM_INT(9)) return 5;
    if (style.aim.thickness_fx != GC89_FX_FROM_INT(2)) return 6;
    if (style.aim.dot_size_fx != GC89_FX_FROM_INT(4)) return 7;
    if (style.normal.color_rgba != GC89_RGBA(209, 235, 255, 255)) return 8;
    if (style.aim.color_rgba != GC89_RGBA(255, 235, 89, 255)) return 9;
    if (GC89_TYPES_ABI_VERSION != 3) return 36;
    if (style.normal.outline_enabled != 0) return 37;
    if (style.normal.outline_width_fx != 0) return 38;
    if (style.normal.outline_color_rgba != GC89_RGBA(0, 0, 0, 255)) return 39;

    spread = gcb89_blank3d_recoil_to_spread_fx(1000L);
    if (spread < 144178L || spread > 144180L) return 10;

    gcb89_make_blank3d_input(&input, 1, 0, 0, 1000L);
    if ((input.flags & GC89_STATE_AIM) == 0) return 11;
    if (input.spread_fx != spread) return 12;

    gcb89_make_blank3d_animation(&anim);
    if (anim.micro_scale_fx >= anim.neutral_scale_fx) return 13;
    if (anim.maxi_scale_fx <= anim.neutral_scale_fx) return 14;

    if (gcb89_preset_count() != GCB89_PRESET_COUNT) return 15;
    if (GCB89_PRESET_COUNT != 192) return 16;
    if (strcmp(gcb89_preset_name(0), "blank3d_default") != 0) return 17;
    if (gcb89_preset_is_valid(-1)) return 18;
    if (gcb89_preset_is_valid(GCB89_PRESET_COUNT)) return 19;

    image_count = 0;
    vector_count = 0;
    abi2_shape_count = 0;
    for (i = 0; i < gcb89_preset_count(); ++i) {
        if (!gcb89_make_preset(i, &style)) return 20;
        if (!gcb89_make_preset_animation(i, &anim)) return 21;
        if (style.normal.draw_mode != GC89_DRAW_VECTOR) return 22;
        ++vector_count;
        if (style.normal.image_id != 0) return 34;
        if (style.normal.color_rgba == 0) return 23;
        if (gcb89_preset_name(i)[0] == '\0') return 24;
        if (gcb89_preset_category(i)[0] == '\0') return 25;
        if (gcb89_preset_shape_type(i) != GC89_SHAPE_CROSS) {
            ++abi2_shape_count;
            if (style.normal.shape_radius_x_fx <= 0) return 30;
            if (style.normal.shape_radius_y_fx <= 0) return 31;
        }
        if (style.normal.spread_mode !=
            gcb89_preset_spread_mode(i)) return 32;

        if (gcb89_preset_requires_image(i)) {
            ++image_count;
            if (gcb89_preset_asset_id(i) == 0) return 26;
            asset = gcb89_asset_filename(gcb89_preset_asset_id(i));
            if (!asset || asset[0] == '\0') return 27;
        }
    }
    if (image_count != 0) return 28;
    if (vector_count != GCB89_PRESET_COUNT) return 35;
    if (abi2_shape_count < 50) return 33;
    if (gcb89_asset_filename(-1)[0] != '\0') return 29;

    printf("gcrosshair_base89: OK (%d presets, %d vector-only, ABI3, %d non-cross shapes)\n",
           gcb89_preset_count(), vector_count, abi2_shape_count);
    return 0;
}
