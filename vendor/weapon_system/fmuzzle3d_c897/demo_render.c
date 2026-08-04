#include <stdio.h>
#include <string.h>

#include "fmuzzle89.h"

#define DEMO_WIDTH  640
#define DEMO_HEIGHT 400
#define DEMO_FRAMES 48
#define DEMO_SHOT_INTERVAL 8
#define DEMO_FOCAL_PIXELS 360
#define DEMO_CORE_SEGMENTS 16
#define DEMO_MAX_POLYGON 6

static unsigned char demo_framebuffer[DEMO_WIDTH * DEMO_HEIGHT * 3];

typedef struct DemoVec3
{
    FM89_Fixed x;
    FM89_Fixed y;
    FM89_Fixed z;
} DemoVec3;

typedef struct DemoPoint
{
    int x;
    int y;
    FM89_Fixed z;
    int valid;
} DemoPoint;

typedef struct DemoColor
{
    int r;
    int g;
    int b;
} DemoColor;

static int demo_clamp_int(int value, int minimum, int maximum)
{
    if (value < minimum)
    {
        return minimum;
    }
    if (value > maximum)
    {
        return maximum;
    }
    return value;
}

static int demo_abs_int(int value)
{
    return value < 0 ? -value : value;
}

static DemoColor demo_scale_color(int r, int g, int b, int brightness)
{
    DemoColor color;

    color.r = demo_clamp_int((r * brightness) >> FM89_FP_SHIFT, 0, 255);
    color.g = demo_clamp_int((g * brightness) >> FM89_FP_SHIFT, 0, 255);
    color.b = demo_clamp_int((b * brightness) >> FM89_FP_SHIFT, 0, 255);
    return color;
}

static void demo_put_pixel(int x, int y, DemoColor color)
{
    int index;

    if (x < 0 || x >= DEMO_WIDTH || y < 0 || y >= DEMO_HEIGHT)
    {
        return;
    }

    index = (y * DEMO_WIDTH + x) * 3;
    demo_framebuffer[index + 0] = (unsigned char)color.r;
    demo_framebuffer[index + 1] = (unsigned char)color.g;
    demo_framebuffer[index + 2] = (unsigned char)color.b;
}

static void demo_clear(void)
{
    int x;
    int y;
    int index;
    int vignette;

    for (y = 0; y < DEMO_HEIGHT; ++y)
    {
        for (x = 0; x < DEMO_WIDTH; ++x)
        {
            index = (y * DEMO_WIDTH + x) * 3;
            vignette = demo_abs_int(x - DEMO_WIDTH / 2)
                     + demo_abs_int(y - DEMO_HEIGHT / 2);
            vignette = demo_clamp_int(vignette / 46, 0, 10);
            demo_framebuffer[index + 0] = (unsigned char)(14 - vignette / 2);
            demo_framebuffer[index + 1] = (unsigned char)(18 - vignette / 2);
            demo_framebuffer[index + 2] = (unsigned char)(25 - vignette / 2);
        }
    }
}

static void demo_line(int x0, int y0, int x1, int y1, DemoColor color)
{
    int dx;
    int sx;
    int dy;
    int sy;
    int error;
    int e2;

    dx = demo_abs_int(x1 - x0);
    sx = x0 < x1 ? 1 : -1;
    dy = -demo_abs_int(y1 - y0);
    sy = y0 < y1 ? 1 : -1;
    error = dx + dy;

    for (;;)
    {
        demo_put_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1)
        {
            break;
        }
        e2 = 2 * error;
        if (e2 >= dy)
        {
            error += dy;
            x0 += sx;
        }
        if (e2 <= dx)
        {
            error += dx;
            y0 += sy;
        }
    }
}

static long demo_edge(int ax, int ay, int bx, int by, int px, int py)
{
    return (long)(px - ax) * (long)(by - ay)
         - (long)(py - ay) * (long)(bx - ax);
}

