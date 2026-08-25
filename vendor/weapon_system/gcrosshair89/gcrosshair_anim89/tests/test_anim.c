#include <stdio.h>
#include <string.h>
#include "gcrosshair_anim89.h"
int main(void)
{
    GCB89_AnimationPreset legacy;
    GCB89_AnimationRecipe recipe;
    GC89A_Animator a;
    memset(&legacy,0,sizeof(legacy)); memset(&recipe,0,sizeof(recipe));
    legacy.micro_scale_fx=52428L; legacy.neutral_scale_fx=65536L; legacy.maxi_scale_fx=91750L;
    recipe.events[GCB89_ANIM_EVENT_FIRE].enabled=1;
    recipe.events[GCB89_ANIM_EVENT_FIRE].scale_target=GCB89_ANIM_SCALE_MAXI;
    recipe.events[GCB89_ANIM_EVENT_FIRE].alpha_fx=GC89_FX_ONE;
    recipe.events[GCB89_ANIM_EVENT_FIRE].thickness_scale_fx=GC89_FX_ONE;
    recipe.events[GCB89_ANIM_EVENT_FIRE].dot_scale_fx=GC89_FX_ONE;
    recipe.events[GCB89_ANIM_EVENT_FIRE].attack_ticks=2;
    recipe.events[GCB89_ANIM_EVENT_FIRE].return_ticks=4;
    recipe.events[GCB89_ANIM_EVENT_FIRE].return_mode=GCB89_ANIM_RETURN_START;
    recipe.events[GCB89_ANIM_EVENT_FIRE].repeat_count=1;
    recipe.events[GCB89_ANIM_EVENT_FIRE].attack_curve=GCB89_ANIM_CURVE_LINEAR;
    recipe.events[GCB89_ANIM_EVENT_FIRE].return_curve=GCB89_ANIM_CURVE_LINEAR;
    gc89a_init(&a,&legacy,&recipe);
    if(!gc89a_trigger(&a,GCB89_ANIM_EVENT_FIRE)) return 1;
    gc89a_update(&a,2);
    if(a.current.scale_fx != legacy.maxi_scale_fx) return 2;
    gc89a_update(&a,4);
    if(a.current.scale_fx != legacy.neutral_scale_fx) return 3;
    puts("gcrosshair_anim89: OK"); return 0;
}
