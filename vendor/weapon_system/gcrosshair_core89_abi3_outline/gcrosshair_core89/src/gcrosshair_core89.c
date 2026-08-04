#include "gcrosshair_core89.h"

static GC89_Fixed gc89_fx_mul(GC89_Fixed a, GC89_Fixed b)
{
    unsigned long ua;
    unsigned long ub;
    unsigned long ah;
    unsigned long al;
    unsigned long bh;
    unsigned long bl;
    unsigned long out;
    int negative;

    negative = 0;
    if (a < 0) {
        ua = (unsigned long)(-a);
        negative = !negative;
    } else {
        ua = (unsigned long)a;
    }
    if (b < 0) {
        ub = (unsigned long)(-b);
        negative = !negative;
    } else {
        ub = (unsigned long)b;
    }

    ah = ua >> 16;
    al = ua & 65535UL;
    bh = ub >> 16;
    bl = ub & 65535UL;
    out = (ah * bh << 16) + (ah * bl) + (al * bh) + ((al * bl) >> 16);

    if (negative) return -(GC89_Fixed)out;
    return (GC89_Fixed)out;
}

static int gc89_fx_to_int_round(GC89_Fixed value)
{
    if (value < 0) {
        return -(int)(((-value) + GC89_FX_HALF) >> GC89_FX_SHIFT);
    }
    return (int)((value + GC89_FX_HALF) >> GC89_FX_SHIFT);
}

static int gc89_scaled_px(GC89_Fixed value_fx, GC89_Fixed scale_fx, int minimum)
{
    int value;
    value = gc89_fx_to_int_round(gc89_fx_mul(value_fx, scale_fx));
    if (value < minimum) value = minimum;
    return value;
}

void gc89_core_init(GC89_Core *core)
{
    if (!core) return;
    core->scale_fx = GC89_FX_ONE;
    core->target_scale_fx = GC89_FX_ONE;
    core->neutral_scale_fx = GC89_FX_ONE;
    core->micro_scale_fx = 57672L;
    core->maxi_scale_fx = 88474L;
    core->speed_fx_per_tick = 3932L;
    core->auto_return = 0;
    core->visible = 1;
}

void gc89_core_set_ranges(GC89_Core *core,
                          GC89_Fixed micro_scale_fx,
                          GC89_Fixed neutral_scale_fx,
                          GC89_Fixed maxi_scale_fx,
                          GC89_Fixed speed_fx_per_tick)
{
    if (!core) return;
    if (micro_scale_fx <= 0) micro_scale_fx = 1;
    if (neutral_scale_fx < micro_scale_fx) neutral_scale_fx = micro_scale_fx;
    if (maxi_scale_fx < neutral_scale_fx) maxi_scale_fx = neutral_scale_fx;
    if (speed_fx_per_tick <= 0) speed_fx_per_tick = 1;
    core->micro_scale_fx = micro_scale_fx;
    core->neutral_scale_fx = neutral_scale_fx;
    core->maxi_scale_fx = maxi_scale_fx;
    core->speed_fx_per_tick = speed_fx_per_tick;
    if (core->scale_fx < micro_scale_fx) core->scale_fx = micro_scale_fx;
    if (core->scale_fx > maxi_scale_fx) core->scale_fx = maxi_scale_fx;
}

void gc89_core_set_visible(GC89_Core *core, int visible)
{
    if (!core) return;
    core->visible = visible ? 1 : 0;
}

void gc89_core_set_scale(GC89_Core *core, GC89_Fixed scale_fx)
{
    if (!core) return;
    if (scale_fx < core->micro_scale_fx) scale_fx = core->micro_scale_fx;
    if (scale_fx > core->maxi_scale_fx) scale_fx = core->maxi_scale_fx;
    core->scale_fx = scale_fx;
    core->target_scale_fx = scale_fx;
    core->auto_return = 0;
}

void gc89_core_trigger_micro(GC89_Core *core, int auto_return)
{
    if (!core) return;
    core->target_scale_fx = core->micro_scale_fx;
    core->auto_return = auto_return ? 1 : 0;
}

void gc89_core_trigger_maxi(GC89_Core *core, int auto_return)
{
    if (!core) return;
    core->target_scale_fx = core->maxi_scale_fx;
    core->auto_return = auto_return ? 1 : 0;
}

void gc89_core_trigger_neutral(GC89_Core *core)
{
    if (!core) return;
    core->target_scale_fx = core->neutral_scale_fx;
    core->auto_return = 0;
}

