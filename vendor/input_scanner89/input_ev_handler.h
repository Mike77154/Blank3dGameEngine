#ifndef INPUT_EV_HANDLER_H
#define INPUT_EV_HANDLER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "input_scanner.h"

typedef enum input_event_kind {
    INPUT_EVENT_PRESS = 0,
    INPUT_EVENT_HOLD = 1,
    INPUT_EVENT_RELEASE = 2,
    INPUT_EVENT_REPEAT = 3
} input_event_kind;

typedef struct input_event {
    int button_index;
    input_event_kind kind;
    unsigned short frames;
} input_event;

typedef void (*input_event_handler_fn)(const input_event *ev, void *user_data);

#define INPUT_DISPATCH_PRESS          0x01u
#define INPUT_DISPATCH_HOLD           0x02u
#define INPUT_DISPATCH_RELEASE        0x04u
#define INPUT_DISPATCH_REPEAT         0x08u
#define INPUT_DISPATCH_HOLD_ON_PRESS  0x10u

int input_dispatch_events_ex(const InputScanner *s,
                             unsigned int flags,
                             unsigned short repeat_delay_frames,
                             unsigned short repeat_interval_frames,
                             input_event_handler_fn handler,
                             void *user_data);
void input_dispatch_events(const InputScanner *s,
                           input_event_handler_fn handler,
                           void *user_data);

#ifdef __cplusplus
}
#endif

#endif /* INPUT_EV_HANDLER_H */
