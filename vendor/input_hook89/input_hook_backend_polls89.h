#ifndef INPUT_HOOK_BACKEND_POLLS89_H
#define INPUT_HOOK_BACKEND_POLLS89_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "input_hook.h"
#include "input_keys89.h"

/* Canonical polls89 -> input_hook89 adapter.
 *
 * Unlike the legacy key_pc bank adapter, this contract asks a polls backend
 * for RAW physical key state using input_keys89 identity.  Therefore:
 *   polls89        = OS acquisition
 *   input_hook89   = full HID snapshot/capture
 *   input_scanner89= temporal semantics later in the chain
 *
 * No allocator and no logical-button rebinding are required here. */

typedef void (*ihk_polls89_update_fn)(void *backend);
typedef int  (*ihk_polls89_key_down_fn)(const void *backend, input_key89 key);

typedef struct ihk_polls89_key_api {
    ihk_polls89_update_fn update;
    ihk_polls89_key_down_fn key_down;
    ihk_u32 capabilities;
} ihk_polls89_key_api;

typedef struct ihk_polls89_backend {
    void *polls_backend;
    const ihk_polls89_key_api *api;
    ihk_backend out_backend;
} ihk_polls89_backend;

int ihk_polls89_backend_init(ihk_polls89_backend *adapter,
                             void *polls_backend,
                             const ihk_polls89_key_api *api);
void ihk_polls89_backend_shutdown(ihk_polls89_backend *adapter);
const ihk_backend *ihk_polls89_backend_as_ihk(const ihk_polls89_backend *adapter);

#ifdef __cplusplus
}
#endif

#endif
