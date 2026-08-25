#include <stdio.h>
#include "../include/gscopevector89.h"

static void print_cmd(void *user, const gsp89_draw_cmd *cmd)
{
    (void)user;
    printf("cmd=%d part=%d (%d,%d)-(%d,%d) outline=%d\n",
           cmd->kind, cmd->part_id,
           cmd->x0, cmd->y0, cmd->x1, cmd->y1,
           cmd->outline_px);
}

int main(void)
{
    gsp89_painter painter;
    gsv89_palette palette;
    gsv89_shape shapes[8];
    gsv89_builder builder;
    gsv89_shape s;

    gsp89_painter_init(&painter, 640, 480, print_cmd, 0);
    gsv89_palette_init(&palette);
    gsv89_palette_set_part(&palette, GSV89_PART_PRIMARY,
                           gsp89_rgba(255, 32, 32, 255),
                           gsp89_rgba(0, 0, 0, 255),
                           1, 2, 10, GSP89_BLEND_ALPHA,
                           GSP89_FLAG_OUTLINE, 1);

    gsv89_builder_init(&builder, shapes, 8);
    s = gsv89_cross(GSV89_PART_PRIMARY, 0, 0,
                    GSP89_NORM(8500), GSP89_NORM(8500),
                    GSP89_NORM(900));
    gsv89_builder_push(&builder, &s);
    s = gsv89_circle(GSV89_PART_CENTER, 0, 0, GSP89_NORM(500), 0);
    gsv89_builder_push(&builder, &s);
    s = gsv89_chevron(GSV89_PART_BDC, 0, GSP89_NORM(2500),
                      GSP89_NORM(700), GSP89_NORM(500),
                      GSV89_DIR_UP, 0);
    gsv89_builder_push(&builder, &s);
    s = gsv89_parenthesis(GSV89_PART_SECONDARY,
                          GSP89_NORM(-2500), 0,
                          GSP89_NORM(900), GSP89_NORM(2500),
                          GSV89_SIDE_LEFT);
    gsv89_builder_push(&builder, &s);
    s = gsv89_parenthesis(GSV89_PART_SECONDARY,
                          GSP89_NORM(2500), 0,
                          GSP89_NORM(900), GSP89_NORM(2500),
                          GSV89_SIDE_RIGHT);
    gsv89_builder_push(&builder, &s);

    gsv89_emit_shapes(&painter, builder.items, builder.count, &palette, 255);
    return builder.overflowed ? 1 : 0;
}