void gc89_core_update(GC89_Core *core, unsigned long ticks)
{
    GC89_Fixed step;
    GC89_Fixed distance;

    if (!core || ticks == 0UL) return;
    if (ticks > 4096UL) ticks = 4096UL;
    step = core->speed_fx_per_tick * (GC89_Fixed)ticks;
    if (step < 1) step = 1;

    if (core->scale_fx < core->target_scale_fx) {
        distance = core->target_scale_fx - core->scale_fx;
        if (step >= distance) core->scale_fx = core->target_scale_fx;
        else core->scale_fx += step;
    } else if (core->scale_fx > core->target_scale_fx) {
        distance = core->scale_fx - core->target_scale_fx;
        if (step >= distance) core->scale_fx = core->target_scale_fx;
        else core->scale_fx -= step;
    }

    if (core->scale_fx == core->target_scale_fx && core->auto_return) {
        core->target_scale_fx = core->neutral_scale_fx;
        core->auto_return = 0;
    }
}


static const GC89_Fixed gc89_sin_deg_lut[91] = {
    0L,
    1144L,
    2287L,
    3430L,
    4572L,
    5712L,
    6850L,
    7987L,
    9121L,
    10252L,
    11380L,
    12505L,
    13626L,
    14742L,
    15855L,
    16962L,
    18064L,
    19161L,
    20252L,
    21336L,
    22415L,
    23486L,
    24550L,
    25607L,
    26656L,
    27697L,
    28729L,
    29753L,
    30767L,
    31772L,
    32768L,
    33754L,
    34729L,
    35693L,
    36647L,
    37590L,
    38521L,
    39441L,
    40348L,
    41243L,
    42126L,
    42995L,
    43852L,
    44695L,
    45525L,
    46341L,
    47143L,
    47930L,
    48703L,
    49461L,
    50203L,
    50931L,
    51643L,
    52339L,
    53020L,
    53684L,
    54332L,
    54963L,
    55578L,
    56175L,
    56756L,
    57319L,
    57865L,
    58393L,
    58903L,
    59396L,
    59870L,
    60326L,
    60764L,
    61183L,
    61584L,
    61966L,
    62328L,
    62672L,
    62997L,
    63303L,
    63589L,
    63856L,
    64104L,
    64332L,
    64540L,
    64729L,
    64898L,
    65048L,
    65177L,
    65287L,
    65376L,
    65446L,
    65496L,
    65526L,
    65536L
};

static const GC89_Fixed gc89_circle32_x[32] = {
    0L,
    12785L,
    25080L,
    36410L,
    46341L,
    54491L,
    60547L,
    64277L,
    65536L,
    64277L,
    60547L,
    54491L,
    46341L,
    36410L,
    25080L,
    12785L,
    0L,
    -12785L,
    -25080L,
    -36410L,
    -46341L,
    -54491L,
    -60547L,
    -64277L,
    -65536L,
    -64277L,
    -60547L,
    -54491L,
    -46341L,
    -36410L,
    -25080L,
    -12785L
};
static const GC89_Fixed gc89_circle32_y[32] = {
    -65536L,
    -64277L,
    -60547L,
    -54491L,
    -46341L,
    -36410L,
    -25080L,
    -12785L,
    0L,
    12785L,
    25080L,
    36410L,
    46341L,
    54491L,
    60547L,
    64277L,
    65536L,
    64277L,
    60547L,
    54491L,
    46341L,
    36410L,
    25080L,
    12785L,
    0L,
    -12785L,
    -25080L,
    -36410L,
    -46341L,
    -54491L,
    -60547L,
    -64277L
};

static GC89_Fixed gc89_abs_fx(GC89_Fixed v)
{
    return v < 0 ? -v : v;
}

static int gc89_abs_int(int v)
{
    return v < 0 ? -v : v;
}

static GC89_Fixed gc89_sin_deg_fx(GC89_Fixed deg_fx)
{
    int deg;
    int quadrant;
    int local;
    GC89_Fixed value;

    deg = gc89_fx_to_int_round(deg_fx);
    deg %= 360;
    if (deg < 0) deg += 360;
    quadrant = deg / 90;
    local = deg % 90;
    if (quadrant == 0) value = gc89_sin_deg_lut[local];
    else if (quadrant == 1) value = gc89_sin_deg_lut[90 - local];
    else if (quadrant == 2) value = -gc89_sin_deg_lut[local];
    else value = -gc89_sin_deg_lut[90 - local];
    return value;
}

