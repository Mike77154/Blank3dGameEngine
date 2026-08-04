#include "gbulletmesh89.h"

static const gbm_fix gbm_cos12[GBM_SEGMENTS] = {
    65536L, 56756L, 32768L, 0L, -32768L, -56756L,
    -65536L, -56756L, -32768L, 0L, 32768L, 56756L
};

static const gbm_fix gbm_sin12[GBM_SEGMENTS] = {
    0L, 32768L, 56756L, 65536L, 56756L, 32768L,
    0L, -32768L, -56756L, -65536L, -56756L, -32768L
};

static gbm_fix gbm_mul(gbm_fix a, gbm_fix b)
{
    return (gbm_fix)((a * b) / GBM_FIX_ONE);
}

gbm_fix gbm_fix_from_int(int x)
{
    return ((gbm_fix)x) * GBM_FIX_ONE;
}

gbm_fix gbm_fix_ratio(int num, int den)
{
    if (den == 0) return 0L;
    return (((gbm_fix)num) * GBM_FIX_ONE) / ((gbm_fix)den);
}

void gbm_mesh_init(GBM_Mesh *m, GBM_Vertex *v, unsigned short v_cap, GBM_Tri *t, unsigned short t_cap)
{
    if (m == 0) return;
    m->v = v;
    m->v_cap = v_cap;
    m->v_count = 0;
    m->t = t;
    m->t_cap = t_cap;
    m->t_count = 0;
    m->error = GBM_OK;
}

void gbm_mesh_reset(GBM_Mesh *m)
{
    if (m == 0) return;
    m->v_count = 0;
    m->t_count = 0;
    m->error = GBM_OK;
}

static unsigned short gbm_add_vertex(GBM_Mesh *m, gbm_fix x, gbm_fix y, gbm_fix z, unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    unsigned short id;
    if (m == 0 || m->v == 0) return 0;
    if (m->v_count >= m->v_cap) {
        m->error = GBM_ERR_CAPACITY;
        return 0;
    }
    id = m->v_count;
    m->v[id].x = x;
    m->v[id].y = y;
    m->v[id].z = z;
    m->v[id].r = r;
    m->v[id].g = g;
    m->v[id].b = b;
    m->v[id].a = a;
    m->v_count = (unsigned short)(m->v_count + 1);
    return id;
}

static void gbm_add_tri(GBM_Mesh *m, unsigned short a, unsigned short b, unsigned short c)
{
    unsigned short id;
    if (m == 0 || m->t == 0) return;
    if (m->t_count >= m->t_cap) {
        m->error = GBM_ERR_CAPACITY;
        return;
    }
    id = m->t_count;
    m->t[id].a = a;
    m->t[id].b = b;
    m->t[id].c = c;
    m->t_count = (unsigned short)(m->t_count + 1);
}

static unsigned short gbm_add_ring(GBM_Mesh *m, gbm_fix z, gbm_fix radius, unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    unsigned short base;
    int i;
    base = m->v_count;
    for (i = 0; i < GBM_SEGMENTS; ++i) {
        gbm_add_vertex(m, gbm_mul(radius, gbm_cos12[i]), gbm_mul(radius, gbm_sin12[i]), z, r, g, b, a);
    }
    return base;
}

static void gbm_stitch_rings(GBM_Mesh *m, unsigned short a, unsigned short b)
{
    int i;
    int j;
    for (i = 0; i < GBM_SEGMENTS; ++i) {
        j = i + 1;
        if (j >= GBM_SEGMENTS) j = 0;
        gbm_add_tri(m, (unsigned short)(a + i), (unsigned short)(b + i), (unsigned short)(b + j));
        gbm_add_tri(m, (unsigned short)(a + i), (unsigned short)(b + j), (unsigned short)(a + j));
    }
}

static void gbm_cap_ring(GBM_Mesh *m, unsigned short ring, gbm_fix z, int top, unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    unsigned short center;
    int i;
    int j;
    center = gbm_add_vertex(m, 0L, 0L, z, r, g, b, a);
    for (i = 0; i < GBM_SEGMENTS; ++i) {
        j = i + 1;
        if (j >= GBM_SEGMENTS) j = 0;
        if (top) gbm_add_tri(m, (unsigned short)(ring + i), center, (unsigned short)(ring + j));
        else gbm_add_tri(m, (unsigned short)(ring + j), center, (unsigned short)(ring + i));
    }
}

