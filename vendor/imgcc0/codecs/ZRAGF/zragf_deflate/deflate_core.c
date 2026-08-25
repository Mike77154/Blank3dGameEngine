#include "deflate_core.h"
#include "deflate_hash.h"
#include "deflate_lazy.h"

static int zragf_deflate_is_asciiish(zragf_u8 b)
{
    return ((b >= 32u && b <= 126u) || b == 9u || b == 10u || b == 13u) ? 1 : 0;
}

void zragf_deflate_core_profile_input(const zragf_u8 *dict,
                                      zragf_size_t    dict_size,
                                      const zragf_u8 *src,
                                      zragf_size_t    src_size,
                                      zragf_deflate_match_profile *out)
{
    unsigned seen[256];
    unsigned unique = 0u;
    unsigned zero_bytes = 0u;
    unsigned ascii_bytes = 0u;
    unsigned digit_bytes = 0u;
    unsigned newline_bytes = 0u;
    unsigned structural_bytes = 0u;
    unsigned longest_run = 0u;
    unsigned long_run_bytes = 0u;
    unsigned period2_hits = 0u;
    unsigned transitions = 0u;
    unsigned run = 0u;
    zragf_size_t i;
    (void)dict;
    (void)dict_size;

    if (!out)
        return;
    memset(out, 0, sizeof(*out));
    if (!src || src_size == 0u)
        return;

    memset(seen, 0, sizeof(seen));
    for (i = 0u; i < src_size; ++i) {
        zragf_u8 b = src[i];
        if (seen[b] == 0u) {
            seen[b] = 1u;
            unique++;
        }
        if (b == 0u)
            zero_bytes++;
        if (zragf_deflate_is_asciiish(b))
            ascii_bytes++;
        if (b >= (zragf_u8)'0' && b <= (zragf_u8)'9')
            digit_bytes++;
        if (b == 10u)
            newline_bytes++;
        if (b == (zragf_u8)'"' || b == (zragf_u8)'{' || b == (zragf_u8)'}' ||
            b == (zragf_u8)'[' || b == (zragf_u8)']' || b == (zragf_u8)',' ||
            b == (zragf_u8)':' || b == (zragf_u8)'=' || b == (zragf_u8)'/' ||
            b == (zragf_u8)'-' || b == (zragf_u8)'_' )
            structural_bytes++;
        if (i > 0u && src[i] != src[i - 1u])
            transitions++;
        if (i == 0u || src[i] == src[i - 1u])
            run++;
        else {
            if (run > longest_run)
                longest_run = run;
            if (run >= 8u)
                long_run_bytes += run;
            run = 1u;
        }
        if (i >= 2u && src[i] == src[i - 2u] && src[i] != src[i - 1u])
            period2_hits++;
    }
    if (run > longest_run)
        longest_run = run;
    if (run >= 8u)
        long_run_bytes += run;

    out->unique_bytes = unique;
    out->zero_bytes = zero_bytes;
    out->ascii_bytes = ascii_bytes;
    out->digit_bytes = digit_bytes;
    out->newline_bytes = newline_bytes;
    out->structural_bytes = structural_bytes;
    out->longest_run = longest_run;
    out->long_run_bytes = long_run_bytes;
    out->period2_hits = period2_hits;
    out->transitions = transitions;

    out->incompressible_like =
        (src_size >= 4096u && unique >= 192u && longest_run < 4u &&
         transitions * 100u > (unsigned)(src_size * 85u));
    out->rle_like =
        (longest_run >= 16u || (src_size > 0u && long_run_bytes * 100u >= (unsigned)(src_size * 20u)) ||
         (src_size > 0u && zero_bytes * 100u >= (unsigned)(src_size * 40u)));
    out->text_like = (src_size > 0u && ascii_bytes * 100u >= (unsigned)(src_size * 85u) && unique < 128u) ? 1 : 0;
    out->structured_text_like =
        (src_size >= 512u && ascii_bytes * 100u >= (unsigned)(src_size * 80u) &&
         ((structural_bytes * 100u >= (unsigned)(src_size * 12u) &&
           digit_bytes * 100u >= (unsigned)(src_size * 2u)) ||
          (newline_bytes * 100u >= (unsigned)(src_size * 1u) &&
           structural_bytes * 100u >= (unsigned)(src_size * 8u))));
    out->alternating_like = (src_size >= 256u && longest_run < 6u &&
                             period2_hits * 100u >= (unsigned)(src_size * 60u)) ? 1 : 0;
}

