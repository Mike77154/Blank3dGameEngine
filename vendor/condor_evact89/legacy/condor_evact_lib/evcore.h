/* evcore.h - C89, estático, sin heap */

#ifndef EVCORE_H
#define EVCORE_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef EVCORE_VALUE_T
#define EVCORE_VALUE_T int
#endif

#ifndef EVCORE_ID_T
#define EVCORE_ID_T unsigned long
#endif

typedef EVCORE_VALUE_T evcore_value_t;
typedef EVCORE_ID_T    evcore_sub_id;

#define EVCORE_SUB_INVALID ((evcore_sub_id)0)
#define EVCORE_MATCH_ANY   ((evcore_value_t)-1)
#define EVCORE_EVENT_ANY   EVCORE_MATCH_ANY

typedef struct evcore_event_t {
    evcore_value_t type;  /* definido por el usuario */
    evcore_value_t code;  /* sub-tipo, botón, ID lógico, etc. */
    void          *data;  /* payload opaco */
} evcore_event_t;

typedef void (*evcore_listener_fn)(const evcore_event_t *evt, void *user);

/* Registra un listener y devuelve un handle estable (>0).
 * Devuelve EVCORE_SUB_INVALID si no hay espacio o fn == NULL.
 */
evcore_sub_id evcore_subscribe(evcore_value_t type,
                               evcore_listener_fn fn,
                               void *user);

/* Igual que evcore_subscribe(), pero si ya existe el mismo triplete
 * (type, fn, user), devuelve el handle existente en vez de duplicarlo.
 */
evcore_sub_id evcore_subscribe_unique(evcore_value_t type,
                                      evcore_listener_fn fn,
                                      void *user);

/* Quita una suscripción exacta por handle. Devuelve 1 si la quitó, 0 si no. */
int evcore_unsubscribe_id(evcore_sub_id id);

/* Quita todas las suscripciones cuyo par (fn, user) coincida.
 * Devuelve cuántas quitó.
 */
int evcore_unsubscribe(evcore_listener_fn fn, void *user);

/* Emite un evento. Seguro ante altas/bajas durante el dispatch:
 * - altas nuevas NO reciben el evento actual
 * - bajas hechas antes de su turno NO se ejecutan
 */
void evcore_emit(const evcore_event_t *evt);

/* Helper para evitar construir manualmente el struct. */
void evcore_emit_values(evcore_value_t type,
                        evcore_value_t code,
                        void *data);

/* Limpia todos los listeners activos.
 * Nota: los seriales internos NO se reinician, para que handles viejos
 * sigan siendo inválidos después de un reset.
 */
void evcore_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* EVCORE_H */
