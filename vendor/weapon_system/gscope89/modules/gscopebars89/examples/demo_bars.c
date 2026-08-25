#include <stdio.h>
#include "../include/gscopebars89.h"

static void print_cmd(void *user, const gsp89_draw_cmd *cmd)
{
    (void)user;
    printf("kind=%d part=%d rect=(%d,%d)-(%d,%d)\n",
           cmd->kind, cmd->part_id,
           cmd->x0, cmd->y0, cmd->x1, cmd->y1);
}

int main(void)
{
    gsp89_painter painter;
    gsb89_channel channels[2];
    gsb89_bar bars[2];
    gsb89_layout layout;

    gsp89_painter_init(&painter, 640, 480, print_cmd, 0);
    channels[0].channel_id = GSB89_CHANNEL_BREATH;
    channels[0].value = 72;
    channels[0].minimum = 0;
    channels[0].maximum = 100;
    channels[0].visible = 1;
    channels[1].channel_id = GSB89_CHANNEL_STABILITY;
    channels[1].value = 45;
    channels[1].minimum = 0;
    channels[1].maximum = 100;
    channels[1].visible = 1;

    bars[0] = gsb89_bar_make(1, GSB89_CHANNEL_BREATH,
                             GSB89_BAR_LINEAR, GSB89_DIR_LEFT_TO_RIGHT,
                             GSP89_NORM(-5000), GSP89_NORM(8500),
                             GSP89_NORM(5000), GSP89_NORM(9000));
    bars[1] = gsb89_bar_make(2, GSB89_CHANNEL_STABILITY,
                             GSB89_BAR_SEGMENTED, GSB89_DIR_BOTTOM_TO_TOP,
                             GSP89_NORM(8200), GSP89_NORM(6000),
                             GSP89_NORM(8800), GSP89_NORM(-6000));
    bars[1].segment_count = 12;

    layout.bars = bars;
    layout.bar_count = 2;
    gsb89_emit_layout_from_array(&painter, &layout, channels, 2, 255);
    return 0;
}
