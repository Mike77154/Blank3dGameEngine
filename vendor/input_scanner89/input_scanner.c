#include "input_scanner.h"

static unsigned short input_u16_saturating_inc(unsigned short value)
{
    if (value == (unsigned short)65535U) {
        return value;
    }
    return (unsigned short)(value + 1U);
}

static void input_scanner_clear_counters(InputScanner *s)
{
    int i;

    for (i = 0; i < INPUT_MAX_BUTTONS; ++i) {
        s->down_frames[i] = 0U;
        s->up_frames[i] = 0U;
        s->last_down_frames[i] = 0U;
    }
}

input_bits_t input_button_bit(int button_index)
{
    if (button_index < 0 ||
        button_index >= INPUT_MAX_BUTTONS ||
        button_index >= INPUT_BITS_CAPACITY) {
        return (input_bits_t)0;
    }

    return ((input_bits_t)1) << button_index;
}

input_bits_t input_mask_from_button_count(int button_count)
{
    input_bits_t mask;
    int i;
    int limit;

    mask = (input_bits_t)0;
    limit = button_count;

    if (limit < 0) {
        limit = 0;
    }
    if (limit > INPUT_MAX_BUTTONS) {
        limit = INPUT_MAX_BUTTONS;
    }
    if (limit > INPUT_BITS_CAPACITY) {
        limit = INPUT_BITS_CAPACITY;
    }

    for (i = 0; i < limit; ++i) {
        mask |= input_button_bit(i);
    }

    return mask;
}

int input_scanner_reset(InputScanner *s)
{
    input_bits_t valid_mask;
    InputBackendPollFn poll_fn;
    void *backend_user_data;

    if (!s) {
        return INPUT_ERR_NULL;
    }

    valid_mask = s->valid_mask;
    poll_fn = s->poll_fn;
    backend_user_data = s->backend_user_data;

    s->current_bits = (input_bits_t)0;
    s->previous_bits = (input_bits_t)0;
    s->pressed_bits = (input_bits_t)0;
    s->released_bits = (input_bits_t)0;
    s->valid_mask = valid_mask;
    s->poll_fn = poll_fn;
    s->backend_user_data = backend_user_data;

    input_scanner_clear_counters(s);
    return INPUT_OK;
}

int input_scanner_init(InputScanner *s,
                       InputBackendPollFn fn,
                       void *user_data)
{
    if (!s) {
        return INPUT_ERR_NULL;
    }

    s->valid_mask = input_mask_from_button_count(INPUT_MAX_BUTTONS);
    s->poll_fn = fn;
    s->backend_user_data = user_data;

    return input_scanner_reset(s);
}

int input_scanner_set_backend(InputScanner *s,
                              InputBackendPollFn fn,
                              void *user_data)
{
    if (!s) {
        return INPUT_ERR_NULL;
    }

    s->poll_fn = fn;
    s->backend_user_data = user_data;
    return INPUT_OK;
}

int input_scanner_set_valid_mask(InputScanner *s, input_bits_t valid_mask)
{
    int i;
    input_bits_t bit;

    if (!s) {
        return INPUT_ERR_NULL;
    }

    s->valid_mask = valid_mask;
    s->current_bits &= valid_mask;
    s->previous_bits &= valid_mask;
    s->pressed_bits &= valid_mask;
    s->released_bits &= valid_mask;

    for (i = 0; i < INPUT_MAX_BUTTONS; ++i) {
        bit = input_button_bit(i);
        if (bit == (input_bits_t)0 || (valid_mask & bit) == (input_bits_t)0) {
            s->down_frames[i] = 0U;
            s->up_frames[i] = 0U;
            s->last_down_frames[i] = 0U;
        }
    }

    return INPUT_OK;
}

int input_scanner_set_button_count(InputScanner *s, int button_count)
{
    if (!s) {
        return INPUT_ERR_NULL;
    }

    return input_scanner_set_valid_mask(s, input_mask_from_button_count(button_count));
}

int input_scanner_update_with_bits(InputScanner *s, input_bits_t new_bits)
{
    int i;
    input_bits_t bit;

    if (!s) {
        return INPUT_ERR_NULL;
    }

    s->previous_bits = s->current_bits;
    s->current_bits = new_bits & s->valid_mask;
    s->pressed_bits = s->current_bits & (~s->previous_bits);
    s->released_bits = s->previous_bits & (~s->current_bits);

    for (i = 0; i < INPUT_MAX_BUTTONS; ++i) {
        bit = input_button_bit(i);

        if (bit == (input_bits_t)0 || (s->valid_mask & bit) == (input_bits_t)0) {
            s->down_frames[i] = 0U;
            s->up_frames[i] = 0U;
            s->last_down_frames[i] = 0U;
            continue;
        }

        if ((s->current_bits & bit) != (input_bits_t)0) {
            if ((s->pressed_bits & bit) != (input_bits_t)0) {
                s->down_frames[i] = 1U;
            } else {
                s->down_frames[i] = input_u16_saturating_inc(s->down_frames[i]);
            }
            s->up_frames[i] = 0U;
        } else if ((s->released_bits & bit) != (input_bits_t)0) {
            s->last_down_frames[i] = s->down_frames[i];
            s->down_frames[i] = 0U;
            s->up_frames[i] = 1U;
        } else {
            s->down_frames[i] = 0U;
            s->up_frames[i] = input_u16_saturating_inc(s->up_frames[i]);
        }
    }

    return INPUT_OK;
}

