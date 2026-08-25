/* deflate_rle.c */

#include "deflate_rle.h"

/*
   Política simple, compatible con RFC1951:

   - longitudes != 0:
       + si se repite 3–6 veces: usar 16
       + si 1–2 veces: emitir longitud cruda (0–15)

   - longitudes == 0:
       + si se repite 3–10 veces: usar 17
       + si > 10: usar 18 (11–138)

   Esto se puede afinar luego; ahora es funcional.
*/

zragf_size_t
zragf_huff_rle_lengths(const int *ll_len, int hlit,
                       const int *d_len,  int hdist,
                       zragf_cl_token *out,
                       zragf_size_t    max_tokens)
{
    int total;
    int i = 0;
    zragf_size_t out_count = 0;

    if (!ll_len || !d_len || !out || max_tokens == 0)
        return 0;

    /* número efectivo de códigos litlen y dist:
       HLIT = #litlen - 257
       HDIST = #dist - 1
       aquí asumimos que callers ya pasó hlit/hdist consistentes. */
    total = (hlit + 257) + (hdist + 1);

    while (i < total && out_count < max_tokens) {
        int cur_len;
        int run = 1;
        int j;

        /* obtener longitud actual desde ll_len/d_len concatenados */
        if (i < hlit + 257)
            cur_len = ll_len[i];
        else
            cur_len = d_len[i - (hlit + 257)];

        /* contar run */
        for (j = i + 1; j < total; ++j) {
            int next_len;
            if (j < hlit + 257)
                next_len = ll_len[j];
            else
                next_len = d_len[j - (hlit + 257)];

            if (next_len != cur_len)
                break;
            run++;
        }

        if (cur_len == 0) {
            /* runs de ceros → 17/18/0 */
            while (run > 0 && out_count < max_tokens) {
                if (run >= 11) {
                    int count = (run > 138) ? 138 : run; /* 11–138 */
                    zragf_cl_token *t = &out[out_count++];
                    t->sym        = 18;
                    t->extra_bits = 7;
                    t->extra_val  = (unsigned)(count - 11);
                    run          -= count;
                } else if (run >= 3) {
                    int count = (run > 10) ? 10 : run; /* 3–10 */
                    zragf_cl_token *t = &out[out_count++];
                    t->sym        = 17;
                    t->extra_bits = 3;
                    t->extra_val  = (unsigned)(count - 3);
                    run          -= count;
                } else {
                    /* 1–2 ceros crudos (0) */
                    zragf_cl_token *t = &out[out_count++];
                    t->sym        = 0;
                    t->extra_bits = 0;
                    t->extra_val  = 0;
                    run--;
                }
            }
        } else {
            /* longitudes no cero → longitud cruda + opcional 16 */
            int first = 1;
            while (run > 0 && out_count < max_tokens) {
                if (first) {
                    /* primer valor: longitud explícita 0–15 */
                    zragf_cl_token *t = &out[out_count++];
                    t->sym        = cur_len;
                    t->extra_bits = 0;
                    t->extra_val  = 0;
                    first = 0;
                    run--;
                } else if (run >= 3) {
                    int count = (run > 6) ? 6 : run; /* 3–6 */
                    zragf_cl_token *t = &out[out_count++];
                    t->sym        = 16;
                    t->extra_bits = 2;
                    t->extra_val  = (unsigned)(count - 3);
                    run          -= count;
                } else {
                    /* 1–2 repeticiones extra como longitud cruda */
                    zragf_cl_token *t = &out[out_count++];
                    t->sym        = cur_len;
                    t->extra_bits = 0;
                    t->extra_val  = 0;
                    run--;
                }
            }
        }

        i = j;
    }

    return out_count;
}
