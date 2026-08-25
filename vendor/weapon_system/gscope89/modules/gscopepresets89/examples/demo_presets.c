#include <stdio.h>
#include "../include/gscopepresets89.h"

static unsigned long command_count = 0;

static void count_cmd(void *user, const gsp89_draw_cmd *cmd)
{
    (void)user;
    if (cmd->kind != GSP89_CMD_NONE) command_count += 1UL;
}

int main(void)
{
    gsp89_painter painter;
    gsv89_palette palette;
    const gsvp89_preset *pso;
    const gsvp89_preset *psg;
    const gsvp89_preset *rpg;

    gsvp89_set_catalog_path("config/reticles/catalog.ini");
    gsp89_painter_init(&painter, 800, 800, count_cmd, 0);

    pso = gsvp89_get(GSVP89_SVD_PSO1_DRAGUNOV);
    psg = gsvp89_find("psg1_hensoldt_6x42");
    rpg = gsvp89_get(GSVP89_RPG7_PGO7);
    if (!pso || !psg || !rpg) return 1;

    gsvp89_default_palette(pso, &palette);
    gsv89_palette_set_part(&palette, GSV89_PART_PRIMARY,
                           gsp89_rgba(48, 255, 96, 255),
                           gsp89_rgba(0, 0, 0, 255),
                           1, 1, 10, GSP89_BLEND_ALPHA,
                           GSP89_FLAG_OUTLINE, 1);
    gsvp89_emit(&painter, pso, &palette, 255);
    gsvp89_emit(&painter, psg, 0, 255);
    gsvp89_emit(&painter, rpg, 0, 255);

    printf("presets=%d commands=%lu dragunov_shapes=%d psg1_shapes=%d rpg_shapes=%d\n",
           gsvp89_count(), command_count,
           pso->shape_count, psg->shape_count, rpg->shape_count);
    return gsvp89_count() == GSVP89_PRESET_COUNT ? 0 : 1;
}
