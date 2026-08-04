/* polls_bits.h - Tipos básicos para polling/bindings (C89 compatible)
 *
 * Este header existe para eliminar la dependencia hardcodeada a "input_scanner".
 *
 * En lugar de asumir un "scanner" específico, polls expone:
 *  - un callback para consultar el estado (DOWN/UP) de un "botón lógico".
 */

#ifndef POLLS_BITS_H
#define POLLS_BITS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Estado actual de un botón lógico (0 = UP, !=0 = DOWN) */
typedef int (*polls_button_state_fn)(void *user_data, int button_index);

#ifdef __cplusplus
}
#endif

#endif /* POLLS_BITS_H */
