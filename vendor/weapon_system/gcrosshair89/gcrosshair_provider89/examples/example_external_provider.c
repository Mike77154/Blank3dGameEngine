#include <stdio.h>
#include <string.h>
#include "gcrosshair_provider89.h"

static int engine_vector(void *user,
                         const GC89P_VectorCommand *command,
                         int *out_emitted)
{
    (void)user;
    printf("HUD/vector provider: shape=%d center=%d,%d scale=%ld\n",
           command->spec->shape_type,
           command->center_x,
           command->center_y,
           command->scale_fx);

    /* A real HUD/vector library would render the whole logical shape here. */
    *out_emitted = 1;
    return GC89P_HANDLED;
}

int main(void)
{
    GC89_Core core;
    GC89_DrawSpec spec;
    GC89P_Runtime runtime;
    GC89P_VectorProvider vector;
    GC89P_DrawReport report;

    gc89_core_init(&core);
    memset(&spec, 0, sizeof(spec));
    spec.visible = 1;
    spec.draw_mode = GC89_DRAW_VECTOR;
    spec.shape_type = GC89_SHAPE_CIRCLE;
    spec.shape_segment_mask = GC89_SEGMENT_ALL;
    spec.shape_direction_mask = GC89_DIRECTION_ALL;
    spec.shape_radius_x_fx = GC89_FX_FROM_INT(16);
    spec.shape_radius_y_fx = GC89_FX_FROM_INT(16);
    spec.thickness_fx = GC89_FX_FROM_INT(2);
    spec.color_rgba = GC89_RGBA(0, 255, 128, 255);

    gc89p_runtime_init(&runtime);
    memset(&vector, 0, sizeof(vector));
    vector.draw_vector = engine_vector;
    gc89p_runtime_set_vector_provider(&runtime, &vector, 0);

    gc89p_draw(&runtime, &core, 1280, 720, &spec, &report);
    return 0;
}
