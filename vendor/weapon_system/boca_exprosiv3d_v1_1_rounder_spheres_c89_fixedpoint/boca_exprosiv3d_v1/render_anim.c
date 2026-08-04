#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "boca_exprosiv3d.h"

#define IMG_W 800
#define IMG_H 450
#define MAX_VERTICES 4096UL
#define MAX_INDICES 32768UL

static unsigned char image[IMG_W * IMG_H * 3];
static BEX3D_Vertex vertices[MAX_VERTICES];
static unsigned short indices[MAX_INDICES];
static int sx[MAX_VERTICES];
static int sy[MAX_VERTICES];

static int clamp_int(int value, int low, int high)
{
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

static void set_pixel_add(int x, int y, int r, int g, int b, int alpha)
{
    unsigned long p;
    int rr;
    int gg;
    int bb;

    if (x < 0 || x >= IMG_W || y < 0 || y >= IMG_H) return;
    p = ((unsigned long)y * (unsigned long)IMG_W + (unsigned long)x) * 3UL;
    rr = (int)image[p] + ((r * alpha) >> 8);
    gg = (int)image[p + 1UL] + ((g * alpha) >> 8);
    bb = (int)image[p + 2UL] + ((b * alpha) >> 8);
    image[p] = (unsigned char)clamp_int(rr, 0, 255);
    image[p + 1UL] = (unsigned char)clamp_int(gg, 0, 255);
    image[p + 2UL] = (unsigned char)clamp_int(bb, 0, 255);
}

static void set_pixel_over(int x, int y, int r, int g, int b)
{
    unsigned long p;
    if (x < 0 || x >= IMG_W || y < 0 || y >= IMG_H) return;
    p = ((unsigned long)y * (unsigned long)IMG_W + (unsigned long)IMG_W * 0UL + (unsigned long)x) * 3UL;
    image[p] = (unsigned char)clamp_int(r, 0, 255);
    image[p + 1UL] = (unsigned char)clamp_int(g, 0, 255);
    image[p + 2UL] = (unsigned char)clamp_int(b, 0, 255);
}

static void clear_background(int frame_index, int total_frames)
{
    int x;
    int y;
    int grid;
    int r;
    int g;
    int b;
    int shift;
    shift = (frame_index * 16) / (total_frames > 1 ? total_frames - 1 : 1);

    for (y = 0; y < IMG_H; ++y)
    {
        for (x = 0; x < IMG_W; ++x)
        {
            grid = ((x % 32) == 0 || (y % 32) == 0) ? 4 : 0;
            r = 4 + grid + shift / 4 + (y * 3) / IMG_H;
            g = 7 + grid + shift / 3 + (y * 5) / IMG_H;
            b = 12 + grid + shift / 2 + (y * 8) / IMG_H;
            set_pixel_over(x, y, r, g, b);
        }
    }
}

static long edge_value(int ax, int ay, int bx, int by, int px, int py)
{
    return (long)(px - ax) * (long)(by - ay) -
           (long)(py - ay) * (long)(bx - ax);
}

static void draw_triangle(const BEX3D_Vertex *a,
                          const BEX3D_Vertex *b,
                          const BEX3D_Vertex *c,
                          int x0, int y0,
                          int x1, int y1,
                          int x2, int y2)
{
    long area;
    long w0;
    long w1;
    long w2;
    long positive_area;
    int min_x;
    int max_x;
    int min_y;
    int max_y;
    int x;
    int y;
    int r;
    int g;
    int bl;
    int alpha;
    int inside;

    area = edge_value(x0, y0, x1, y1, x2, y2);
    if (area == 0L) return;

    min_x = x0;
    if (x1 < min_x) min_x = x1;
    if (x2 < min_x) min_x = x2;
    max_x = x0;
    if (x1 > max_x) max_x = x1;
    if (x2 > max_x) max_x = x2;
    min_y = y0;
    if (y1 < min_y) min_y = y1;
    if (y2 < min_y) min_y = y2;
    max_y = y0;
    if (y1 > max_y) max_y = y1;
    if (y2 > max_y) max_y = y2;

    min_x = clamp_int(min_x, 0, IMG_W - 1);
    max_x = clamp_int(max_x, 0, IMG_W - 1);
    min_y = clamp_int(min_y, 0, IMG_H - 1);
    max_y = clamp_int(max_y, 0, IMG_H - 1);

    positive_area = area < 0L ? -area : area;

    for (y = min_y; y <= max_y; ++y)
    {
        for (x = min_x; x <= max_x; ++x)
        {
            w0 = edge_value(x1, y1, x2, y2, x, y);
            w1 = edge_value(x2, y2, x0, y0, x, y);
            w2 = edge_value(x0, y0, x1, y1, x, y);

            if (area < 0L)
            {
                w0 = -w0;
                w1 = -w1;
                w2 = -w2;
            }

            inside = (w0 >= 0L && w1 >= 0L && w2 >= 0L);
            if (inside)
            {
                r = (int)((w0 * (long)a->r + w1 * (long)b->r +
                           w2 * (long)c->r) / positive_area);
                g = (int)((w0 * (long)a->g + w1 * (long)b->g +
                           w2 * (long)c->g) / positive_area);
                bl = (int)((w0 * (long)a->b + w1 * (long)b->b +
                            w2 * (long)c->b) / positive_area);
                alpha = (int)((w0 * (long)a->a + w1 * (long)b->a +
                               w2 * (long)c->a) / positive_area);
                alpha = (alpha * 230) >> 8;
                set_pixel_add(x, y, r, g, bl, alpha);
            }
        }
    }
}

static void draw_line_add(int x0, int y0, int x1, int y1,
                          int r, int g, int b, int alpha)
{
    int dx;
    int sx_step;
    int dy;
    int sy_step;
    int err;
    int e2;

    dx = x1 > x0 ? x1 - x0 : x0 - x1;
    sx_step = x0 < x1 ? 1 : -1;
    dy = y1 > y0 ? y0 - y1 : y1 - y0;
    sy_step = y0 < y1 ? 1 : -1;
    err = dx + dy;

    for (;;)
    {
        set_pixel_add(x0, y0, r, g, b, alpha);
        if (x0 == x1 && y0 == y1) break;
        e2 = err << 1;
        if (e2 >= dy)
        {
            err += dy;
            x0 += sx_step;
        }
        if (e2 <= dx)
        {
            err += dx;
            y0 += sy_step;
        }
    }
}

static void project_mesh(const BEX3D_MeshBuffer *mesh)
{
    unsigned long i;
    long u;
    long v;
    long min_u;
    long max_u;
    long min_v;
    long max_v;
    long range_u;
    long range_v;
    long scale_u;
    long scale_v;
    long scale;
    long center_v;
    long usable_w;
    long usable_h;

    min_u = 2147483647L;
    max_u = -2147483647L;
    min_v = 2147483647L;
    max_v = -2147483647L;

    for (i = 0UL; i < mesh->vertex_count; ++i)
    {
        u = mesh->vertices[i].z + mesh->vertices[i].x / 3L;
        v = -mesh->vertices[i].y + mesh->vertices[i].x / 6L -
            mesh->vertices[i].z / 12L;
        if (u < min_u) min_u = u;
        if (u > max_u) max_u = u;
        if (v < min_v) min_v = v;
        if (v > max_v) max_v = v;
    }

    range_u = max_u - min_u;
    range_v = max_v - min_v;
    if (range_u < BEX3D_FP_ONE) range_u = BEX3D_FP_ONE;
    if (range_v < BEX3D_FP_ONE) range_v = BEX3D_FP_ONE;

    usable_w = IMG_W - 160;
    usable_h = IMG_H - 160;
    scale_u = (usable_w * 1024L) / range_u;
    scale_v = (usable_h * 1024L) / range_v;
    scale = scale_u < scale_v ? scale_u : scale_v;
    center_v = (min_v + max_v) / 2L;

    for (i = 0UL; i < mesh->vertex_count; ++i)
    {
        u = mesh->vertices[i].z + mesh->vertices[i].x / 3L;
        v = -mesh->vertices[i].y + mesh->vertices[i].x / 6L -
            mesh->vertices[i].z / 12L;
        sx[i] = 60 + (int)(((u - min_u) * scale) >> 10);
        sy[i] = IMG_H / 2 + 10 + (int)(((v - center_v) * scale) >> 10);
    }
}

static void draw_muzzle_marker(const BEX3D_MeshBuffer *mesh)
{
    unsigned long i;
    int muzzle_x;
    int muzzle_y;
    int best;
    long best_z;

    best = 0;
    best_z = mesh->vertices[0].z;
    for (i = 1UL; i < mesh->vertex_count; ++i)
    {
        if (mesh->vertices[i].z < best_z)
        {
            best_z = mesh->vertices[i].z;
            best = (int)i;
        }
    }
    muzzle_x = sx[best];
    muzzle_y = sy[best];

    draw_line_add(5, muzzle_y, muzzle_x, muzzle_y, 42, 55, 72, 180);
    draw_line_add(5, muzzle_y - 7, muzzle_x, muzzle_y - 7, 34, 44, 58, 150);
    draw_line_add(5, muzzle_y + 7, muzzle_x, muzzle_y + 7, 34, 44, 58, 150);
}

static int write_ppm(const char *filename)
{
    FILE *file;
    file = fopen(filename, "wb");
    if (file == (FILE *)0) return 0;
    fprintf(file, "P6\n%d %d\n255\n", IMG_W, IMG_H);
    if (fwrite(image, 1U, sizeof(image), file) != sizeof(image))
    {
        fclose(file);
        return 0;
    }
    fclose(file);
    return 1;
}

static int render_frame(int profile_id,
                        unsigned long time_us,
                        unsigned long duration_us,
                        int frame_index,
                        int total_frames,
                        const char *out_path)
{
    BEX3D_State state;
    BEX3D_Profile profile;
    BEX3D_MeshBuffer mesh;
    unsigned long t;
    unsigned long i;
    unsigned short ia;
    unsigned short ib;
    unsigned short ic;
    int result;

    mesh.vertices = vertices;
    mesh.indices = indices;
    mesh.vertex_capacity = MAX_VERTICES;
    mesh.index_capacity = MAX_INDICES;
    mesh.vertex_count = 0UL;
    mesh.index_count = 0UL;
    mesh.truncated = 0;

    bex3d_default_profile(&profile, profile_id);
    bex3d_init(&state, 0xB0CA3D00UL + (unsigned long)profile_id);
    bex3d_set_profile(&state, &profile);
    bex3d_fire(&state);
    if (time_us > 0UL) bex3d_update_us(&state, time_us);

    clear_background(frame_index, total_frames);

    result = bex3d_build_mesh(&state, &mesh);
    if (result != BEX3D_BUILD_OK)
    {
        return 0;
    }

    if (mesh.vertex_count > 0UL)
    {
        project_mesh(&mesh);
        draw_muzzle_marker(&mesh);
        for (t = 0UL; t + 2UL < mesh.index_count; t += 3UL)
        {
            ia = mesh.indices[t];
            ib = mesh.indices[t + 1UL];
            ic = mesh.indices[t + 2UL];
            draw_triangle(&mesh.vertices[ia], &mesh.vertices[ib], &mesh.vertices[ic],
                          sx[ia], sy[ia], sx[ib], sy[ib], sx[ic], sy[ic]);
        }
        for (t = 0UL; t + 2UL < mesh.index_count; t += 3UL)
        {
            ia = mesh.indices[t];
            ib = mesh.indices[t + 1UL];
            ic = mesh.indices[t + 2UL];
            draw_line_add(sx[ia], sy[ia], sx[ib], sy[ib], 255, 190, 76, 18);
            draw_line_add(sx[ib], sy[ib], sx[ic], sy[ic], 255, 190, 76, 18);
            draw_line_add(sx[ic], sy[ic], sx[ia], sy[ia], 255, 190, 76, 18);
        }
        for (i = 0UL; i < mesh.vertex_count; ++i)
        {
            if (mesh.vertices[i].a > 220U)
            {
                set_pixel_add(sx[i], sy[i], 255, 245, 210, 90);
            }
        }
    }

    /* timeline bar */
    {
        int bar_x0 = 40;
        int bar_y = IMG_H - 30;
        int bar_w = IMG_W - 80;
        int fill_w = duration_us ? (int)((time_us * (unsigned long)bar_w) / duration_us) : 0;
        int x;
        for (x = 0; x < bar_w; ++x)
        {
            if (x <= fill_w)
                set_pixel_over(bar_x0 + x, bar_y, 245, 164, 60);
            else
                set_pixel_over(bar_x0 + x, bar_y, 58, 71, 88);
            set_pixel_over(bar_x0 + x, bar_y + 1, 40, 48, 60);
        }
    }

    return write_ppm(out_path);
}

int main(int argc, char **argv)
{
    int profile_id;
    int frame_count;
    int frame;
    unsigned long duration_us;
    unsigned long time_us;
    char out_path[512];
    const char *out_dir;
    BEX3D_Profile profile;

    if (argc < 4)
    {
        fprintf(stderr, "usage: %s <profile_id> <frame_count> <out_dir>\n", argv[0]);
        return 1;
    }

    profile_id = atoi(argv[1]);
    frame_count = atoi(argv[2]);
    out_dir = argv[3];
    if (frame_count < 2) frame_count = 2;

    bex3d_default_profile(&profile, profile_id);
    duration_us = profile.event_duration_us;

    for (frame = 0; frame < frame_count; ++frame)
    {
        time_us = (unsigned long)(((unsigned long)frame * duration_us) / (unsigned long)(frame_count - 1));
        sprintf(out_path, "%s/frame_%03d.ppm", out_dir, frame);
        if (!render_frame(profile_id, time_us, duration_us, frame, frame_count, out_path))
        {
            fprintf(stderr, "render failed on frame %d\n", frame);
            return 1;
        }
    }

    return 0;
}
