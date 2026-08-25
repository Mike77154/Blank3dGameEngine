#include "gveh_fx.h"

void gveh_fx_clear(gveh_fx_queue *q)
{
    q->count = 0;
}

void gveh_fx_push(gveh_fx_queue *q, gveh_i16 type, gveh_i16 intensity, gveh_fx x, gveh_fx y, gveh_fx z)
{
    gveh_i16 i;
    i = q->count;
    if (i >= 32) return;
    q->events[i].type = type;
    q->events[i].intensity = intensity;
    q->events[i].x = x;
    q->events[i].y = y;
    q->events[i].z = z;
    q->count = (gveh_i16)(i + 1);
}
