#ifndef ZRAGF_DEFLATE_HUFFMAN_H_INCLUDED
#define ZRAGF_DEFLATE_HUFFMAN_H_INCLUDED

#include "../zragflib_internal.h"
#include "deflate_blocks.h"
#include "deflate_huffman_shared.h"

/* ------------------------------------------------------------
 * Tamaños máximos según DEFLATE (RFC 1951)
 * ------------------------------------------------------------ */
#define ZRAGF_MAX_LITLEN  286   /* 0-285  (literales/length) */
#define ZRAGF_MAX_DIST     30   /* 0-29   (distancias)        */
#define ZRAGF_MAX_CODELEN  19   /* 0-18   (códigos CL)       */

/* ------------------------------------------------------------
 * Estructura de un código Huffman DEFLATE (len + code)
 * ------------------------------------------------------------ */
typedef struct
{
    unsigned code;  /* código canónico (MSB/LSB según uso) */
    int      len;   /* longitud en bits */
} zragf_huff_code;

/* ------------------------------------------------------------
 * Conjunto de tablas Huffman DEFLATE
 * ------------------------------------------------------------ */
typedef struct
{
    /* Códigos de literal/length (0..285) */
    zragf_huff_code litlen[ZRAGF_MAX_LITLEN];

    /* Códigos de distancia (0..29) */
    zragf_huff_code dist[ZRAGF_MAX_DIST];

    /* Códigos de longitudes de código (CL) (0..18) */
    zragf_huff_code codelen[ZRAGF_MAX_CODELEN];

    /* Longitudes crudas:
       - ll_len[i]  = longitud de símbolo litlen i
       - d_len[i]   = longitud de símbolo dist i
       - cl_len[i]  = longitud de símbolo CL i
    */
    int ll_len[ZRAGF_MAX_LITLEN];
    int d_len[ZRAGF_MAX_DIST];
    int cl_len[ZRAGF_MAX_CODELEN];

    /* HLIT / HDIST / HCLEN ya empacados al estilo DEFLATE */
    int hlit;
    int hdist;
    int hclen;

} zragf_huff_tables;

/* ------------------------------------------------------------
 * API
 * ------------------------------------------------------------ */

/* A partir de las estadísticas de un bloque (frecuencias),
 * genera ll_len[] y d_len[] (longitudes de código).
 */
void zragf_huff_build_lengths(zragf_huff_tables *ht,
                              const zragf_block_stats *st);

/* Convierte ll_len[] y d_len[] en códigos canónicos
 * litlen[] y dist[].
 */
void zragf_huff_make_canonical(zragf_huff_tables *ht);

/* Construye las longitudes CL (cl_len[]) y sus códigos (codelen[]). */
void zragf_huff_build_cl(zragf_huff_tables *ht);

/* Calcula HLIT, HDIST, HCLEN en base a ll_len[]/d_len[]/cl_len[]. */
void zragf_huff_compute_header_sizes(zragf_huff_tables *ht);

#endif /* ZRAGF_DEFLATE_HUFFMAN_H_INCLUDED */
