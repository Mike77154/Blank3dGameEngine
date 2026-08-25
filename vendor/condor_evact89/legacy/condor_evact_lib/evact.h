/* evact.h - pegamento entre eventos, condiciones y acciones */

#ifndef EVACT_H
#define EVACT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "actioncore.h"
#include "condcore.h"

typedef EVCORE_ID_T evact_rule_id;

#define EVACT_RULE_INVALID ((evact_rule_id)0)
#define EVACT_COND_ALWAYS  CONDCORE_ID_INVALID

/* Regla:
 * - type == EVCORE_MATCH_ANY para cualquier tipo
 * - code == EVCORE_MATCH_ANY para cualquier code
 * - cond_id == EVACT_COND_ALWAYS para "sin condición"
 * - act_id debe ser válido
 */
evact_rule_id evact_rule_register(evcore_value_t type,
                                  evcore_value_t code,
                                  condcore_id cond_id,
                                  actcore_id act_id);

/* Igual que register, pero reusa la regla si ya existe exactamente igual. */
evact_rule_id evact_rule_register_unique(evcore_value_t type,
                                         evcore_value_t code,
                                         condcore_id cond_id,
                                         actcore_id act_id);

/* Quita una regla por handle. Devuelve 1 si la quitó, 0 si no. */
int evact_rule_unregister(evact_rule_id id);

/* Procesa un evento contra la tabla de reglas.
 * Devuelve cuántas acciones llegaron a ejecutarse.
 */
int evact_process(const evcore_event_t *evt);

/* Adaptador para usar evact_process() como listener de evcore. */
void evact_listener(const evcore_event_t *evt, void *user);

/* Se engancha a evcore en modo "todos los tipos" y evita duplicados. */
evcore_sub_id evact_attach_all(void);

/* Limpia la tabla de reglas sin reiniciar seriales. */
void evact_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* EVACT_H */
