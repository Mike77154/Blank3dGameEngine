#include "input_backend_keyboard.h"

int input_keyboard_backend_clear_state(InputKeyboardBackend *backend)
{
    int i;

    if (!backend) {
        return INPUT_ERR_NULL;
    }

    for (i = 0; i < INPUT_KEYBOARD_MAX_KEYS; ++i) {
        backend->key_down[i] = 0U;
    }

    return INPUT_OK;
}

int input_keyboard_backend_clear_bindings(InputKeyboardBackend *backend)
{
    int i;

    if (!backend) {
        return INPUT_ERR_NULL;
    }

    for (i = 0; i < INPUT_MAX_BUTTONS; ++i) {
        backend->button_to_key[i] = INPUT_KEY_NONE;
    }
    backend->valid_mask = (input_bits_t)0;

    return INPUT_OK;
}

int input_keyboard_backend_init(InputKeyboardBackend *backend)
{
    int result;

    if (!backend) {
        return INPUT_ERR_NULL;
    }

    result = input_keyboard_backend_clear_state(backend);
    if (result != INPUT_OK) {
        return result;
    }

    return input_keyboard_backend_clear_bindings(backend);
}

int input_keyboard_backend_bind(InputKeyboardBackend *backend,
                                int button_index,
                                int key_code)
{
    input_bits_t bit;

    if (!backend) {
        return INPUT_ERR_NULL;
    }
    if (key_code < 0 || key_code >= INPUT_KEYBOARD_MAX_KEYS) {
        return INPUT_ERR_BAD_INDEX;
    }

    bit = input_button_bit(button_index);
    if (bit == (input_bits_t)0) {
        return INPUT_ERR_BAD_INDEX;
    }

    backend->button_to_key[button_index] = key_code;
    backend->valid_mask |= bit;
    return INPUT_OK;
}

int input_keyboard_backend_unbind(InputKeyboardBackend *backend, int button_index)
{
    input_bits_t bit;

    if (!backend) {
        return INPUT_ERR_NULL;
    }

    bit = input_button_bit(button_index);
    if (bit == (input_bits_t)0) {
        return INPUT_ERR_BAD_INDEX;
    }

    backend->button_to_key[button_index] = INPUT_KEY_NONE;
    backend->valid_mask &= ~bit;
    return INPUT_OK;
}

int input_keyboard_backend_set_key(InputKeyboardBackend *backend,
                                   int key_code,
                                   int is_down)
{
    if (!backend) {
        return INPUT_ERR_NULL;
    }
    if (key_code < 0 || key_code >= INPUT_KEYBOARD_MAX_KEYS) {
        return INPUT_ERR_BAD_INDEX;
    }

    backend->key_down[key_code] = (unsigned char)(is_down ? 1U : 0U);
    return INPUT_OK;
}

int input_keyboard_backend_key_down(const InputKeyboardBackend *backend, int key_code)
{
    if (!backend || key_code < 0 || key_code >= INPUT_KEYBOARD_MAX_KEYS) {
        return 0;
    }

    return (backend->key_down[key_code] != 0U);
}


int input_keyboard_backend_bind_input_key89(InputKeyboardBackend *backend,
                                            int button_index,
                                            input_key89 key)
{
    unsigned int usage;
    if (!input_keys89_keyboard_usage(key, &usage)) return INPUT_ERR_BAD_INDEX;
    if (usage >= INPUT_KEYBOARD_MAX_KEYS) return INPUT_ERR_BAD_INDEX;
    return input_keyboard_backend_bind(backend, button_index, (int)usage);
}

int input_keyboard_backend_set_input_key89(InputKeyboardBackend *backend,
                                           input_key89 key,
                                           int is_down)
{
    unsigned int usage;
    if (!input_keys89_keyboard_usage(key, &usage)) return INPUT_ERR_BAD_INDEX;
    if (usage >= INPUT_KEYBOARD_MAX_KEYS) return INPUT_ERR_BAD_INDEX;
    return input_keyboard_backend_set_key(backend, (int)usage, is_down);
}

int input_keyboard_backend_input_key89_down(const InputKeyboardBackend *backend,
                                             input_key89 key)
{
    unsigned int usage;
    if (!input_keys89_keyboard_usage(key, &usage)) return 0;
    if (usage >= INPUT_KEYBOARD_MAX_KEYS) return 0;
    return input_keyboard_backend_key_down(backend, (int)usage);
}

int input_keyboard_backend_poll(void *user_data, input_bits_t *out_bits)
{
    InputKeyboardBackend *backend;
    input_bits_t bits;
    input_bits_t bit;
    int i;
    int key_code;

    if (!user_data || !out_bits) {
        return INPUT_ERR_NULL;
    }

    backend = (InputKeyboardBackend *)user_data;
    bits = (input_bits_t)0;

    for (i = 0; i < INPUT_MAX_BUTTONS; ++i) {
        bit = input_button_bit(i);
        if (bit == (input_bits_t)0 || (backend->valid_mask & bit) == (input_bits_t)0) {
            continue;
        }

        key_code = backend->button_to_key[i];
        if (key_code >= 0 && key_code < INPUT_KEYBOARD_MAX_KEYS && backend->key_down[key_code] != 0U) {
            bits |= bit;
        }
    }

    *out_bits = bits & backend->valid_mask;
    return INPUT_OK;
}
