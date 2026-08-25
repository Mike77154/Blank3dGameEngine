#ifndef WINPCKEYS_BACKEND_H
#define WINPCKEYS_BACKEND_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "key_pc.h"
#include "polls_scanner_iface.h"
#include "input_keys89.h"

#define WINPCKEYS_MAX_BUTTONS 64

/* Backend de teclado PC para Windows (win32).
 *
 * ✅ Sin dependencia hardcodeada a input_scanner.
 * ✅ Consume un scanner externo mediante polls_scanner_iface.
 *
 * Flujo típico:
 *   1) winpckeys_backend_init(&kb)
 *   2) winpckeys_backend_attach_scanner(&kb, &scanner_iface)
 *   3) winpckeys_bind_button(&kb, 0, KEY_PC_SPACE) ...
 *   4) Por frame: winpckeys_backend_update(&kb)
 *   5) Consultas: winpckeys_button_pressed/hold/released
 */

typedef struct winpckeys_backend {
    key_pc_context key_ctx;

    /* bindings: botón lógico -> tecla física (KEY_PC_*) */
    key_pc_code button_keys[WINPCKEYS_MAX_BUTTONS];
    int          button_capacity;

    /* Scanner enchufable (NO builtin) */
    polls_scanner_iface scanner;
    int                scanner_attached;
} winpckeys_backend;

/* Inicializa key_pc + limpia bindings. No adjunta scanner. */
void winpckeys_backend_init(winpckeys_backend *kb);

/* Limpia estado interno. No destruye el scanner externo. */
void winpckeys_backend_shutdown(winpckeys_backend *kb);

/* Adjunta un scanner externo (input_scanner u otro).
 * - Copia el iface internamente
 * - Llama iface.connect(scanner, state_fn, kb)
 */
int  winpckeys_backend_attach_scanner(winpckeys_backend *kb,
                                     const polls_scanner_iface *scanner_iface);

/* Liga un botón lógico a una tecla física */
int  winpckeys_bind_button(winpckeys_backend *kb,
                           int button_index,
                           key_pc_code key);

/* Por frame: refresca estado de hardware y actualiza el scanner */
void winpckeys_backend_update(winpckeys_backend *kb);

/* Consultas directas */
int  winpckeys_button_hold(const winpckeys_backend *kb, int button_index);
int  winpckeys_button_pressed(const winpckeys_backend *kb, int button_index);
int  winpckeys_button_released(const winpckeys_backend *kb, int button_index);

/* Canonical/raw path used by the four-head input stack. */
int  winpckeys_input_key_down(const winpckeys_backend *kb, input_key89 key);
int  winpckeys_bind_input_key89(winpckeys_backend *kb,
                                int button_index,
                                input_key89 key);

#ifdef __cplusplus
}
#endif

#endif /* WINPCKEYS_BACKEND_H */
