#ifndef INPUT_BACKEND_KEYBOARD_H
#define INPUT_BACKEND_KEYBOARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "input_scanner.h"

#define INPUT_KEYBOARD_MAX_KEYS 256
#define INPUT_KEY_NONE (-1)

enum input_keycode {
    INPUT_KEY_BACKSPACE = 8,
    INPUT_KEY_TAB = 9,
    INPUT_KEY_ENTER = 13,
    INPUT_KEY_ESCAPE = 27,
    INPUT_KEY_SPACE = 32,
    INPUT_KEY_DELETE = 127,
    INPUT_KEY_UP = 128,
    INPUT_KEY_DOWN = 129,
    INPUT_KEY_LEFT = 130,
    INPUT_KEY_RIGHT = 131,
    INPUT_KEY_INSERT = 132,
    INPUT_KEY_HOME = 133,
    INPUT_KEY_END = 134,
    INPUT_KEY_PAGEUP = 135,
    INPUT_KEY_PAGEDOWN = 136,
    INPUT_KEY_LSHIFT = 137,
    INPUT_KEY_RSHIFT = 138,
    INPUT_KEY_LCTRL = 139,
    INPUT_KEY_RCTRL = 140,
    INPUT_KEY_LALT = 141,
    INPUT_KEY_RALT = 142,
    INPUT_KEY_CAPSLOCK = 143,
    INPUT_KEY_F1 = 144,
    INPUT_KEY_F2 = 145,
    INPUT_KEY_F3 = 146,
    INPUT_KEY_F4 = 147,
    INPUT_KEY_F5 = 148,
    INPUT_KEY_F6 = 149,
    INPUT_KEY_F7 = 150,
    INPUT_KEY_F8 = 151,
    INPUT_KEY_F9 = 152,
    INPUT_KEY_F10 = 153,
    INPUT_KEY_F11 = 154,
    INPUT_KEY_F12 = 155
};

typedef struct InputKeyboardBackend {
    unsigned char key_down[INPUT_KEYBOARD_MAX_KEYS];
    int button_to_key[INPUT_MAX_BUTTONS];
    input_bits_t valid_mask;
} InputKeyboardBackend;

int input_keyboard_backend_init(InputKeyboardBackend *backend);
int input_keyboard_backend_clear_state(InputKeyboardBackend *backend);
int input_keyboard_backend_clear_bindings(InputKeyboardBackend *backend);
int input_keyboard_backend_bind(InputKeyboardBackend *backend,
                                int button_index,
                                int key_code);
int input_keyboard_backend_unbind(InputKeyboardBackend *backend, int button_index);
int input_keyboard_backend_set_key(InputKeyboardBackend *backend,
                                   int key_code,
                                   int is_down);
int input_keyboard_backend_key_down(const InputKeyboardBackend *backend, int key_code);
int input_keyboard_backend_poll(void *user_data, input_bits_t *out_bits);

#ifdef __cplusplus
}
#endif

#endif /* INPUT_BACKEND_KEYBOARD_H */