static GC89_Fixed gc89_cos_deg_fx(GC89_Fixed deg_fx)
{
    return gc89_sin_deg_fx(deg_fx + GC89_FX_FROM_INT(90));
}

static void gc89_rotate_point(GC89_Fixed x_fx,
                              GC89_Fixed y_fx,
                              GC89_Fixed rotation_deg_fx,
                              GC89_Fixed *out_x_fx,
                              GC89_Fixed *out_y_fx)
{
    GC89_Fixed s;
    GC89_Fixed c;
    s = gc89_sin_deg_fx(rotation_deg_fx);
    c = gc89_cos_deg_fx(rotation_deg_fx);
    *out_x_fx = gc89_fx_mul(x_fx, c) - gc89_fx_mul(y_fx, s);
    *out_y_fx = gc89_fx_mul(x_fx, s) + gc89_fx_mul(y_fx, c);
}

static void gc89_point_to_screen(int cx, int cy,
                                 GC89_Fixed x_fx, GC89_Fixed y_fx,
                                 GC89_Fixed scale_fx,
                                 GC89_Fixed rotation_deg_fx,
                                 int *out_x, int *out_y)
{
    GC89_Fixed rx;
    GC89_Fixed ry;
    gc89_rotate_point(x_fx, y_fx, rotation_deg_fx, &rx, &ry);
    *out_x = cx + gc89_fx_to_int_round(gc89_fx_mul(rx, scale_fx));
    *out_y = cy + gc89_fx_to_int_round(gc89_fx_mul(ry, scale_fx));
}

static int gc89_emit_line(const GC89_DrawCallbacks *callbacks,
                          void *user,
                          int x0, int y0, int x1, int y1,
                          int thickness_px,
                          unsigned long color_rgba)
{
    if (!callbacks->draw_line) return 0;
    callbacks->draw_line(user, x0, y0, x1, y1,
                         thickness_px, color_rgba);
    return 1;
}

static int gc89_emit_broken_line(const GC89_DrawCallbacks *callbacks,
                                 void *user,
                                 int x0, int y0, int x1, int y1,
                                 int thickness_px,
                                 unsigned long color_rgba,
                                 int break_px)
{
    int dx;
    int dy;
    int length;
    int half_gap;
    int left_num;
    int right_num;
    int ax;
    int ay;
    int bx;
    int by;
    int emitted;

    if (break_px <= 0) {
        return gc89_emit_line(callbacks, user, x0, y0, x1, y1,
                              thickness_px, color_rgba);
    }
    dx = x1 - x0;
    dy = y1 - y0;
    length = gc89_abs_int(dx);
    if (gc89_abs_int(dy) > length) length = gc89_abs_int(dy);
    if (length <= 1 || break_px >= length) return 0;
    half_gap = break_px / 2;
    left_num = length / 2 - half_gap;
    right_num = length / 2 + (break_px - half_gap);
    if (left_num <= 0 || right_num >= length) return 0;
    ax = x0 + (dx * left_num) / length;
    ay = y0 + (dy * left_num) / length;
    bx = x0 + (dx * right_num) / length;
    by = y0 + (dy * right_num) / length;
    emitted = 0;
    emitted += gc89_emit_line(callbacks, user, x0, y0, ax, ay,
                              thickness_px, color_rgba);
    emitted += gc89_emit_line(callbacks, user, bx, by, x1, y1,
                              thickness_px, color_rgba);
    return emitted;
}

static int gc89_emit_shape_segment(const GC89_DrawCallbacks *callbacks,
                                   void *user,
                                   int cx, int cy,
                                   GC89_Fixed x0_fx, GC89_Fixed y0_fx,
                                   GC89_Fixed x1_fx, GC89_Fixed y1_fx,
                                   GC89_Fixed scale_fx,
                                   GC89_Fixed rotation_deg_fx,
                                   int thickness_px,
                                   unsigned long color_rgba,
                                   int break_px)
{
    int x0;
    int y0;
    int x1;
    int y1;
    gc89_point_to_screen(cx, cy, x0_fx, y0_fx, scale_fx,
                         rotation_deg_fx, &x0, &y0);
    gc89_point_to_screen(cx, cy, x1_fx, y1_fx, scale_fx,
                         rotation_deg_fx, &x1, &y1);
    return gc89_emit_broken_line(callbacks, user, x0, y0, x1, y1,
                                 thickness_px, color_rgba, break_px);
}

