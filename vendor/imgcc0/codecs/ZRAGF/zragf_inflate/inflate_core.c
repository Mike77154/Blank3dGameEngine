#include "inflate_core.h"
#include <string.h>

/* ------------------------------------------------------------
   Wrapper para usar zragf_br_getbit como zragf_getbit_fn
   ------------------------------------------------------------ */

static void zragf_inf_free_table(zragf_idec_table *tab)
{
    if (!tab)
        return;
    if (tab->table)
        zragf_free_default(NULL, tab->table);
    memset(tab, 0, sizeof(*tab));
}

static int zragf_inf_getbit_wrapper(void *ctx, int *bit)
{
    zragf_inflate_state_z *zs = (zragf_inflate_state_z *)ctx;
    return zragf_br_getbit(zs, bit);
}

/* ------------------------------------------------------------
   Inicialización del core
   ------------------------------------------------------------ */
int zragf_inflate_core_init(zragf_inflate_core   *core,
                            zragf_inflate_state_z *zs,
                            int window_bits)
{
    if (!core || !zs)
        return 0;

    memset(core, 0, sizeof(*core));

    core->zs = zs;

    /* ventana ya debe estar creada en zs (p.ej. 32 KB) */
    core->window      = zs->window;
    core->window_size = zs->window_size;
    core->wpos        = zs->window_pos;

    (void)window_bits; /* por ahora asumimos que coincide con zs */

    core->huff.litlen.table = NULL;
    core->huff.litlen.table_bits = 0;
    core->huff.litlen.count = 0;
    core->huff.dist.table = NULL;
    core->huff.dist.table_bits = 0;
    core->huff.dist.count = 0;
    core->dist_table_dummy = 0;
    if (zs->window_filled > (zragf_size_t)core->window_size)
        zs->window_filled = (zragf_size_t)core->window_size;

    return 1;
}

/* ------------------------------------------------------------
   Leer 'n' bits como entero LSB-first desde zs
   ------------------------------------------------------------ */
static int zragf_inf_read_bits(zragf_inflate_state_z *zs,
                               int n,
                               int *out_value)
{
    int i;
    int bit = 0;
    int v = 0;

    for (i = 0; i < n; ++i) {
        if (!zragf_br_getbit(zs, &bit))
            return 0;
        v |= (bit & 1) << i;
    }
    *out_value = v;
    return 1;
}

/* ------------------------------------------------------------
   Alinear a byte (descartar bits restantes en bitbuf)
   ------------------------------------------------------------ */
static void zragf_inf_align_byte(zragf_inflate_state_z *zs)
{
    zs->bitbuf   = 0;
    zs->bitcount = 0;
}

/* ------------------------------------------------------------
   Leer un bloque STORED (BTYPE=00)
   ------------------------------------------------------------ */
static int zragf_inf_read_stored_block(zragf_inflate_core *core,
                                       zragf_u8           *out,
                                       zragf_size_t       *out_pos,
                                       zragf_size_t        out_cap,
                                       int                 final_block)
{
    zragf_inflate_state_z *zs = core->zs;
    zragf_u32 len, nlen;
    zragf_size_t i;

    (void)final_block;

    /* Debemos estar alineados a byte */
    zragf_inf_align_byte(zs);

    /* Necesitamos al menos 4 bytes para LEN + NLEN */
    if (zs->in_size < 4)
        return 0;

    len  = (zragf_u32)zs->in_buf[0] |
           ((zragf_u32)zs->in_buf[1] << 8);
    nlen = (zragf_u32)zs->in_buf[2] |
           ((zragf_u32)zs->in_buf[3] << 8);

    if ((len ^ nlen) != 0xFFFFu)
        return 0;

    /* Consumir encabezado LEN/NLEN */
    memmove(zs->in_buf, zs->in_buf + 4, zs->in_size - 4);
    zs->in_size -= 4;

    if (zs->in_size < len)
        return 0; /* falta input */

    if (*out_pos + len > out_cap)
        return 0; /* salida insuficiente */

    /* Copiar LEN bytes a salida y ventana */
    for (i = 0; i < len; ++i) {
        zragf_u8 c = zs->in_buf[i];

        out[*out_pos] = c;
        *out_pos += 1;

        zragf_inflate_window_put_byte(core->window,
                                      core->window_size,
                                      &core->wpos,
                                      &zs->window_filled,
                                      c);
    }

    /* Consumir bytes de entrada */
    memmove(zs->in_buf, zs->in_buf + len, zs->in_size - len);
    zs->in_size -= len;

    return 1;
}

/* ------------------------------------------------------------
   Leer cabecera de bloque dinámico (HLIT/HDIST/HCLEN,
   longitudes de códigos y construir tablas Huffman)
   ------------------------------------------------------------ */