static void gbm_add_cylinder(GBM_Mesh *m, gbm_fix z0, gbm_fix z1, gbm_fix radius, int cap0, int cap1, unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    unsigned short r0;
    unsigned short r1;
    r0 = gbm_add_ring(m, z0, radius, r, g, b, a);
    r1 = gbm_add_ring(m, z1, radius, r, g, b, a);
    gbm_stitch_rings(m, r0, r1);
    if (cap0) gbm_cap_ring(m, r0, z0, 0, r, g, b, a);
    if (cap1) gbm_cap_ring(m, r1, z1, 1, r, g, b, a);
}

static void gbm_add_frustum(GBM_Mesh *m, gbm_fix z0, gbm_fix radius0, gbm_fix z1, gbm_fix radius1, unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    unsigned short base0;
    unsigned short base1;
    unsigned short tip;
    int i;
    int j;
    if (radius0 <= 0L && radius1 <= 0L) return;
    if (radius1 <= 0L) {
        base0 = gbm_add_ring(m, z0, radius0, r, g, b, a);
        tip = gbm_add_vertex(m, 0L, 0L, z1, r, g, b, a);
        for (i = 0; i < GBM_SEGMENTS; ++i) {
            j = i + 1;
            if (j >= GBM_SEGMENTS) j = 0;
            gbm_add_tri(m, (unsigned short)(base0 + i), tip, (unsigned short)(base0 + j));
        }
        return;
    }
    if (radius0 <= 0L) {
        tip = gbm_add_vertex(m, 0L, 0L, z0, r, g, b, a);
        base1 = gbm_add_ring(m, z1, radius1, r, g, b, a);
        for (i = 0; i < GBM_SEGMENTS; ++i) {
            j = i + 1;
            if (j >= GBM_SEGMENTS) j = 0;
            gbm_add_tri(m, tip, (unsigned short)(base1 + i), (unsigned short)(base1 + j));
        }
        return;
    }
    base0 = gbm_add_ring(m, z0, radius0, r, g, b, a);
    base1 = gbm_add_ring(m, z1, radius1, r, g, b, a);
    gbm_stitch_rings(m, base0, base1);
}

static void gbm_add_low_sphere(GBM_Mesh *m, gbm_fix cx, gbm_fix cy, gbm_fix cz, gbm_fix radius, unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    unsigned short top;
    unsigned short mid;
    unsigned short bot;
    int i;
    int j;
    top = gbm_add_vertex(m, cx, cy, cz + radius, r, g, b, a);
    mid = m->v_count;
    for (i = 0; i < GBM_SEGMENTS; ++i) {
        gbm_add_vertex(m, cx + gbm_mul(radius, gbm_cos12[i]), cy + gbm_mul(radius, gbm_sin12[i]), cz, r, g, b, a);
    }
    bot = gbm_add_vertex(m, cx, cy, cz - radius, r, g, b, a);
    for (i = 0; i < GBM_SEGMENTS; ++i) {
        j = i + 1;
        if (j >= GBM_SEGMENTS) j = 0;
        gbm_add_tri(m, top, (unsigned short)(mid + i), (unsigned short)(mid + j));
        gbm_add_tri(m, bot, (unsigned short)(mid + j), (unsigned short)(mid + i));
    }
}

static void gbm_add_box(GBM_Mesh *m, gbm_fix x0, gbm_fix y0, gbm_fix z0, gbm_fix x1, gbm_fix y1, gbm_fix z1, unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    unsigned short v0;
    unsigned short v1;
    unsigned short v2;
    unsigned short v3;
    unsigned short v4;
    unsigned short v5;
    unsigned short v6;
    unsigned short v7;
    v0 = gbm_add_vertex(m, x0, y0, z0, r, g, b, a);
    v1 = gbm_add_vertex(m, x1, y0, z0, r, g, b, a);
    v2 = gbm_add_vertex(m, x1, y1, z0, r, g, b, a);
    v3 = gbm_add_vertex(m, x0, y1, z0, r, g, b, a);
    v4 = gbm_add_vertex(m, x0, y0, z1, r, g, b, a);
    v5 = gbm_add_vertex(m, x1, y0, z1, r, g, b, a);
    v6 = gbm_add_vertex(m, x1, y1, z1, r, g, b, a);
    v7 = gbm_add_vertex(m, x0, y1, z1, r, g, b, a);
    gbm_add_tri(m, v0, v1, v2); gbm_add_tri(m, v0, v2, v3);
    gbm_add_tri(m, v4, v6, v5); gbm_add_tri(m, v4, v7, v6);
    gbm_add_tri(m, v0, v4, v5); gbm_add_tri(m, v0, v5, v1);
    gbm_add_tri(m, v1, v5, v6); gbm_add_tri(m, v1, v6, v2);
    gbm_add_tri(m, v2, v6, v7); gbm_add_tri(m, v2, v7, v3);
    gbm_add_tri(m, v3, v7, v4); gbm_add_tri(m, v3, v4, v0);
}

