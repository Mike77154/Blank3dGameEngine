#ifndef MACPCKEYS_BACKEND_H
#define MACPCKEYS_BACKEND_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "key_pc.h"
#include "polls_scanner_iface.h"
#include "input_keys89.h"

#define MACPCKEYS_MAX_BUTTONS 64

/* Backend de teclado PC para macOS. */

typedef struct macpckeys_backend {
    key_pc_context key_ctx;

    /* bindings: botón lógico -> tecla física (KEY_PC_*) */
    key_pc_code button_keys[MACPCKEYS_MAX_BUTTONS];
    int          button_capacity;

    /* Scanner enchufable (NO builtin) */
    polls_scanner_iface scanner;
    int                scanner_attached;
} macpckeys_backend;

void macpckeys_backend_init(macpckeys_backend *kb);
void macpckeys_backend_shutdown(macpckeys_backend *kb);

int  macpckeys_backend_attach_scanner(macpckeys_backend *kb,
                                     const polls_scanner_iface *scanner_iface);

int  macpckeys_bind_button(macpckeys_backend *kb, int button_index, key_pc_code key);

void macpckeys_backend_update(macpckeys_backend *kb);

int  macpckeys_button_hold(const macpckeys_backend *kb, int button_index);
int  macpckeys_button_pressed(const macpckeys_backend *kb, int button_index);
int  macpckeys_button_released(const macpckeys_backend *kb, int button_index);
int  macpckeys_input_key_down(const macpckeys_backend *kb, input_key89 key);
int  macpckeys_bind_input_key89(macpckeys_backend *kb, int button_index, input_key89 key);

#ifdef __cplusplus
}
#endif

#endif /* MACPCKEYS_BACKEND_H */
