# greload89 — recarga temporizada

Se encarga exclusivamente de la orden de recarga:

- recibe duración en milisegundos enteros;
- cuenta el tiempo restante;
- reporta si está activa o terminó;
- al terminar devuelve cuántos cartuchos deben regresar al cargador.

No modifica por sí misma el cargador ni la reserva. Así puede conectarse a recarga completa, parcial, por cartucho, cargadores intercambiables o munición infinita.
