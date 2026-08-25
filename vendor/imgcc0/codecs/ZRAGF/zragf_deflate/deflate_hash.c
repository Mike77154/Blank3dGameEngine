#include "deflate_hash.h"

/* ------------------------------------------------------------
   Hash rápido de 3 bytes (igual a zlib, pero en C89 puro)
   ------------------------------------------------------------ */
static ZRAGF_INLINE int zragf_hash3(int b1, int b2, int b3)
{
    /* polynomial rolling hash simple */
    unsigned h = (unsigned)b1 * 0x1e35a7bdu;
    h ^= (unsigned)b2 * 0x3c6ef372u;
    h ^= (unsigned)b3 * 0x9e3779b9u;
    return (int)((h >> (32 - ZRAGF_HASH_BITS)) & ZRAGF_HASH_MASK);
}

/* ------------------------------------------------------------
   Comparar dos regiones para extender el match
   max = 258 (límite DEFLATE)
   ------------------------------------------------------------ */
int zragf_hash_run_compare(const zragf_u8 *a,
                           const zragf_u8 *b,
                           int max)
{
    int i;

    for (i = 0; i < max; ++i) {
        if (a[i] != b[i])
            return i;
    }
    return max;
}

/* ------------------------------------------------------------
   Inserta pos en el hash y devuelve el antiguo head
   ------------------------------------------------------------ */
int zragf_hash_insert(zragf_deflate_state *st, int pos)
{
    int winmask = st->wmask;

    /* si no hay 3 bytes válidos, no se puede hashear */
    int p0 = st->window[pos];
    int p1 = st->window[(pos + 1) & winmask];
    int p2 = st->window[(pos + 2) & winmask];

    int h = zragf_hash3(p0, p1, p2);

    st->prev[pos] = st->head[h];
    st->head[h] = pos;

    return st->prev[pos];
}

/* ------------------------------------------------------------
   Búsqueda de match con heurísticas avanzadas
   ------------------------------------------------------------ */
void zragf_hash_find(zragf_deflate_state *st,
                     int *out_len,
                     int *out_dist)
{
    int limit_chain;
    int chain_pos;
    int cur_pos;
    int best_len = 0;
    int best_dist = 0;

    int wmask = st->wmask;
    int wsize = st->wsize;

    /* posición actual = pos anterior en la ventana */
    cur_pos = (st->wpos - 1) & wmask;

    /* Necesitamos al menos 3 bytes válidos; no inferir validez por contenido. */
    if (st->loaded_bytes < 3) {
        *out_len = 0;
        *out_dist = 0;
        return;
    }

    /* HASH del literal actual */
    {
        int b0 = st->window[cur_pos];
        int b1 = st->window[(cur_pos + 1) & wmask];
        int b2 = st->window[(cur_pos + 2) & wmask];
        int h = zragf_hash3(b0, b1, b2);
        chain_pos = st->head[h];
    }

    /* Profundidad base (dependiente del nivel) */
    if (st->p.level <= 3)
        limit_chain = 64;
    else if (st->p.level <= 6)
        limit_chain = 128;
    else
        limit_chain = ZRAGF_MAX_CHAIN_DEFAULT;

    /* Heurística: si el contenido es muy repetitivo,
       aumentamos profundidad */
    {
        int r0 = st->window[cur_pos];
        int r1 = st->window[(cur_pos - 1) & wmask];
        int r2 = st->window[(cur_pos - 2) & wmask];
        if (r0 == r1 || r0 == r2)
            limit_chain *= 2;
    }

    /* Búsqueda en hash-chain */
    while (chain_pos >= 0 && limit_chain-- > 0) {

        if (chain_pos == cur_pos) {
            chain_pos = st->prev[chain_pos];
            continue;
        }

        /* Distancia DEFLATE */
        {
            int dist = (cur_pos - chain_pos) & wmask;
            if (dist <= 0 || dist >= wsize) {
                chain_pos = st->prev[chain_pos];
                continue;
            }

            /* Comparar runs */
            {
                int len = zragf_hash_run_compare(
                    &st->window[cur_pos],
                    &st->window[chain_pos],
                    258
                );

                if (len > best_len) {
                    best_len = len;
                    best_dist = dist;

                    /* Corte agresivo si logramos un match excelente */
                    if (len >= 32)
                        break;
                }
            }
        }

        chain_pos = st->prev[chain_pos];
    }

    *out_len = best_len;
    *out_dist = best_dist;
}
