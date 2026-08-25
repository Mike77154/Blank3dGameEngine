#include <stdio.h>
#include "gcrosshair_runtime89.h"
int main(void)
{
    GC89R_Runtime r;
    const GC89A_Modifier *m;
    int i;
    unsigned long attack;
    if(!gc89r_init(&r,"../gcrosshair_base89/recipes/gcrosshair.ini")) return 1;
    for(i=0;i<gcb89_preset_count();++i) {
        if(!gc89r_set_preset(&r,i)) return 2;
        attack=r.animation_recipe.events[GCB89_ANIM_EVENT_FIRE].attack_ticks;
        if(!gc89r_trigger_event(&r,GCB89_ANIM_EVENT_FIRE)) return 3;
        if(attack) gc89r_update(&r,attack);
        m=gc89r_animation_modifier(&r);
        if(!m) return 4;
        if(m->scale_fx==r.animation.neutral_scale_fx &&
           m->rotation_deg_fx==0 && m->offset_x_fx==0 && m->offset_y_fx==0 &&
           m->thickness_scale_fx==GC89_FX_ONE && m->dot_scale_fx==GC89_FX_ONE)
            return 5;
    }
    printf("runtime animation coverage: OK (%d/192 presets animate on FIRE)\n",gcb89_preset_count());
    return 0;
}
