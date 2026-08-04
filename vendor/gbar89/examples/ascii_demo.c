
#include <stdio.h>
#include "gbar89.h"

#define CANVAS_W 80
#define CANVAS_H 24

static char g_canvas[CANVAS_H][CANVAS_W + 1];

static void canvas_clear(void)
{
    int y;
    int x;

    for (y = 0; y < CANVAS_H; ++y) {
        for (x = 0; x < CANVAS_W; ++x) {
            g_canvas[y][x] = ' ';
        }
        g_canvas[y][CANVAS_W] = '\0';
    }
}

static void canvas_rect(void *user, int x, int y, int w, int h,
                        unsigned long rgba)
{
    int iy;
    int ix;
    char ch;

    (void)user;

    ch = '?';
    if (rgba == 0x202020FFUL || rgba == 0x303030FFUL) {
        ch = '.';
    } else if (rgba == 0x40C060FFUL) {
        ch = '#';
    } else if (rgba == 0xE0B040FFUL) {
        ch = '-';
    } else if (rgba == 0xFFFFFFFFUL) {
        ch = '+';
    } else {
        ch = '*';
    }

    for (iy = y; iy < y + h; ++iy) {
        if (iy < 0 || iy >= CANVAS_H) {
            continue;
        }
        for (ix = x; ix < x + w; ++ix) {
            if (ix < 0 || ix >= CANVAS_W) {
                continue;
            }
            g_canvas[iy][ix] = ch;
        }
    }
}

static void canvas_line(void *user, int x1, int y1, int x2, int y2,
                        unsigned long rgba)
{
    GBar89_Rect r;

    (void)x2;
    (void)y2;

    r.x = x1;
    r.y = y1;
    r.w = 1;
    r.h = 1;
    canvas_rect(user, r.x, r.y, r.w, r.h, rgba);
}

static void canvas_triangles(void *user,
                             const GBar89_Vertex *vertices, int vertex_count,
                             const int *indices, int index_count,
                             int sprite_id)
{
    int i;

    (void)user;
    (void)indices;
    (void)index_count;
    (void)sprite_id;

    for (i = 0; i < vertex_count; ++i) {
        if (vertices[i].x >= 0 && vertices[i].x < CANVAS_W &&
            vertices[i].y >= 0 && vertices[i].y < CANVAS_H) {
            g_canvas[vertices[i].y][vertices[i].x] = '@';
        }
    }
}

static void canvas_print(void)
{
    int y;

    for (y = 0; y < CANVAS_H; ++y) {
        puts(g_canvas[y]);
    }
}

int main(void)
{
    GBar89_RenderOps ops;
    GBar89_Meter hp;
    GBar89_Meter stamina;
    GBar89_Meter boss;
    GBar89_Meter ring;

    canvas_clear();

    ops.user = 0;
    ops.draw_rect = canvas_rect;
    ops.draw_line = canvas_line;
    ops.draw_sprite = 0;
    ops.draw_triangles = canvas_triangles;
    ops.push_clip = 0;
    ops.pop_clip = 0;

    gbar89_init(&hp);
    gbar89_set_rect(&hp, 2, 2, 50, 5);
    gbar89_set_range(&hp, 0, 100);
    gbar89_set_value(&hp, 72);
    hp.flags |= GBAR89_FLAG_DAMAGE_LAG;
    hp.visual_value = 72;
    hp.lag_value = 90;
    gbar89_add_marker(&hp, 25);
    gbar89_add_marker(&hp, 50);
    gbar89_add_marker(&hp, 75);
    hp.flags |= GBAR89_FLAG_DRAW_MARKERS;
    gbar89_draw(&hp, &ops);

    gbar89_init(&stamina);
    gbar89_set_kind(&stamina, GBAR89_KIND_SEGMENTED);
    gbar89_set_rect(&stamina, 2, 9, 50, 3);
    gbar89_set_range(&stamina, 0, 100);
    gbar89_set_value(&stamina, 37);
    gbar89_set_segments(&stamina, 10, 1);
    gbar89_tick(&stamina, 0);
    gbar89_draw(&stamina, &ops);

    gbar89_init(&boss);
    gbar89_set_rect(&boss, 2, 15, 70, 4);
    gbar89_set_range(&boss, 0, 3000);
    gbar89_set_value(&boss, 2450);
    boss.flags |= GBAR89_FLAG_DRAW_MARKERS;
    gbar89_add_marker(&boss, 1000);
    gbar89_add_marker(&boss, 2000);
    gbar89_tick(&boss, 0);
    gbar89_draw(&boss, &ops);

    gbar89_init(&ring);
    gbar89_set_kind(&ring, GBAR89_KIND_RADIAL_RING);
    gbar89_set_rect(&ring, 58, 2, 16, 10);
    gbar89_set_range(&ring, 0, 100);
    gbar89_set_value(&ring, 65);
    gbar89_set_radial(&ring, -90, 360, 55, 32);
    ring.flags = GBAR89_FLAG_CLAMP_VALUE | GBAR89_FLAG_USE_VISUAL;
    gbar89_tick(&ring, 0);
    gbar89_draw(&ring, &ops);

    canvas_print();

    return 0;
}
