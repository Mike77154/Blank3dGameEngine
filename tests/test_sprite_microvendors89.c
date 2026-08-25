#include "staticsprite89.h"
#include "imagesequencer89.h"
#include "renlist89.h"
#include "tilecell89.h"
#include "renlist89_imagesequencer89.h"
#include "tilecell89_imagesequencer89.h"
#include "imagesequencer89_staticsprite89.h"
#include <stdio.h>
#include <string.h>

int main(void)
{
    static const char script[] =
        "image flame loose:\n"
        "  billboard view\n"
        "  frame \"fire_a\" for 30\n"
        "  frame \"fire_b\" for 40\n"
        "  repeat\n";
    RenList89 rl;
    ImageSequencer89 seqs;
    TileCell89 tc;
    SS89_StaticSprite sprite;
    SS89_Sample sample;
    rl89_id anim;
    is89_id loose;
    is89_id atlas_seq;
    is89_id player;
    tc89_id atlas;
    rl89_init(&rl);
    is89_init(&seqs);
    tc89_init(&tc);
    ss89_init(&sprite);
    if (!rl89_parse(&rl, script, (rl89_u32)(sizeof(script) - 1U))) return 2;
    anim = rl89_find_animation(&rl, "flame", "loose");
    if (anim == RL89_INVALID_ID || !rl89_to_imagesequencer89(&rl, anim, &seqs, &loose)) return 3;
    player = is89_player_create(&seqs);
    if (player == IS89_INVALID_ID || !is89_player_play(&seqs, player, loose)) return 4;
    if (!is89_apply_to_staticsprite89(&seqs, player, &sprite) || !ss89_sample(&sprite, &sample)) return 5;
    if (strcmp(sample.image_request, "fire_a") != 0) return 6;
    atlas = tc89_add_atlas(&tc, "sheet", "fire_sheet", 64U, 32U, 16U, 16U, 0U, 0U, 0U, 0U);
    if (atlas == TC89_INVALID_ID) return 7;
    if (!tc89_strip_to_imagesequencer89(&tc, atlas, 0U, 1U, 1, 0, 4U, 33U, "gridburn", &seqs, &atlas_seq)) return 8;
    if (!is89_player_play(&seqs, player, atlas_seq)) return 9;
    if (!is89_apply_to_staticsprite89(&seqs, player, &sprite) || !ss89_sample(&sprite, &sample)) return 10;
    if (strcmp(sample.image_request, "fire_sheet") != 0 || !sample.source_enabled) return 11;
    if (sample.source.x != 0 || sample.source.y != 16 || sample.source.w != 16 || sample.source.h != 16) return 12;
    printf("Sprite microvendors89 integration PASS\n");
    return 0;
}
