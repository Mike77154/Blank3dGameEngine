#ifndef INPUT_BACKEND_MASK_H
#define INPUT_BACKEND_MASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "input_scanner.h"

typedef struct InputMaskBackend {
    input_bits_t bits;
    input_bits_t valid_mask;
} InputMaskBackend;

int input_mask_backend_init(InputMaskBackend *backend, input_bits_t valid_mask);
int input_mask_backend_set_bits(InputMaskBackend *backend, input_bits_t bits);
int input_mask_backend_poll(void *user_data, input_bits_t *out_bits);

#ifdef __cplusplus
}
#endif

#endif /* INPUT_BACKEND_MASK_H */