static void demo_triangle(int x0, int y0,
                          int x1, int y1,
                          int x2, int y2,
                          DemoColor color)
{
    int minimum_x;
    int maximum_x;
    int minimum_y;
    int maximum_y;
    int x;
    int y;
    long area;
    long e0;
    long e1;
    long e2;

    minimum_x = x0;
    if (x1 < minimum_x) minimum_x = x1;
    if (x2 < minimum_x) minimum_x = x2;
    maximum_x = x0;
    if (x1 > maximum_x) maximum_x = x1;
    if (x2 > maximum_x) maximum_x = x2;
    minimum_y = y0;
    if (y1 < minimum_y) minimum_y = y1;
    if (y2 < minimum_y) minimum_y = y2;
    maximum_y = y0;
    if (y1 > maximum_y) maximum_y = y1;
    if (y2 > maximum_y) maximum_y = y2;

    minimum_x = demo_clamp_int(minimum_x, 0, DEMO_WIDTH - 1);
    maximum_x = demo_clamp_int(maximum_x, 0, DEMO_WIDTH - 1);
    minimum_y = demo_clamp_int(minimum_y, 0, DEMO_HEIGHT - 1);
    maximum_y = demo_clamp_int(maximum_y, 0, DEMO_HEIGHT - 1);

    area = demo_edge(x0, y0, x1, y1, x2, y2);
    if (area == 0)
    {
        return;
    }

    for (y = minimum_y; y <= maximum_y; ++y)
    {
        for (x = minimum_x; x <= maximum_x; ++x)
        {
            e0 = demo_edge(x0, y0, x1, y1, x, y);
            e1 = demo_edge(x1, y1, x2, y2, x, y);
            e2 = demo_edge(x2, y2, x0, y0, x, y);

            if (area > 0)
            {
                if (e0 >= 0 && e1 >= 0 && e2 >= 0)
                {
                    demo_put_pixel(x, y, color);
                }
            }
            else
            {
                if (e0 <= 0 && e1 <= 0 && e2 <= 0)
                {
                    demo_put_pixel(x, y, color);
                }
            }
        }
    }
}

static void demo_draw_background_grid(void)
{
    DemoColor grid;
    DemoColor horizon;
    int x;
    int y;

    grid.r = 24;
    grid.g = 31;
    grid.b = 43;
    horizon.r = 40;
    horizon.g = 50;
    horizon.b = 66;

    for (x = 0; x < DEMO_WIDTH; x += 40)
    {
        demo_line(x, 0, x, DEMO_HEIGHT - 1, grid);
    }
    for (y = 0; y < DEMO_HEIGHT; y += 32)
    {
        demo_line(0, y, DEMO_WIDTH - 1, y, grid);
    }
    demo_line(0, DEMO_HEIGHT / 2, DEMO_WIDTH - 1, DEMO_HEIGHT / 2, horizon);
    demo_line(DEMO_WIDTH / 2, 0, DEMO_WIDTH / 2, DEMO_HEIGHT - 1, horizon);
}

static DemoVec3 demo_transform_local(const FM89_State *state, DemoVec3 local)
{
    DemoVec3 result;
    FM89_Fixed cp;
    FM89_Fixed sp;
    FM89_Fixed cy;
    FM89_Fixed sy;
    FM89_Fixed y1;
    FM89_Fixed z1;
    FM89_Fixed x2;
    FM89_Fixed z2;

    cp = fm89_cos(state->pitch);
    sp = fm89_sin(state->pitch);
    cy = fm89_cos(state->yaw);
    sy = fm89_sin(state->yaw);

    y1 = fm89_mul(local.y, cp) - fm89_mul(local.z, sp);
    z1 = fm89_mul(local.y, sp) + fm89_mul(local.z, cp);

    x2 = fm89_mul(local.x, cy) + fm89_mul(z1, sy);
    z2 = -fm89_mul(local.x, sy) + fm89_mul(z1, cy);

    result.x = x2 + state->center_x;
    result.y = y1 + state->center_y;
    result.z = z2 + state->center_z;
    return result;
}

