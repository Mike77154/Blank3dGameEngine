#ifndef ZRAGF_DEFLATE_LAZY_H_INCLUDED
#define ZRAGF_DEFLATE_LAZY_H_INCLUDED

#include "../zragflib_internal.h"
#include "deflate_core.h"

/* ------------------------------------------------------------
   Lazy Matching Avanzado
   ------------------------------------------------------------
   Esta capa toma los matches encontrados por zragf_hash_find()
   y decide si:

   - usar el match actual,
   - esperar al siguiente literal para ver si mejora,
   - o emitir literal si el match no es suficientemente bueno.
   ------------------------------------------------------------ */

/* Selecciona match final (longitud + distancia) */
void zragf_lazy_select(zragf_deflate_state *st,
                       int raw_len,
                       int raw_dist,
                       int *out_len,
                       int *out_dist);

#endif /* ZRAGF_DEFLATE_LAZY_H_INCLUDED */
