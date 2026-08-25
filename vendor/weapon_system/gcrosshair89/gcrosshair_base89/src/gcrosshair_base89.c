#include <string.h>
#include "gcrosshair_base89.h"
#include "gcrosshair_recipe89.h"
#include "gcrosshair_recipe89_internal.h"

static int gcb89_ensure(void)
{
    return gcb89_recipe__ensure();
}

void gcb89_make_blank3d_original_style(GC89_Style *style)
{
    const GC89_Style *src;
    if (!style) return;
    memset(style, 0, sizeof(*style));
    if (!gcb89_ensure()) return;
    src = gcb89_recipe__style(GCB89_PRESET_BLANK3D_DEFAULT);
    if (src) *style = *src;
}

void gcb89_make_blank3d_animation(GCB89_AnimationPreset *preset)
{
    const GCB89_AnimationPreset *src;
    if (!preset) return;
    memset(preset, 0, sizeof(*preset));
    if (!gcb89_ensure()) return;
    src = gcb89_recipe__animation(GCB89_PRESET_BLANK3D_DEFAULT);
    if (src) *preset = *src;
}

int gcb89_preset_count(void)
{
    if (!gcb89_ensure()) return 0;
    return gcb89_recipe__count();
}

int gcb89_preset_is_valid(int preset_id)
{
    if (!gcb89_ensure()) return 0;
    return gcb89_recipe__valid(preset_id);
}

const char *gcb89_preset_name(int preset_id)
{
    if (!gcb89_ensure()) return "invalid";
    return gcb89_recipe__name(preset_id);
}

const char *gcb89_preset_category(int preset_id)
{
    if (!gcb89_ensure()) return "invalid";
    return gcb89_recipe__category(preset_id);
}

int gcb89_preset_draw_mode(int preset_id)
{
    const GC89_Style *style;
    if (!gcb89_ensure()) return 0;
    style = gcb89_recipe__style(preset_id);
    return style ? style->normal.draw_mode : 0;
}

int gcb89_preset_asset_id(int preset_id)
{
    const GC89_Style *style;
    if (!gcb89_ensure()) return 0;
    style = gcb89_recipe__style(preset_id);
    return style ? style->normal.image_id : 0;
}

int gcb89_preset_shape_type(int preset_id)
{
    const GC89_Style *style;
    if (!gcb89_ensure()) return GC89_SHAPE_CROSS;
    style = gcb89_recipe__style(preset_id);
    return style ? style->normal.shape_type : GC89_SHAPE_CROSS;
}

int gcb89_preset_spread_mode(int preset_id)
{
    const GC89_Style *style;
    if (!gcb89_ensure()) return GC89_SPREAD_NONE;
    style = gcb89_recipe__style(preset_id);
    return style ? style->normal.spread_mode : GC89_SPREAD_NONE;
}

int gcb89_preset_requires_image(int preset_id)
{
    int mode;
    mode = gcb89_preset_draw_mode(preset_id);
    return (mode & GC89_DRAW_IMAGE) != 0;
}

int gcb89_make_preset(int preset_id, GC89_Style *style)
{
    const GC89_Style *src;
    if (!style || !gcb89_ensure()) return 0;
    src = gcb89_recipe__style(preset_id);
    if (!src) return 0;
    *style = *src;
    return 1;
}

int gcb89_make_preset_animation(int preset_id,
                                GCB89_AnimationPreset *preset)
{
    const GCB89_AnimationPreset *src;
    if (!preset || !gcb89_ensure()) return 0;
    src = gcb89_recipe__animation(preset_id);
    if (!src) return 0;
    *preset = *src;
    return 1;
}

int gcb89_make_preset_animation_recipe(int preset_id,
                                       GCB89_AnimationRecipe *recipe)
{
    const GCB89_AnimationRecipe *src;
    if (!recipe || !gcb89_ensure()) return 0;
    src = gcb89_recipe__animation_recipe(preset_id);
    if (!src) return 0;
    *recipe = *src;
    return 1;
}

const char *gcb89_asset_filename(int image_id)
{
    if (!gcb89_ensure()) return "";
    return gcb89_recipe__asset_filename(image_id);
}

GC89_Fixed gcb89_blank3d_recoil_to_spread_fx(long camera_recoil_x1000)
{
    int negative;
    unsigned long magnitude;
    unsigned long whole;
    unsigned long remainder;
    unsigned long out;

    negative = camera_recoil_x1000 < 0;
    if (negative) magnitude = (unsigned long)(-camera_recoil_x1000);
    else magnitude = (unsigned long)camera_recoil_x1000;

    /* recoil * 2.2 pixels, input expressed as recoil*1000. */
    whole = magnitude / 5000UL;
    remainder = magnitude % 5000UL;
    out = whole * 720896UL + (remainder * 720896UL) / 5000UL;
    if (negative) return -(GC89_Fixed)out;
    return (GC89_Fixed)out;
}

void gcb89_make_blank3d_input(GC89_InputState *input,
                              int aiming,
                              int firing,
                              int hit,
                              long camera_recoil_x1000)
{
    if (!input) return;
    input->flags = 0;
    if (aiming) input->flags |= GC89_STATE_AIM;
    if (firing) input->flags |= GC89_STATE_FIRE;
    if (hit) input->flags |= GC89_STATE_HIT;
    input->spread_fx =
        gcb89_blank3d_recoil_to_spread_fx(camera_recoil_x1000);
    if (input->spread_fx < 0) input->spread_fx = 0;
}
