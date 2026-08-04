#ifndef INPUT_BACKEND_MOUSE_H
#define INPUT_BACKEND_MOUSE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "input_scanner.h"

enum input_mouse_button {
    INPUT_MOUSE_LEFT = 0,
    INPUT_MOUSE_RIGHT = 1,
    INPUT_MOUSE_MIDDLE = 2,
    INPUT_MOUSE_X1 = 3,
    INPUT_MOUSE_X2 = 4,
    INPUT_MOUSE_AUX1 = 5,
    INPUT_MOUSE_AUX2 = 6,
    INPUT_MOUSE_AUX3 = 7
};

typedef struct InputMouseSnapshot {
    int x;
    int y;
    int wheel_x;
    int wheel_y;
    input_bits_t buttons_down;
} InputMouseSnapshot;

typedef struct InputMouseBackend {
    InputMouseSnapshot current;
    InputMouseSnapshot previous;
    input_bits_t valid_mask;
} InputMouseBackend;

int input_mouse_backend_init(InputMouseBackend *backend);
int input_mouse_backend_set_valid_mask(InputMouseBackend *backend,
                                       input_bits_t valid_mask);
int input_mouse_backend_commit(InputMouseBackend *backend,
                               const InputMouseSnapshot *snapshot);
int input_mouse_backend_poll(void *user_data, input_bits_t *out_bits);

int input_mouse_x(const InputMouseBackend *backend);
int input_mouse_y(const InputMouseBackend *backend);
int input_mouse_delta_x(const InputMouseBackend *backend);
int input_mouse_delta_y(const InputMouseBackend *backend);
int input_mouse_wheel_x(const InputMouseBackend *backend);
int input_mouse_wheel_y(const InputMouseBackend *backend);
int input_mouse_wheel_delta_x(const InputMouseBackend *backend);
int input_mouse_wheel_delta_y(const InputMouseBackend *backend);

#ifdef __cplusplus
}
#endif

#endif /* INPUT_BACKEND_MOUSE_H */
