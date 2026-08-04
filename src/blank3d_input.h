#ifndef BLANK3D_INPUT_H
#define BLANK3D_INPUT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "input_hook.h"
#include "input_scanner.h"
#include "key_pc.h"
#include "scanemu89/scanemu89.h"

#ifdef _WIN32
#include "input_hook_backend_win32_async.h"
#endif

#define B3D_INPUT_KEY_BANKS 8
#define B3D_INPUT_BANK_WIDTH 32
#define B3D_INPUT_CAPTURE_BINDINGS 32

typedef enum Blank3DInputStateTag {
    B3D_INPUT_HOLD = 0,
    B3D_INPUT_PRESSED,
    B3D_INPUT_RELEASED,
    B3D_INPUT_REPEAT,
    B3D_INPUT_TAPPED,
    B3D_INPUT_LONG_HOLD
} Blank3DInputState;

typedef struct Blank3DInputTag {
    input_hook keyboard;
    ihk_backend backend;
#ifdef _WIN32
    ihk_win32_async_backend win32_async;
#endif
    InputScanner key_banks[B3D_INPUT_KEY_BANKS];
    int mouse_current[5];
    int mouse_previous[5];
    int mouse_pressed[5];
    int mouse_released[5];
    scanemu_ctx capture;
    scanemu_binding capture_storage[B3D_INPUT_CAPTURE_BINDINGS];
    int initialized;
} Blank3DInput;

void blank3d_input_init(Blank3DInput *input);
void blank3d_input_set_backend(Blank3DInput *input,
                               const ihk_backend *backend);
void blank3d_input_set_mouse_buttons(Blank3DInput *input,
                                     int left, int right, int middle,
                                     int x1, int x2);
void blank3d_input_update(Blank3DInput *input);
void blank3d_input_shutdown(Blank3DInput *input);

int blank3d_input_query(const Blank3DInput *input,
                        Blank3DInputState state,
                        const char *control_name);
int blank3d_input_query_name(const Blank3DInput *input,
                             const char *state_name,
                             const char *control_name);
key_pc_code blank3d_input_key_from_name(const char *name);
const char *blank3d_input_key_name(key_pc_code key);

void blank3d_input_begin_capture(Blank3DInput *input,
                                 const char *symbol,
                                 unsigned long listen_flags);
void blank3d_input_cancel_capture(Blank3DInput *input);
int blank3d_input_capture_get(const Blank3DInput *input,
                              const char *symbol,
                              scanemu_token *out_token);

#ifdef __cplusplus
}
#endif

#endif