static int zragf_inf_read_dynamic_header(zragf_inflate_core *core)
{
    zragf_inflate_state_z *zs = core->zs;
    int hlit, hdist, hclen;
    int i, j;
    int cl_len[19];
    int total;
    int lens[286 + 30];
    zragf_idec_table cl_tab;
    int litlen_len[ZRAGF_INF_MAX_LITLEN];
    int dist_len[ZRAGF_INF_MAX_DIST];

    /* HLIT (5 bits), HDIST (5 bits), HCLEN (4 bits) */
    if (!zragf_inf_read_bits(zs, 5, &hlit)) return 0;
    if (!zragf_inf_read_bits(zs, 5, &hdist)) return 0;
    if (!zragf_inf_read_bits(zs, 4, &hclen)) return 0;

    hlit  += 257;
    hdist += 1;
    hclen += 4;

    if (hlit < 257 || hlit > 286)
        return 0;
    if (hdist < 1 || hdist > 30)
        return 0;

    /* longitudes CL (0..18) */
    for (i = 0; i < 19; ++i)
        cl_len[i] = 0;

    for (i = 0; i < hclen; ++i) {
        int len3 = 0;
        if (!zragf_inf_read_bits(zs, 3, &len3))
            return 0;
        cl_len[zragf_inf_cl_order[i]] = len3;
    }

    /* Construir tabla de decodificación para CL */
    if (!zragf_inflate_build_table(cl_len, 19, 7, &cl_tab))
        return 0;

    total = hlit + hdist;
    for (i = 0; i < total; ++i)
        lens[i] = 0;

    i = 0;
    while (i < total) {
        int sym;
        if (!zragf_inflate_decode_symbol(&cl_tab,
                                          zragf_inf_getbit_wrapper,
                                          zs,
                                          &sym)) {
            zragf_inf_free_table(&cl_tab);
            return 0;
        }

        if (sym <= 15) {
            lens[i++] = sym;
        } else if (sym == 16) {
            int repeat = 0;
            if (!zragf_inf_read_bits(zs, 2, &repeat)) {
                zragf_inf_free_table(&cl_tab);
                return 0;
            }
            repeat += 3;
            if (i == 0) {
                zragf_inf_free_table(&cl_tab);
                return 0;
            }
            {
                int prev = lens[i - 1];
                while (repeat-- > 0) {
                    if (i >= total) {
                        zragf_inf_free_table(&cl_tab);
                        return 0;
                    }
                    lens[i++] = prev;
                }
            }
        } else if (sym == 17) {
            int repeat = 0;
            if (!zragf_inf_read_bits(zs, 3, &repeat)) {
                zragf_inf_free_table(&cl_tab);
                return 0;
            }
            repeat += 3;
            while (repeat-- > 0) {
                if (i >= total) {
                    zragf_inf_free_table(&cl_tab);
                    return 0;
                }
                lens[i++] = 0;
            }
        } else if (sym == 18) {
            int repeat = 0;
            if (!zragf_inf_read_bits(zs, 7, &repeat)) {
                zragf_inf_free_table(&cl_tab);
                return 0;
            }
            repeat += 11;
            while (repeat-- > 0) {
                if (i >= total) {
                    zragf_inf_free_table(&cl_tab);
                    return 0;
                }
                lens[i++] = 0;
            }
        } else {
            zragf_inf_free_table(&cl_tab);
            return 0;
        }
    }

    /* ya no necesitamos cl_tab */
    zragf_inf_free_table(&cl_tab);

    /* Separar litlen y dist */
    for (i = 0; i < hlit; ++i)
        litlen_len[i] = lens[i];
    for (; i < ZRAGF_INF_MAX_LITLEN; ++i)
        litlen_len[i] = 0;

    j = 0;
    for (; j < hdist; ++j)
        dist_len[j] = lens[hlit + j];
    for (; j < ZRAGF_INF_MAX_DIST; ++j)
        dist_len[j] = 0;

    if (litlen_len[256] == 0)
        return 0;

    /* Si la cabecera codifica cero distancias reales, construimos una tabla
       dummy para que el bitstream siga siendo decodificable, pero fallamos
       si luego aparece un match. */
    if (!zragf_inflate_prepare_dist_lengths(dist_len,
                                            ZRAGF_INF_MAX_DIST,
                                            &core->dist_table_dummy))
        return 0;

    /* construir tablas litlen y dist */
    if (!zragf_inflate_build_table(litlen_len,
                                   ZRAGF_INF_MAX_LITLEN,
                                   9,
                                   &core->huff.litlen))
        return 0;

    if (!zragf_inflate_build_table(dist_len,
                                   ZRAGF_INF_MAX_DIST,
                                   5,
                                   &core->huff.dist))
        return 0;

    return 1;
}

/* ------------------------------------------------------------
   Decodificar un bloque comprimido (FIXED o DYNAMIC)
   ------------------------------------------------------------ */
