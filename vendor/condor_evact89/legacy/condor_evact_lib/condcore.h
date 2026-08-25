/* condcore.h - C89, condiciones estáticas */

#ifndef CONDCORE_H
#define CONDCORE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "evcore.h"

typedef EVCORE_ID_T condcore_id;

#define CONDCORE_ID_INVALID ((condcore_id)0)

typedef int (*condcore_eval_fn)(const evcore_event_t *evt, void *user);
/* devuelve 1 = condición verdadera, 0 = falsa */

/* Registra una condición y devuelve un handle estable (>0). */
condcore_id condcore_register(condcore_eval_fn fn, void *user);

/* Elimina una condición. Devuelve 1 si la quitó, 0 si no. */
int condcore_unregister(condcore_id id);

/* Evalúa una condición. Si el handle es inválido, devuelve 0. */
int condcore_eval(condcore_id id, const evcore_event_t *evt);

/* Limpia la tabla activa sin reiniciar seriales. */
void condcore_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* CONDCORE_H */
