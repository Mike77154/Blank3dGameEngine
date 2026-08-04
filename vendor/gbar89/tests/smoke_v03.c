#include <assert.h>
#include "gbar89.h"

static const GBar89_VectorPoint diamond[4] = {
    {500, 0}, {1000, 500}, {500, 1000}, {0, 500}
};

int main(void)
{
    GBar89_Meter m;
    GBar89_CommandBuffer cb;
    GBar89_RenderOps ops;

    gbar89_init(&m);
    assert(GBAR89_VERSION_MINOR >= 3);

    gbar89_set_rect(&m, 10, 10, 220, 40);
    gbar89_set_range(&m, 0, 100);
    gbar89_set_value(&m, 55);
    m.visual_value = 55;
    gbar89_set_mid_value(&m, 78);
    m.mid_visual_value = 78;
    gbar89_set_mid_speed(&m, 240);
    gbar89_set_mid_direction(&m, GBAR89_DIR_LEFT_TO_RIGHT);
    m.flags |= GBAR89_FLAG_DRAW_MID_VALUE |
               GBAR89_FLAG_DRAW_VECTOR_UNITS |
               GBAR89_FLAG_PIXEL_QUANTIZE;

    gbar89_set_frame(&m, GBAR89_FRAME_DOUBLE, 2, 1, 8);
    gbar89_set_outline(&m, 2, 0x000000FFUL);
    gbar89_set_background(&m, GBAR89_BG_GRID, 8, 1, 0x40608080UL);
    gbar89_set_fx_flags(&m, GBAR89_FX_OUTER_OUTLINE |
                            GBAR89_FX_DROP_SHADOW |
                            GBAR89_FX_GLOSS |
                            GBAR89_FX_PIXEL_CELLS);
    gbar89_set_pixel_size(&m, 6);
    gbar89_set_unit_vector(&m, diamond, 4, 1, 1, 6, 2, 75);

    assert(gbar89_mid_ratio(&m) > gbar89_current_ratio(&m));
    assert(m.style.frame_kind == GBAR89_FRAME_DOUBLE);
    assert(m.style.bg_kind == GBAR89_BG_GRID);
    assert(m.unit_count == 6);

    gbar89_command_buffer_init(&cb);
    ops = gbar89_command_buffer_make_ops(&cb);
    gbar89_draw(&m, &ops);

    assert(cb.command_count > 20);
    assert(cb.vertex_count >= 24);
    assert(cb.index_count >= 36);
    assert(gbar89_command_buffer_overflowed(&cb) == 0);

    gbar89_set_mid_value(&m, 20);
    gbar89_tick(&m, GBAR89_FIX_ONE / 4);
    assert(m.mid_visual_value < 78);

    return 0;
}
