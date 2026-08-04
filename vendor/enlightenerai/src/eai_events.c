#include "eai_events.h"
#include "eai_context.h"

void eai_events_clear(EAI_Context* ctx)
{
    ctx->event_head = 0;
    ctx->event_tail = 0;
}

int eai_events_push(EAI_Context* ctx, const EAI_Event* event_data)
{
    EAI_U16 next_tail;

    next_tail = (EAI_U16)((ctx->event_tail + 1u) % EAI_MAX_EVENTS);
    if (next_tail == ctx->event_head)
    {
        return EAI_FALSE;
    }

    ctx->event_queue[ctx->event_tail] = *event_data;
    ctx->event_tail = next_tail;
    return EAI_TRUE;
}

int eai_events_pop(EAI_Context* ctx, EAI_Event* out_event)
{
    if (ctx->event_head == ctx->event_tail)
    {
        return EAI_FALSE;
    }

    *out_event = ctx->event_queue[ctx->event_head];
    ctx->event_head = (EAI_U16)((ctx->event_head + 1u) % EAI_MAX_EVENTS);
    return EAI_TRUE;
}

int eai_events_count(const EAI_Context* ctx)
{
    if (ctx->event_tail >= ctx->event_head)
    {
        return (int)(ctx->event_tail - ctx->event_head);
    }
    return (int)((EAI_MAX_EVENTS - ctx->event_head) + ctx->event_tail);
}
