#include "ccs_broad_grid3d.h"

/* ============================================================
   Grid storage (static, sin malloc)
   ============================================================ */

#define CCS_BG3D_CELL_COUNT (CCS_BG3D_X * CCS_BG3D_Y * CCS_BG3D_Z)

static int g_cell_head[CCS_BG3D_CELL_COUNT];
static int g_next[CCS_MAX_BODIES];

static int g_inserted[CCS_MAX_BODIES];
static int g_inserted_count = 0;

static int g_overflow[CCS_MAX_BODIES];
static int g_overflow_count = 0;

static unsigned char g_mark[CCS_MAX_BODIES];
static unsigned char g_is_overflow[CCS_MAX_BODIES];

static int cell_index(int x, int y, int z)
{
    return x + y * CCS_BG3D_X + z * (CCS_BG3D_X * CCS_BG3D_Y);
}

/* ============================================================
   API
   ============================================================ */

void ccs_broad_grid3d_clear(void)
{
    int i;

    for (i = 0; i < CCS_BG3D_CELL_COUNT; ++i)
        g_cell_head[i] = -1;

    g_inserted_count = 0;
    g_overflow_count = 0;

    for (i = 0; i < CCS_MAX_BODIES; ++i) {
        g_mark[i] = 0;
        g_is_overflow[i] = 0;
        g_next[i] = -1;
    }
}

void ccs_broad_grid3d_insert(int id, int gx, int gy, int gz)
{
    int idx;

    if (id < 0 || id >= CCS_MAX_BODIES)
        return;

    /* registrar id una sola vez por frame */
    if (!g_mark[id]) {
        g_mark[id] = 1;
        if (g_inserted_count < CCS_MAX_BODIES) {
            g_inserted[g_inserted_count] = id;
            g_inserted_count++;
        }
    }

    /* bounds */
    if (gx < 0 || gy < 0 || gz < 0 ||
        gx >= CCS_BG3D_X || gy >= CCS_BG3D_Y || gz >= CCS_BG3D_Z) {

        /* overflow list */
        if (!g_is_overflow[id]) {
            g_is_overflow[id] = 1;
            if (g_overflow_count < CCS_MAX_BODIES) {
                g_overflow[g_overflow_count] = id;
                g_overflow_count++;
            }
        }

        return;
    }

    g_is_overflow[id] = 0;

    idx = cell_index(gx, gy, gz);

    g_next[id] = g_cell_head[idx];
    g_cell_head[idx] = id;
}

static void emit_pairs_between_lists(int headA, int headB, ccs_pair_fn fn)
{
    int a;

    if (!fn)
        return;

    for (a = headA; a != -1; a = g_next[a]) {
        int b;
        for (b = headB; b != -1; b = g_next[b]) {
            fn(a, b);
        }
    }
}

static void emit_pairs_within_cell(int head, ccs_pair_fn fn)
{
    int a;

    if (!fn)
        return;

    for (a = head; a != -1; a = g_next[a]) {
        int b;
        for (b = g_next[a]; b != -1; b = g_next[b]) {
            fn(a, b);
        }
    }
}

void ccs_broad_grid3d_for_each_pair(ccs_pair_fn fn)
{
    int x;
    int y;
    int z;

    int dx;
    int dy;
    int dz;

    int range;

    if (!fn)
        return;

    range = CCS_BG3D_NEIGHBOR_RANGE;
    if (range < 0)
        range = 0;

    /* --------------------------------------------------------
       Grid pairs: within cell + neighbor cells
       -------------------------------------------------------- */

    for (z = 0; z < CCS_BG3D_Z; ++z) {
        for (y = 0; y < CCS_BG3D_Y; ++y) {
            for (x = 0; x < CCS_BG3D_X; ++x) {
                int idx0;
                int head0;

                idx0 = cell_index(x, y, z);
                head0 = g_cell_head[idx0];

                if (head0 == -1)
                    continue;

                /* pairs within cell */
                emit_pairs_within_cell(head0, fn);

                /* pairs with neighbor cells (lexicographic positive offsets) */
                for (dz = -range; dz <= range; ++dz) {
                    for (dy = -range; dy <= range; ++dy) {
                        for (dx = -range; dx <= range; ++dx) {
                            int nx;
                            int ny;
                            int nz;
                            int idx1;
                            int head1;

                            if (dx == 0 && dy == 0 && dz == 0)
                                continue;

                            /* enforce ordering to avoid duplicates */
                            if (dz < 0)
                                continue;
                            if (dz == 0 && dy < 0)
                                continue;
                            if (dz == 0 && dy == 0 && dx < 0)
                                continue;

                            nx = x + dx;
                            ny = y + dy;
                            nz = z + dz;

                            if (nx < 0 || ny < 0 || nz < 0)
                                continue;
                            if (nx >= CCS_BG3D_X || ny >= CCS_BG3D_Y || nz >= CCS_BG3D_Z)
                                continue;

                            idx1 = cell_index(nx, ny, nz);
                            head1 = g_cell_head[idx1];
                            if (head1 == -1)
                                continue;

                            emit_pairs_between_lists(head0, head1, fn);
                        }
                    }
                }
            }
        }
    }

    /* --------------------------------------------------------
       Overflow pairs: ensure no object is silently dropped
       -------------------------------------------------------- */

    if (g_overflow_count > 0) {
        int oi;

        /* overflow vs non-overflow */
        for (oi = 0; oi < g_overflow_count; ++oi) {
            int a;
            int j;

            a = g_overflow[oi];
            if (a < 0 || a >= CCS_MAX_BODIES)
                continue;
            if (!g_is_overflow[a])
                continue;

            for (j = 0; j < g_inserted_count; ++j) {
                int b;
                b = g_inserted[j];

                if (b == a)
                    continue;

                if (b < 0 || b >= CCS_MAX_BODIES)
                    continue;

                if (g_is_overflow[b])
                    continue;

                fn(a, b);
            }
        }

        /* overflow vs overflow (unique pairs) */
        for (oi = 0; oi < g_overflow_count; ++oi) {
            int oj;
            int a;

            a = g_overflow[oi];
            if (a < 0 || a >= CCS_MAX_BODIES)
                continue;
            if (!g_is_overflow[a])
                continue;

            for (oj = oi + 1; oj < g_overflow_count; ++oj) {
                int b;
                b = g_overflow[oj];
                if (b < 0 || b >= CCS_MAX_BODIES)
                    continue;
                if (!g_is_overflow[b])
                    continue;

                fn(a, b);
            }
        }
    }
}

int ccs_broad_grid3d_overflow_count(void)
{
    return g_overflow_count;
}