static DemoPoint demo_project(DemoVec3 point)
{
    DemoPoint result;

    result.valid = 0;
    result.x = 0;
    result.y = 0;
    result.z = point.z;

    if (point.z < FM89_FP_ONE / 2)
    {
        return result;
    }

    result.x = DEMO_WIDTH / 2
             + (int)((point.x * DEMO_FOCAL_PIXELS) / point.z);
    result.y = DEMO_HEIGHT / 2
             - (int)((point.y * DEMO_FOCAL_PIXELS) / point.z);
    result.valid = 1;
    return result;
}

static DemoPoint demo_local_to_screen(const FM89_State *state, DemoVec3 local)
{
    return demo_project(demo_transform_local(state, local));
}

static void demo_draw_weapon_silhouette(const FM89_State *state)
{
    DemoVec3 rear_local;
    DemoPoint rear;
    DemoColor dark;
    DemoColor edge;
    DemoColor highlight;

    rear_local.x = 0;
    rear_local.y = 0;
    rear_local.z = 0;
    rear = demo_local_to_screen(state, rear_local);
    if (!rear.valid)
    {
        return;
    }

    dark.r = 27;
    dark.g = 30;
    dark.b = 36;
    edge.r = 56;
    edge.g = 62;
    edge.b = 72;
    highlight.r = 83;
    highlight.g = 90;
    highlight.b = 102;

    demo_triangle(rear.x + 4, rear.y - 11,
                  DEMO_WIDTH - 1, DEMO_HEIGHT - 142,
                  DEMO_WIDTH - 1, DEMO_HEIGHT - 47,
                  dark);
    demo_triangle(rear.x + 4, rear.y - 11,
                  DEMO_WIDTH - 1, DEMO_HEIGHT - 47,
                  rear.x - 8, rear.y + 13,
                  dark);
    demo_line(rear.x, rear.y - 8,
              DEMO_WIDTH - 1, DEMO_HEIGHT - 129,
              highlight);
    demo_line(rear.x - 7, rear.y + 13,
              DEMO_WIDTH - 1, DEMO_HEIGHT - 49,
              edge);
}

static DemoVec3 demo_fin_point(FM89_Fixed direction_x,
                               FM89_Fixed direction_y,
                               FM89_Fixed width_x,
                               FM89_Fixed width_y,
                               FM89_Fixed width_z,
                               FM89_Fixed radial,
                               FM89_Fixed width,
                               FM89_Fixed anchor_z)
{
    DemoVec3 point;

    point.x = fm89_mul(direction_x, radial) + fm89_mul(width_x, width);
    point.y = fm89_mul(direction_y, radial) + fm89_mul(width_y, width);
    point.z = anchor_z + fm89_mul(width_z, width);
    return point;
}