static void gbm_add_round_nose_bullet(GBM_Mesh *m, gbm_fix radius, gbm_fix body_len, gbm_fix zbase)
{
    unsigned char cr;
    unsigned char cg;
    unsigned char cb;
    gbm_fix z0;
    gbm_fix z1;
    unsigned short r0;
    unsigned short r1;
    unsigned short r2;
    unsigned short r3;
    unsigned short tip;
    int i;
    int j;
    cr = 214U; cg = 152U; cb = 58U;
    z0 = zbase;
    z1 = zbase + body_len;
    gbm_add_cylinder(m, z0, z1, radius, 1, 0, cr, cg, cb, 255U);
    r0 = gbm_add_ring(m, z1, radius, cr, cg, cb, 255U);
    r1 = gbm_add_ring(m, z1 + gbm_fix_ratio(9, 100), gbm_mul(radius, gbm_fix_ratio(93, 100)), cr, cg, cb, 255U);
    r2 = gbm_add_ring(m, z1 + gbm_fix_ratio(22, 100), gbm_mul(radius, gbm_fix_ratio(66, 100)), cr, cg, cb, 255U);
    r3 = gbm_add_ring(m, z1 + gbm_fix_ratio(32, 100), gbm_mul(radius, gbm_fix_ratio(28, 100)), cr, cg, cb, 255U);
    gbm_stitch_rings(m, r0, r1);
    gbm_stitch_rings(m, r1, r2);
    gbm_stitch_rings(m, r2, r3);
    tip = gbm_add_vertex(m, 0L, 0L, z1 + gbm_fix_ratio(37, 100), cr, cg, cb, 255U);
    for (i = 0; i < GBM_SEGMENTS; ++i) {
        j = i + 1;
        if (j >= GBM_SEGMENTS) j = 0;
        gbm_add_tri(m, (unsigned short)(r3 + i), tip, (unsigned short)(r3 + j));
    }
}

static void gbm_add_spitzer_bullet(GBM_Mesh *m, gbm_fix radius, gbm_fix body_len, gbm_fix nose_len, int boat_tail, gbm_fix zbase)
{
    unsigned char cr;
    unsigned char cg;
    unsigned char cb;
    gbm_fix z0;
    gbm_fix z1;
    unsigned short b0;
    unsigned short b1;
    unsigned short r0;
    unsigned short r1;
    unsigned short r2;
    unsigned short r3;
    unsigned short tip;
    int i;
    int j;
    cr = 218U; cg = 158U; cb = 57U;
    z0 = zbase;
    if (boat_tail) {
        b0 = gbm_add_ring(m, z0, gbm_mul(radius, gbm_fix_ratio(70, 100)), cr, cg, cb, 255U);
        b1 = gbm_add_ring(m, z0 + gbm_fix_ratio(14, 100), radius, cr, cg, cb, 255U);
        gbm_stitch_rings(m, b0, b1);
        gbm_cap_ring(m, b0, z0, 0, cr, cg, cb, 255U);
        z0 = z0 + gbm_fix_ratio(14, 100);
    }
    z1 = z0 + body_len;
    gbm_add_cylinder(m, z0, z1, radius, boat_tail ? 0 : 1, 0, cr, cg, cb, 255U);
    r0 = gbm_add_ring(m, z1, radius, cr, cg, cb, 255U);
    r1 = gbm_add_ring(m, z1 + gbm_mul(nose_len, gbm_fix_ratio(34, 100)), gbm_mul(radius, gbm_fix_ratio(82, 100)), cr, cg, cb, 255U);
    r2 = gbm_add_ring(m, z1 + gbm_mul(nose_len, gbm_fix_ratio(66, 100)), gbm_mul(radius, gbm_fix_ratio(48, 100)), cr, cg, cb, 255U);
    r3 = gbm_add_ring(m, z1 + gbm_mul(nose_len, gbm_fix_ratio(86, 100)), gbm_mul(radius, gbm_fix_ratio(18, 100)), cr, cg, cb, 255U);
    gbm_stitch_rings(m, r0, r1);
    gbm_stitch_rings(m, r1, r2);
    gbm_stitch_rings(m, r2, r3);
    tip = gbm_add_vertex(m, 0L, 0L, z1 + nose_len, cr, cg, cb, 255U);
    for (i = 0; i < GBM_SEGMENTS; ++i) {
        j = i + 1;
        if (j >= GBM_SEGMENTS) j = 0;
        gbm_add_tri(m, (unsigned short)(r3 + i), tip, (unsigned short)(r3 + j));
    }
}

