#ifndef ZRAGF_DEFLATE_CORE_H_INCLUDED
#define ZRAGF_DEFLATE_CORE_H_INCLUDED

#include "../zragflib_internal.h"

/* ------------------------------------------------------------
   Parámetros del compresor DEFLATE
   ------------------------------------------------------------ */

typedef struct
{
    int level;          /* nivel de compresión (1–9) */
    int strategy;       /* estrategia (0 = default) */
    int window_bits;    /* 15 para ventana 32 KB */
    int use_big_blocks; /* 0 = off, 1 = on */

    /* tamaños derivados */
    int window_size;    /* 32768 */
} zragf_deflate_params;

/* ------------------------------------------------------------
   Estado principal del compresor DEFLATE
   ------------------------------------------------------------ */

typedef struct
{
    /* parámetros */
    zragf_deflate_params p;

    /* ventana circular real */
    zragf_u8  *window;
    int        wpos;            /* posición en ventana */
    int        wsize;           /* 32768 */
    int        wmask;           /* wsize - 1 */
    int        loaded_bytes;    /* bytes válidos actualmente en ventana */

    /* input acumulado */
    const zragf_u8 *next_in;
    zragf_size_t    avail_in;

    /* salida pendiente (hacia deflate_stream) */
    zragf_u8  *pending_buf;
    zragf_size_t pending_size;
    zragf_size_t pending_cap;

    /* hash table + prev chains */
    int *head;
    int *prev;

    /* match temporales */
    int best_len;
    int best_dist;

    /* Función para escribir salida (inyectada por deflate_stream) */
    void (*emit_byte)(void *ctx, zragf_u8 b);
    void *emit_ctx;

} zragf_deflate_state;

typedef struct
{
    unsigned unique_bytes;
    unsigned zero_bytes;
    unsigned ascii_bytes;
    unsigned digit_bytes;
    unsigned newline_bytes;
    unsigned structural_bytes;
    unsigned longest_run;
    unsigned long_run_bytes;
    unsigned period2_hits;
    unsigned transitions;
    int incompressible_like;
    int rle_like;
    int text_like;
    int structured_text_like;
    int alternating_like;
} zragf_deflate_match_profile;

typedef struct
{
    int good_len;
    int max_chain;
    int nice_len;
    int lazy_probe;
    int allow_lazy;
} zragf_deflate_match_policy;

/* ------------------------------------------------------------
   Funciones expuestas del core
   ------------------------------------------------------------ */

void zragf_deflate_core_profile_input(const zragf_u8 *dict,
                                      zragf_size_t    dict_size,
                                      const zragf_u8 *src,
                                      zragf_size_t    src_size,
                                      zragf_deflate_match_profile *out);

void zragf_deflate_core_select_policy(int level,
                                      int strategy,
                                      int tune_set,
                                      int good_length,
                                      int max_lazy,
                                      int nice_length,
                                      int max_chain,
                                      const zragf_deflate_match_profile *profile,
                                      zragf_deflate_match_policy *out);

/* Inicializa estado DEFLATE */
int zragf_deflate_core_init(zragf_deflate_state *st,
                            const zragf_deflate_params *params,
                            void (*emit_byte)(void *, zragf_u8),
                            void *emit_ctx);

/* Inserta bytes nuevos en la ventana + actualiza hash */
void zragf_deflate_core_load(zragf_deflate_state *st,
                             const zragf_u8 *src,
                             zragf_size_t n);

/* Encuentra match (llama a hash/lazy) */
void zragf_deflate_core_find_match(zragf_deflate_state *st,
                                   int *out_len,
                                   int *out_dist);

/* Procesa la entrada y produce tokens length/dist o literales
   (deflate_blocks tomará estos tokens y los codificará) */
void zragf_deflate_core_process(zragf_deflate_state *st);

#endif /* ZRAGF_DEFLATE_CORE_H_INCLUDED */
