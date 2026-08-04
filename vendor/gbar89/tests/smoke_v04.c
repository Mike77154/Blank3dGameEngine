#include <assert.h>
#include "gbar89.h"

static const GBar89_VectorPoint diamond[4] = {
    {500, 0}, {1000, 500}, {500, 1000}, {0, 500}
};

int main(void)
{
    GBar89_Meter radial;
    GBar89_Meter vertical;
    GBar89_CommandBuffer cb;
    GBar89_RenderOps ops;

    assert(GBAR89_VERSION_MINOR == 4);

    gbar89_init(&radial);
    gbar89_set_kind(&radial, GBAR89_KIND_RADIAL_RING);
    gbar89_set_rect(&radial, 20, 20, 220, 220);
    gbar89_set_range(&radial, 0, 100);
    gbar89_set_value(&radial, 63);
    radial.visual_value = 63;
    radial.lag_value = 78;
    gbar89_set_mid_value(&radial, 71);
    radial.mid_visual_value = 71;
    gbar89_set_radial(&radial, -90, 300, 60, 64);
    gbar89_set_radial_style(&radial, 16, 3,
                            GBAR89_RADIAL_CAP_ROUND, 3, 82);
    gbar89_set_radial_phase(&radial, 11);
    gbar89_set_frame(&radial, GBAR89_FRAME_RAIL, 3, 2, 18);
    gbar89_set_background(&radial, GBAR89_BG_GRID, 18, 1,
                          0x4080A080UL);
    gbar89_set_pattern(&radial, GBAR89_PATTERN_CROSSHATCH, 12, 1);
    gbar89_set_outline(&radial, 2, 0x000000FFUL);
    gbar89_set_fx_flags(&radial, GBAR89_FX_OUTER_OUTLINE |
                                  GBAR89_FX_DROP_SHADOW |
                                  GBAR89_FX_EXTRUDE |
                                  GBAR89_FX_GLOSS |
                                  GBAR89_FX_INNER_SHADOW |
                                  GBAR89_FX_PIXEL_CELLS |
                                  GBAR89_FX_RADIAL_SPOKES |
                                  GBAR89_FX_RADIAL_RINGS |
                                  GBAR89_FX_RADIAL_TICKS |
                                  GBAR89_FX_RADIAL_SWEEP_HIGHLIGHT);
    gbar89_set_unit_vector(&radial, diamond, 4, 1, 1, 8, 1, 70);
    radial.flags |= GBAR89_FLAG_DAMAGE_LAG |
                    GBAR89_FLAG_DRAW_MID_VALUE |
                    GBAR89_FLAG_DRAW_PATTERN |
                    GBAR89_FLAG_DRAW_VECTOR_UNITS |
                    GBAR89_FLAG_DRAW_MARKERS;
    assert(gbar89_add_marker(&radial, 25) == GBAR89_OK);
    assert(gbar89_add_marker(&radial, 50) == GBAR89_OK);
    assert(gbar89_add_marker(&radial, 75) == GBAR89_OK);

    gbar89_command_buffer_init(&cb);
    ops = gbar89_command_buffer_make_ops(&cb);
    gbar89_draw(&radial, &ops);
    assert(cb.command_count > 20);
    assert(cb.vertex_count > 100);
    assert(cb.index_count > 150);
    assert(gbar89_command_buffer_overflowed(&cb) == 0);

    gbar89_init(&vertical);
    gbar89_set_kind(&vertical, GBAR89_KIND_SEGMENTED);
    gbar89_set_direction(&vertical, GBAR89_DIR_BOTTOM_TO_TOP);
    gbar89_set_rect(&vertical, 260, 20, 48, 240);
    gbar89_set_range(&vertical, 0, 100);
    gbar89_set_value(&vertical, 57);
    vertical.visual_value = 57;
    gbar89_set_segments(&vertical, 12, 3);
    gbar89_set_mid_value(&vertical, 70);
    vertical.mid_visual_value = 70;
    vertical.flags |= GBAR89_FLAG_DRAW_MID_VALUE |
                      GBAR89_FLAG_DRAW_PATTERN;
    gbar89_set_frame(&vertical, GBAR89_FRAME_BEVEL_OUT, 3, 1, 10);
    gbar89_set_background(&vertical, GBAR89_BG_CHECKER, 8, 1,
                          0x40608080UL);
    gbar89_set_fx_flags(&vertical, GBAR89_FX_OUTER_OUTLINE |
                                  GBAR89_FX_EXTRUDE |
                                  GBAR89_FX_GLOSS |
                                  GBAR89_FX_FILL_GRID |
                                  GBAR89_FX_PIXEL_CELLS);
    gbar89_command_buffer_init(&cb);
    gbar89_draw(&vertical, &ops);
    assert(cb.command_count > 20);
    assert(gbar89_command_buffer_overflowed(&cb) == 0);

    return 0;
}
