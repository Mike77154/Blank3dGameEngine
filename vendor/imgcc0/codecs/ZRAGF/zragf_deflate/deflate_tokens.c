/* deflate_tokens.c */

#include "deflate_tokens.h"

int zragf_tokens_reserve_extra(zragf_token_buffer *tb, zragf_size_t extra)
{
    zragf_size_t need, newcap;
    zragf_token *nbuf;

    if (!tb)
        return 0;

    need = tb->size + extra;
    if (need <= tb->cap)
        return 1;

    newcap = (tb->cap == 0) ? 128u : tb->cap * 2u;
    while (newcap < need)
        newcap *= 2u;

    nbuf = (zragf_token *)zragf_alloc_default(NULL, (unsigned)newcap,
                                              (unsigned)sizeof(zragf_token));
    if (!nbuf)
        return 0;

    if (tb->size > 0 && tb->data)
        memcpy(nbuf, tb->data, (size_t)(tb->size * sizeof(zragf_token)));

    if (tb->data)
        zragf_free_default(NULL, tb->data);

    tb->data = nbuf;
    tb->cap  = newcap;
    return 1;
}

int zragf_tokens_init(zragf_token_buffer *tb, zragf_size_t initial_cap)
{
    if (!tb)
        return 0;
    tb->data = NULL;
    tb->size = 0;
    tb->cap  = 0;
    if (initial_cap == 0)
        return 1;
    return zragf_tokens_reserve_extra(tb, initial_cap);
}

void zragf_tokens_free(zragf_token_buffer *tb)
{
    if (!tb)
        return;
    if (tb->data)
        zragf_free_default(NULL, tb->data);
    tb->data = NULL;
    tb->size = 0;
    tb->cap  = 0;
}

void zragf_tokens_reset(zragf_token_buffer *tb)
{
    if (!tb)
        return;
    tb->size = 0;
}

/* clamp helpers básicos para no salirnos de rango DEFLATE */
static int zragf_clamp_lit(int lit)
{
    if (lit < 0)   lit = 0;
    if (lit > 255) lit = 255;
    return lit;
}

static void zragf_clamp_match(int *len, int *dist)
{
    if (*len < 3)   *len = 3;
    if (*len > 258) *len = 258;
    if (*dist < 1)  *dist = 1;
    /* dist máximo real lo puedes ajustar según ventana;
       aquí no clampamos por arriba para no mentirle a core. */
}

int zragf_tokens_push_literal(zragf_token_buffer *tb, int lit)
{
    zragf_token *t;
    if (!tb)
        return 0;
    if (!zragf_tokens_reserve_extra(tb, 1))
        return 0;

    t = &tb->data[tb->size++];
    t->type = ZRAGF_TOK_LITERAL;
    t->lit  = zragf_clamp_lit(lit);
    t->len  = 0;
    t->dist = 0;
    return 1;
}

int zragf_tokens_push_match(zragf_token_buffer *tb, int len, int dist)
{
    zragf_token *t;
    if (!tb)
        return 0;
    zragf_clamp_match(&len, &dist);
    if (!zragf_tokens_reserve_extra(tb, 1))
        return 0;

    t = &tb->data[tb->size++];
    t->type = ZRAGF_TOK_MATCH;
    t->lit  = -1;
    t->len  = len;
    t->dist = dist;
    return 1;
}