static int demo_build_fin_polygon(const FM89_State *state,
                                  const FM89_Fin *fin,
                                  FM89_Fixed inset,
                                  DemoVec3 *vertices)
{
    int angle;
    FM89_Fixed direction_x;
    FM89_Fixed direction_y;
    FM89_Fixed tangent_x;
    FM89_Fixed tangent_y;
    FM89_Fixed roll_cos;
    FM89_Fixed roll_sin;
    FM89_Fixed width_x;
    FM89_Fixed width_y;
    FM89_Fixed width_z;
    FM89_Fixed anchor_radius;
    FM89_Fixed anchor_z;
    FM89_Fixed length;
    FM89_Fixed half_width;
    FM89_Fixed outer_half_width;
    FM89_Fixed inner_radial;
    FM89_Fixed outer_radial;
    FM89_Fixed near_radial;
    FM89_Fixed far_radial;
    FM89_Fixed near_width;
    FM89_Fixed far_width;

    angle = (fin->angle + state->global_angle) & 255;
    direction_x = fm89_cos(angle);
    direction_y = fm89_sin(angle);
    tangent_x = -direction_y;
    tangent_y = direction_x;

    roll_cos = fm89_cos(fin->roll);
    roll_sin = fm89_sin(fin->roll);
    width_x = fm89_mul(tangent_x, roll_cos);
    width_y = fm89_mul(tangent_y, roll_cos);
    width_z = roll_sin;

    anchor_radius = fm89_mul(fin->anchor_radius, state->scale);
    anchor_z = fm89_mul(fin->anchor_z, state->scale);
    length = fm89_mul(fin->length, state->scale);
    half_width = fm89_mul(fin->half_width, state->scale);
    outer_half_width = fm89_mul(fin->outer_half_width, state->scale);

    if (inset > half_width - 10)
    {
        inset = half_width - 10;
    }
    if (inset < 0)
    {
        inset = 0;
    }

    inner_radial = anchor_radius + inset;
    outer_radial = anchor_radius + length - inset;
    half_width -= inset;
    if (half_width < 10)
    {
        half_width = 10;
    }

    if (fin->shape == FM89_SHAPE_TRIANGLE)
    {
        vertices[0] = demo_fin_point(direction_x, direction_y,
                                     width_x, width_y, width_z,
                                     inner_radial, -half_width, anchor_z);
        vertices[1] = demo_fin_point(direction_x, direction_y,
                                     width_x, width_y, width_z,
                                     inner_radial, half_width, anchor_z);
        vertices[2] = demo_fin_point(direction_x, direction_y,
                                     width_x, width_y, width_z,
                                     outer_radial, 0, anchor_z);
        return 3;
    }

    if (fin->shape == FM89_SHAPE_BOX)
    {
        outer_half_width -= inset;
        if (outer_half_width < 10)
        {
            outer_half_width = 10;
        }
        vertices[0] = demo_fin_point(direction_x, direction_y,
                                     width_x, width_y, width_z,
                                     inner_radial, -half_width, anchor_z);
        vertices[1] = demo_fin_point(direction_x, direction_y,
                                     width_x, width_y, width_z,
                                     inner_radial, half_width, anchor_z);
        vertices[2] = demo_fin_point(direction_x, direction_y,
                                     width_x, width_y, width_z,
                                     outer_radial, outer_half_width, anchor_z);
        vertices[3] = demo_fin_point(direction_x, direction_y,
                                     width_x, width_y, width_z,
                                     outer_radial, -outer_half_width, anchor_z);
        return 4;
    }

    near_radial = inner_radial + (length * 3L) / 10L;
    far_radial = inner_radial + (length * 7L) / 10L;
    near_width = (half_width * 7L) / 10L;
    far_width = half_width;

    vertices[0] = demo_fin_point(direction_x, direction_y,
                                 width_x, width_y, width_z,
                                 inner_radial, 0, anchor_z);
    vertices[1] = demo_fin_point(direction_x, direction_y,
                                 width_x, width_y, width_z,
                                 near_radial, near_width, anchor_z);
    vertices[2] = demo_fin_point(direction_x, direction_y,
                                 width_x, width_y, width_z,
                                 far_radial, far_width, anchor_z);
    vertices[3] = demo_fin_point(direction_x, direction_y,
                                 width_x, width_y, width_z,
                                 outer_radial, 0, anchor_z);
    vertices[4] = demo_fin_point(direction_x, direction_y,
                                 width_x, width_y, width_z,
                                 far_radial, -far_width, anchor_z);
    vertices[5] = demo_fin_point(direction_x, direction_y,
                                 width_x, width_y, width_z,
                                 near_radial, -near_width, anchor_z);
    return 6;
}

static void demo_fill_polygon(const FM89_State *state,
                              const DemoVec3 *vertices,
                              int count,
                              DemoColor color)
{
    DemoPoint points[DEMO_MAX_POLYGON];
    DemoPoint center;
    DemoVec3 local_center;
    int i;
    int next;

    local_center.x = 0;
    local_center.y = 0;
    local_center.z = 0;

    for (i = 0; i < count; ++i)
    {
        points[i] = demo_local_to_screen(state, vertices[i]);
        if (!points[i].valid)
        {
            return;
        }
        local_center.x += vertices[i].x;
        local_center.y += vertices[i].y;
        local_center.z += vertices[i].z;
    }

    local_center.x /= count;
    local_center.y /= count;
    local_center.z /= count;
    center = demo_local_to_screen(state, local_center);
    if (!center.valid)
    {
        return;
    }

    for (i = 0; i < count; ++i)
    {
        next = (i + 1) % count;
        demo_triangle(center.x, center.y,
                      points[i].x, points[i].y,
                      points[next].x, points[next].y,
                      color);
    }
}

