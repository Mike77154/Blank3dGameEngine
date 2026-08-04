#include "input_backend_mask.h"

int input_mask_backend_init(InputMaskBackend *backend, input_bits_t valid_mask)
{
    if (!backend) {
        return INPUT_ERR_NULL;
    }

    backend->bits = (input_bits_t)0;
    backend->valid_mask = valid_mask;
    return INPUT_OK;
}

int input_mask_backend_set_bits(InputMaskBackend *backend, input_bits_t bits)
{
    if (!backend) {
        return INPUT_ERR_NULL;
    }

    backend->bits = bits & backend->valid_mask;
    return INPUT_OK;
}

int input_mask_backend_poll(void *user_data, input_bits_t *out_bits)
{
    InputMaskBackend *backend;

    if (!user_data || !out_bits) {
        return INPUT_ERR_NULL;
    }

    backend = (InputMaskBackend *)user_data;
    *out_bits = backend->bits & backend->valid_mask;
    return INPUT_OK;
}