static int gc89_draw_cross(const GC89_Core *core,
                           const GC89_DrawSpec *spec,
                           const GC89_DrawCallbacks *callbacks,
                           void *user,
                           int cx, int cy,
                           int thick)
{
    int gap;
    int arm;
    int emitted;
    gap = gc89_scaled_px(spec->gap_fx, core->scale_fx, 0);
    arm = gc89_scaled_px(spec->arm_length_fx, core->scale_fx, 1);
    emitted = 0;
    if ((spec->arm_mask & GC89_ARM_LEFT) != 0)
        emitted += gc89_emit_line(callbacks, user,
                                  cx - gap - arm, cy, cx - gap, cy,
                                  thick, spec->color_rgba);
    if ((spec->arm_mask & GC89_ARM_RIGHT) != 0)
        emitted += gc89_emit_line(callbacks, user,
                                  cx + gap, cy, cx + gap + arm, cy,
                                  thick, spec->color_rgba);
    if ((spec->arm_mask & GC89_ARM_UP) != 0)
        emitted += gc89_emit_line(callbacks, user,
                                  cx, cy - gap - arm, cx, cy - gap,
                                  thick, spec->color_rgba);
    if ((spec->arm_mask & GC89_ARM_DOWN) != 0)
        emitted += gc89_emit_line(callbacks, user,
                                  cx, cy + gap, cx, cy + gap + arm,
                                  thick, spec->color_rgba);
    return emitted;
}

static int gc89_draw_circle(const GC89_Core *core,
                            const GC89_DrawSpec *spec,
                            const GC89_DrawCallbacks *callbacks,
                            void *user,
                            int cx, int cy,
                            int thick,
                            int break_px)
{
    int octant;
    int edge;
    int i0;
    int i1;
    int emitted;
    GC89_Fixed x0;
    GC89_Fixed y0;
    GC89_Fixed x1;
    GC89_Fixed y1;
    int local_break;

    emitted = 0;
    for (octant = 0; octant < 8; ++octant) {
        if ((spec->shape_segment_mask & (1 << octant)) == 0) continue;
        for (edge = 0; edge < 4; ++edge) {
            i0 = octant * 4 + edge;
            i1 = (i0 + 1) & 31;
            if (break_px > 0 && (edge == 1 || edge == 2)) continue;
            x0 = gc89_fx_mul(spec->shape_radius_x_fx, gc89_circle32_x[i0]);
            y0 = gc89_fx_mul(spec->shape_radius_y_fx, gc89_circle32_y[i0]);
            x1 = gc89_fx_mul(spec->shape_radius_x_fx, gc89_circle32_x[i1]);
            y1 = gc89_fx_mul(spec->shape_radius_y_fx, gc89_circle32_y[i1]);
            local_break = 0;
            emitted += gc89_emit_shape_segment(callbacks, user, cx, cy,
                                                x0, y0, x1, y1,
                                                core->scale_fx,
                                                spec->shape_rotation_deg_fx,
                                                thick, spec->color_rgba,
                                                local_break);
        }
    }
    return emitted;
}

static int gc89_draw_polygon(const GC89_Core *core,
                             const GC89_DrawSpec *spec,
                             const GC89_DrawCallbacks *callbacks,
                             void *user,
                             int cx, int cy,
                             int thick,
                             int break_px,
                             const GC89_Fixed *xs,
                             const GC89_Fixed *ys,
                             int count)
{
    int i;
    int next;
    int emitted;
    emitted = 0;
    for (i = 0; i < count; ++i) {
        if ((spec->shape_segment_mask & (1 << i)) == 0) continue;
        next = (i + 1) % count;
        emitted += gc89_emit_shape_segment(callbacks, user, cx, cy,
                                            xs[i], ys[i], xs[next], ys[next],
                                            core->scale_fx,
                                            spec->shape_rotation_deg_fx,
                                            thick, spec->color_rgba,
                                            break_px);
    }
    return emitted;
}

