#include <stdio.h>
#include "../include/gscoperaster89.h"

static void print_cmd(void *user, const gsp89_draw_cmd *cmd)
{
    (void)user;
    printf("kind=%d asset=%d rect=(%d,%d)-(%d,%d) uv=%d,%d,%d,%d\n",
           cmd->kind, cmd->asset_id,
           cmd->x0, cmd->y0, cmd->x1, cmd->y1,
           cmd->uv_x0, cmd->uv_y0, cmd->uv_x1, cmd->uv_y1);
}

int main(void)
{
    gsp89_painter painter;
    gsr89_layer layers[3];
    gsr89_stack stack;

    gsp89_painter_init(&painter, 800, 600, print_cmd, 0);
    layers[0] = gsr89_layer_make(10, 0, 1, 0, 0,
                                 GSP89_NORM(20000), GSP89_NORM(20000));
    layers[1] = gsr89_layer_make(11, 1, 2, 0, 0,
                                 GSP89_NORM(18000), GSP89_NORM(18000));
    layers[1].blend_mode = GSP89_BLEND_SCREEN;
    layers[2] = gsr89_layer_make(12, 2, 3, 0, 0,
                                 GSP89_NORM(18000), GSP89_NORM(18000));
    layers[2].tint = gsp89_rgba(64, 255, 64, 160);

    stack.layers = layers;
    stack.layer_count = 3;
    stack.clip_to_scope = 1;
    stack.clip_radius = GSP89_FX_ONE;
    gsr89_emit_stack(&painter, &stack, 255);
    return 0;
}
