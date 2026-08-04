#ifndef GCROSSHAIR_CORE89_H
#define GCROSSHAIR_CORE89_H

#include "gcrosshair89_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GC89_CORE_ABI_VERSION 3

#define GC89_ANIM_HOLD   0
#define GC89_ANIM_RETURN 1

typedef struct GC89_Core {
    GC89_Fixed scale_fx;
    GC89_Fixed target_scale_fx;
    GC89_Fixed neutral_scale_fx;
    GC89_Fixed micro_scale_fx;
    GC89_Fixed maxi_scale_fx;
    GC89_Fixed speed_fx_per_tick;
    int auto_return;
    int visible;
} GC89_Core;

typedef void (*GC89_DrawLineFn)(void *user,
                                int x0, int y0, int x1, int y1,
                                int thickness_px,
                                unsigned long color_rgba);

typedef void (*GC89_DrawDotFn)(void *user,
                               int center_x, int center_y,
                               int diameter_px,
                               unsigned long color_rgba);

typedef void (*GC89_DrawImageFn)(void *user,
                                 int image_id,
                                 int x, int y,
                                 int width, int height,
                                 unsigned long tint_rgba);

typedef struct GC89_DrawCallbacks {
    GC89_DrawLineFn draw_line;
    GC89_DrawDotFn draw_dot;
    GC89_DrawImageFn draw_image;
} GC89_DrawCallbacks;

typedef struct GC89_DrawResult {
    int center_x;
    int center_y;
    int emitted_primitives;
} GC89_DrawResult;

void gc89_core_init(GC89_Core *core);
void gc89_core_set_ranges(GC89_Core *core,
                          GC89_Fixed micro_scale_fx,
                          GC89_Fixed neutral_scale_fx,
                          GC89_Fixed maxi_scale_fx,
                          GC89_Fixed speed_fx_per_tick);
void gc89_core_set_visible(GC89_Core *core, int visible);
void gc89_core_set_scale(GC89_Core *core, GC89_Fixed scale_fx);
void gc89_core_trigger_micro(GC89_Core *core, int auto_return);
void gc89_core_trigger_maxi(GC89_Core *core, int auto_return);
void gc89_core_trigger_neutral(GC89_Core *core);
void gc89_core_update(GC89_Core *core, unsigned long ticks);

int gc89_core_draw(const GC89_Core *core,
                   int screen_width,
                   int screen_height,
                   const GC89_DrawSpec *spec,
                   const GC89_DrawCallbacks *callbacks,
                   void *user,
                   GC89_DrawResult *result);

#ifdef __cplusplus
}
#endif

#endif
