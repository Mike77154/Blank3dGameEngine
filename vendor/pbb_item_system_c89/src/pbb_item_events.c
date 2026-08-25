#include "pbb_item_internal.h"
#include <string.h>

int pbb_item_event_push(PBB_ItemWorld *world,
                        int type,
                        int actor_id,
                        int item_id,
                        int def_id,
                        int a,
                        int b,
                        int c,
                        PBB_Fixed x,
                        PBB_Fixed y)
{
    PBB_ItemEvent *event;

    if (world == 0) {
        return 0;
    }

    if (world->event_count >= PBB_ITEM_MAX_EVENTS) {
        world->events_lost += 1;
        return 0;
    }

    event = &world->events[world->event_write];
    memset(event, 0, sizeof(*event));
    event->used = 1;
    event->type = type;
    event->actor_id = actor_id;
    event->item_id = item_id;
    event->def_id = def_id;
    event->a = a;
    event->b = b;
    event->c = c;
    event->x = x;
    event->y = y;

    world->event_write += 1;
    if (world->event_write >= PBB_ITEM_MAX_EVENTS) {
        world->event_write = 0;
    }
    world->event_count += 1;

    return 1;
}

int pbb_item_event_pop(PBB_ItemWorld *world, PBB_ItemEvent *out_event)
{
    PBB_ItemEvent *event;

    if (world == 0 || out_event == 0) {
        return 0;
    }

    if (world->event_count <= 0) {
        return 0;
    }

    event = &world->events[world->event_read];
    *out_event = *event;
    memset(event, 0, sizeof(*event));

    world->event_read += 1;
    if (world->event_read >= PBB_ITEM_MAX_EVENTS) {
        world->event_read = 0;
    }
    world->event_count -= 1;

    return 1;
}

void pbb_item_event_clear(PBB_ItemWorld *world)
{
    if (world == 0) {
        return;
    }

    memset(world->events, 0, sizeof(world->events));
    world->event_read = 0;
    world->event_write = 0;
    world->event_count = 0;
    world->events_lost = 0;
}

