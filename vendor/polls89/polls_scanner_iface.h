/* polls_scanner_iface.h - Interfaz mínima para enchufar scanners (C89)
 *
 * polls NO incluye un scanner builtin.
 *
 * En su lugar, expone una interfaz (vtable) muy pequeña para que:
 *   - input_scanner (u otros) se "bindee" a polls
 *   - los backends por sistema (win32/linux/mac, etc.) consuman el scanner
 *
 * La idea es:
 *   backend (teclado real) -> polls_button_state_fn -> scanner externo
 */

#ifndef POLLS_SCANNER_IFACE_H
#define POLLS_SCANNER_IFACE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "polls_bits.h"

/* Conecta un scanner con una función que responde el estado de cada botón.
 *
 * Debe retornar 0 en éxito.
 */
typedef int (*polls_scanner_connect_fn)(void *scanner,
                                       polls_button_state_fn state_fn,
                                       void *state_user_data);

/* Tick / frame update */
typedef void (*polls_scanner_update_fn)(void *scanner);

/* Consultas para un botón (0..max_buttons-1) */
typedef int (*polls_scanner_query_fn)(const void *scanner, int button_index);

/* Interfaz enchufable */
typedef struct polls_scanner_iface {
    void *scanner;
    int   max_buttons; /* número máximo de botones lógicos */

    polls_scanner_connect_fn connect;
    polls_scanner_update_fn  update;

    polls_scanner_query_fn   hold;
    polls_scanner_query_fn   pressed;
    polls_scanner_query_fn   released;
} polls_scanner_iface;

#ifdef __cplusplus
}
#endif

#endif /* POLLS_SCANNER_IFACE_H */