static int zragf_inf_decode_compressed_block(zragf_inflate_core *core,
                                             zragf_u8           *out,
                                             zragf_size_t       *out_pos,
                                             zragf_size_t        out_cap)
{
    zragf_inflate_state_z *zs = core->zs;
    int done = 0;

    while (!done) {
        int sym;

        if (!zragf_inflate_decode_symbol(&core->huff.litlen,
                                         zragf_inf_getbit_wrapper,
                                         zs,
                                         &sym))
            return 0;

        if (sym < 256) {
            /* literal */
            if (*out_pos >= out_cap)
                return 0;

            out[*out_pos] = (zragf_u8)sym;
            *out_pos += 1;

            zragf_inflate_window_put_byte(core->window,
                                          core->window_size,
                                          &core->wpos,
                                          &zs->window_filled,
                                          (zragf_u8)sym);
        }
        else if (sym == 256) {
            /* end-of-block */
            done = 1;
        }
        else if (sym >= 257 && sym <= 285) {
            int len_index = sym - 257;
            int length;
            int extra_len_bits;
            int dist_sym;
            int dist;
            int extra_dist_bits;
            int i;
            int bit = 0;
        
            if (len_index < 0 || len_index >= 29)
                return 0;

            length = zragf_inf_len_base[len_index];
            extra_len_bits = zragf_inf_len_extra[len_index];

            for (i = 0; i < extra_len_bits; ++i) {
                if (!zragf_br_getbit(zs, &bit))
                    return 0;
                length += (bit & 1) << i;
            }

            /* distancia */
            if (!zragf_inflate_decode_symbol(&core->huff.dist,
                                             zragf_inf_getbit_wrapper,
                                             zs,
                                             &dist_sym))
                return 0;

            if (dist_sym < 0 || dist_sym >= 30)
                return 0;

            dist = zragf_inf_dist_base[dist_sym];
            extra_dist_bits = zragf_inf_dist_extra[dist_sym];

            for (i = 0; i < extra_dist_bits; ++i) {
                if (!zragf_br_getbit(zs, &bit))
                    return 0;
                dist += (bit & 1) << i;
            }

            if (core->dist_table_dummy)
                return 0;
            if (!zragf_inflate_distance_ok(zs->window_filled, core->window_size, dist))
                return 0;

            if (*out_pos + (zragf_size_t)length > out_cap)
                return 0;

            /* copiar desde ventana */
            for (i = 0; i < length; ++i) {
                int ref_pos =
                    (core->wpos - dist + core->window_size) % core->window_size;
                zragf_u8 c = core->window[ref_pos];

                out[*out_pos] = c;
                *out_pos += 1;

                zragf_inflate_window_put_byte(core->window,
                                              core->window_size,
                                              &core->wpos,
                                              &zs->window_filled,
                                              c);
            }
        }
        else {
            /* símbolo reservado 286–287 */
            return 0;
        }
    }

    return 1;
}

/* ------------------------------------------------------------
   Descomprimir stream DEFLATE completo
   ------------------------------------------------------------ */
zragf_inflate_status
zragf_inflate_core_decompress(zragf_inflate_core *core,
                              zragf_u8           *out,
                              zragf_size_t       *out_size)
{
    zragf_inflate_state_z *zs;
    zragf_size_t out_cap;
    zragf_size_t out_pos = 0;
    int last_block = 0;

    if (!core || !core->zs || !out || !out_size)
        return ZRAGF_INF_STATUS_ERROR;

    zs = core->zs;
    out_cap = *out_size;

    /* bucle de bloques */
    while (!last_block) {
        int bfinal;
        int btype;
        int bit0, bit1;

        /* leer BFINAL */
        if (!zragf_br_getbit(zs, &bfinal))
            return ZRAGF_INF_STATUS_ERROR;

        last_block = (bfinal & 1);

        /* leer BTYPE (2 bits) */
        if (!zragf_br_getbit(zs, &bit0))
            return ZRAGF_INF_STATUS_ERROR;
        if (!zragf_br_getbit(zs, &bit1))
            return ZRAGF_INF_STATUS_ERROR;

        btype = bit0 | (bit1 << 1);

        if (btype == 0) {
            /* STORED */
            core->dist_table_dummy = 0;
            if (!zragf_inf_read_stored_block(core,
                                             out,
                                             &out_pos,
                                             out_cap,
                                             last_block))
                return ZRAGF_INF_STATUS_ERROR;
        }
        else if (btype == 1) {
            /* FIXED */
            if (!zragf_inflate_init_fixed(&core->huff))
                return ZRAGF_INF_STATUS_ERROR;
            core->dist_table_dummy = 0;

            if (!zragf_inf_decode_compressed_block(core,
                                                   out,
                                                   &out_pos,
                                                   out_cap)) {
                zragf_inflate_free(&core->huff);
                return ZRAGF_INF_STATUS_ERROR;
            }

            zragf_inflate_free(&core->huff);
        }
        else if (btype == 2) {
            /* DYNAMIC */
            if (!zragf_inf_read_dynamic_header(core))
                return ZRAGF_INF_STATUS_ERROR;

            if (!zragf_inf_decode_compressed_block(core,
                                                   out,
                                                   &out_pos,
                                                   out_cap)) {
                zragf_inflate_free(&core->huff);
                return ZRAGF_INF_STATUS_ERROR;
            }

            zragf_inflate_free(&core->huff);
        }
        else {
            /* BTYPE=11 es reservado → error */
            return ZRAGF_INF_STATUS_ERROR;
        }
    }

    zs->window_pos = core->wpos;
    *out_size = out_pos;
    return ZRAGF_INF_STATUS_STREAM_END;
}
