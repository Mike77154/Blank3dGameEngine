/* ============================================================
 * SICOL - Broadphase
 * ============================================================ */

#include "sicol_broadphase.h"
#include "sicol_shape.h"

typedef struct {
    fx min_x;
    fx max_x;
    fx min_y;
    fx max_y;
    fx min_z;
    fx max_z;
    int index;
} sicol_bp_interval_t;

static int bp_is_volume_shape(const sicol_shape_t* s)
{
    if (!s) return 0;

    switch (s->type) {
    case SICOL_SHAPE_AABB:
    case SICOL_SHAPE_OBB:
    case SICOL_SHAPE_SPHERE:
    case SICOL_SHAPE_CAPSULE:
    case SICOL_SHAPE_CONVEX:
    case SICOL_SHAPE_TRIANGLE:
    case SICOL_SHAPE_MESH:
        return 1;
    default:
        break;
    }

    return 0;
}

static void bp_fill_interval(const sicol_shape_t* s, sicol_bp_interval_t* out)
{
    fx min_v[3];
    fx max_v[3];
    sicol_shape_compute_aabb(s, min_v, max_v);
    out->min_x = min_v[0];
    out->max_x = max_v[0];
    out->min_y = min_v[1];
    out->max_y = max_v[1];
    out->min_z = min_v[2];
    out->max_z = max_v[2];
}

static void bp_sort(sicol_bp_interval_t* arr, int count)
{
    int i;
    for (i = 1; i < count; ++i) {
        sicol_bp_interval_t key = arr[i];
        int j = i - 1;
        while (j >= 0 && arr[j].min_x > key.min_x) {
            arr[j + 1] = arr[j];
            --j;
        }
        arr[j + 1] = key;
    }
}

sicol_bp_result_t sicol_broadphase_sap_ex(
    const sicol_shape_t* shapes,
    int count,
    sicol_pair_t* out_pairs,
    int max_pairs
)
{
    sicol_bp_interval_t intervals[SICOL_BP_MAX_OBJECTS];
    sicol_bp_result_t result;
    int interval_count = 0;
    int i, j;

    result.pair_count = 0;
    result.truncated = 0;

    if (!shapes || !out_pairs || max_pairs <= 0) return result;

    for (i = 0; i < count && interval_count < SICOL_BP_MAX_OBJECTS; ++i) {
        if (!bp_is_volume_shape(&shapes[i])) continue;
        intervals[interval_count].index = i;
        bp_fill_interval(&shapes[i], &intervals[interval_count]);
        ++interval_count;
    }

    bp_sort(intervals, interval_count);

    for (i = 0; i < interval_count; ++i) {
        for (j = i + 1; j < interval_count; ++j) {
            if (intervals[j].min_x > intervals[i].max_x) break;
            if (intervals[j].max_y < intervals[i].min_y || intervals[j].min_y > intervals[i].max_y) continue;
            if (intervals[j].max_z < intervals[i].min_z || intervals[j].min_z > intervals[i].max_z) continue;

            if (result.pair_count < max_pairs) {
                out_pairs[result.pair_count].a = intervals[i].index;
                out_pairs[result.pair_count].b = intervals[j].index;
                ++result.pair_count;
            } else {
                result.truncated = 1;
            }
        }
    }

    return result;
}

int sicol_broadphase_sap(
    const sicol_shape_t* shapes,
    int count,
    sicol_pair_t* out_pairs,
    int max_pairs
)
{
    sicol_bp_result_t result = sicol_broadphase_sap_ex(shapes, count, out_pairs, max_pairs);
    return result.pair_count;
}