static void demo_cover_triangle_root(const FM89_State *state,
                                     const DemoVec3 *outer,
                                     const DemoVec3 *inner,
                                     DemoColor color)
{
    DemoPoint outer_left;
    DemoPoint outer_right;
    DemoPoint inner_left;
    DemoPoint inner_right;

    outer_left = demo_local_to_screen(state, outer[0]);
    outer_right = demo_local_to_screen(state, outer[1]);
    inner_left = demo_local_to_screen(state, inner[0]);
    inner_right = demo_local_to_screen(state, inner[1]);

    if (!outer_left.valid || !outer_right.valid
        || !inner_left.valid || !inner_right.valid)
    {
        return;
    }

    demo_triangle(outer_left.x, outer_left.y,
                  outer_right.x, outer_right.y,
                  inner_right.x, inner_right.y,
                  color);
    demo_triangle(outer_left.x, outer_left.y,
                  inner_right.x, inner_right.y,
                  inner_left.x, inner_left.y,
                  color);
}

static void demo_render_fin(const FM89_State *state, const FM89_Fin *fin)
{
    DemoVec3 outer[DEMO_MAX_POLYGON];
    DemoVec3 inner[DEMO_MAX_POLYGON];
    DemoColor yellow;
    DemoColor white;
    FM89_Fixed outline;
    int outer_count;
    int inner_count;

    outline = fm89_mul((fin->tier == FM89_FIN_CYL_REPEAT
                      || fin->tier == FM89_FIN_CONE_REPEAT) ? 30 : 40,
                       state->scale);
    if (outline < 8)
    {
        outline = 8;
    }

    outer_count = demo_build_fin_polygon(state, fin, 0, outer);
    inner_count = demo_build_fin_polygon(state, fin, outline, inner);

    yellow = demo_scale_color(255, 184, 22, state->brightness);
    white = demo_scale_color(255, 253, 232, state->brightness);

    demo_fill_polygon(state, outer, outer_count, yellow);
    demo_fill_polygon(state, inner, inner_count, white);

    /*
     * Open-root look:
     * remove the yellow base edge so triangle fins read as / \ instead of /_\\.
     * The fin still starts at the cylinder base ring; only the lower orange edge is hidden.
     */
    if (fin->shape == FM89_SHAPE_TRIANGLE && outer_count == 3 && inner_count == 3)
    {
        demo_cover_triangle_root(state, outer, inner, white);
    }
}
static void demo_build_ring(const FM89_State *state,
                            FM89_Fixed local_z,
                            FM89_Fixed local_radius,
                            DemoPoint *points)
{
    DemoVec3 local;
    FM89_Fixed z;
    FM89_Fixed radius;
    int i;
    int angle;

    z = fm89_mul(local_z, state->scale);
    radius = fm89_mul(local_radius, state->scale);

    for (i = 0; i < DEMO_CORE_SEGMENTS; ++i)
    {
        angle = (i * 256) / DEMO_CORE_SEGMENTS;
        local.x = fm89_mul(fm89_cos(angle), radius);
        local.y = fm89_mul(fm89_sin(angle), radius);
        local.z = z;
        points[i] = demo_local_to_screen(state, local);
    }
}

static void demo_join_rings(const DemoPoint *rear,
                            const DemoPoint *front,
                            DemoColor color)
{
    int i;
    int next;

    for (i = 0; i < DEMO_CORE_SEGMENTS; ++i)
    {
        next = (i + 1) % DEMO_CORE_SEGMENTS;
        if (rear[i].valid && rear[next].valid
            && front[i].valid && front[next].valid)
        {
            demo_triangle(rear[i].x, rear[i].y,
                          front[i].x, front[i].y,
                          front[next].x, front[next].y,
                          color);
            demo_triangle(rear[i].x, rear[i].y,
                          front[next].x, front[next].y,
                          rear[next].x, rear[next].y,
                          color);
        }
    }
}

