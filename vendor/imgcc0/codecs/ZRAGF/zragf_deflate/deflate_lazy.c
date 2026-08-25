#include "deflate_lazy.h"
#include "deflate_hash.h"

/* ------------------------------------------------------------
   lazy matching avanzado
   ------------------------------------------------------------ */
void zragf_lazy_select(zragf_deflate_state *st,
                       int raw_len,
                       int raw_dist,
                       int *out_len,
                       int *out_dist)
{
    int cur_pos;
    int wmask = st->wmask;

    /* Sin match válido → literal directo */
    if (raw_len < 3) {
        *out_len = 0;
        *out_dist = 0;
        return;
    }

    cur_pos = (st->wpos - 1) & wmask;

    /* --------------------------------------------------------
       Lazy Matching Estándar (como zlib)
       -------------------------------------------------------- */
    {
        int next_pos = (cur_pos + 1) & wmask;
        int next_len = 0;

        /* Simular "buscar match en siguiente byte" */
        {
            int b0 = st->window[next_pos];
            int b1 = st->window[(next_pos + 1) & wmask];
            int b2 = st->window[(next_pos + 2) & wmask];
            int h = (int)((((unsigned)b0 * 0x1e35a7bdU) ^
                           ((unsigned)b1 * 0x3c6ef372U) ^
                           ((unsigned)b2 * 0x9e3779b9U))
                           >> (32 - ZRAGF_HASH_BITS));
            int chain = st->head[h];
            int limit = 48;

            while (chain >= 0 && limit-- > 0) {
                int dist =
                    (next_pos - chain) & wmask;
                if (dist > 0 && dist < st->wsize) {
                    int len = zragf_hash_run_compare(
                        &st->window[next_pos],
                        &st->window[chain],
                        258
                    );

                    if (len > next_len) {
                        next_len = len;
                    }
                }
                chain = st->prev[chain];
            }
        }

        /* ----------------------------------------------------
           Lazy Matching Avanzado (mejorado sobre zlib)
           ---------------------------------------------------- */

        /* Caso 1: siguiente match es MEJOR → retrasar literal */
        if (next_len > raw_len) {
            *out_len = 0;
            *out_dist = 0;
            return;
        }

        /* Caso 2: siguiente match es ligeramente mejor (+1) y nivel > 5 */
        if (st->p.level > 5 && next_len == raw_len + 1) {
            *out_len = 0;
            *out_dist = 0;
            return;
        }

        /* Caso 3: match fuertemente mejorado (+4) en lazy doble */
        if (st->p.level >= 8 && next_len >= raw_len + 4) {
            *out_len = 0;
            *out_dist = 0;
            return;
        }

        /* Caso 4: heurística de entropía baja (texto repetitivo)
           → forzar lazy aún con igual longitud */
        {
            int a = st->window[cur_pos];
            int b = st->window[(cur_pos - 1) & wmask];
            int c = st->window[(cur_pos - 2) & wmask];

            if (st->p.level >= 6 &&
                (a == b || a == c) &&
                next_len >= raw_len) {

                *out_len = 0;
                *out_dist = 0;
                return;
            }
        }

        /* Si no se retrasó, usar match actual */
        *out_len = raw_len;
        *out_dist = raw_dist;
        return;
    }
}