void zragf_deflate_core_select_policy(int level,
                                      int strategy,
                                      int tune_set,
                                      int good_length,
                                      int max_lazy,
                                      int nice_length,
                                      int max_chain,
                                      const zragf_deflate_match_profile *profile,
                                      zragf_deflate_match_policy *out)
{
    zragf_deflate_match_policy mp;

    if (!out)
        return;

    if (level <= 1) {
        mp.good_len = 4;
        mp.max_chain = 8;
        mp.nice_len = 16;
        mp.lazy_probe = 0;
        mp.allow_lazy = 0;
    } else if (level <= 3) {
        mp.good_len = 8;
        mp.max_chain = 32;
        mp.nice_len = 32;
        mp.lazy_probe = 0;
        mp.allow_lazy = 0;
    } else if (level <= 5) {
        mp.good_len = 16;
        mp.max_chain = 96;
        mp.nice_len = 96;
        mp.lazy_probe = 8;
        mp.allow_lazy = 1;
    } else if (level <= 7) {
        mp.good_len = 32;
        mp.max_chain = 256;
        mp.nice_len = 128;
        mp.lazy_probe = 16;
        mp.allow_lazy = 1;
    } else {
        mp.good_len = 64;
        mp.max_chain = 768;
        mp.nice_len = 258;
        mp.lazy_probe = 32;
        mp.allow_lazy = 1;
    }

    if (strategy == ZRAGF_Z_FILTERED) {
        if (mp.max_chain < 128)
            mp.max_chain = 128;
        if (mp.nice_len < 64)
            mp.nice_len = 64;
    }

    if (tune_set) {
        mp.good_len = good_length;
        mp.max_chain = max_chain;
        mp.nice_len = nice_length;
        mp.lazy_probe = max_lazy;
        mp.allow_lazy = (mp.lazy_probe > 0) ? 1 : 0;
    }

    if (profile) {
        if (profile->rle_like && strategy == ZRAGF_Z_DEFAULT_STRATEGY) {
            if (mp.good_len < 24)
                mp.good_len = 24;
            if (mp.nice_len < 128)
                mp.nice_len = 128;
            if (mp.max_chain < 128)
                mp.max_chain = 128;
            if (mp.lazy_probe < 8)
                mp.lazy_probe = 8;
            mp.allow_lazy = 1;
        }
        if (profile->text_like && strategy == ZRAGF_Z_DEFAULT_STRATEGY) {
            if (mp.good_len < 16)
                mp.good_len = 16;
            if (mp.max_chain < 128 && level >= 4)
                mp.max_chain = 128;
        }
        if (profile->structured_text_like &&
            (strategy == ZRAGF_Z_DEFAULT_STRATEGY || strategy == ZRAGF_Z_FILTERED)) {
            if (mp.good_len < 20)
                mp.good_len = 20;
            if (level >= 5) {
                if (mp.max_chain < 224)
                    mp.max_chain = 224;
                if (mp.max_chain > 256)
                    mp.max_chain = 256;
            }
            if (mp.nice_len < 144)
                mp.nice_len = 144;
            if (mp.nice_len > 160 && level <= 7)
                mp.nice_len = 160;
            if (mp.lazy_probe < 12)
                mp.lazy_probe = 12;
            if (mp.lazy_probe > 14)
                mp.lazy_probe = 14;
            mp.allow_lazy = 1;
        }
        if (profile->alternating_like) {
            if (mp.max_chain > 96)
                mp.max_chain = 96;
            if (mp.nice_len > 64)
                mp.nice_len = 64;
            if (mp.lazy_probe > 6)
                mp.lazy_probe = 6;
        }
        if (profile->incompressible_like) {
            if (mp.max_chain > ((level >= 7) ? 64 : 32))
                mp.max_chain = (level >= 7) ? 64 : 32;
            if (mp.nice_len > 32)
                mp.nice_len = 32;
            if (mp.lazy_probe > 4)
                mp.lazy_probe = 4;
            if (mp.lazy_probe == 0)
                mp.allow_lazy = 0;
        }
    }

    if (mp.good_len < 0)
        mp.good_len = 0;
    if (mp.good_len > 258)
        mp.good_len = 258;
    if (mp.max_chain < 1)
        mp.max_chain = 1;
    if (mp.max_chain > 8192)
        mp.max_chain = 8192;
    if (mp.nice_len < 3)
        mp.nice_len = 3;
    if (mp.nice_len > 258)
        mp.nice_len = 258;
    if (mp.good_len > mp.nice_len)
        mp.good_len = mp.nice_len;
    if (mp.lazy_probe < 0)
        mp.lazy_probe = 0;
    if (mp.lazy_probe > 258)
        mp.lazy_probe = 258;
    mp.allow_lazy = (mp.lazy_probe > 0) ? 1 : 0;

    *out = mp;
}


/* ------------------------------------------------------------
   Helper: expand pending buffer
   ------------------------------------------------------------ */
static int zragf_deflate_expand_pending(zragf_deflate_state *st,
                                        zragf_size_t extra)
{
    zragf_size_t need = st->pending_size + extra;
    zragf_size_t newcap;
    zragf_u8 *nbuf;

    if (need <= st->pending_cap)
        return 1;

    newcap = (st->pending_cap == 0) ? 4096 : st->pending_cap * 2;
    while (newcap < need)
        newcap *= 2;

    nbuf = (zragf_u8 *)zragf_alloc_default(NULL, newcap, 1);
    if (!nbuf)
        return 0;

    if (st->pending_size > 0)
        memcpy(nbuf, st->pending_buf, st->pending_size);

    if (st->pending_buf)
        zragf_free_default(NULL, st->pending_buf);

    st->pending_buf = nbuf;
    st->pending_cap = newcap;
    return 1;
}

