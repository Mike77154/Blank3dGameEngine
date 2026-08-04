#include <stdio.h>
#include "../include/gscopepaint89.h"

static void print_cmd(void *user, const gsp89_draw_cmd *cmd)
{
    (void)user;
    printf("kind=%d part=%d layer=%d xy=(%d,%d)-(%d,%d) r=(%d,%d) rgba=%u,%u,%u,%u\n",
           cmd->kind,
           cmd->part_id,
           cmd->layer,
           cmd->x0,
           cmd->y0,
           cmd->x1,
           cmd->y1,
           cmd->radius_x,
           cmd->radius_y,
           (unsigned int)cmd->color.r,
           (unsigned int)cmd->color.g,
           (unsigned int)cmd->color.b,
           (unsigned int)cmd->color.a);
}

int main(void)
{
    gsp89_painter p;
    gsp89_style line;
    gsp89_style fill;

    gsp89_painter_init(&p, 640, 480, print_cmd, 0);
    line = gsp89_style_make(gsp89_rgba(255, 32, 32, 255),
                            gsp89_rgba(0, 0, 0, 255),
                            1, 2, 10, 1,
                            GSP89_BLEND_ALPHA,
                            GSP89_FLAG_OUTLINE);
    fill = gsp89_style_make(gsp89_rgba(32, 255, 64, 180),
                            gsp89_rgba(0, 0, 0, 255),
                            1, 1, 5, 2,
                            GSP89_BLEND_ALPHA,
                            GSP89_FLAG_FILLED | GSP89_FLAG_OUTLINE);

    gsp89_paint_line(&p, GSP89_NORM(-8000), 0, GSP89_NORM(8000), 0, &line);
    gsp89_paint_circle(&p, 0, 0, GSP89_NORM(2500), &fill);
    gsp89_paint_triangle(&p,
                         0, GSP89_NORM(-4000),
                         GSP89_NORM(-2500), GSP89_NORM(2000),
                         GSP89_NORM(2500), GSP89_NORM(2000),
                         &line);
    return 0;
}
