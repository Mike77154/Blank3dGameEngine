#ifndef LINUXPCKEYS_BACKEND_H
#define LINUXPCKEYS_BACKEND_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "polls/key_pc.h"
#include "polls/polls_scanner_iface.h"

/* Backend de teclado PC para Linux (evdev). */

typedef struct linuxpckeys_backend {
    key_pc_context key_ctx;

    /* bindings: botón lógico -> tecla física (KEY_PC_*) */
    key_pc_code *button_keys;
    int          button_capacity;

    /* Scanner enchufable (NO builtin) */
    polls_scanner_iface scanner;
    int                scanner_attached;

    /* evdev */
    int fd;

    /* snapshot de teclas (EVIOCGKEY). Se refresca en linuxpckeys_backend_update(). */
    unsigned long *key_state;
    int            key_state_words;
} linuxpckeys_backend;

/* Inicializa el backend y abre un dispositivo de teclado (auto-detect).
 * No adjunta scanner.
 */
void linuxpckeys_backend_init(linuxpckeys_backend *kb);

/* Cierra fd y libera bindings. No destruye el scanner externo. */
void linuxpckeys_backend_shutdown(linuxpckeys_backend *kb);

/* Adjunta scanner externo. */
int  linuxpckeys_backend_attach_scanner(linuxpckeys_backend *kb,
                                       const polls_scanner_iface *scanner_iface);

/* Liga un botón lógico a una tecla física */
int  linuxpckeys_bind_button(linuxpckeys_backend *kb,
                             int button_index,
                             key_pc_code key);

/* Por frame: refresca estado de hardware + scanner.update() */
void linuxpckeys_backend_update(linuxpckeys_backend *kb);

/* Consultas directas */
int  linuxpckeys_button_hold(const linuxpckeys_backend *kb, int button_index);
int  linuxpckeys_button_pressed(const linuxpckeys_backend *kb, int button_index);
int  linuxpckeys_button_released(const linuxpckeys_backend *kb, int button_index);

#ifdef __cplusplus
}
#endif

#endif /* LINUXPCKEYS_BACKEND_H */
