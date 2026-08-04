#ifndef MACPCKEYS_BACKEND_H
#define MACPCKEYS_BACKEND_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "polls/key_pc.h"
#include "polls/polls_scanner_iface.h"

/* Backend de teclado PC para macOS. */

typedef struct macpckeys_backend {
    key_pc_context key_ctx;

    /* bindings: botón lógico -> tecla física (KEY_PC_*) */
    key_pc_code *button_keys;
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

#ifdef __cplusplus
}
#endif

#endif /* MACPCKEYS_BACKEND_H */