static void gbm_add_wadcutter_bullet(GBM_Mesh *m, gbm_fix radius, gbm_fix body_len, gbm_fix zbase, int hollow_hint)
{
    unsigned char cr;
    unsigned char cg;
    unsigned char cb;
    gbm_fix z0;
    gbm_fix z1;
    cr = 205U; cg = 134U; cb = 47U;
    z0 = zbase;
    z1 = zbase + body_len;
    gbm_add_cylinder(m, z0, z0 + gbm_fix_ratio(46, 100), radius, 1, 0, cr, cg, cb, 255U);
    gbm_add_frustum(m, z0 + gbm_fix_ratio(46, 100), radius, z1 - gbm_fix_ratio(12, 100), gbm_mul(radius, gbm_fix_ratio(72, 100)), cr, cg, cb, 255U);
    gbm_add_cylinder(m, z1 - gbm_fix_ratio(12, 100), z1, gbm_mul(radius, gbm_fix_ratio(72, 100)), 0, hollow_hint ? 0 : 1, cr, cg, cb, 255U);
    if (hollow_hint) {
        gbm_add_frustum(m, z1 - gbm_fix_ratio(2, 100), gbm_mul(radius, gbm_fix_ratio(30, 100)), z1 + gbm_fix_ratio(7, 100), 0L, 74U, 45U, 29U, 255U);
    }
}

static void gbm_add_mg_link_raw(GBM_Mesh *m);

static void gbm_add_shot_pellets_at(GBM_Mesh *m, gbm_fix zoff)
{
    gbm_fix pr;
    pr = gbm_fix_ratio(10, 100);
    gbm_add_low_sphere(m, 0L, 0L, zoff + gbm_fix_ratio(0, 100), pr, 185U, 185U, 180U, 255U);
    gbm_add_low_sphere(m, gbm_fix_ratio(16, 100), 0L, zoff + gbm_fix_ratio(5, 100), pr, 185U, 185U, 180U, 255U);
    gbm_add_low_sphere(m, gbm_fix_ratio(-16, 100), 0L, zoff + gbm_fix_ratio(5, 100), pr, 185U, 185U, 180U, 255U);
    gbm_add_low_sphere(m, 0L, gbm_fix_ratio(16, 100), zoff + gbm_fix_ratio(9, 100), pr, 185U, 185U, 180U, 255U);
    gbm_add_low_sphere(m, 0L, gbm_fix_ratio(-16, 100), zoff + gbm_fix_ratio(9, 100), pr, 185U, 185U, 180U, 255U);
    gbm_add_low_sphere(m, gbm_fix_ratio(11, 100), gbm_fix_ratio(11, 100), zoff + gbm_fix_ratio(18, 100), pr, 185U, 185U, 180U, 255U);
    gbm_add_low_sphere(m, gbm_fix_ratio(-11, 100), gbm_fix_ratio(-11, 100), zoff + gbm_fix_ratio(18, 100), pr, 185U, 185U, 180U, 255U);
}

