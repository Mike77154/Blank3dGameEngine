#include <stdio.h>
#include "gcrosshair_base89.h"

int main(void)
{
    GC89_Style style;
    GC89_InputState input;
    int preset_id;

    preset_id = GCB89_PRESET_RING_CHEVRON;
    if (!gcb89_make_preset(preset_id, &style)) return 1;
    gcb89_make_blank3d_input(&input, 1, 0, 0, 250L);

    printf("preset=%s category=%s mode=%d asset=%s state=%d\n",
           gcb89_preset_name(preset_id),
           gcb89_preset_category(preset_id),
           style.normal.draw_mode,
           gcb89_asset_filename(style.normal.image_id),
           input.flags);
    return 0;
}
