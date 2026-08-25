/* deflate_fixed.h */

#ifndef ZRAGF_DEFLATE_FIXED_H_INCLUDED
#define ZRAGF_DEFLATE_FIXED_H_INCLUDED

#include "zragflib_internal.h"
#include "deflate_tokens.h"

/* Este bitwriter debe coincidir con el que usas en deflate_stream.c */
typedef struct {
    zragf_u8    *out;
    zragf_size_t out_pos;
    zragf_size_t out_cap;
    unsigned      bitbuf;
    int           bitcount;
} zragf_bw_lsbf;

/* Tablas Huffman fijas (lit/len y dist) */
typedef struct {
    unsigned code;
    int      len;
} zragf_hcode_fixed;

typedef struct {
    zragf_hcode_fixed litlen[288]; /* 0–287 (RFC1951 fixed tree completo) */
    zragf_hcode_fixed dist[30];    /* 0–29  */
} zragf_fixed_tables;

/* Inicializa tablas fijas según RFC1951 */
void zragf_fixed_init_tables(zragf_fixed_tables *ft);

/* Devuelve tablas fijas preinicializadas para el hot path. */
const zragf_fixed_tables *zragf_fixed_get_tables(void);

/* Emite un bloque FIXED completo:
   - cabecera (BFINAL/BTYPE=01)
   - todos los tokens
   - símbolo 256 (EOB)
 */
int zragf_fixed_emit_block(zragf_bw_lsbf            *bw,
                           const zragf_fixed_tables *ft,
                           const zragf_token        *toks,
                           zragf_size_t              ntoks,
                           int                       final_block);

#endif /* ZRAGF_DEFLATE_FIXED_H_INCLUDED */