static void demo_cap_ring(const FM89_State *state,
                          const DemoPoint *ring,
                          FM89_Fixed local_z,
                          DemoColor color)
{
    DemoVec3 local;
    DemoPoint center;
    int i;
    int next;

    local.x = 0;
    local.y = 0;
    local.z = fm89_mul(local_z, state->scale);
    center = demo_local_to_screen(state, local);
    if (!center.valid)
    {
        return;
    }

    for (i = 0; i < DEMO_CORE_SEGMENTS; ++i)
    {
        next = (i + 1) % DEMO_CORE_SEGMENTS;
        if (ring[i].valid && ring[next].valid)
        {
            demo_triangle(center.x, center.y,
                          ring[i].x, ring[i].y,
                          ring[next].x, ring[next].y,
                          color);
        }
    }
}

static void demo_render_core(const FM89_State *state)
{
    DemoPoint rear[DEMO_CORE_SEGMENTS];
    DemoPoint join[DEMO_CORE_SEGMENTS];
    DemoPoint tip[DEMO_CORE_SEGMENTS];
    DemoColor white;

    white = demo_scale_color(255, 253, 232, state->brightness);

    demo_build_ring(state, 0,
                    state->config.core_radius, rear);
    demo_build_ring(state, state->config.cylinder_length,
                    state->config.core_radius, join);
    demo_build_ring(state, fm89_core_total_length(state),
                    state->config.cone_tip_radius, tip);

    demo_join_rings(rear, join, white);
    demo_join_rings(join, tip, white);
    demo_cap_ring(state, rear, 0, white);
    demo_cap_ring(state, tip, fm89_core_total_length(state), white);
}

static void demo_render_bank(const FM89_State *state, int offset)
{
    int i;

    if (offset < 0)
    {
        return;
    }

    for (i = 0; i < state->primary_count; ++i)
    {
        demo_render_fin(state, &state->fins[offset + i]);
    }
}

static void demo_render_muzzle(const FM89_State *state)
{
    if (!state->active || state->scale <= 0 || state->brightness <= 0)
    {
        return;
    }

    /* Small repeaters first, larger crowns next, core last to seal the roots. */
    demo_render_bank(state, state->cone_repeat_offset);
    demo_render_bank(state, state->cylinder_repeat_offset);
    demo_render_bank(state, state->cone_primary_offset);
    demo_render_bank(state, state->cylinder_primary_offset);
    demo_render_core(state);
}

static int demo_write_ppm(const char *filename)
{
    FILE *file;
    size_t expected;
    size_t written;

    file = fopen(filename, "wb");
    if (file == NULL)
    {
        return 0;
    }

    fprintf(file, "P6\n%d %d\n255\n", DEMO_WIDTH, DEMO_HEIGHT);
    expected = (size_t)DEMO_WIDTH * (size_t)DEMO_HEIGHT * 3U;
    written = fwrite(demo_framebuffer, 1U, expected, file);
    fclose(file);
    return written == expected;
}

static void demo_select_variant(FM89_State *muzzle, int shot)
{
    FM89_Config config;

    fm89_default_config(&config);
    config.repeater_enabled = 1;
    config.cone_bank_enabled = 1;
    config.cone_repeater_enabled = 1;
    config.repeat_scale_numerator = 1;
    config.repeat_scale_denominator = 2;
    config.cone_scale_numerator = 1;
    config.cone_scale_denominator = 2;

    switch (shot % 6)
    {
        case 0:
            config.minimum_primary_fins = 5;
            config.maximum_primary_fins = 5;
            config.allowed_shapes = FM89_SHAPE_MASK_TRIANGLE;
            config.minimum_fin_length = FM89_CM(5);
            config.maximum_fin_length = FM89_CM(5);
            config.minimum_fin_width = FM89_MM(25);
            config.maximum_fin_width = FM89_MM(25);
            break;
        case 1:
            config.minimum_primary_fins = 4;
            config.maximum_primary_fins = 4;
            config.allowed_shapes = FM89_SHAPE_MASK_TRIANGLE;
            break;
        case 2:
            config.minimum_primary_fins = 6;
            config.maximum_primary_fins = 6;
            config.allowed_shapes = FM89_SHAPE_MASK_LEAF;
            break;
        case 3:
            config.minimum_primary_fins = 5;
            config.maximum_primary_fins = 5;
            config.allowed_shapes = FM89_SHAPE_MASK_LEAF;
            config.minimum_fin_length = FM89_MM(38);
            config.maximum_fin_length = FM89_CM(5);
            break;
        case 4:
            config.minimum_primary_fins = 7;
            config.maximum_primary_fins = 7;
            config.allowed_shapes = FM89_SHAPE_MASK_TRIANGLE;
            break;
        default:
            config.minimum_primary_fins = 8;
            config.maximum_primary_fins = 8;
            config.allowed_shapes = FM89_SHAPE_MASK_ENABLED;
            config.mixed_shapes = 1;
            break;
    }

    fm89_set_config(muzzle, &config);
}

