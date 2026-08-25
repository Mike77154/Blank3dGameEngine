#ifndef ZRAGF_INFLATE_HUFFMAN_H_INCLUDED
#define ZRAGF_INFLATE_HUFFMAN_H_INCLUDED

#include "../zragflib_internal.h"

/* Tamaños máximos DEFLATE (RFC 1951) */
#define ZRAGF_INF_MAX_LITLEN 286
#define ZRAGF_INF_FIXED_LITLEN 288
#define ZRAGF_INF_MAX_DIST    30

/* ------------------------------------------------------------
 * Entrada de tabla de decodificación:
 *  code   = código canónico invertido para lectura LSB-first
 *  bits   = cuántos bits consume ese símbolo
 *  symbol = símbolo decodificado
 * ------------------------------------------------------------ */
typedef struct
{
    unsigned code;
    int      bits;
    int      symbol;
} zragf_idec_entry;

/* Tabla de decodificación compacta */
typedef struct
{
    zragf_idec_entry *table;
    int               table_bits; /* reservado para compatibilidad */
    int               count;
    int               max_bits;
} zragf_idec_table;

/* Conjunto de tablas Huffman para inflate */
typedef struct
{
    zragf_idec_table litlen; /* códigos de literal/length */
    zragf_idec_table dist;   /* códigos de distancia      */
} zragf_inflate_huff;

/* Callback de bitreader: getbit(ctx, &b) → 0/1 en *b, 1 = ok */
typedef int (*zragf_getbit_fn)(void *ctx, int *bit);

/* Construye tabla de decodificación a partir de longitudes
 * lengths[i] = longitud en bits del símbolo i (0 = no usado).
 */
int zragf_inflate_build_table(const int *lengths,
                              int nlen,
                              int table_bits,
                              zragf_idec_table *out_tab);

/* Inicializa tablas fijas DEFLATE (BTYPE=01) */
int zragf_inflate_init_fixed(zragf_inflate_huff *h);

/* Libera tablas de decodificación */
void zragf_inflate_free(zragf_inflate_huff *h);

/* Decodifica un símbolo usando la tabla y getbit() */
int zragf_inflate_decode_symbol(zragf_idec_table *tab,
                                zragf_getbit_fn   getbit,
                                void             *ctx,
                                int              *out_sym);

#endif /* ZRAGF_INFLATE_HUFFMAN_H_INCLUDED */
