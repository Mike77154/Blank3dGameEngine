#include "ccs_broad_sweep.h"

/* Swap helper */
static void sweep_swap(CCS_SweepObject* a, CCS_SweepObject* b)
{
    CCS_SweepObject tmp;
    tmp = *a;
    *a = *b;
    *b = tmp;
}

/* Insertion sort por eje X (min.x) */
static void sweep_sort(CCS_SweepContext* ctx)
{
    int i;

    for (i = 1; i < ctx->object_count; ++i) {
        int j;
        j = i;
        while (j > 0 && ctx->objects[j - 1].aabb.min.x > ctx->objects[j].aabb.min.x) {
            sweep_swap(&ctx->objects[j - 1], &ctx->objects[j]);
            --j;
        }
    }
}

void ccs_sweep_init(CCS_SweepContext* ctx)
{
    if (!ctx) return;
    ctx->object_count = 0;
    ctx->pair_count = 0;
    ctx->object_overflow = 0;
    ctx->pair_overflow = 0;
}

void ccs_sweep_add(CCS_SweepContext* ctx, const ccs_aabb* aabb, int object_id)
{
    if (!ctx || !aabb) return;
    if (ctx->object_count >= CCS_SWEEP_MAX_OBJECTS) {
        ctx->object_overflow = 1;
        return;
    }

    ctx->objects[ctx->object_count].aabb = *aabb;
    ctx->objects[ctx->object_count].object_id = object_id;
    ctx->object_count++;
}

/* Overlap test en Y y Z */
static int sweep_overlap_yz(const ccs_aabb* a, const ccs_aabb* b)
{
    if (a->max.y < b->min.y || a->min.y > b->max.y) return 0;
    if (a->max.z < b->min.z || a->min.z > b->max.z) return 0;
    return 1;
}

void ccs_sweep_compute(CCS_SweepContext* ctx)
{
    int i;

    if (!ctx) return;
    ctx->pair_count = 0;
    ctx->pair_overflow = 0;

    /* Ordena por eje X */
    sweep_sort(ctx);

    for (i = 0; i < ctx->object_count; ++i) {
        const ccs_aabb* a;
        int j;

        a = &ctx->objects[i].aabb;

        for (j = i + 1; j < ctx->object_count; ++j) {
            const ccs_aabb* b;

            b = &ctx->objects[j].aabb;

            /* Early out en X */
            if (b->min.x > a->max.x)
                break;

            /* Check YZ */
            if (sweep_overlap_yz(a, b)) {
                if (ctx->pair_count < CCS_SWEEP_MAX_PAIRS) {
                    ctx->pairs[ctx->pair_count].a = ctx->objects[i].object_id;
                    ctx->pairs[ctx->pair_count].b = ctx->objects[j].object_id;
                    ctx->pair_count++;
                } else {
                    ctx->pair_overflow = 1;
                }
            }
        }
    }
}
