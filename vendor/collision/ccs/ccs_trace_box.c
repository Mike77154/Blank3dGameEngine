#include "ccs_trace_box.h"

#include "ccs_fixed.h"
#include "ccs_math.h"

/* dot product fixed */
static ccs_fixed dot3(ccs_vec3 a, ccs_vec3 b)
{
    return ccs_fixed_mul(a.x, b.x)
         + ccs_fixed_mul(a.y, b.y)
         + ccs_fixed_mul(a.z, b.z);
}

/* soporte extremo de AABB contra normal */
static ccs_fixed box_support(const ccs_aabb* box, ccs_vec3 n)
{
    ccs_vec3 p;
    p.x = (n.x >= 0) ? box->max.x : box->min.x;
    p.y = (n.y >= 0) ? box->max.y : box->min.y;
    p.z = (n.z >= 0) ? box->max.z : box->min.z;
    return dot3(p, n);
}

void ccs_trace_box_against_planes(
    const CCS_TraceBox* trace,
    const CCS_Plane* planes,
    int plane_count,
    CCS_TraceResult* out
)
{
    ccs_fixed enter;
    ccs_fixed exit;
    int i;

    if (!trace || !planes || plane_count <= 0 || !out) {
        return;
    }

    enter = 0;
    exit  = CCS_FIXED_ONE;

    out->hit = 0;
    out->fraction = CCS_FIXED_ONE;
    out->hit_normal = ccs_vec3_zero();

    for (i = 0; i < plane_count; ++i) {
        const CCS_Plane* pl;
        ccs_fixed support;
        ccs_fixed start_dist;
        ccs_fixed end_dist;

        pl = &planes[i];
        support = box_support(&trace->box, pl->normal);

        /*
            start_dist/end_dist son distancias firmadas del AABB "inflado" hacia el plano:
            dot(p, n) - (dist + support)
        */
        start_dist = dot3(trace->start, pl->normal) - (pl->dist + support);
        end_dist   = dot3(trace->end,   pl->normal) - (pl->dist + support);

        /* completamente fuera */
        if (start_dist > 0 && end_dist > 0)
            return;

        /* completamente dentro */
        if (start_dist <= 0 && end_dist <= 0)
            continue;

        /* cruza el plano */
        {
            ccs_fixed denom;
            ccs_fixed frac;

            /* frac = start / (start - end) */
            denom = (start_dist - end_dist);
            if (denom == 0)
                continue;

            frac = ccs_fixed_div(start_dist, denom);

            if (start_dist > end_dist) {
                /* entrando */
                if (frac > enter) {
                    enter = frac;
                    out->hit_normal = pl->normal;
                }
            } else {
                /* saliendo */
                if (frac < exit)
                    exit = frac;
            }

            if (enter > exit)
                return;
        }
    }

    out->hit = 1;
    out->fraction = enter;
}
