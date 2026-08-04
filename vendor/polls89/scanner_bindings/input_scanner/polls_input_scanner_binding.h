#ifndef POLLS_INPUT_SCANNER_BINDING_H
#define POLLS_INPUT_SCANNER_BINDING_H

#ifdef __cplusplus
extern "C" {
#endif

/* Este adapter permite enchufar la lib externa "input_scanner" a polls.
 *
 * ⚠️ Importante:
 *   - Este target SÍ depende de input_scanner.
 *   - La lib base polls NO depende de input_scanner.
 */

#include "polls/polls_scanner_iface.h"

/* Incluye del proyecto externo (input_scanner)
 *
 * Se asume que el include path apunta al folder que contiene "input_scanner.h"
 * (por ejemplo, el target input_scanner expone su include dir como PUBLIC).
 */
#include "input_scanner.h"

/* Wrapper que guarda:
 *  - el InputScanner real
 *  - el callback de polls (state_fn) que consulta si un botón está DOWN
 */
typedef struct polls_input_scanner_binding {
    InputScanner scanner;
    polls_button_state_fn state_fn;
    void *state_user_data;
} polls_input_scanner_binding;

/* Devuelve una interfaz polls_scanner_iface para consumir este scanner.
 *
 * Uso típico:
 *   polls_input_scanner_binding b;
 *   polls_scanner_iface iface = polls_make_input_scanner_iface(&b);
 *   winpckeys_backend_attach_scanner(&kb, &iface);
 */
polls_scanner_iface polls_make_input_scanner_iface(polls_input_scanner_binding *binding);

#ifdef __cplusplus
}
#endif

#endif /* POLLS_INPUT_SCANNER_BINDING_H */