static int gc89_draw_chevrons(const GC89_Core *core,
                              const GC89_DrawSpec *spec,
                              const GC89_DrawCallbacks *callbacks,
                              void *user,
                              int cx, int cy,
                              int thick,
                              int break_px)
{
    GC89_Fixed rx;
    GC89_Fixed ry;
    GC89_Fixed d;
    int emitted;
    rx = spec->shape_radius_x_fx;
    ry = spec->shape_radius_y_fx;
    d = spec->shape_depth_fx;
    if (d <= 0) d = rx < ry ? rx : ry;
    emitted = 0;
    if ((spec->shape_direction_mask & GC89_DIRECTION_UP) != 0) {
        emitted += gc89_emit_shape_segment(callbacks, user, cx, cy,
                    -rx, -ry + d, 0, -ry, core->scale_fx,
                    spec->shape_rotation_deg_fx, thick, spec->color_rgba, break_px);
        emitted += gc89_emit_shape_segment(callbacks, user, cx, cy,
                    0, -ry, rx, -ry + d, core->scale_fx,
                    spec->shape_rotation_deg_fx, thick, spec->color_rgba, break_px);
    }
    if ((spec->shape_direction_mask & GC89_DIRECTION_DOWN) != 0) {
        emitted += gc89_emit_shape_segment(callbacks, user, cx, cy,
                    -rx, ry - d, 0, ry, core->scale_fx,
                    spec->shape_rotation_deg_fx, thick, spec->color_rgba, break_px);
        emitted += gc89_emit_shape_segment(callbacks, user, cx, cy,
                    0, ry, rx, ry - d, core->scale_fx,
                    spec->shape_rotation_deg_fx, thick, spec->color_rgba, break_px);
    }
    if ((spec->shape_direction_mask & GC89_DIRECTION_LEFT) != 0) {
        emitted += gc89_emit_shape_segment(callbacks, user, cx, cy,
                    -rx + d, -ry, -rx, 0, core->scale_fx,
                    spec->shape_rotation_deg_fx, thick, spec->color_rgba, break_px);
        emitted += gc89_emit_shape_segment(callbacks, user, cx, cy,
                    -rx, 0, -rx + d, ry, core->scale_fx,
                    spec->shape_rotation_deg_fx, thick, spec->color_rgba, break_px);
    }
    if ((spec->shape_direction_mask & GC89_DIRECTION_RIGHT) != 0) {
        emitted += gc89_emit_shape_segment(callbacks, user, cx, cy,
                    rx - d, -ry, rx, 0, core->scale_fx,
                    spec->shape_rotation_deg_fx, thick, spec->color_rgba, break_px);
        emitted += gc89_emit_shape_segment(callbacks, user, cx, cy,
                    rx, 0, rx - d, ry, core->scale_fx,
                    spec->shape_rotation_deg_fx, thick, spec->color_rgba, break_px);
    }
    return emitted;
}

