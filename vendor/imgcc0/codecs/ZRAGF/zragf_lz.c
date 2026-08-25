/* zragf_lz.c - implementación de match finder LZ con hash-chain */

#include "zragf_lz.h"

/* Para los enlaces de hash-chain usamos un tipo con signo para poder
 * representar -1 como "no hay siguiente".
 */
typedef long zragf_lz_link_t;

/* Estado interno (global) para la búsqueda hash-chain.
 * No es thread-safe, pero respeta la API existente.
 */
static zragf_lz_link_t zragf_lz_head[ZRAGF_LZ_HASH_SIZE];
static zragf_lz_link_t zragf_lz_next[ZRAGF_MAX_WINDOW];

static const zragf_u8 *zragf_lz_base = 0;
static zragf_u32       zragf_lz_size = 0;
static zragf_u32       zragf_lz_last_pos = 0;
static int             zragf_lz_inited = 0;

/* Hash simple sobre 3 bytes (mínimo ZRAGF_MIN_MATCH) */
static zragf_u32
zragf_lz_hash3(const zragf_u8 *p)
{
    zragf_u32 h = (zragf_u32)p[0];
    h = h * 257u + (zragf_u32)p[1];
    h = h * 257u + (zragf_u32)p[2];
    return h & ZRAGF_LZ_HASH_MASK;
}

/* Resetea el estado interno de hash-chain para un nuevo buffer */
static void
zragf_lz_reset_state(const zragf_u8 *base, zragf_u32 size)
{
    zragf_u32 i;
    for (i = 0; i < ZRAGF_LZ_HASH_SIZE; ++i)
        zragf_lz_head[i] = -1;
    for (i = 0; i < ZRAGF_MAX_WINDOW; ++i)
        zragf_lz_next[i] = -1;

    zragf_lz_base     = base;
    zragf_lz_size     = size;
    zragf_lz_last_pos = 0;
    zragf_lz_inited   = 1;
}

/* Asegura que todas las posiciones [zragf_lz_last_pos, pos) se hayan
 * insertado en las estructuras de hash-chain.
 */
static void
zragf_lz_ensure_built_up_to(zragf_u32 pos)
{
    zragf_u32 p;

    if (!zragf_lz_inited)
        return;

    if (zragf_lz_last_pos >= pos)
        return;

    /* Insertamos todas las posiciones desde zragf_lz_last_pos hasta pos-1.
     * Sólo tiene sentido hashear posiciones donde existan al menos
     * ZRAGF_MIN_MATCH bytes.
     */
    for (p = zragf_lz_last_pos;
         p + (zragf_u32)ZRAGF_MIN_MATCH <= zragf_lz_size && p < pos;
         ++p)
    {
        zragf_u32 h   = zragf_lz_hash3(zragf_lz_base + p);
        zragf_u32 idx = p & (ZRAGF_MAX_WINDOW - 1u);

        zragf_lz_next[idx] = zragf_lz_head[h];
        zragf_lz_head[h]   = (zragf_lz_link_t)p;
    }

    zragf_lz_last_pos = pos;
}

/*
 * Implementación pública: búsqueda de match LZ mediante hash-chain.
 *
 * Mantiene la firma original de zragf_find_match(), pero internamente
 * usa una tabla hash y listas encadenadas para acelerar la búsqueda.
 */
zragf_u32
zragf_find_match(const zragf_u8 *base,
                 zragf_u32 pos,
                 zragf_u32 size,
                 zragf_u32 *out_offset,
                 zragf_u32 max_window,
                 zragf_u32 max_len)
{
    zragf_u32 best_len = 0;
    zragf_u32 best_off = 0;
    zragf_u32 window;
    zragf_u32 start_limit;
    zragf_u32 max_chain;
    zragf_u32 h;
    zragf_lz_link_t cur_link;

    if (!base || !out_offset)
        return 0;

    if (pos >= size)
        return 0;

    /* Si no hay espacio para un match mínimo, salimos. */
    if (pos + (zragf_u32)ZRAGF_MIN_MATCH > size)
        return 0;

    /* Re-inicializar el estado si cambiaron base/size o empezamos en 0. */
    if (!zragf_lz_inited ||
        base != zragf_lz_base ||
        size != zragf_lz_size ||
        pos == 0u)
    {
        zragf_lz_reset_state(base, size);
    }

    /* Aseguramos que todas las posiciones anteriores estén en el hash. */
    zragf_lz_ensure_built_up_to(pos);

    /* Limitamos ventana efectiva a ZRAGF_MAX_WINDOW. */
    window = max_window;
    if (window > (zragf_u32)ZRAGF_MAX_WINDOW)
        window = (zragf_u32)ZRAGF_MAX_WINDOW;

    /* Inicio mínimo de la ventana (no retroceder más de window). */
    start_limit = (pos > window) ? (pos - window) : 0;

    /* Profundidad máxima de la cadena: tuning simple. */
    max_chain = 64u; /* se podría tunear por nivel más adelante */

    h        = zragf_lz_hash3(base + pos);
    cur_link = zragf_lz_head[h];

    while (cur_link >= 0 && max_chain-- > 0u)
    {
        zragf_u32 cur = (zragf_u32)cur_link;
        zragf_u32 off;
        zragf_u32 len = 0;
        zragf_u32 p   = pos;
        zragf_u32 q   = cur;

        /* Fuera de ventana, dejamos de seguir la cadena. */
        if (cur < start_limit)
            break;

        off = pos - cur;
        if (off == 0u)
            goto next_in_chain; /* debería ser imposible, pero por si acaso */

        /* Comparar bytes hasta max_len, fin de buffer o llegar a pos. */
        while (p < size && q < pos && len < max_len && base[p] == base[q])
        {
            ++p;
            ++q;
            ++len;
        }

        if (len >= (zragf_u32)ZRAGF_MIN_MATCH && len > best_len)
        {
            best_len = len;
            best_off = off;
            if (len == max_len)
                break;
        }

    next_in_chain:
        /* Siguiente en la cadena para este hash. */
        cur_link = zragf_lz_next[cur & (ZRAGF_MAX_WINDOW - 1u)];
    }

    if (best_len >= (zragf_u32)ZRAGF_MIN_MATCH)
    {
        *out_offset = best_off;
        return best_len;
    }

    return 0;
}