#include "ccs_manifold.h"

static ccs_fixed ccs_fixed_min(ccs_fixed a, ccs_fixed b)
{
    return (a < b) ? a : b;
}

void ccs_manifold_clear(ccs_manifold* m)
{
    int i;

    if (!m)
        return;

    m->count = 0;
    for (i = 0; i < CCS_MAX_CONTACTS; ++i) {
        m->contacts[i].point = ccs_vec3_zero();
        m->contacts[i].normal = ccs_vec3_zero();
        m->contacts[i].penetration = 0;
        m->contacts[i].feature_id = 0;
        m->contacts[i].normal_impulse = 0;
        m->contacts[i].tangent_impulse1 = 0;
        m->contacts[i].tangent_impulse2 = 0;
    }
}

void ccs_manifold_reset_impulses(ccs_manifold* m)
{
    int i;
    if (!m)
        return;

    for (i = 0; i < m->count; ++i) {
        m->contacts[i].normal_impulse = 0;
        m->contacts[i].tangent_impulse1 = 0;
        m->contacts[i].tangent_impulse2 = 0;
    }
}

void ccs_manifold_warm_start(ccs_manifold* current, const ccs_manifold* prev)
{
    int i;
    int j;
    ccs_fixed max_dist;
    ccs_fixed max_dist_sq;

    if (!current) return;

    /* reset impulses by default */
    ccs_manifold_reset_impulses(current);

    if (!prev) return;
    if (prev->count <= 0) return;

    max_dist = CCS_WARMSTART_DISTANCE;
    max_dist_sq = ccs_fixed_mul(max_dist, max_dist);

    for (i = 0; i < current->count; ++i) {
        int best_j;
        ccs_fixed best_dist_sq;

        best_j = -1;
        best_dist_sq = (ccs_fixed)CCS_I32_MAX;

        /* 1) match exact feature_id */
        for (j = 0; j < prev->count; ++j) {
            if (current->contacts[i].feature_id != 0 &&
                current->contacts[i].feature_id == prev->contacts[j].feature_id) {
                best_j = j;
                best_dist_sq = 0;
                break;
            }
        }

        /* 2) fallback: match by proximity (and similar normal) */
        if (best_j < 0) {
            for (j = 0; j < prev->count; ++j) {
                ccs_vec3 d;
                ccs_fixed dist_sq;
                ccs_fixed ndot;

                d = ccs_vec3_sub(current->contacts[i].point, prev->contacts[j].point);
                dist_sq = ccs_vec3_len_sq(d);

                if (dist_sq > max_dist_sq)
                    continue;

                ndot = ccs_vec3_dot(current->contacts[i].normal, prev->contacts[j].normal);
                /* require somewhat similar normal (>= 0.5) */
                if (ndot < (ccs_fixed)(CCS_FIXED_ONE / 2))
                    continue;

                if (dist_sq < best_dist_sq) {
                    best_dist_sq = dist_sq;
                    best_j = j;
                }
            }
        }

        if (best_j >= 0) {
            current->contacts[i].normal_impulse = prev->contacts[best_j].normal_impulse;
            current->contacts[i].tangent_impulse1 = prev->contacts[best_j].tangent_impulse1;
            current->contacts[i].tangent_impulse2 = prev->contacts[best_j].tangent_impulse2;
        }

        /* clamp negative penetration impulses if any (safety) */
        if (current->contacts[i].normal_impulse < 0)
            current->contacts[i].normal_impulse = 0;

        /* friction impulses can be signed; keep as-is */
        (void)ccs_fixed_min;
    }
}
