#include "eai_fsm_adapter.h"

void eai_fsm_dispatch_events(EAI_Context* ctx, EAI_FsmEventFn fn, void* user)
{
    EAI_Event ev;

    if (fn == 0)
    {
        eai_events_clear(ctx);
        return;
    }

    while (eai_events_pop(ctx, &ev))
    {
        fn(user, &ev);
    }
}