/* ------------------------------------------------------------
   emit_byte wrapper for internal pending buffer
   ------------------------------------------------------------ */
static void zragf_deflate_pending_emit(void *ctx, zragf_u8 b)
{
    zragf_deflate_state *st = (zragf_deflate_state *)ctx;

    if (!zragf_deflate_expand_pending(st, 1))
        return; /* out of memory → silently fail for now */

    st->pending_buf[st->pending_size++] = b;
}

/* ------------------------------------------------------------
   deflate_core_init
   ------------------------------------------------------------ */
int zragf_deflate_core_init(zragf_deflate_state *st,
                            const zragf_deflate_params *params,
                            void (*emit_byte)(void *, zragf_u8),
                            void *emit_ctx)
{
    int i;

    if (!st || !params)
        return 0;

    memset(st, 0, sizeof(*st));

    /* parámetros */
    st->p = *params;
    st->p.window_size = 1 << params->window_bits;

    st->wsize = st->p.window_size;
    st->wmask = st->wsize - 1;
    st->wpos  = 0;
    st->loaded_bytes = 0;

    /* ventana */
    st->window = (zragf_u8 *)zragf_alloc_default(NULL, (zragf_size_t)st->wsize, 1u);
    if (!st->window)
        return 0;

    /* hash tables */
    st->head = (int *)zragf_alloc_default(NULL, (zragf_size_t)ZRAGF_HASH_SIZE, sizeof(int));
    st->prev = (int *)zragf_alloc_default(NULL, (zragf_size_t)st->wsize, sizeof(int));
    if (!st->head || !st->prev)
        return 0;

    for (i = 0; i < ZRAGF_HASH_SIZE; ++i)
        st->head[i] = -1;
    for (i = 0; i < st->wsize; ++i)
        st->prev[i] = -1;

    /* pending */
    st->pending_buf  = NULL;
    st->pending_size = 0;
    st->pending_cap  = 0;

    /* match temporales */
    st->best_len  = 0;
    st->best_dist = 0;

    if (emit_byte)
        st->emit_byte = emit_byte;
    else
        st->emit_byte = zragf_deflate_pending_emit;

    if (emit_ctx)
        st->emit_ctx = emit_ctx;
    else
        st->emit_ctx = st;

    return 1;
}

/* ------------------------------------------------------------
   deflate_core_load
   Inserta datos a la ventana y actualiza hash.
   ------------------------------------------------------------ */
void zragf_deflate_core_load(zragf_deflate_state *st,
                             const zragf_u8 *src,
                             zragf_size_t n)
{
    zragf_size_t i;

    if (!st || !src)
        return;

    for (i = 0u; i < n; ++i) {
        int insert_pos;

        /* Insertar byte en ventana circular */
        st->window[st->wpos] = src[i];

        if (st->loaded_bytes < st->wsize)
            st->loaded_bytes++;

        /* Una secuencia de 3 bytes se vuelve válida en (wpos - 2). */
        if (st->loaded_bytes >= 3) {
            insert_pos = (st->wpos - 2) & st->wmask;
            (void)zragf_hash_insert(st, insert_pos);
        }

        st->wpos = (st->wpos + 1) & st->wmask;
    }
}

/* ------------------------------------------------------------
   deflate_core_find_match
   (llama al buscador hash → lazy matching)
   ------------------------------------------------------------ */
void zragf_deflate_core_find_match(zragf_deflate_state *st,
                                   int *out_len,
                                   int *out_dist)
{
    int raw_len = 0;
    int raw_dist = 0;

    /* Búsqueda rápida por hash */
    zragf_hash_find(st, &raw_len, &raw_dist);

    /* Lazy matching avanzado */
    zragf_lazy_select(st, raw_len, raw_dist, out_len, out_dist);
}

/* ------------------------------------------------------------
   deflate_core_process
   Procesa input → genera tokens (literal o match).
   Estos tokens NO se escriben aquí. Solo se pasa info
   a deflate_blocks u otra capa superior.
   ------------------------------------------------------------ */
void zragf_deflate_core_process(zragf_deflate_state *st)
{
    /* Aquí NO hacemos la salida final.
       Solo ilustramos el pipeline:

       1. Buscar match
       2. Decidir literal o match
       3. Empacar en pending o llamar a emit_byte
       4. Mover ventana
    */

    int len = 0;
    int dist = 0;

    zragf_deflate_core_find_match(st, &len, &dist);

    if (len >= 3) {
        /* Output match token (length, distance)
           Por ahora emitimos literal de prueba (hasta que blocks se encargue) */

        st->emit_byte(st->emit_ctx, (zragf_u8)257); /* código fake */
        (void)dist;
    }
    else {
        /* Literal */
        st->emit_byte(st->emit_ctx, st->window[(st->wpos - 1) & st->wmask]);
    }
}
