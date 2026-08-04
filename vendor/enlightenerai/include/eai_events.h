#ifndef EAI_EVENTS_H
#define EAI_EVENTS_H

#include "eai_types.h"

#define EAI_EVENT_NONE            0
#define EAI_EVENT_SEE_TARGET      1
#define EAI_EVENT_LOSE_TARGET     2
#define EAI_EVENT_HEAR_SOUND      3
#define EAI_EVENT_TARGET_ACQUIRED 4
#define EAI_EVENT_TARGET_LOST     5
#define EAI_EVENT_ENTER_ZONE      6
#define EAI_EVENT_LEAVE_ZONE      7

typedef struct EAI_Event
{
    EAI_U8 type;
    EAI_EntityId self_id;
    EAI_EntityId other_id;
    EAI_Vec3 position;
    EAI_Fixed value;
} EAI_Event;

#include "eai_fwd.h"

void eai_events_clear(EAI_Context* ctx);
int  eai_events_push(EAI_Context* ctx, const EAI_Event* event_data);
int  eai_events_pop(EAI_Context* ctx, EAI_Event* out_event);
int  eai_events_count(const EAI_Context* ctx);

#endif