int input_scanner_update(InputScanner *s)
{
    input_bits_t new_bits;
    int result;

    if (!s) {
        return INPUT_ERR_NULL;
    }
    if (!s->poll_fn) {
        return INPUT_ERR_NO_BACKEND;
    }

    new_bits = (input_bits_t)0;
    result = s->poll_fn(s->backend_user_data, &new_bits);
    if (result != INPUT_OK) {
        return result;
    }

    return input_scanner_update_with_bits(s, new_bits);
}

int input_button_is_valid(const InputScanner *s, int button_index)
{
    input_bits_t bit;

    if (!s) {
        return 0;
    }

    bit = input_button_bit(button_index);
    if (bit == (input_bits_t)0) {
        return 0;
    }

    return ((s->valid_mask & bit) != (input_bits_t)0);
}

int input_button_down(const InputScanner *s, int button_index)
{
    input_bits_t bit;

    if (!s) {
        return 0;
    }

    bit = input_button_bit(button_index);
    if (bit == (input_bits_t)0 || (s->valid_mask & bit) == (input_bits_t)0) {
        return 0;
    }

    return ((s->current_bits & bit) != (input_bits_t)0);
}

int input_button_hold(const InputScanner *s, int button_index)
{
    return input_button_down(s, button_index);
}

int input_button_pressed(const InputScanner *s, int button_index)
{
    input_bits_t bit;

    if (!s) {
        return 0;
    }

    bit = input_button_bit(button_index);
    if (bit == (input_bits_t)0 || (s->valid_mask & bit) == (input_bits_t)0) {
        return 0;
    }

    return ((s->pressed_bits & bit) != (input_bits_t)0);
}

int input_button_released(const InputScanner *s, int button_index)
{
    input_bits_t bit;

    if (!s) {
        return 0;
    }

    bit = input_button_bit(button_index);
    if (bit == (input_bits_t)0 || (s->valid_mask & bit) == (input_bits_t)0) {
        return 0;
    }

    return ((s->released_bits & bit) != (input_bits_t)0);
}

int input_button_repeat(const InputScanner *s,
                        int button_index,
                        unsigned short delay_frames,
                        unsigned short interval_frames)
{
    unsigned short down_frames;
    unsigned short delta;

    if (!s || !input_button_is_valid(s, button_index)) {
        return 0;
    }
    if (delay_frames == 0U || interval_frames == 0U) {
        return 0;
    }

    down_frames = s->down_frames[button_index];
    if (down_frames <= delay_frames) {
        return 0;
    }

    delta = (unsigned short)(down_frames - delay_frames - 1U);
    return ((delta % interval_frames) == 0U);
}

int input_button_long_press(const InputScanner *s,
                            int button_index,
                            unsigned short frames)
{
    if (!s || !input_button_is_valid(s, button_index) || frames == 0U) {
        return 0;
    }

    return (s->down_frames[button_index] == frames);
}

int input_button_long_hold(const InputScanner *s,
                           int button_index,
                           unsigned short frames)
{
    if (!s || !input_button_is_valid(s, button_index) || frames == 0U) {
        return 0;
    }

    return (s->down_frames[button_index] >= frames);
}

int input_button_tapped(const InputScanner *s,
                        int button_index,
                        unsigned short max_down_frames)
{
    if (!s || !input_button_is_valid(s, button_index) || max_down_frames == 0U) {
        return 0;
    }
    if (!input_button_released(s, button_index)) {
        return 0;
    }

    return (s->last_down_frames[button_index] <= max_down_frames);
}

unsigned short input_button_down_frames(const InputScanner *s, int button_index)
{
    if (!s || !input_button_is_valid(s, button_index)) {
        return 0U;
    }

    return s->down_frames[button_index];
}

unsigned short input_button_up_frames(const InputScanner *s, int button_index)
{
    if (!s || !input_button_is_valid(s, button_index)) {
        return 0U;
    }

    return s->up_frames[button_index];
}

unsigned short input_button_last_down_frames(const InputScanner *s, int button_index)
{
    if (!s || !input_button_is_valid(s, button_index)) {
        return 0U;
    }

    return s->last_down_frames[button_index];
}