int gbm_build_shell(GBM_Mesh *m, int ammo_type)
{
    if (m == 0) return GBM_ERR_NULL;
    gbm_mesh_reset(m);
    if (ammo_type == GBM_AMMO_PISTOL) {
        gbm_add_cylinder(m, gbm_fix_ratio(-74, 100), gbm_fix_ratio(0, 100), gbm_fix_ratio(18, 100), 1, 1, 202U, 145U, 48U, 255U);
        gbm_add_cylinder(m, gbm_fix_ratio(-86, 100), gbm_fix_ratio(-74, 100), gbm_fix_ratio(20, 100), 1, 0, 166U, 111U, 38U, 255U);
        gbm_add_cylinder(m, gbm_fix_ratio(-88, 100), gbm_fix_ratio(-86, 100), gbm_fix_ratio(14, 100), 1, 0, 79U, 62U, 45U, 255U);
    } else if (ammo_type == GBM_AMMO_SHOTGUN) {
        gbm_add_cylinder(m, gbm_fix_ratio(-96, 100), gbm_fix_ratio(0, 100), gbm_fix_ratio(30, 100), 0, 1, 164U, 27U, 27U, 255U);
        gbm_add_cylinder(m, gbm_fix_ratio(-128, 100), gbm_fix_ratio(-96, 100), gbm_fix_ratio(31, 100), 1, 0, 197U, 145U, 54U, 255U);
        gbm_add_cylinder(m, gbm_fix_ratio(-132, 100), gbm_fix_ratio(-128, 100), gbm_fix_ratio(23, 100), 1, 0, 84U, 61U, 38U, 255U);
        gbm_add_frustum(m, gbm_fix_ratio(-1, 100), gbm_fix_ratio(30, 100), gbm_fix_ratio(14, 100), gbm_fix_ratio(12, 100), 135U, 20U, 20U, 255U);
    } else if (ammo_type == GBM_AMMO_SNIPER) {
        gbm_add_cylinder(m, gbm_fix_ratio(-150, 100), gbm_fix_ratio(-48, 100), gbm_fix_ratio(20, 100), 1, 0, 205U, 150U, 52U, 255U);
        gbm_add_frustum(m, gbm_fix_ratio(-48, 100), gbm_fix_ratio(20, 100), gbm_fix_ratio(-26, 100), gbm_fix_ratio(13, 100), 205U, 150U, 52U, 255U);
        gbm_add_cylinder(m, gbm_fix_ratio(-26, 100), gbm_fix_ratio(0, 100), gbm_fix_ratio(13, 100), 0, 1, 205U, 150U, 52U, 255U);
        gbm_add_cylinder(m, gbm_fix_ratio(-160, 100), gbm_fix_ratio(-150, 100), gbm_fix_ratio(21, 100), 1, 0, 160U, 105U, 37U, 255U);
    } else if (ammo_type == GBM_AMMO_MAGNUM) {
        gbm_add_cylinder(m, gbm_fix_ratio(-112, 100), gbm_fix_ratio(0, 100), gbm_fix_ratio(22, 100), 1, 1, 205U, 146U, 48U, 255U);
        gbm_add_cylinder(m, gbm_fix_ratio(-124, 100), gbm_fix_ratio(-112, 100), gbm_fix_ratio(26, 100), 1, 0, 166U, 105U, 36U, 255U);
        gbm_add_cylinder(m, gbm_fix_ratio(-127, 100), gbm_fix_ratio(-124, 100), gbm_fix_ratio(17, 100), 1, 0, 83U, 61U, 39U, 255U);
    } else if (ammo_type == GBM_AMMO_MACHINEGUN) {
        gbm_add_cylinder(m, gbm_fix_ratio(-118, 100), gbm_fix_ratio(-42, 100), gbm_fix_ratio(18, 100), 1, 0, 198U, 142U, 48U, 255U);
        gbm_add_frustum(m, gbm_fix_ratio(-42, 100), gbm_fix_ratio(18, 100), gbm_fix_ratio(-24, 100), gbm_fix_ratio(12, 100), 198U, 142U, 48U, 255U);
        gbm_add_cylinder(m, gbm_fix_ratio(-24, 100), gbm_fix_ratio(0, 100), gbm_fix_ratio(12, 100), 0, 1, 198U, 142U, 48U, 255U);
        gbm_add_cylinder(m, gbm_fix_ratio(-128, 100), gbm_fix_ratio(-118, 100), gbm_fix_ratio(19, 100), 1, 0, 160U, 105U, 37U, 255U);
    } else {
        m->error = GBM_ERR_BAD_TYPE;
    }
    return m->error;
}

int gbm_build_shotgun_pellet(GBM_Mesh *m)
{
    if (m == 0) return GBM_ERR_NULL;
    gbm_mesh_reset(m);
    gbm_add_low_sphere(m, 0L, 0L, 0L, gbm_fix_ratio(10, 100),
                       185U, 185U, 180U, 255U);
    return m->error;
}

