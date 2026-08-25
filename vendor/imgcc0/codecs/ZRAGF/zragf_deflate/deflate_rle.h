/* deflate_rle.h */

#ifndef ZRAGF_DEFLATE_RLE_H_INCLUDED
#define ZRAGF_DEFLATE_RLE_H_INCLUDED

#include "zragflib_internal.h"

/* Entrada típica: 
   - ll_len[0..hlit+256]
   - d_len[0..hdist]
   Orden exacto DEFLATE: todas las ll_len seguidas de d_len.
*/

typedef struct {
    int      sym;        /* 0–18 (código CL) */
    unsigned extra_bits; /* # de bits extra */
    unsigned extra_val;  /* valor de esos bits */
} zragf_cl_token;

/* Genera secuencia RLE de longitudes de código (para HLIT+HDIST)
   Devuelve cuántos tokens generó.
   max_tokens = capacidad del buffer out (ej: 1024)
*/
zragf_size_t
zragf_huff_rle_lengths(const int *ll_len, int hlit,
                       const int *d_len,  int hdist,
                       zragf_cl_token *out,
                       zragf_size_t    max_tokens);

#endif /* ZRAGF_DEFLATE_RLE_H_INCLUDED */
