#ifndef GVEH_FX_H
#define GVEH_FX_H

#include "gveh_types.h"

#define GVEH_FX_EVENT_NONE     0
#define GVEH_FX_EVENT_DUST     1
#define GVEH_FX_EVENT_SKID     2
#define GVEH_FX_EVENT_SPLASH   3
#define GVEH_FX_EVENT_IMPACT   4
#define GVEH_FX_EVENT_ENGINE   5

typedef struct gveh_fx_event_s {
    gveh_i16 type;
    gveh_i16 intensity;
    gveh_fx x;
    gveh_fx y;
    gveh_fx z;
} gveh_fx_event;

typedef struct gveh_fx_queue_s {
    gveh_fx_event events[32];
    gveh_i16 count;
} gveh_fx_queue;

void gveh_fx_clear(gveh_fx_queue *q);
void gveh_fx_push(gveh_fx_queue *q, gveh_i16 type, gveh_i16 intensity, gveh_fx x, gveh_fx y, gveh_fx z);

#endif
