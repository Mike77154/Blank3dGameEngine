#ifndef ZRAGF_DEFLATE_BLOCKS_H_INCLUDED
#define ZRAGF_DEFLATE_BLOCKS_H_INCLUDED

#include "../zragflib_internal.h"
#include "deflate_core.h"

/* ------------------------------------------------------------
   Tipo de bloque DEFLATE
   ------------------------------------------------------------ */
typedef enum
{
    ZRAGF_BLOCK_STORED = 0,
    ZRAGF_BLOCK_FIXED  = 1,
    ZRAGF_BLOCK_DYNAMIC= 2
} zragf_block_type;

/* ------------------------------------------------------------
   Estadísticas de un bloque
   (para decidir BTYPE y construir Huffman luego)
   ------------------------------------------------------------ */
typedef struct
{
    /* Frecuencias de literal/length (0–285) */
    unsigned litlen_freq[286];

    /* Frecuencias de distancias (0–29) */
    unsigned dist_freq[30];

    /* Tamaño real del bloque en bytes de entrada */
    int      raw_size;

} zragf_block_stats;

/* ------------------------------------------------------------
   API
   ------------------------------------------------------------ */

/* Inicializa las estadísticas a cero */
void zragf_blocks_reset_stats(zragf_block_stats *st);

/* Tablas rápidas para mapear length/dist -> símbolo RFC1951. */
const unsigned short *zragf_blocks_len_sym_table(void);
const unsigned short *zragf_blocks_dist_sym_table(void);

/* Registra un literal en las estadísticas */
void zragf_blocks_add_literal(zragf_block_stats *st, int lit);

/* Registra un match (length+dist) en las estadísticas */
void zragf_blocks_add_match(zragf_block_stats *st,
                            int length,
                            int dist);

/* Decide BTYPE óptimo según heurísticas simples:
   - Si raw_size pequeño → STORED
   - Si distribución "uniforme" → FIXED
   - Si hay sesgo fuerte → DYNAMIC
*/
zragf_block_type zragf_blocks_choose_type(const zragf_block_stats *st,
                                          const zragf_deflate_state *core);

#endif /* ZRAGF_DEFLATE_BLOCKS_H_INCLUDED */
