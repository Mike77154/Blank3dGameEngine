#include "imagesequencer89.h"

int main(void)
{
    ImageSequencer89 seqs;
    is89_id clip;
    is89_id player;
    is89_init(&seqs);
    clip = is89_sequence_begin(&seqs, "burn");
    is89_sequence_set_loop(&seqs, clip, IS89_LOOP_FORWARD);
    is89_sequence_add_frame(&seqs, clip, "fire_001", 33U);
    is89_sequence_add_frame(&seqs, clip, "fire_002", 33U);
    player = is89_player_create(&seqs);
    is89_player_play(&seqs, player, clip);
    is89_player_step(&seqs, player, 16U);
    /* Host resolves and renders is89_player_current_frame(). */
    return 0;
}
