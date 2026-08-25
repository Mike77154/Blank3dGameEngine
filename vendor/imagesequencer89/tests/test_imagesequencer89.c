#include "imagesequencer89.h"
#include <stdio.h>
#include <string.h>

int main(void)
{
    ImageSequencer89 ctx;
    is89_id seq;
    is89_id player;
    const IS89_Frame *f;
    is89_init(&ctx);
    seq = is89_sequence_begin(&ctx, "fire");
    if (seq == IS89_INVALID_ID) return 2;
    if (!is89_sequence_set_loop(&ctx, seq, IS89_LOOP_FORWARD)) return 3;
    if (is89_sequence_add_frame(&ctx, seq, "fire_001", 30U) == IS89_INVALID_ID) return 4;
    if (is89_sequence_add_frame(&ctx, seq, "fire_002", 40U) == IS89_INVALID_ID) return 5;
    if (is89_sequence_add_frame_rect(&ctx, seq, "sheet", 16, 0, 16, 16, 50U) == IS89_INVALID_ID) return 6;
    player = is89_player_create(&ctx);
    if (player == IS89_INVALID_ID || !is89_player_play(&ctx, player, seq)) return 7;
    f = is89_player_current_frame(&ctx, player);
    if (!f || strcmp(f->image_request, "fire_001") != 0) return 8;
    is89_player_step(&ctx, player, 30U);
    f = is89_player_current_frame(&ctx, player);
    if (!f || strcmp(f->image_request, "fire_002") != 0) return 9;
    is89_player_step(&ctx, player, 40U);
    f = is89_player_current_frame(&ctx, player);
    if (!f || strcmp(f->image_request, "sheet") != 0 || !(f->flags & IS89_FRAME_SOURCE_RECT)) return 10;
    is89_player_step(&ctx, player, 50U);
    f = is89_player_current_frame(&ctx, player);
    if (!f || strcmp(f->image_request, "fire_001") != 0) return 11;
    printf("ImageSequencer89 PASS sequences=%u frames=%u\n", (unsigned)ctx.sequence_count, (unsigned)ctx.frame_count);
    return 0;
}
