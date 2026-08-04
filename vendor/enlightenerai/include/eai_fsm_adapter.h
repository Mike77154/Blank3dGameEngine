#ifndef EAI_FSM_ADAPTER_H
#define EAI_FSM_ADAPTER_H

#include "eai_context.h"

typedef void (*EAI_FsmEventFn)(void* user, const EAI_Event* event_data);

void eai_fsm_dispatch_events(EAI_Context* ctx, EAI_FsmEventFn fn, void* user);

#endif