static int gc89_draw_brackets(const GC89_Core *core,
                              const GC89_DrawSpec *spec,
                              const GC89_DrawCallbacks *callbacks,
                              void *user,
                              int cx, int cy,
                              int thick,
                              int break_px)
{
    GC89_Fixed rx;
    GC89_Fixed ry;
    GC89_Fixed d;
    int emitted;
    rx = spec->shape_radius_x_fx;
    ry = spec->shape_radius_y_fx;
    d = spec->shape_depth_fx;
    if (d < 0) d = 0;
    if (d > rx) d = rx;
    if (d > ry) d = ry;
    emitted = 0;
    if ((spec->shape_direction_mask & GC89_DIRECTION_LEFT) != 0) {
        emitted += gc89_emit_shape_segment(callbacks, user, cx, cy,
                    -rx, -ry, -rx, ry, core->scale_fx,
                    spec->shape_rotation_deg_fx, thick, spec->color_rgba, break_px);
        emitted += gc89_emit_shape_segment(callbacks, user, cx, cy,
                    -rx, -ry, -rx + d, -ry, core->scale_fx,
                    spec->shape_rotation_deg_fx, thick, spec->color_rgba, 0);
        emitted += gc89_emit_shape_segment(callbacks, user, cx, cy,
                    -rx, ry, -rx + d, ry, core->scale_fx,
                    spec->shape_rotation_deg_fx, thick, spec->color_rgba, 0);
    }
    if ((spec->shape_direction_mask & GC89_DIRECTION_RIGHT) != 0) {
        emitted += gc89_emit_shape_segment(callbacks, user, cx, cy,
                    rx, -ry, rx, ry, core->scale_fx,
                    spec->shape_rotation_deg_fx, thick, spec->color_rgba, break_px);
        emitted += gc89_emit_shape_segment(callbacks, user, cx, cy,
                    rx, -ry, rx - d, -ry, core->scale_fx,
                    spec->shape_rotation_deg_fx, thick, spec->color_rgba, 0);
        emitted += gc89_emit_shape_segment(callbacks, user, cx, cy,
                    rx, ry, rx - d, ry, core->scale_fx,
                    spec->shape_rotation_deg_fx, thick, spec->color_rgba, 0);
    }
    if ((spec->shape_direction_mask & GC89_DIRECTION_UP) != 0) {
        emitted += gc89_emit_shape_segment(callbacks, user, cx, cy,
                    -rx, -ry, rx, -ry, core->scale_fx,
                    spec->shape_rotation_deg_fx, thick, spec->color_rgba, break_px);
        emitted += gc89_emit_shape_segment(callbacks, user, cx, cy,
                    -rx, -ry, -rx, -ry + d, core->scale_fx,
                    spec->shape_rotation_deg_fx, thick, spec->color_rgba, 0);
        emitted += gc89_emit_shape_segment(callbacks, user, cx, cy,
                    rx, -ry, rx, -ry + d, core->scale_fx,
                    spec->shape_rotation_deg_fx, thick, spec->color_rgba, 0);
    }
    if ((spec->shape_direction_mask & GC89_DIRECTION_DOWN) != 0) {
        emitted += gc89_emit_shape_segment(callbacks, user, cx, cy,
                    -rx, ry, rx, ry, core->scale_fx,
                    spec->shape_rotation_deg_fx, thick, spec->color_rgba, break_px);
        emitted += gc89_emit_shape_segment(callbacks, user, cx, cy,
                    -rx, ry, -rx, ry - d, core->scale_fx,
                    spec->shape_rotation_deg_fx, thick, spec->color_rgba, 0);
        emitted += gc89_emit_shape_segment(callbacks, user, cx, cy,
                    rx, ry, rx, ry - d, core->scale_fx,
                    spec->shape_rotation_deg_fx, thick, spec->color_rgba, 0);
    }
    return emitted;
}

static int gc89_draw_vector_shape(const GC89_Core *core,
                                  const GC89_DrawSpec *spec,
                                  const GC89_DrawCallbacks *callbacks,
                                  void *user,
                                  int cx, int cy)
{
    GC89_Fixed xs[6];
    GC89_Fixed ys[6];
    GC89_Fixed rx;
    GC89_Fixed ry;
    int thick;
    int break_px;
    int shape;

    thick = gc89_scaled_px(spec->thickness_fx, core->scale_fx, 1);
    break_px = gc89_scaled_px(spec->shape_break_fx, core->scale_fx, 0);
    shape = spec->shape_type;
    if (shape < 0 || shape >= GC89_SHAPE_COUNT) shape = GC89_SHAPE_CROSS;
    if (shape == GC89_SHAPE_CROSS)
        return gc89_draw_cross(core, spec, callbacks, user, cx, cy, thick);
    if (!callbacks->draw_line) return 0;

    rx = gc89_abs_fx(spec->shape_radius_x_fx);
    ry = gc89_abs_fx(spec->shape_radius_y_fx);
    if (rx <= 0) rx = gc89_abs_fx(spec->arm_length_fx);
    if (ry <= 0) ry = rx;

    if (shape == GC89_SHAPE_CIRCLE)
        return gc89_draw_circle(core, spec, callbacks, user, cx, cy,
                                thick, break_px);
    if (shape == GC89_SHAPE_SQUARE) {
        xs[0] = -rx; ys[0] = -ry;
        xs[1] = rx;  ys[1] = -ry;
        xs[2] = rx;  ys[2] = ry;
        xs[3] = -rx; ys[3] = ry;
        return gc89_draw_polygon(core, spec, callbacks, user, cx, cy,
                                 thick, break_px, xs, ys, 4);
    }
    if (shape == GC89_SHAPE_DIAMOND) {
        xs[0] = 0;   ys[0] = -ry;
        xs[1] = rx;  ys[1] = 0;
        xs[2] = 0;   ys[2] = ry;
        xs[3] = -rx; ys[3] = 0;
        return gc89_draw_polygon(core, spec, callbacks, user, cx, cy,
                                 thick, break_px, xs, ys, 4);
    }
    if (shape == GC89_SHAPE_CHEVRONS)
        return gc89_draw_chevrons(core, spec, callbacks, user, cx, cy,
                                  thick, break_px);
    if (shape == GC89_SHAPE_HEXAGON) {
        xs[0] = 0;          ys[0] = -ry;
        xs[1] = gc89_fx_mul(rx, 56756L); ys[1] = -ry / 2;
        xs[2] = gc89_fx_mul(rx, 56756L); ys[2] = ry / 2;
        xs[3] = 0;          ys[3] = ry;
        xs[4] = -(gc89_fx_mul(rx, 56756L)); ys[4] = ry / 2;
        xs[5] = -(gc89_fx_mul(rx, 56756L)); ys[5] = -ry / 2;
        return gc89_draw_polygon(core, spec, callbacks, user, cx, cy,
                                 thick, break_px, xs, ys, 6);
    }
    if (shape == GC89_SHAPE_BRACKETS)
        return gc89_draw_brackets(core, spec, callbacks, user, cx, cy,
                                  thick, break_px);
    xs[0] = 0;   ys[0] = -ry;
    xs[1] = rx;  ys[1] = ry;
    xs[2] = -rx; ys[2] = ry;
    return gc89_draw_polygon(core, spec, callbacks, user, cx, cy,
                             thick, break_px, xs, ys, 3);
}

