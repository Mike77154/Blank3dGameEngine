#include <stdio.h>
#include "gcrosshair_base89.h"
int main(void)
{
    int i, e;
    GCB89_AnimationRecipe r;
    if (gcb89_preset_count() != 192) return 1;
    for (i=0;i<192;++i) {
        if(!gcb89_make_preset_animation_recipe(i,&r)) return 2;
        if(!r.auto_input_events) return 3;
        for(e=0;e<6;++e) if(!r.events[e].enabled) return 4;
    }
    puts("animation recipes: OK (192 presets, 6 standard events each)");
    return 0;
}
