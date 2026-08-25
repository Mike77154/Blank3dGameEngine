/* evcore_input_bridge.h - C89
 * Puente opcional: InputScanner -> input_ev_handler -> evcore.
 *
 * El header público NO depende del tipo real InputScanner para que esta
 * librería pueda compilar de forma autónoma. El .c sí requiere la
 * dependencia externa al construir el bridge.
 */

#ifndef EVCORE_INPUT_BRIDGE_H
#define EVCORE_INPUT_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "evcore.h"

#define EV_INPUT_PRESS    1001
#define EV_INPUT_HOLD     1002
#define EV_INPUT_RELEASE  1003

/* scanner debe apuntar a un InputScanner válido de la librería externa. */
void evcore_input_bridge_dispatch(const void *scanner);

#ifdef __cplusplus
}
#endif

#endif /* EVCORE_INPUT_BRIDGE_H */
