/* evcore_input_bridge.c - C89, requiere input_ev_handler externo */

#include "evcore_input_bridge.h"
#include "input_ev_handler.h"

static void evcore_input_bridge_handler(const input_event *iev, void *user_data)
{
    evcore_event_t evt;

    (void)user_data;

    switch (iev->kind) {
    case INPUT_EVENT_PRESS:
        evt.type = EV_INPUT_PRESS;
        break;
    case INPUT_EVENT_HOLD:
        evt.type = EV_INPUT_HOLD;
        break;
    case INPUT_EVENT_RELEASE:
        evt.type = EV_INPUT_RELEASE;
        break;
    default:
        return;
    }

    evt.code = (evcore_value_t)iev->button_index;
    evt.data = 0;
    evcore_emit(&evt);
}

void evcore_input_bridge_dispatch(const void *scanner)
{
    const InputScanner *typed_scanner;

    if (scanner == 0) {
        return;
    }

    typed_scanner = (const InputScanner *)scanner;
    input_dispatch_events(typed_scanner, evcore_input_bridge_handler, 0);
}
