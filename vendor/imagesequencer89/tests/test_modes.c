#include "imagesequencer89.h"
#include <stdio.h>

static int frame_pos(const ImageSequencer89 *ctx, is89_id p)
{
    const IS89_Player *player;
    if (!ctx || p >= IS89_MAX_PLAYERS) return -1;
    player = &ctx->players[p];
    return (int)player->frame_pos;
}

int main(void)
{
    ImageSequencer89 ctx;
    is89_id seq;
    is89_id p;
    is89_init(&ctx);
    seq = is89_sequence_begin(&ctx, "ping");
    if (seq == IS89_INVALID_ID) return 2;
    is89_sequence_set_loop(&ctx, seq, IS89_LOOP_PINGPONG);
    if (is89_sequence_add_frame(&ctx, seq, "a", 10U) == IS89_INVALID_ID) return 3;
    if (is89_sequence_add_frame(&ctx, seq, "b", 10U) == IS89_INVALID_ID) return 4;
    if (is89_sequence_add_frame(&ctx, seq, "c", 10U) == IS89_INVALID_ID) return 5;
    p = is89_player_create(&ctx);
    if (!is89_player_play(&ctx, p, seq)) return 6;
    is89_player_step(&ctx, p, 10U); if (frame_pos(&ctx,p) != 1) return 7;
    is89_player_step(&ctx, p, 10U); if (frame_pos(&ctx,p) != 2) return 8;
    is89_player_step(&ctx, p, 10U); if (frame_pos(&ctx,p) != 1) return 9;
    is89_player_step(&ctx, p, 10U); if (frame_pos(&ctx,p) != 0) return 10;
    is89_player_step(&ctx, p, 10U); if (frame_pos(&ctx,p) != 1) return 11;
    printf("ImageSequencer89 modes PASS\n");
    return 0;
}
