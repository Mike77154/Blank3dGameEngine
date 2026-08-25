#ifndef ZRAGF_DEFLATE_HASH_H_INCLUDED
#define ZRAGF_DEFLATE_HASH_H_INCLUDED

#include "../zragflib_internal.h"
#include "deflate_core.h"

/* ------------------------------------------------------------
   Tamaño del hash DEFLATE (15 bits = 32768 entradas)
   ------------------------------------------------------------ */
#define ZRAGF_HASH_BITS   15
#define ZRAGF_HASH_SIZE   (1 << ZRAGF_HASH_BITS)
#define ZRAGF_HASH_MASK   (ZRAGF_HASH_SIZE - 1)

/* Fuerza máxima de búsqueda (evitar cadenas infinitas) */
#define ZRAGF_MAX_CHAIN_DEFAULT  256

/* ------------------------------------------------------------
   Funciones expuestas
   ------------------------------------------------------------ */

/* Inserta posición actual en el hash y devuelve head anterior */
int zragf_hash_insert(zragf_deflate_state *st, int pos);

/* Busca un match (rápido, con heurísticas) */
void zragf_hash_find(zragf_deflate_state *st,
                     int *out_len,
                     int *out_dist);

/* Función interna para comparar runs rápidamente */
int zragf_hash_run_compare(const zragf_u8 *a,
                           const zragf_u8 *b,
                           int max);

#endif /* ZRAGF_DEFLATE_HASH_H_INCLUDED */
