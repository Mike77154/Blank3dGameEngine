#include "spriteasset89.h"
#include "spriteasset89_renlist89.h"
#include <stdio.h>
int main(void)
{
    SpriteAsset89 a; sa89_id id; sa89_id clip;
    const char script[] = "image hero idle:\n    \"hero0.png\"\n    pause 0.08\n    \"hero1.png\"\n    pause 0.12\n    repeat\n";
    sa89_init(&a);
    if(!sa89_renlist_import(&a,script,(unsigned int)(sizeof(script)-1U),&id)) return 2;
    clip=sa89_find_clip(&a,id,"default"); if(clip==SA89_INVALID_ID) return 3;
    if(a.clips[clip].frame_count!=2U) return 4;
    if(a.frames[a.clips[clip].first_frame].duration_ms!=80U) return 5;
    if(a.frames[a.clips[clip].first_frame+1U].duration_ms!=120U) return 6;
    if(a.clips[clip].loop_mode!=SA89_LOOP_FORWARD) return 7;
    if(!sa89_define_grid(&a,"hero run","run","hero_sheet.png",16,16,4U,2U,75U,SA89_LOOP_FORWARD)) return 8;
    printf("SpriteAsset89 RenList/Grid PASS assets=%u clips=%u\n",(unsigned)a.asset_count,(unsigned)a.clip_count);
    return 0;
}
