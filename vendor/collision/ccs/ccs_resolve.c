#include "ccs_resolve.h"

/*
    Resuelve un solo contacto.
    La penetración se reparte 50/50.
*/
void ccs_resolve_contact(
    ccs_vec3* posA,
    ccs_vec3* posB,
    const ccs_contact* c
)
{
    ccs_fixed half;
    ccs_vec3 push;

    if (!posA || !posB || !c)
        return;

    /* push = normal * (penetration / 2) */
    half = (ccs_fixed)(c->penetration / 2);
    push = ccs_vec3_scale(c->normal, half);

    *posA = ccs_vec3_sub(*posA, push);
    *posB = ccs_vec3_add(*posB, push);
}

/*
    Resuelve un manifold completo.
    Usa la normal dominante y la mayor penetración.
*/
void ccs_resolve_manifold(
    ccs_vec3* posA,
    ccs_vec3* posB,
    const ccs_manifold* m
)
{
    int best;
    int i;

    if (!posA || !posB || !m)
        return;

    if (m->count <= 0)
        return;

    /* elegir el contacto con mayor penetración */
    best = 0;
    for (i = 1; i < m->count; ++i) {
        if (m->contacts[i].penetration > m->contacts[best].penetration)
            best = i;
    }

    /* ccs_resolve_contact espera ccs_contact (single), así que adaptamos */
    {
        ccs_contact c;
        c.normal = m->contacts[best].normal;
        c.penetration = m->contacts[best].penetration;
        ccs_resolve_contact(posA, posB, &c);
    }
}
