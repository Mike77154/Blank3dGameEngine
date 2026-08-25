#include <stdio.h>
#include "gcrosshair_base89.h"

int main(void)
{
    GC89_Style style;
    GCB89_AnimationPreset animation;
    int i;
    int asset_id;

    for (i = 0; i < gcb89_preset_count(); ++i) {
        gcb89_make_preset(i, &style);
        gcb89_make_preset_animation(i, &animation);
        asset_id = gcb89_preset_asset_id(i);

        printf("%02d %-24s %-14s mode=%d",
               i,
               gcb89_preset_name(i),
               gcb89_preset_category(i),
               gcb89_preset_draw_mode(i));

        if (asset_id != 0) {
            printf(" image=%d file=%s",
                   asset_id,
                   gcb89_asset_filename(asset_id));
        }
        printf("\n");
    }
    return 0;
}
