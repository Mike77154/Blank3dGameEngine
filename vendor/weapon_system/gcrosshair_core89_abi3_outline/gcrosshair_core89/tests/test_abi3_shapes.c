#include <stdio.h>
#include <string.h>
#include "gcrosshair_core89.h"

typedef struct Sink { int lines; int dots; int images; } Sink;
static void line_cb(void *u,int x0,int y0,int x1,int y1,int t,unsigned long c)
{ Sink *s=(Sink*)u; s->lines++; (void)x0;(void)y0;(void)x1;(void)y1;(void)t;(void)c; }
static void dot_cb(void *u,int x,int y,int d,unsigned long c)
{ Sink *s=(Sink*)u; s->dots++; (void)x;(void)y;(void)d;(void)c; }
static void image_cb(void *u,int id,int x,int y,int w,int h,unsigned long c)
{ Sink *s=(Sink*)u; s->images++; (void)id;(void)x;(void)y;(void)w;(void)h;(void)c; }

static int run_shape(GC89_Core *core, GC89_DrawCallbacks *cb, int shape)
{
    GC89_DrawSpec spec; Sink sink; int n;
    memset(&spec,0,sizeof(spec)); memset(&sink,0,sizeof(sink));
    spec.visible=1; spec.draw_mode=GC89_DRAW_VECTOR; spec.shape_type=shape;
    spec.arm_mask=GC89_ARM_ALL; spec.shape_segment_mask=GC89_SEGMENT_ALL;
    spec.shape_direction_mask=GC89_DIRECTION_ALL;
    spec.gap_fx=GC89_FX_FROM_INT(4); spec.arm_length_fx=GC89_FX_FROM_INT(10);
    spec.thickness_fx=GC89_FX_FROM_INT(1);
    spec.shape_radius_x_fx=GC89_FX_FROM_INT(18);
    spec.shape_radius_y_fx=GC89_FX_FROM_INT(14);
    spec.shape_depth_fx=GC89_FX_FROM_INT(5);
    spec.shape_rotation_deg_fx=GC89_FX_FROM_INT(17);
    spec.shape_break_fx=GC89_FX_FROM_INT(2);
    spec.color_rgba=GC89_RGBA(255,255,255,255);
    n=gc89_core_draw(core,800,600,&spec,cb,&sink,0);
    if (n <= 0 || sink.lines <= 0) return 0;
    return 1;
}

int main(void)
{
    GC89_Core core; GC89_DrawCallbacks cb; int i;
    gc89_core_init(&core); cb.draw_line=line_cb; cb.draw_dot=dot_cb; cb.draw_image=image_cb;
    if (GC89_TYPES_ABI_VERSION != 3 || GC89_CORE_ABI_VERSION != 3) return 1;
    for (i=GC89_SHAPE_CROSS;i<GC89_SHAPE_COUNT;i++) if (!run_shape(&core,&cb,i)) return 10+i;
    puts("gcrosshair_core89 ABI3 shapes: OK");
    return 0;
}
