/* actioncore.h - C89, acciones estáticas */

#ifndef ACTCORE_H
#define ACTCORE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "evcore.h"

typedef EVCORE_ID_T actcore_id;

#define ACTCORE_ID_INVALID ((actcore_id)0)

typedef void (*actcore_exec_fn)(const evcore_event_t *evt, void *user);

/* Registra una acción y devuelve un handle estable (>0). */
actcore_id actcore_register(actcore_exec_fn fn, void *user);

/* Elimina una acción. Devuelve 1 si la quitó, 0 si no. */
int actcore_unregister(actcore_id id);

/* Ejecuta la acción y devuelve 1 si realmente la ejecutó, 0 si no. */
int actcore_exec(actcore_id id, const evcore_event_t *evt);

/* Limpia la tabla activa sin reiniciar seriales. */
void actcore_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* ACTCORE_H */