int gbm_build_projectile(GBM_Mesh *m, int ammo_type)
{
    if (m == 0) return GBM_ERR_NULL;
    gbm_mesh_reset(m);
    if (ammo_type == GBM_AMMO_PISTOL) {
        gbm_add_round_nose_bullet(m, gbm_fix_ratio(16, 100), gbm_fix_ratio(42, 100), gbm_fix_ratio(0, 100));
    } else if (ammo_type == GBM_AMMO_SHOTGUN) {
        gbm_add_shot_pellets_at(m, 0L);
    } else if (ammo_type == GBM_AMMO_SNIPER) {
        gbm_add_spitzer_bullet(m, gbm_fix_ratio(12, 100), gbm_fix_ratio(56, 100), gbm_fix_ratio(64, 100), 1, gbm_fix_ratio(0, 100));
    } else if (ammo_type == GBM_AMMO_MAGNUM) {
        gbm_add_wadcutter_bullet(m, gbm_fix_ratio(20, 100), gbm_fix_ratio(56, 100), gbm_fix_ratio(0, 100), 1);
    } else if (ammo_type == GBM_AMMO_MACHINEGUN) {
        gbm_add_spitzer_bullet(m, gbm_fix_ratio(11, 100), gbm_fix_ratio(48, 100), gbm_fix_ratio(50, 100), 1, gbm_fix_ratio(0, 100));
    } else {
        m->error = GBM_ERR_BAD_TYPE;
    }
    return m->error;
}

int gbm_build_full_round(GBM_Mesh *m, int ammo_type)
{
    if (m == 0) return GBM_ERR_NULL;
    gbm_mesh_reset(m);
    if (ammo_type == GBM_AMMO_PISTOL) {
        gbm_build_shell(m, ammo_type);
        gbm_add_round_nose_bullet(m, gbm_fix_ratio(16, 100), gbm_fix_ratio(28, 100), gbm_fix_ratio(0, 100));
    } else if (ammo_type == GBM_AMMO_SHOTGUN) {
        gbm_build_shell(m, ammo_type);
        gbm_add_shot_pellets_at(m, gbm_fix_ratio(18, 100));
    } else if (ammo_type == GBM_AMMO_SNIPER) {
        gbm_build_shell(m, ammo_type);
        gbm_add_spitzer_bullet(m, gbm_fix_ratio(12, 100), gbm_fix_ratio(36, 100), gbm_fix_ratio(60, 100), 1, gbm_fix_ratio(0, 100));
    } else if (ammo_type == GBM_AMMO_MAGNUM) {
        gbm_build_shell(m, ammo_type);
        gbm_add_wadcutter_bullet(m, gbm_fix_ratio(20, 100), gbm_fix_ratio(44, 100), gbm_fix_ratio(0, 100), 1);
    } else if (ammo_type == GBM_AMMO_MACHINEGUN) {
        gbm_build_shell(m, ammo_type);
        gbm_add_spitzer_bullet(m, gbm_fix_ratio(11, 100), gbm_fix_ratio(34, 100), gbm_fix_ratio(48, 100), 1, gbm_fix_ratio(0, 100));
        gbm_add_mg_link_raw(m);
    } else {
        m->error = GBM_ERR_BAD_TYPE;
    }
    return m->error;
}

static void gbm_add_mg_link_raw(GBM_Mesh *m)
{
    gbm_fix s;
    s = gbm_fix_ratio(18, 100);
    gbm_add_box(m, -s, gbm_fix_ratio(-6, 100), gbm_fix_ratio(-42, 100), s, gbm_fix_ratio(6, 100), gbm_fix_ratio(-24, 100), 72U, 77U, 82U, 255U);
    gbm_add_box(m, -s, gbm_fix_ratio(-6, 100), gbm_fix_ratio(-8, 100), s, gbm_fix_ratio(6, 100), gbm_fix_ratio(10, 100), 72U, 77U, 82U, 255U);
    gbm_add_box(m, gbm_fix_ratio(-24, 100), gbm_fix_ratio(-5, 100), gbm_fix_ratio(-26, 100), gbm_fix_ratio(-14, 100), gbm_fix_ratio(5, 100), gbm_fix_ratio(-6, 100), 72U, 77U, 82U, 255U);
    gbm_add_box(m, gbm_fix_ratio(14, 100), gbm_fix_ratio(-5, 100), gbm_fix_ratio(-26, 100), gbm_fix_ratio(24, 100), gbm_fix_ratio(5, 100), gbm_fix_ratio(-6, 100), 72U, 77U, 82U, 255U);
}

int gbm_build_mg_link(GBM_Mesh *m)
{
    if (m == 0) return GBM_ERR_NULL;
    gbm_mesh_reset(m);
    gbm_add_mg_link_raw(m);
    return m->error;
}

void gbm_mesh_translate(GBM_Mesh *m, gbm_fix tx, gbm_fix ty, gbm_fix tz)
{
    unsigned short i;
    if (m == 0 || m->v == 0) return;
    for (i = 0; i < m->v_count; ++i) {
        m->v[i].x += tx;
        m->v[i].y += ty;
        m->v[i].z += tz;
    }
}