int gc89_core_draw(const GC89_Core *core,
                   int screen_width,
                   int screen_height,
                   const GC89_DrawSpec *spec,
                   const GC89_DrawCallbacks *callbacks,
                   void *user,
                   GC89_DrawResult *result)
{
    int cx;
    int cy;
    int dot;
    int image_w;
    int image_h;
    int emitted;
    GC89_DrawSpec outline_spec;

    if (result) {
        result->center_x = 0;
        result->center_y = 0;
        result->emitted_primitives = 0;
    }
    if (!core || !spec || !callbacks) return 0;
    if (!core->visible || !spec->visible) return 0;
    if (screen_width <= 0 || screen_height <= 0) return 0;

    cx = screen_width / 2;
    cy = screen_height / 2;
    cx += gc89_fx_to_int_round(gc89_fx_mul(spec->center_offset_x_fx,
                                           core->scale_fx));
    cy += gc89_fx_to_int_round(gc89_fx_mul(spec->center_offset_y_fx,
                                           core->scale_fx));
    emitted = 0;

    if ((spec->draw_mode & GC89_DRAW_VECTOR) != 0 &&
        spec->outline_enabled && spec->outline_width_fx > 0) {
        outline_spec = *spec;
        outline_spec.thickness_fx = spec->thickness_fx +
            spec->outline_width_fx + spec->outline_width_fx;
        outline_spec.dot_size_fx = spec->dot_size_fx +
            spec->outline_width_fx + spec->outline_width_fx;
        outline_spec.color_rgba = spec->outline_color_rgba;
        outline_spec.outline_enabled = 0;

        emitted += gc89_draw_vector_shape(core, &outline_spec, callbacks,
                                          user, cx, cy);
        if (outline_spec.dot_enabled && callbacks->draw_dot) {
            dot = gc89_scaled_px(outline_spec.dot_size_fx,
                                 core->scale_fx, 1);
            callbacks->draw_dot(user, cx, cy, dot,
                                outline_spec.color_rgba);
            emitted++;
        }
    }

    if ((spec->draw_mode & GC89_DRAW_VECTOR) != 0)
        emitted += gc89_draw_vector_shape(core, spec, callbacks,
                                          user, cx, cy);

    if ((spec->draw_mode & GC89_DRAW_VECTOR) != 0 &&
        spec->dot_enabled && callbacks->draw_dot) {
        dot = gc89_scaled_px(spec->dot_size_fx, core->scale_fx, 1);
        callbacks->draw_dot(user, cx, cy, dot, spec->color_rgba);
        emitted++;
    }

    if ((spec->draw_mode & GC89_DRAW_IMAGE) != 0 && callbacks->draw_image) {
        image_w = gc89_scaled_px(spec->image_width_fx, core->scale_fx, 1);
        image_h = gc89_scaled_px(spec->image_height_fx, core->scale_fx, 1);
        callbacks->draw_image(user, spec->image_id,
                              cx - image_w / 2, cy - image_h / 2,
                              image_w, image_h, spec->image_tint_rgba);
        emitted++;
    }

    if (result) {
        result->center_x = cx;
        result->center_y = cy;
        result->emitted_primitives = emitted;
    }
    return emitted;
}
