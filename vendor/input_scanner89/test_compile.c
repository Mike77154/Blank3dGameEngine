#include "input_scanner.h"
#include "input_ev_handler.h"
#include "input_backend_mask.h"
#include "input_backend_keyboard.h"
#include "input_backend_mouse.h"
#include "input_backend_pad.h"

static void input_test_handler(const input_event *ev, void *user_data)
{
    int *counter;

    counter = (int *)user_data;
    if (!ev || !counter) {
        return;
    }

    *counter += ev->button_index;
    *counter += (int)ev->kind;
    *counter += (int)ev->frames;
}

int main(void)
{
    InputScanner scanner;
    InputMaskBackend mask_backend;
    InputKeyboardBackend keyboard_backend;
    InputMouseBackend mouse_backend;
    InputPadBackend pad_backend;
    InputMouseSnapshot mouse_snapshot;
    InputPadSnapshot pad_snapshot;
    int result;
    int counter;

    counter = 0;

    result = input_mask_backend_init(&mask_backend, input_mask_from_button_count(16));
    if (result != INPUT_OK) {
        return 1;
    }

    result = input_scanner_init(&scanner, input_mask_backend_poll, &mask_backend);
    if (result != INPUT_OK) {
        return 2;
    }

    result = input_scanner_set_button_count(&scanner, 16);
    if (result != INPUT_OK) {
        return 3;
    }

    result = input_mask_backend_set_bits(&mask_backend,
                                         input_button_bit(0) |
                                         input_button_bit(1));
    if (result != INPUT_OK) {
        return 4;
    }

    result = input_scanner_update(&scanner);
    if (result != INPUT_OK || !input_button_pressed(&scanner, 0)) {
        return 5;
    }

    result = input_dispatch_events_ex(&scanner,
                                      INPUT_DISPATCH_PRESS |
                                      INPUT_DISPATCH_HOLD,
                                      3U,
                                      2U,
                                      input_test_handler,
                                      &counter);
    if (result != INPUT_OK || counter <= 0) {
        return 6;
    }

    result = input_keyboard_backend_init(&keyboard_backend);
    if (result != INPUT_OK) {
        return 7;
    }
    result = input_keyboard_backend_bind(&keyboard_backend, 0, 'Z');
    if (result != INPUT_OK) {
        return 8;
    }
    result = input_keyboard_backend_set_key(&keyboard_backend, 'Z', 1);
    if (result != INPUT_OK) {
        return 9;
    }

    result = input_scanner_set_backend(&scanner, input_keyboard_backend_poll, &keyboard_backend);
    if (result != INPUT_OK) {
        return 10;
    }
    result = input_scanner_set_valid_mask(&scanner, keyboard_backend.valid_mask);
    if (result != INPUT_OK) {
        return 11;
    }
    result = input_scanner_reset(&scanner);
    if (result != INPUT_OK) {
        return 12;
    }
    result = input_scanner_update(&scanner);
    if (result != INPUT_OK || !input_button_pressed(&scanner, 0)) {
        return 13;
    }

    result = input_mouse_backend_init(&mouse_backend);
    if (result != INPUT_OK) {
        return 14;
    }

    mouse_snapshot.x = 20;
    mouse_snapshot.y = 10;
    mouse_snapshot.wheel_x = 0;
    mouse_snapshot.wheel_y = 2;
    mouse_snapshot.buttons_down = input_button_bit(INPUT_MOUSE_LEFT);

    result = input_mouse_backend_commit(&mouse_backend, &mouse_snapshot);
    if (result != INPUT_OK) {
        return 15;
    }

    result = input_scanner_set_backend(&scanner, input_mouse_backend_poll, &mouse_backend);
    if (result != INPUT_OK) {
        return 16;
    }
    result = input_scanner_set_valid_mask(&scanner, mouse_backend.valid_mask);
    if (result != INPUT_OK) {
        return 17;
    }
    result = input_scanner_reset(&scanner);
    if (result != INPUT_OK) {
        return 18;
    }
    result = input_scanner_update(&scanner);
    if (result != INPUT_OK || !input_button_down(&scanner, INPUT_MOUSE_LEFT)) {
        return 19;
    }
    if (input_mouse_delta_x(&mouse_backend) != 20 || input_mouse_wheel_delta_y(&mouse_backend) != 2) {
        return 20;
    }

    result = input_pad_backend_init_xinput(&pad_backend);
    if (result != INPUT_OK) {
        return 21;
    }

    pad_snapshot.buttons_down = input_button_bit(INPUT_PAD_FACE_SOUTH);
    pad_snapshot.left_x = 0;
    pad_snapshot.left_y = 0;
    pad_snapshot.right_x = 0;
    pad_snapshot.right_y = 0;
    pad_snapshot.left_trigger = 0U;
    pad_snapshot.right_trigger = 40000U;
    pad_snapshot.hat_x = 1;
    pad_snapshot.hat_y = 0;

    result = input_pad_backend_set_snapshot(&pad_backend, &pad_snapshot);
    if (result != INPUT_OK) {
        return 22;
    }

    result = input_scanner_set_backend(&scanner, input_pad_backend_poll, &pad_backend);
    if (result != INPUT_OK) {
        return 23;
    }
    result = input_scanner_set_valid_mask(&scanner, pad_backend.valid_mask);
    if (result != INPUT_OK) {
        return 24;
    }
    result = input_scanner_reset(&scanner);
    if (result != INPUT_OK) {
        return 25;
    }
    result = input_scanner_update(&scanner);
    if (result != INPUT_OK) {
        return 26;
    }
    if (!input_button_down(&scanner, INPUT_PAD_FACE_SOUTH) ||
        !input_button_down(&scanner, INPUT_PAD_RIGHT) ||
        !input_button_down(&scanner, INPUT_PAD_R2)) {
        return 27;
    }

    return 0;
}
