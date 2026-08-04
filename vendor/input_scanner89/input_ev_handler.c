#include "input_ev_handler.h"

static void input_dispatch_emit_event(int button_index,
                                      input_event_kind kind,
                                      unsigned short frames,
                                      input_event_handler_fn handler,
                                      void *user_data)
{
    input_event ev;

    ev.button_index = button_index;
    ev.kind = kind;
    ev.frames = frames;
    handler(&ev, user_data);
}

int input_dispatch_events_ex(const InputScanner *s,
                             unsigned int flags,
                             unsigned short repeat_delay_frames,
                             unsigned short repeat_interval_frames,
                             input_event_handler_fn handler,
                             void *user_data)
{
    int i;
    int emit_hold_on_press;

    if (!s || !handler) {
        return INPUT_ERR_NULL;
    }

    emit_hold_on_press = ((flags & INPUT_DISPATCH_HOLD_ON_PRESS) != 0U);

    for (i = 0; i < INPUT_MAX_BUTTONS; ++i) {
        if (!input_button_is_valid(s, i)) {
            continue;
        }

        if ((flags & INPUT_DISPATCH_PRESS) != 0U && input_button_pressed(s, i)) {
            input_dispatch_emit_event(i,
                                      INPUT_EVENT_PRESS,
                                      input_button_down_frames(s, i),
                                      handler,
                                      user_data);
        }

        if ((flags & INPUT_DISPATCH_REPEAT) != 0U &&
            input_button_repeat(s, i, repeat_delay_frames, repeat_interval_frames)) {
            input_dispatch_emit_event(i,
                                      INPUT_EVENT_REPEAT,
                                      input_button_down_frames(s, i),
                                      handler,
                                      user_data);
        }

        if ((flags & INPUT_DISPATCH_HOLD) != 0U && input_button_down(s, i)) {
            if (emit_hold_on_press || !input_button_pressed(s, i)) {
                input_dispatch_emit_event(i,
                                          INPUT_EVENT_HOLD,
                                          input_button_down_frames(s, i),
                                          handler,
                                          user_data);
            }
        }

        if ((flags & INPUT_DISPATCH_RELEASE) != 0U && input_button_released(s, i)) {
            input_dispatch_emit_event(i,
                                      INPUT_EVENT_RELEASE,
                                      input_button_last_down_frames(s, i),
                                      handler,
                                      user_data);
        }
    }

    return INPUT_OK;
}

void input_dispatch_events(const InputScanner *s,
                           input_event_handler_fn handler,
                           void *user_data)
{
    (void)input_dispatch_events_ex(s,
                                   INPUT_DISPATCH_PRESS |
                                   INPUT_DISPATCH_HOLD |
                                   INPUT_DISPATCH_RELEASE,
                                   0U,
                                   0U,
                                   handler,
                                   user_data);
}
