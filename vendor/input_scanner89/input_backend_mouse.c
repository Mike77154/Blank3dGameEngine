#include "input_backend_mouse.h"

int input_mouse_backend_init(InputMouseBackend *backend)
{
    if (!backend) {
        return INPUT_ERR_NULL;
    }

    backend->current.x = 0;
    backend->current.y = 0;
    backend->current.wheel_x = 0;
    backend->current.wheel_y = 0;
    backend->current.buttons_down = (input_bits_t)0;

    backend->previous = backend->current;
    backend->valid_mask = input_mask_from_button_count(8);
    return INPUT_OK;
}

int input_mouse_backend_set_valid_mask(InputMouseBackend *backend,
                                       input_bits_t valid_mask)
{
    if (!backend) {
        return INPUT_ERR_NULL;
    }

    backend->valid_mask = valid_mask;
    backend->current.buttons_down &= valid_mask;
    backend->previous.buttons_down &= valid_mask;
    return INPUT_OK;
}

int input_mouse_backend_commit(InputMouseBackend *backend,
                               const InputMouseSnapshot *snapshot)
{
    if (!backend || !snapshot) {
        return INPUT_ERR_NULL;
    }

    backend->previous = backend->current;
    backend->current = *snapshot;
    backend->current.buttons_down &= backend->valid_mask;
    return INPUT_OK;
}

int input_mouse_backend_poll(void *user_data, input_bits_t *out_bits)
{
    InputMouseBackend *backend;

    if (!user_data || !out_bits) {
        return INPUT_ERR_NULL;
    }

    backend = (InputMouseBackend *)user_data;
    *out_bits = backend->current.buttons_down & backend->valid_mask;
    return INPUT_OK;
}

int input_mouse_x(const InputMouseBackend *backend)
{
    if (!backend) {
        return 0;
    }

    return backend->current.x;
}

int input_mouse_y(const InputMouseBackend *backend)
{
    if (!backend) {
        return 0;
    }

    return backend->current.y;
}

int input_mouse_delta_x(const InputMouseBackend *backend)
{
    if (!backend) {
        return 0;
    }

    return backend->current.x - backend->previous.x;
}

int input_mouse_delta_y(const InputMouseBackend *backend)
{
    if (!backend) {
        return 0;
    }

    return backend->current.y - backend->previous.y;
}

int input_mouse_wheel_x(const InputMouseBackend *backend)
{
    if (!backend) {
        return 0;
    }

    return backend->current.wheel_x;
}

int input_mouse_wheel_y(const InputMouseBackend *backend)
{
    if (!backend) {
        return 0;
    }

    return backend->current.wheel_y;
}

int input_mouse_wheel_delta_x(const InputMouseBackend *backend)
{
    if (!backend) {
        return 0;
    }

    return backend->current.wheel_x - backend->previous.wheel_x;
}

int input_mouse_wheel_delta_y(const InputMouseBackend *backend)
{
    if (!backend) {
        return 0;
    }

    return backend->current.wheel_y - backend->previous.wheel_y;
}
