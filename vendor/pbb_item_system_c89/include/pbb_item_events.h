#ifndef PBB_ITEM_EVENTS_H
#define PBB_ITEM_EVENTS_H
#include "pbb_item_types.h"
#ifdef __cplusplus
extern "C" {
#endif
int pbb_item_event_push(PBB_ItemWorld *world, int type, int actor_id, int item_id, int def_id, int a, int b, int c, PBB_Fixed x, PBB_Fixed y);
int pbb_item_event_pop(PBB_ItemWorld *world, PBB_ItemEvent *out_event);
void pbb_item_event_clear(PBB_ItemWorld *world);
#ifdef __cplusplus
}
#endif
#endif