int main(void)
{
    FM89_State muzzle;
    int frame;
    int shot;
    char filename[64];

    fm89_init(&muzzle, 0x47494646UL);
    shot = 0;

    for (frame = 0; frame < DEMO_FRAMES; ++frame)
    {
        if ((frame % DEMO_SHOT_INTERVAL) == 0)
        {
            demo_select_variant(&muzzle, shot);
            fm89_fire(&muzzle);
            shot += 1;
        }

        fm89_update(&muzzle);

        demo_clear();
        demo_draw_background_grid();
        demo_draw_weapon_silhouette(&muzzle);
        demo_render_muzzle(&muzzle);

        sprintf(filename, "preview_frames/frame_%03d.ppm", frame);
        if (!demo_write_ppm(filename))
        {
            fprintf(stderr, "Could not write %s\n", filename);
            return 1;
        }
    }

    /*
     * Deterministic side-profile inspection frames. These are also rendered
     * by this C89 rasterizer; no external image renderer constructs them.
     */
    demo_select_variant(&muzzle, 0);
    fm89_fire(&muzzle);
    muzzle.active = 1;
    muzzle.pitch = 0;
    muzzle.yaw = -64;
    muzzle.global_angle = 0;
    muzzle.center_x = FM89_CM(8);
    muzzle.center_y = 0;
    muzzle.center_z = FM89_CM(55);
    muzzle.scale = FM89_FP_ONE;
    muzzle.brightness = FM89_FP_ONE;

    demo_clear();
    demo_draw_background_grid();
    demo_draw_weapon_silhouette(&muzzle);
    demo_render_muzzle(&muzzle);
    if (!demo_write_ppm("preview_frames/profile_full.ppm"))
    {
        fprintf(stderr, "Could not write profile_full.ppm\n");
        return 1;
    }

    muzzle.primary_count = 0;
    muzzle.total_count = 0;
    demo_clear();
    demo_draw_background_grid();
    demo_draw_weapon_silhouette(&muzzle);
    demo_render_muzzle(&muzzle);
    if (!demo_write_ppm("preview_frames/profile_core.ppm"))
    {
        fprintf(stderr, "Could not write profile_core.ppm\n");
        return 1;
    }

    /*
     * Deterministic front view, looking from the cone tip toward the gun.
     * The muzzle axis points back into the screen, so the base 5 + 5 crown
     * and the cone-base 5 + 5 crown stack around the 2.5 cm core diameter.
     * This image is generated by the same C89 software rasterizer used for
     * every other preview.
     */
    demo_select_variant(&muzzle, 0);
    fm89_fire(&muzzle);
    muzzle.active = 1;
    muzzle.pitch = 0;
    muzzle.yaw = 128;
    muzzle.global_angle = 0;
    muzzle.center_x = 0;
    muzzle.center_y = 0;
    muzzle.center_z = FM89_CM(35);
    muzzle.scale = FM89_FP_ONE;
    muzzle.brightness = FM89_FP_ONE;

    demo_clear();
    demo_draw_background_grid();
    demo_render_muzzle(&muzzle);
    if (!demo_write_ppm("preview_frames/front_5plus5plus5plus5.ppm"))
    {
        fprintf(stderr, "Could not write front_5plus5.ppm\n");
        return 1;
    }

    printf("Generated %d animated frames, two side profiles, and one C89 front view.\n",
           DEMO_FRAMES);
    return 0;
}
