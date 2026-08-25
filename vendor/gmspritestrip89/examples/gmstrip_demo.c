#include "../include/gmspritestrip89.h"

int main(void)
{
    GMSpritestrip89 ctx;
    gmss89_id p;
    gmss89_init(&ctx);
    /* Hook your providers here. */
    gmss89_define_strip_from_path(&ctx, "assets/spr_x_walk_strip14.png", "default", 80U, GMSS89_LOOP_FORWARD);
    p = gmss89_player_create(&ctx);
    gmss89_player_play(&ctx, p, "spr_x_walk", "default");
    gmss89_player_set_position(&ctx, p, 160, 96);
    gmss89_player_step(&ctx, p, 16U);
    gmss89_player_render(&ctx, p);
    return 0;
}
