#include <stdio.h>
#include "gscopepresets89.h"

static unsigned long h = 2166136261UL;

static void mix_u(unsigned long v)
{
    h ^= v;
    h *= 16777619UL;
}

static void mix_s(const char *s)
{
    if (!s) { mix_u(0UL); return; }
    while (*s) {
        mix_u((unsigned long)(unsigned char)*s);
        ++s;
    }
    mix_u(0UL);
}

int main(void)
{
    short i;
    short j;
    const gsvp89_preset *p;
    const gsv89_shape *s;
    unsigned long shapes;
    shapes = 0UL;
    mix_u((unsigned long)(unsigned short)gsvp89_count());
    for (i = 0; i < gsvp89_count(); ++i) {
        p = gsvp89_get(i);
        if (!p) return 2;
        mix_u((unsigned long)(unsigned short)p->id);
        mix_s(p->name);
        mix_s(p->family_name);
        mix_u((unsigned long)(unsigned short)p->family);
        mix_u((unsigned long)(unsigned short)p->flags);
        mix_u((unsigned long)(unsigned short)p->focal_plane);
        mix_u((unsigned long)(unsigned short)p->recommended_zoom_x100);
        mix_u((unsigned long)(unsigned short)p->calibration_zoom_x100);
        mix_u((unsigned long)(unsigned short)p->default_theme);
        mix_u((unsigned long)(unsigned short)p->shape_count);
        shapes += (unsigned long)(unsigned short)p->shape_count;
        for (j = 0; j < p->shape_count; ++j) {
            s = &p->shapes[j];
            mix_u((unsigned long)(unsigned short)s->kind);
            mix_u((unsigned long)(unsigned short)s->part_id);
            mix_u((unsigned long)(unsigned short)s->flags);
            mix_u((unsigned long)(unsigned short)s->layer);
            mix_u((unsigned long)(unsigned short)s->thickness_px);
            mix_u((unsigned long)(unsigned short)s->outline_px);
            mix_u((unsigned long)s->x0); mix_u((unsigned long)s->y0);
            mix_u((unsigned long)s->x1); mix_u((unsigned long)s->y1);
            mix_u((unsigned long)s->x2); mix_u((unsigned long)s->y2);
            mix_u((unsigned long)s->x3); mix_u((unsigned long)s->y3);
            mix_u((unsigned long)s->a); mix_u((unsigned long)s->b);
            mix_u((unsigned long)s->c); mix_u((unsigned long)s->d);
            mix_u((unsigned long)(unsigned short)s->i0);
            mix_u((unsigned long)(unsigned short)s->i1);
            mix_u((unsigned long)(unsigned short)s->i2);
            mix_u((unsigned long)(unsigned short)s->i3);
            mix_u((unsigned long)(unsigned short)s->point_count);
        }
    }
    printf("presets=%d shapes=%lu hash=%lu\n", (int)gsvp89_count(), shapes, h);
    return 0;
}
