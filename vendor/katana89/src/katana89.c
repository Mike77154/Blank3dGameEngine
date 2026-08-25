#include "katana89.h"
#include <string.h>

#define KM89_TRIG_ONE 16384L
#define KM89_BLADE_STATIONS 25
#define KM89_RING8 8
#define KM89_RING16 16

static const short km89_cos16[16] = {
    16384,15137,11585,6270,0,-6270,-11585,-15137,
    -16384,-15137,-11585,-6270,0,6270,11585,15137
};
static const short km89_sin16[16] = {
    0,6270,11585,15137,16384,15137,11585,6270,
    0,-6270,-11585,-15137,-16384,-15137,-11585,-6270
};

static const km89_palette km89_palettes[] = {
    /* steel, edge, hamon, guard, wrap, same, fitting, lacquer, groove */
    {{{125,132,140},{220,225,230},{185,198,210},{42,44,48},{26,28,32},{202,196,176},{168,125,54},{26,27,31},{62,68,76}}},
    {{{132,138,145},{230,233,236},{197,205,214},{36,24,18},{126,24,22},{211,194,164},{177,132,55},{94,8,12},{54,62,70}}},
    {{{124,133,144},{226,232,238},{186,207,221},{30,34,40},{228,225,214},{60,62,66},{188,147,72},{220,218,207},{52,61,73}}},
    {{{120,132,142},{218,227,234},{171,203,214},{32,45,38},{18,62,48},{207,201,178},{176,137,60},{12,58,43},{49,65,68}}},
    {{{128,134,142},{223,229,234},{185,198,210},{59,50,35},{54,41,26},{205,193,164},{146,112,61},{90,66,36},{58,64,72}}},
    {{{122,130,139},{222,229,235},{180,199,212},{25,26,28},{20,21,23},{186,179,160},{128,128,134},{12,13,15},{50,56,63}}},
    {{{130,135,142},{228,231,235},{189,200,209},{73,42,19},{37,38,76},{202,197,178},{180,132,57},{27,28,64},{55,61,70}}},
    {{{127,136,145},{226,232,238},{191,209,220},{47,32,50},{74,23,70},{213,203,186},{178,128,61},{79,18,74},{55,63,72}}}
};

static const km89_desc km89_presets[] = {
    {"Edo Standard Maru",        KM89_CM(70),KM89_CM(26),KM89_CM(1),KM89_CM(3),KM89_CM(2),KM89_CM(0)+77,KM89_CM(4),KM89_CM(0)+128,KM89_BLADE_SHINOGI,KM89_KISSAKI_CHU,KM89_TSUBA_MARU,KM89_TSUKA_RIKKO,0,KM89_FLAG_HAMON|KM89_FLAG_MENUKI|KM89_FLAG_DOUBLE_WRAP},
    {"Kambun Shallow",           KM89_CM(71),KM89_CM(25),KM89_CM(0)+128,KM89_CM(3),KM89_CM(1)+205,KM89_CM(0)+72,KM89_CM(3)+205,KM89_CM(0)+115,KM89_BLADE_SHINOGI,KM89_KISSAKI_KO,KM89_TSUBA_NADEKAKU,KM89_TSUKA_STRAIGHT,5,KM89_FLAG_HAMON|KM89_FLAG_BOHI|KM89_FLAG_MENUKI},
    {"Kamakura Deep Sori",       KM89_CM(73),KM89_CM(27),KM89_CM(2)+179,KM89_CM(3)+51,KM89_CM(2),KM89_CM(0)+82,KM89_CM(4)+51,KM89_CM(0)+128,KM89_BLADE_SHINOGI,KM89_KISSAKI_KO,KM89_TSUBA_MOKKO,KM89_TSUKA_TACHI,4,KM89_FLAG_HAMON|KM89_FLAG_MENUKI|KM89_FLAG_DOUBLE_WRAP|KM89_FLAG_SAYA},
    {"Momoyama Broad",           KM89_CM(68),KM89_CM(27),KM89_CM(1)+128,KM89_CM(3)+179,KM89_CM(2)+51,KM89_CM(0)+92,KM89_CM(4)+77,KM89_CM(0)+141,KM89_BLADE_SHINOGI,KM89_KISSAKI_O,KM89_TSUBA_MOKKO,KM89_TSUKA_HA_AGARI,1,KM89_FLAG_HAMON|KM89_FLAG_BOHI|KM89_FLAG_MENUKI|KM89_FLAG_SUKASHI|KM89_FLAG_DOUBLE_WRAP},
    {"Long Kissaki Ronin",       KM89_CM(72),KM89_CM(28),KM89_CM(1)+51,KM89_CM(3)+26,KM89_CM(1)+230,KM89_CM(0)+79,KM89_CM(3)+230,KM89_CM(0)+115,KM89_BLADE_SHINOGI,KM89_KISSAKI_O,KM89_TSUBA_HACHI,KM89_TSUKA_STRAIGHT,5,KM89_FLAG_HAMON|KM89_FLAG_BOHI|KM89_FLAG_MENUKI},
    {"Compact Ko Kissaki",       KM89_CM(64),KM89_CM(24),KM89_CM(1),KM89_CM(2)+205,KM89_CM(1)+179,KM89_CM(0)+72,KM89_CM(3)+154,KM89_CM(0)+102,KM89_BLADE_SHINOGI,KM89_KISSAKI_KO,KM89_TSUBA_MARU,KM89_TSUKA_RIKKO,6,KM89_FLAG_HAMON|KM89_FLAG_MENUKI|KM89_FLAG_DOUBLE_WRAP},
    {"O Kissaki War Blade",      KM89_CM(75),KM89_CM(29),KM89_CM(1)+205,KM89_CM(3)+128,KM89_CM(2)+77,KM89_CM(0)+90,KM89_CM(4)+128,KM89_CM(0)+141,KM89_BLADE_SHINOGI,KM89_KISSAKI_O,KM89_TSUBA_CROSS,KM89_TSUKA_HA_AGARI,4,KM89_FLAG_HAMON|KM89_FLAG_BOHI|KM89_FLAG_MENUKI|KM89_FLAG_DOUBLE_WRAP},
    {"Hira Zukuri Minimal",      KM89_CM(65),KM89_CM(25),KM89_CM(1)+26,KM89_CM(3),KM89_CM(1)+230,KM89_CM(0)+64,KM89_CM(3)+179,KM89_CM(0)+102,KM89_BLADE_HIRA,KM89_KISSAKI_CHU,KM89_TSUBA_MARU,KM89_TSUKA_STRAIGHT,2,KM89_FLAG_HAMON|KM89_FLAG_MENUKI},
    {"Shobu Zukuri Field",       KM89_CM(69),KM89_CM(26),KM89_CM(1)+102,KM89_CM(3)+26,KM89_CM(1)+205,KM89_CM(0)+74,KM89_CM(3)+205,KM89_CM(0)+115,KM89_BLADE_SHOBU,KM89_KISSAKI_SHOBU,KM89_TSUBA_AOI,KM89_TSUKA_RIKKO,3,KM89_FLAG_HAMON|KM89_FLAG_BOHI|KM89_FLAG_MENUKI|KM89_FLAG_DOUBLE_WRAP},
    {"Unokubi Light",            KM89_CM(67),KM89_CM(25),KM89_CM(1)+77,KM89_CM(3),KM89_CM(1)+179,KM89_CM(0)+68,KM89_CM(3)+179,KM89_CM(0)+102,KM89_BLADE_UNOKUBI,KM89_KISSAKI_CHU,KM89_TSUBA_HACHI,KM89_TSUKA_HA_AGARI,0,KM89_FLAG_HAMON|KM89_FLAG_BOHI|KM89_FLAG_MENUKI},
    {"Kiriha Archaic",           KM89_CM(66),KM89_CM(24),KM89_CM(1)+154,KM89_CM(3)+26,KM89_CM(2),KM89_CM(0)+78,KM89_CM(3)+230,KM89_CM(0)+115,KM89_BLADE_KIRIHA,KM89_KISSAKI_CHU,KM89_TSUBA_MOKKO,KM89_TSUKA_TACHI,4,KM89_FLAG_HAMON|KM89_FLAG_MENUKI|KM89_FLAG_DOUBLE_WRAP},
    {"Kata Shinogi Asym",        KM89_CM(70),KM89_CM(26),KM89_CM(1)+51,KM89_CM(3)+26,KM89_CM(1)+230,KM89_CM(0)+80,KM89_CM(4),KM89_CM(0)+115,KM89_BLADE_KATASHINOGI,KM89_KISSAKI_CHU,KM89_TSUBA_NADEKAKU,KM89_TSUKA_RIKKO,7,KM89_FLAG_HAMON|KM89_FLAG_BOHI|KM89_FLAG_MENUKI|KM89_FLAG_SUKASHI},
    {"Handachi Black Gold",      KM89_CM(72),KM89_CM(28),KM89_CM(1)+179,KM89_CM(3)+51,KM89_CM(2),KM89_CM(0)+82,KM89_CM(4)+26,KM89_CM(0)+128,KM89_BLADE_SHINOGI,KM89_KISSAKI_CHU,KM89_TSUBA_MOKKO,KM89_TSUKA_TACHI,4,KM89_FLAG_HAMON|KM89_FLAG_BOHI|KM89_FLAG_MENUKI|KM89_FLAG_DOUBLE_WRAP|KM89_FLAG_SAYA},
    {"Red Lacquer Daimyo",       KM89_CM(70),KM89_CM(27),KM89_CM(1)+102,KM89_CM(3)+26,KM89_CM(2),KM89_CM(0)+80,KM89_CM(4)+51,KM89_CM(0)+128,KM89_BLADE_SHINOGI,KM89_KISSAKI_CHU,KM89_TSUBA_AOI,KM89_TSUKA_RIKKO,1,KM89_FLAG_HAMON|KM89_FLAG_MENUKI|KM89_FLAG_SUKASHI|KM89_FLAG_DOUBLE_WRAP|KM89_FLAG_SAYA},
    {"White Ito Court",          KM89_CM(69),KM89_CM(26),KM89_CM(1)+26,KM89_CM(3),KM89_CM(1)+230,KM89_CM(0)+76,KM89_CM(4),KM89_CM(0)+115,KM89_BLADE_SHINOGI,KM89_KISSAKI_KO,KM89_TSUBA_MARU,KM89_TSUKA_STRAIGHT,2,KM89_FLAG_HAMON|KM89_FLAG_MENUKI|KM89_FLAG_DOUBLE_WRAP|KM89_FLAG_SAYA},
    {"Indigo Night",             KM89_CM(71),KM89_CM(27),KM89_CM(1)+77,KM89_CM(3)+26,KM89_CM(2),KM89_CM(0)+79,KM89_CM(4)+26,KM89_CM(0)+115,KM89_BLADE_SHINOGI,KM89_KISSAKI_CHU,KM89_TSUBA_HACHI,KM89_TSUKA_HA_AGARI,6,KM89_FLAG_HAMON|KM89_FLAG_BOHI|KM89_FLAG_MENUKI|KM89_FLAG_DOUBLE_WRAP|KM89_FLAG_SAYA},
    {"Emerald Clan",             KM89_CM(70),KM89_CM(27),KM89_CM(1)+128,KM89_CM(3)+51,KM89_CM(2)+26,KM89_CM(0)+82,KM89_CM(4)+26,KM89_CM(0)+128,KM89_BLADE_SHINOGI,KM89_KISSAKI_O,KM89_TSUBA_MOKKO,KM89_TSUKA_RIKKO,3,KM89_FLAG_HAMON|KM89_FLAG_BOHI|KM89_FLAG_MENUKI|KM89_FLAG_SUKASHI|KM89_FLAG_DOUBLE_WRAP|KM89_FLAG_SAYA},
    {"Bone and Iron",            KM89_CM(68),KM89_CM(25),KM89_CM(1)+51,KM89_CM(3),KM89_CM(1)+205,KM89_CM(0)+74,KM89_CM(3)+205,KM89_CM(0)+115,KM89_BLADE_HIRA,KM89_KISSAKI_CHU,KM89_TSUBA_CROSS,KM89_TSUKA_STRAIGHT,2,KM89_FLAG_HAMON|KM89_FLAG_MENUKI},
    {"Ash Wanderer",             KM89_CM(73),KM89_CM(28),KM89_CM(1)+102,KM89_CM(3)+26,KM89_CM(1)+230,KM89_CM(0)+78,KM89_CM(3)+230,KM89_CM(0)+102,KM89_BLADE_SHOBU,KM89_KISSAKI_SHOBU,KM89_TSUBA_NONE,KM89_TSUKA_RIKKO,5,KM89_FLAG_HAMON|KM89_FLAG_BOHI|KM89_FLAG_MENUKI},
    {"Temple Bronze",            KM89_CM(67),KM89_CM(25),KM89_CM(1)+154,KM89_CM(3)+26,KM89_CM(2),KM89_CM(0)+81,KM89_CM(4)+51,KM89_CM(0)+141,KM89_BLADE_UNOKUBI,KM89_KISSAKI_CHU,KM89_TSUBA_AOI,KM89_TSUKA_TACHI,4,KM89_FLAG_HAMON|KM89_FLAG_BOHI|KM89_FLAG_MENUKI|KM89_FLAG_DOUBLE_WRAP},
    {"Floral Mokko",             KM89_CM(70),KM89_CM(26),KM89_CM(1)+26,KM89_CM(3),KM89_CM(2),KM89_CM(0)+77,KM89_CM(4)+77,KM89_CM(0)+128,KM89_BLADE_SHINOGI,KM89_KISSAKI_KO,KM89_TSUBA_MOKKO,KM89_TSUKA_RIKKO,7,KM89_FLAG_HAMON|KM89_FLAG_MENUKI|KM89_FLAG_SUKASHI|KM89_FLAG_DOUBLE_WRAP},
    {"Square Guard Duelist",     KM89_CM(72),KM89_CM(27),KM89_CM(0)+230,KM89_CM(2)+230,KM89_CM(1)+205,KM89_CM(0)+72,KM89_CM(4),KM89_CM(0)+115,KM89_BLADE_SHINOGI,KM89_KISSAKI_O,KM89_TSUBA_NADEKAKU,KM89_TSUKA_STRAIGHT,0,KM89_FLAG_HAMON|KM89_FLAG_BOHI|KM89_FLAG_MENUKI},
    {"Aoi Guard Deep Curve",     KM89_CM(71),KM89_CM(27),KM89_CM(2),KM89_CM(3)+26,KM89_CM(2),KM89_CM(0)+80,KM89_CM(4)+51,KM89_CM(0)+128,KM89_BLADE_SHINOGI,KM89_KISSAKI_CHU,KM89_TSUBA_AOI,KM89_TSUKA_TACHI,3,KM89_FLAG_HAMON|KM89_FLAG_MENUKI|KM89_FLAG_DOUBLE_WRAP|KM89_FLAG_SAYA},
    {"Octagonal Guard",          KM89_CM(69),KM89_CM(26),KM89_CM(1)+77,KM89_CM(3),KM89_CM(1)+230,KM89_CM(0)+76,KM89_CM(4),KM89_CM(0)+115,KM89_BLADE_SHINOGI,KM89_KISSAKI_KO,KM89_TSUBA_HACHI,KM89_TSUKA_HA_AGARI,6,KM89_FLAG_HAMON|KM89_FLAG_BOHI|KM89_FLAG_MENUKI|KM89_FLAG_DOUBLE_WRAP},
    {"Shirasaya Plain",          KM89_CM(70),KM89_CM(25),KM89_CM(1)+26,KM89_CM(3),KM89_CM(2),KM89_CM(0)+75,0,0,KM89_BLADE_SHINOGI,KM89_KISSAKI_CHU,KM89_TSUBA_NONE,KM89_TSUKA_STRAIGHT,4,KM89_FLAG_HAMON|KM89_FLAG_SAYA},
    {"Twin Detail Bohi",         KM89_CM(74),KM89_CM(28),KM89_CM(1)+102,KM89_CM(3)+51,KM89_CM(2),KM89_CM(0)+80,KM89_CM(4)+26,KM89_CM(0)+115,KM89_BLADE_SHINOGI,KM89_KISSAKI_O,KM89_TSUBA_MARU,KM89_TSUKA_RIKKO,0,KM89_FLAG_HAMON|KM89_FLAG_BOHI|KM89_FLAG_MENUKI|KM89_FLAG_DOUBLE_WRAP},
    {"Heavy Wide Cutter",        KM89_CM(68),KM89_CM(29),KM89_CM(1)+128,KM89_CM(4),KM89_CM(2)+128,KM89_CM(0)+98,KM89_CM(4)+128,KM89_CM(0)+154,KM89_BLADE_SHINOGI,KM89_KISSAKI_O,KM89_TSUBA_CROSS,KM89_TSUKA_HA_AGARI,1,KM89_FLAG_HAMON|KM89_FLAG_BOHI|KM89_FLAG_MENUKI|KM89_FLAG_DOUBLE_WRAP},
    {"Slim Fencing Katana",      KM89_CM(72),KM89_CM(25),KM89_CM(0)+179,KM89_CM(2)+179,KM89_CM(1)+154,KM89_CM(0)+64,KM89_CM(3)+154,KM89_CM(0)+102,KM89_BLADE_SHINOGI,KM89_KISSAKI_KO,KM89_TSUBA_NADEKAKU,KM89_TSUKA_STRAIGHT,5,KM89_FLAG_HAMON|KM89_FLAG_BOHI|KM89_FLAG_MENUKI},
    {"Long Reach 78",            KM89_CM(78),KM89_CM(30),KM89_CM(2),KM89_CM(3)+77,KM89_CM(2)+26,KM89_CM(0)+85,KM89_CM(4)+77,KM89_CM(0)+141,KM89_BLADE_SHINOGI,KM89_KISSAKI_O,KM89_TSUBA_MOKKO,KM89_TSUKA_TACHI,4,KM89_FLAG_HAMON|KM89_FLAG_BOHI|KM89_FLAG_MENUKI|KM89_FLAG_DOUBLE_WRAP|KM89_FLAG_SAYA},
    {"Short Katana 61",          KM89_CM(61),KM89_CM(23),KM89_CM(1),KM89_CM(2)+230,KM89_CM(1)+179,KM89_CM(0)+70,KM89_CM(3)+154,KM89_CM(0)+102,KM89_BLADE_SHINOGI,KM89_KISSAKI_KO,KM89_TSUBA_MARU,KM89_TSUKA_RIKKO,6,KM89_FLAG_HAMON|KM89_FLAG_MENUKI|KM89_FLAG_DOUBLE_WRAP},
    {"Shobu Long Field",         KM89_CM(75),KM89_CM(29),KM89_CM(1)+179,KM89_CM(3)+51,KM89_CM(2),KM89_CM(0)+78,KM89_CM(4)+26,KM89_CM(0)+128,KM89_BLADE_SHOBU,KM89_KISSAKI_SHOBU,KM89_TSUBA_AOI,KM89_TSUKA_HA_AGARI,3,KM89_FLAG_HAMON|KM89_FLAG_BOHI|KM89_FLAG_MENUKI|KM89_FLAG_DOUBLE_WRAP},
    {"Unokubi Mountain",         KM89_CM(72),KM89_CM(27),KM89_CM(2)+51,KM89_CM(3)+51,KM89_CM(2),KM89_CM(0)+76,KM89_CM(4)+51,KM89_CM(0)+128,KM89_BLADE_UNOKUBI,KM89_KISSAKI_CHU,KM89_TSUBA_HACHI,KM89_TSUKA_TACHI,0,KM89_FLAG_HAMON|KM89_FLAG_BOHI|KM89_FLAG_MENUKI|KM89_FLAG_DOUBLE_WRAP|KM89_FLAG_SAYA},
    {"Hira Crimson",             KM89_CM(66),KM89_CM(25),KM89_CM(1)+77,KM89_CM(3)+26,KM89_CM(2),KM89_CM(0)+70,KM89_CM(4),KM89_CM(0)+115,KM89_BLADE_HIRA,KM89_KISSAKI_CHU,KM89_TSUBA_MOKKO,KM89_TSUKA_RIKKO,1,KM89_FLAG_HAMON|KM89_FLAG_MENUKI|KM89_FLAG_DOUBLE_WRAP|KM89_FLAG_SAYA}
};


static const unsigned short km89_station_t[KM89_BLADE_STATIONS] = {
    0,10,20,30,40,52,64,76,88,100,112,124,136,
    148,160,172,184,194,204,214,224,234,242,249,256
};

static const signed char km89_hamon_wave[KM89_BLADE_STATIONS] = {
    0,2,-1,3,0,4,-2,2,-1,5,1,3,-2,4,0,3,-1,2,0,3,-1,2,0,1,0
};

static long km89_lerp(long a, long b, long t)
{
    return a + ((b - a) * t) / 256L;
}

static long km89_abs(long v)
{
    return v < 0 ? -v : v;
}

static long km89_mul_trig(long v, long trig)
{
    return (v * trig) / KM89_TRIG_ONE;
}

static long km89_smoothstep(long t)
{
    long t2;
    long t3;
    if (t <= 0L) return 0L;
    if (t >= 256L) return 256L;
    t2 = (t * t) / 256L;
    t3 = (t2 * t) / 256L;
    return 3L * t2 - 2L * t3;
}

static int km89_add_vertex(km89_mesh *mesh, long x, long y, long z)
{
    unsigned short i;
    if (mesh->vertex_count >= mesh->vertex_capacity) return KM89_ERR_VERTEX_CAP;
    i = mesh->vertex_count++;
    mesh->vertices[i].x = x;
    mesh->vertices[i].y = y;
    mesh->vertices[i].z = z;
    return (int)i;
}

static int km89_add_triangle(km89_mesh *mesh, int a, int b, int c, unsigned char material)
{
    unsigned short i;
    if (a < 0 || b < 0 || c < 0) return KM89_ERR_ARGUMENT;
    if (mesh->triangle_count >= mesh->triangle_capacity) return KM89_ERR_TRIANGLE_CAP;
    i = mesh->triangle_count++;
    mesh->triangles[i].a = (unsigned short)a;
    mesh->triangles[i].b = (unsigned short)b;
    mesh->triangles[i].c = (unsigned short)c;
    mesh->triangles[i].material = material;
    return KM89_OK;
}

static int km89_add_quad(km89_mesh *mesh, int a, int b, int c, int d, unsigned char material)
{
    int r;
    r = km89_add_triangle(mesh,a,b,c,material); if (r) return r;
    return km89_add_triangle(mesh,a,c,d,material);
}

static void km89_range_begin(km89_part_range *r, const km89_mesh *mesh)
{
    r->first_vertex = mesh->vertex_count;
    r->first_triangle = mesh->triangle_count;
    r->vertex_count = 0;
    r->triangle_count = 0;
}

static void km89_range_end(km89_part_range *r, const km89_mesh *mesh)
{
    r->vertex_count = (unsigned short)(mesh->vertex_count - r->first_vertex);
    r->triangle_count = (unsigned short)(mesh->triangle_count - r->first_triangle);
}

/* Sori is the maximum distance from the chord, expressed by a smooth arch. */
static long km89_curve_at(const km89_desc *d, long t)
{
    long p;
    long y;
    p = t * (256L - t);
    y = (4L * d->sori * p) / 65536L;
    if (d->tsuka_style == KM89_TSUKA_TACHI) {
        long skew;
        skew = (p * (144L - t)) / 4194304L;
        y += (d->sori * skew) / 64L;
    }
    return y;
}

/* Historical-looking kissaki lengths: short ko, medium chu, long o. */
static long km89_kissaki_start(unsigned char style)
{
    if (style == KM89_KISSAKI_KO) return 246L;
    if (style == KM89_KISSAKI_O) return 216L;
    if (style == KM89_KISSAKI_SHOBU) return 205L;
    return 236L;
}

static void km89_blade_edges(const km89_desc *d,
                             long t,
                             long *ha,
                             long *mune,
                             long *thick,
                             long *ridge_y)
{
    long curve;
    long taper_t;
    long width;
    long ks;
    long u;
    long s;
    long tip_y;
    long ridge_ratio;

    curve = km89_curve_at(d,t);
    taper_t = t + (t * (256L - t)) / 896L;
    if (taper_t > 256L) taper_t = 256L;
    width = km89_lerp(d->blade_width_base,d->blade_width_tip,taper_t);
    *thick = km89_lerp(d->blade_thickness,(d->blade_thickness*58L)/100L,t);

    /* Slightly more blade mass above the edge than below the spine chord. */
    *ha = curve - (width * 140L) / 256L;
    *mune = curve + (width * 116L) / 256L;

    if (d->blade_style == KM89_BLADE_UNOKUBI && t > 72L && t < 196L) {
        long depth;
        depth = 256L - km89_abs(134L - t) * 2L;
        if (depth < 184L) depth = 184L;
        *mune -= (width * (256L - depth)) / 640L;
    }

    ks = km89_kissaki_start(d->kissaki_style);
    if (t > ks) {
        u = ((t - ks) * 256L) / (256L - ks);
        s = km89_smoothstep(u);
        tip_y = km89_curve_at(d,256L) + d->blade_width_tip / 10L;
        *ha = km89_lerp(*ha,tip_y,s);
        *mune = km89_lerp(*mune,tip_y,(u+s)/2L);
        *thick = (*thick * (256L-s)) / 256L;
        if (*thick < 12L) *thick = 12L;
        if (t >= 256L) {
            *ha = tip_y - 4L;
            *mune = tip_y + 4L;
        }
    }

    if (d->blade_style == KM89_BLADE_HIRA) ridge_ratio = 184L;
    else if (d->blade_style == KM89_BLADE_KIRIHA) ridge_ratio = 96L;
    else if (d->blade_style == KM89_BLADE_SHOBU) ridge_ratio = 170L;
    else if (d->blade_style == KM89_BLADE_UNOKUBI) ridge_ratio = 166L;
    else ridge_ratio = 160L;
    *ridge_y = *ha + ((*mune - *ha) * ridge_ratio) / 256L;

    if (d->kissaki_style == KM89_KISSAKI_SHOBU && t > ks) {
        u = ((t - ks) * 256L) / (256L - ks);
        *ridge_y = km89_lerp(*ridge_y,(*ha+*mune)/2L,km89_smoothstep(u));
    }
}

static int km89_add_blade(const km89_desc *d, km89_mesh *m)
{
    int rings[KM89_BLADE_STATIONS][8];
    long ha_y[KM89_BLADE_STATIONS];
    long mune_y[KM89_BLADE_STATIONS];
    long ridge_y[KM89_BLADE_STATIONS];
    long thicks[KM89_BLADE_STATIONS];
    int i;
    int j;
    int r;

    for (i=0;i<KM89_BLADE_STATIONS;i++) {
        long t;
        long ha;
        long mune;
        long thick;
        long ridge;
        long hira_y;
        long z;
        long edge_x;
        long mune_x;
        long full_x;
        long ridge_x;
        t = (long)km89_station_t[i];
        km89_blade_edges(d,t,&ha,&mune,&thick,&ridge);
        ha_y[i]=ha;
        mune_y[i]=mune;
        ridge_y[i]=ridge;
        thicks[i]=thick;
        hira_y = ha + (ridge-ha)*44L/100L;
        z = (d->blade_length*t)/256L;
        edge_x = thick/18L; if (edge_x < 1L) edge_x=1L;
        mune_x = thick*32L/100L;
        full_x = thick/2L;
        if (mune_x <= edge_x) mune_x=edge_x+1L;
        if (full_x <= mune_x) full_x=mune_x+1L;
        ridge_x=full_x;
        if (d->blade_style == KM89_BLADE_HIRA) {
            ridge_x=full_x*72L/100L;
            if(ridge_x<=mune_x)ridge_x=mune_x+1L;
        }
        rings[i][0]=km89_add_vertex(m,-edge_x,ha,z);
        rings[i][1]=km89_add_vertex(m,-full_x,hira_y,z);
        rings[i][2]=km89_add_vertex(m,-ridge_x,ridge,z);
        rings[i][3]=km89_add_vertex(m,-mune_x,mune,z);
        rings[i][4]=km89_add_vertex(m,mune_x,mune,z);
        rings[i][5]=km89_add_vertex(m,ridge_x,ridge,z);
        rings[i][6]=km89_add_vertex(m,full_x,hira_y,z);
        rings[i][7]=km89_add_vertex(m,edge_x,ha,z);
        for (j=0;j<8;j++) if (rings[i][j] < 0) return rings[i][j];
    }

    for (i=0;i<KM89_BLADE_STATIONS-1;i++) {
        for (j=0;j<8;j++) {
            unsigned char mat;
            mat = (j==0 || j==6 || j==7) ? KM89_MAT_EDGE : KM89_MAT_STEEL;
            r=km89_add_quad(m,rings[i][j],rings[i+1][j],rings[i+1][(j+1)%8],rings[i][(j+1)%8],mat);
            if (r) return r;
        }
    }

    /* Base cap. The terminal ring is tiny enough to read as a polished point. */
    for (j=1;j<7;j++) {
        r=km89_add_triangle(m,rings[0][0],rings[0][j+1],rings[0][j],KM89_MAT_STEEL);
        if (r) return r;
    }
    for (j=1;j<7;j++) {
        r=km89_add_triangle(m,rings[KM89_BLADE_STATIONS-1][0],
                            rings[KM89_BLADE_STATIONS-1][j],
                            rings[KM89_BLADE_STATIONS-1][j+1],KM89_MAT_EDGE);
        if (r) return r;
    }

    /* Hamon as a subtle face ribbon following the ha, with deterministic waves. */
    if (d->flags & KM89_FLAG_HAMON) {
        int side;
        for (side=-1;side<=1;side+=2) {
            int prev0;
            int prev1;
            prev0=-1; prev1=-1;
            for (i=0;i<KM89_BLADE_STATIONS-1;i++) {
                long span;
                long y0;
                long y1;
                long x;
                long z;
                long wave;
                int a;
                int b;
                span=mune_y[i]-ha_y[i];
                wave=(long)km89_hamon_wave[i]*span/96L;
                y0=ha_y[i]+span*12L/100L+wave;
                y1=ha_y[i]+span*25L/100L+wave;
                x=side*(thicks[i]/2L+2L);
                z=(d->blade_length*(long)km89_station_t[i])/256L;
                a=km89_add_vertex(m,x,y0,z);
                b=km89_add_vertex(m,x,y1,z);
                if(a<0||b<0)return KM89_ERR_VERTEX_CAP;
                if(prev0>=0){r=km89_add_quad(m,prev0,a,b,prev1,KM89_MAT_HAMON);if(r)return r;}
                prev0=a;prev1=b;
            }
        }
    }

    /* Bohi cue: correctly starts after habaki and dies before the kissaki. */
    if (d->flags & KM89_FLAG_BOHI) {
        int side;
        long ks;
        ks=km89_kissaki_start(d->kissaki_style);
        for(side=-1;side<=1;side+=2) {
            int prev0;
            int prev1;
            prev0=-1;prev1=-1;
            for(i=2;i<KM89_BLADE_STATIONS-1;i++) {
                long t;
                long span;
                long y0;
                long y1;
                long x;
                long z;
                int a;
                int b;
                t=(long)km89_station_t[i];
                if(t>=ks-8L)break;
                span=mune_y[i]-ha_y[i];
                y0=ridge_y[i]+span*5L/100L;
                y1=ridge_y[i]+span*13L/100L;
                x=side*(thicks[i]/2L+3L);
                z=(d->blade_length*t)/256L;
                a=km89_add_vertex(m,x,y0,z);
                b=km89_add_vertex(m,x,y1,z);
                if(a<0||b<0)return KM89_ERR_VERTEX_CAP;
                if(prev0>=0){r=km89_add_quad(m,prev0,a,b,prev1,KM89_MAT_GROOVE);if(r)return r;}
                prev0=a;prev1=b;
            }
        }
    }

    /* Yokote as an oblique polished band, omitted on shobu-zukuri. */
    if (d->kissaki_style != KM89_KISSAKI_SHOBU) {
        long ks;
        long ha;
        long mune;
        long thick;
        long ridge;
        long z;
        int side;
        ks=km89_kissaki_start(d->kissaki_style);
        km89_blade_edges(d,ks,&ha,&mune,&thick,&ridge);
        z=(d->blade_length*ks)/256L;
        for(side=-1;side<=1;side+=2) {
            long x;
            int a;
            int b;
            int c;
            int e;
            x=side*(thick/2L+4L);
            a=km89_add_vertex(m,x,ha+(mune-ha)/16L,z-5L);
            b=km89_add_vertex(m,x,mune-(mune-ha)/12L,z+5L);
            c=km89_add_vertex(m,x,mune-(mune-ha)/12L,z+10L);
            e=km89_add_vertex(m,x,ha+(mune-ha)/16L,z);
            if(a<0||b<0||c<0||e<0)return KM89_ERR_VERTEX_CAP;
            r=km89_add_quad(m,a,b,c,e,KM89_MAT_EDGE);if(r)return r;
        }
    }
    return KM89_OK;
}

static void km89_tsuba_xy(unsigned char style, int i, long radius, long *x, long *y)
{
    long cx;
    long sy;
    long rx;
    long ry;
    long mod;
    cx=km89_cos16[i];sy=km89_sin16[i];rx=radius;ry=radius;
    if(style==KM89_TSUBA_NADEKAKU){
        static const short sx[16]={16384,16384,13926,8192,0,-8192,-13926,-16384,-16384,-16384,-13926,-8192,0,8192,13926,16384};
        static const short syq[16]={0,8192,13926,16384,16384,16384,13926,8192,0,-8192,-13926,-16384,-16384,-16384,-13926,-8192};
        *x=km89_mul_trig(radius,sx[i]);*y=km89_mul_trig(radius,syq[i]);return;
    }
    if(style==KM89_TSUBA_MOKKO){mod=(i%4==0)?256L:((i%2)==0?236L:222L);rx=radius*mod/256L;ry=rx;}
    else if(style==KM89_TSUBA_AOI){mod=(i%4==0)?256L:((i%2)==0?216L:198L);rx=radius*mod/256L;ry=rx;}
    else if(style==KM89_TSUBA_HACHI){mod=((i%2)==0)?250L:235L;rx=radius*mod/256L;ry=rx;}
    else if(style==KM89_TSUBA_CROSS){mod=(i%4==0)?256L:((i%2)==0?218L:188L);rx=radius*mod/256L;ry=rx;}
    *x=km89_mul_trig(rx,cx);*y=km89_mul_trig(ry,sy);
}

static int km89_add_tsuba(const km89_desc *d, km89_mesh *m)
{
    int ot[16];
    int ob[16];
    int it[16];
    int ib[16];
    int i;
    int r;
    long z0;
    long z1;
    long hole_x;
    long hole_y;
    if(d->tsuba_style==KM89_TSUBA_NONE||d->guard_radius<=0)return KM89_OK;
    z0=-d->guard_thickness/2L;z1=d->guard_thickness/2L;
    hole_x=KM89_CM(0)+115;
    hole_y=KM89_CM(1)+102;
    for(i=0;i<16;i++){
        long x;
        long y;
        long ix;
        long iy;
        km89_tsuba_xy(d->tsuba_style,i,d->guard_radius,&x,&y);
        ix=km89_mul_trig(hole_x,km89_cos16[i]);
        iy=km89_mul_trig(hole_y,km89_sin16[i]);
        ot[i]=km89_add_vertex(m,x,y,z1);
        ob[i]=km89_add_vertex(m,x,y,z0);
        it[i]=km89_add_vertex(m,ix,iy,z1);
        ib[i]=km89_add_vertex(m,ix,iy,z0);
        if(ot[i]<0||ob[i]<0||it[i]<0||ib[i]<0)return KM89_ERR_VERTEX_CAP;
    }
    for(i=0;i<16;i++){
        int n;
        n=(i+1)%16;
        r=km89_add_quad(m,ot[i],ot[n],it[n],it[i],KM89_MAT_GUARD);if(r)return r;
        r=km89_add_quad(m,ob[i],ib[i],ib[n],ob[n],KM89_MAT_GUARD);if(r)return r;
        r=km89_add_quad(m,ob[i],ob[n],ot[n],ot[i],KM89_MAT_GUARD);if(r)return r;
        r=km89_add_quad(m,ib[i],it[i],it[n],ib[n],KM89_MAT_GROOVE);if(r)return r;
    }
    if(d->flags&KM89_FLAG_SUKASHI){
        for(i=0;i<4;i++){
            long px;
            long py;
            long s;
            int a;
            int b;
            int c;
            int e;
            px=km89_mul_trig(d->guard_radius*58L/100L,km89_cos16[i*4+2]);
            py=km89_mul_trig(d->guard_radius*58L/100L,km89_sin16[i*4+2]);
            s=d->guard_radius/10L;
            a=km89_add_vertex(m,px-s,py,z1+2L);
            b=km89_add_vertex(m,px,py+s,z1+2L);
            c=km89_add_vertex(m,px+s,py,z1+2L);
            e=km89_add_vertex(m,px,py-s,z1+2L);
            if(a<0||b<0||c<0||e<0)return KM89_ERR_VERTEX_CAP;
            r=km89_add_quad(m,a,b,c,e,KM89_MAT_GROOVE);if(r)return r;
        }
    }
    return KM89_OK;
}

static long km89_tsuka_curve(unsigned char style,long t)
{
    if(style==KM89_TSUKA_TACHI)return -(t*(256L-t))/4608L;
    if(style==KM89_TSUKA_HA_AGARI)return -(t*t)/10240L;
    return 0L;
}

static int km89_add_tsuka_ring(km89_mesh *m,int out[8],long rx,long ry,long z,long yoff)
{
    static const short px[8]={10650,16384,16384,10650,-10650,-16384,-16384,-10650};
    static const short py[8]={-16384,-10650,10650,16384,16384,10650,-10650,-16384};
    int i;
    for(i=0;i<8;i++){
        out[i]=km89_add_vertex(m,km89_mul_trig(rx,px[i]),yoff+km89_mul_trig(ry,py[i]),z);
        if(out[i]<0)return out[i];
    }
    return KM89_OK;
}

static int km89_join_ring8(km89_mesh *m,int a[8],int b[8],unsigned char mat)
{
    int i;
    int r;
    for(i=0;i<8;i++){r=km89_add_quad(m,a[i],b[i],b[(i+1)%8],a[(i+1)%8],mat);if(r)return r;}
    return KM89_OK;
}

static int km89_add_handle(const km89_desc *d,km89_mesh *m)
{
    int prev[8];
    int cur[8];
    int i;
    int r;
    int stations;
    long rx0;
    long ry0;
    stations=9;
    rx0=KM89_CM(1)+5L;
    ry0=KM89_CM(1)+128L;
    r=km89_add_tsuka_ring(m,prev,rx0,ry0,-d->guard_thickness/2L,0L);if(r)return r;
    for(i=1;i<stations;i++){
        long t;
        long z;
        long rx;
        long ry;
        long yoff;
        t=(long)i*256L/(stations-1);
        z=-(d->handle_length*t)/256L-d->guard_thickness/2L;
        rx=km89_lerp(rx0,rx0*90L/100L,t);
        ry=km89_lerp(ry0,ry0*91L/100L,t);
        if(d->tsuka_style==KM89_TSUKA_RIKKO){
            long waist;
            waist=km89_abs(128L-t);
            rx=rx*(236L+waist/7L)/256L;
            ry=ry*(236L+waist/7L)/256L;
        }
        yoff=km89_tsuka_curve(d->tsuka_style,t);
        r=km89_add_tsuka_ring(m,cur,rx,ry,z,yoff);if(r)return r;
        r=km89_join_ring8(m,prev,cur,KM89_MAT_SAME);if(r)return r;
        memcpy(prev,cur,sizeof(prev));
    }

    /* True crossing tsukamaki ribbons rather than one zig-zag line. */
    {
        int wraps;
        wraps=(d->flags&KM89_FLAG_DOUBLE_WRAP)?11:9;
        for(i=0;i<wraps;i++){
            long ta;
            long tb;
            long za;
            long zb;
            long ya;
            long yb;
            long half;
            long ribbon;
            int side;
            ta=(long)i*256L/wraps;
            tb=(long)(i+1)*256L/wraps;
            za=-(d->handle_length*ta)/256L-d->guard_thickness/2L;
            zb=-(d->handle_length*tb)/256L-d->guard_thickness/2L;
            ya=km89_tsuka_curve(d->tsuka_style,ta);
            yb=km89_tsuka_curve(d->tsuka_style,tb);
            half=ry0*53L/100L;
            ribbon=KM89_CM(0)+20L;
            for(side=-1;side<=1;side+=2){
                long x;
                int a;
                int b;
                int c;
                int e;
                int f;
                int g;
                int h;
                int k;
                x=side*(rx0+5L);
                a=km89_add_vertex(m,x,ya-half,za);
                b=km89_add_vertex(m,x,ya-half+ribbon,za);
                c=km89_add_vertex(m,x,yb+half+ribbon,zb);
                e=km89_add_vertex(m,x,yb+half,zb);
                f=km89_add_vertex(m,x,ya+half-ribbon,za);
                g=km89_add_vertex(m,x,ya+half,za);
                h=km89_add_vertex(m,x,yb-half,zb);
                k=km89_add_vertex(m,x,yb-half+ribbon,zb);
                if(a<0||b<0||c<0||e<0||f<0||g<0||h<0||k<0)return KM89_ERR_VERTEX_CAP;
                r=km89_add_quad(m,a,b,c,e,KM89_MAT_WRAP);if(r)return r;
                r=km89_add_quad(m,f,g,h,k,KM89_MAT_WRAP);if(r)return r;
            }
        }
    }

    if(d->flags&KM89_FLAG_MENUKI){
        int side;
        for(side=-1;side<=1;side+=2){
            long zc;
            long x;
            long sy;
            long sz;
            int a;
            int b;
            int c;
            int e;
            zc=-d->handle_length*58L/100L-d->guard_thickness/2L;
            x=side*(rx0+8L);
            sy=KM89_CM(0)+68L;
            sz=KM89_CM(1)+26L;
            a=km89_add_vertex(m,x,-sy,zc-sz);
            b=km89_add_vertex(m,x,sy,zc);
            c=km89_add_vertex(m,x,-sy,zc+sz);
            e=km89_add_vertex(m,x,-sy/2L,zc);
            if(a<0||b<0||c<0||e<0)return KM89_ERR_VERTEX_CAP;
            r=km89_add_quad(m,a,b,c,e,KM89_MAT_FITTING);if(r)return r;
        }
    }
    return KM89_OK;
}

static int km89_add_collar(km89_mesh *m,long rx,long ry,long z0,long z1,long yoff,unsigned char mat)
{
    int a[8];
    int b[8];
    int r;
    r=km89_add_tsuka_ring(m,a,rx,ry,z0,yoff);if(r)return r;
    r=km89_add_tsuka_ring(m,b,rx,ry,z1,yoff);if(r)return r;
    return km89_join_ring8(m,a,b,mat);
}

static int km89_add_fittings(const km89_desc *d,km89_mesh *m)
{
    int r;
    long base_rx;
    long base_ry;
    base_rx=KM89_CM(1)+20L;
    base_ry=KM89_CM(1)+148L;
    /* Habaki: tapered, compact collar instead of a rectangular brick. */
    {
        int a[8];
        int b[8];
        r=km89_add_tsuka_ring(m,a,d->blade_thickness*72L/100L,d->blade_width_base*56L/100L,0L,0L);if(r)return r;
        r=km89_add_tsuka_ring(m,b,d->blade_thickness*62L/100L,d->blade_width_base*51L/100L,KM89_CM(2)+64L,0L);if(r)return r;
        r=km89_join_ring8(m,a,b,KM89_MAT_FITTING);if(r)return r;
    }
    /* Seppa on both sides of the tsuba. */
    r=km89_add_collar(m,base_rx+18L,base_ry+18L,d->guard_thickness/2L,d->guard_thickness/2L+18L,0L,KM89_MAT_FITTING);if(r)return r;
    r=km89_add_collar(m,base_rx+18L,base_ry+18L,-d->guard_thickness/2L-18L,-d->guard_thickness/2L,0L,KM89_MAT_FITTING);if(r)return r;
    /* Fuchi and kashira follow the handle cross-section. */
    r=km89_add_collar(m,base_rx,base_ry,-KM89_CM(1)-d->guard_thickness/2L,-d->guard_thickness/2L,0L,KM89_MAT_FITTING);if(r)return r;
    r=km89_add_collar(m,base_rx*92L/100L,base_ry*92L/100L,-d->handle_length-d->guard_thickness/2L-KM89_CM(1),-d->handle_length-d->guard_thickness/2L,km89_tsuka_curve(d->tsuka_style,256L),KM89_MAT_FITTING);if(r)return r;
    return KM89_OK;
}

static int km89_add_saya_ring(km89_mesh *m,int out[8],long offset,long rx,long ry,long z,long yoff)
{
    static const short px[8]={10650,16384,16384,10650,-10650,-16384,-16384,-10650};
    static const short py[8]={-16384,-10650,10650,16384,16384,10650,-10650,-16384};
    int i;
    for(i=0;i<8;i++){
        out[i]=km89_add_vertex(m,offset+km89_mul_trig(rx,px[i]),yoff+km89_mul_trig(ry,py[i]),z);
        if(out[i]<0)return out[i];
    }
    return KM89_OK;
}

static int km89_add_saya_collar(km89_mesh *m,long offset,long rx,long ry,long z0,long z1,long yoff,unsigned char mat)
{
    int a[8];
    int b[8];
    int r;
    r=km89_add_saya_ring(m,a,offset,rx,ry,z0,yoff);if(r)return r;
    r=km89_add_saya_ring(m,b,offset,rx,ry,z1,yoff);if(r)return r;
    return km89_join_ring8(m,a,b,mat);
}

static int km89_add_saya(const km89_desc *d,km89_mesh *m)
{
    int prev[8];
    int cur[8];
    int i;
    int r;
    long offset;
    long extra;
    int stations;
    offset=KM89_CM(6);
    extra=KM89_CM(3);
    stations=18;
    {
        long ha;
        long mune;
        long th;
        long ridge;
        long c;
        long w;
        km89_blade_edges(d,0L,&ha,&mune,&th,&ridge);
        c=(ha+mune)/2L;w=mune-ha;
        r=km89_add_saya_ring(m,prev,offset,th+KM89_CM(0)+102,w/2L+KM89_CM(0)+90L,-KM89_CM(1),c);if(r)return r;
    }
    for(i=1;i<stations;i++){
        long t;
        long z;
        long ha;
        long mune;
        long th;
        long ridge;
        long c;
        long w;
        long rx;
        long ry;
        t=(long)i*256L/(stations-1);
        km89_blade_edges(d,t,&ha,&mune,&th,&ridge);
        c=(ha+mune)/2L;w=mune-ha;
        z=((d->blade_length+extra)*t)/256L-KM89_CM(1);
        rx=th+KM89_CM(0)+102L;
        ry=w/2L+KM89_CM(0)+90L;
        if(i==stations-1){rx=KM89_CM(0)+90L;ry=KM89_CM(0)+115L;}
        r=km89_add_saya_ring(m,cur,offset,rx,ry,z,c);if(r)return r;
        r=km89_join_ring8(m,prev,cur,KM89_MAT_LACQUER);if(r)return r;
        memcpy(prev,cur,sizeof(prev));
    }
    r=km89_add_saya_collar(m,offset,KM89_CM(1)+51L,KM89_CM(2),-KM89_CM(1)-50L,-KM89_CM(1),0L,KM89_MAT_FITTING);if(r)return r;
    return KM89_OK;
}

void km89_mesh_init(km89_mesh *mesh,
                    km89_vertex *vertices,
                    unsigned short vertex_capacity,
                    km89_triangle *triangles,
                    unsigned short triangle_capacity)
{
    if(!mesh)return;
    mesh->vertices=vertices;mesh->triangles=triangles;
    mesh->vertex_capacity=vertex_capacity;mesh->triangle_capacity=triangle_capacity;
    mesh->vertex_count=0;mesh->triangle_count=0;
}

void km89_mesh_reset(km89_mesh *mesh)
{
    if(!mesh)return;
    mesh->vertex_count=0;mesh->triangle_count=0;
}

int km89_build(const km89_desc *desc,km89_mesh *mesh,km89_build_info *info)
{
    int r;
    km89_build_info local_info;
    if(!desc||!mesh||!mesh->vertices||!mesh->triangles)return KM89_ERR_ARGUMENT;
    if(!info)info=&local_info;
    km89_mesh_reset(mesh);memset(info,0,sizeof(*info));
    km89_range_begin(&info->blade,mesh);r=km89_add_blade(desc,mesh);if(r)return r;km89_range_end(&info->blade,mesh);
    km89_range_begin(&info->guard,mesh);r=km89_add_tsuba(desc,mesh);if(r)return r;km89_range_end(&info->guard,mesh);
    km89_range_begin(&info->handle,mesh);r=km89_add_handle(desc,mesh);if(r)return r;km89_range_end(&info->handle,mesh);
    km89_range_begin(&info->fittings,mesh);r=km89_add_fittings(desc,mesh);if(r)return r;km89_range_end(&info->fittings,mesh);
    km89_range_begin(&info->saya,mesh);if(desc->flags&KM89_FLAG_SAYA){r=km89_add_saya(desc,mesh);if(r)return r;}km89_range_end(&info->saya,mesh);
    return KM89_OK;
}

int km89_build_preset(unsigned short preset_index,km89_mesh *mesh,km89_build_info *info)
{
    if(preset_index>=km89_preset_count())return KM89_ERR_PRESET;
    return km89_build(&km89_presets[preset_index],mesh,info);
}

unsigned short km89_preset_count(void)
{
    return (unsigned short)(sizeof(km89_presets)/sizeof(km89_presets[0]));
}

const km89_desc *km89_preset(unsigned short preset_index)
{
    if(preset_index>=km89_preset_count())return 0;
    return &km89_presets[preset_index];
}

const char *km89_preset_name(unsigned short preset_index)
{
    const km89_desc *p;
    p=km89_preset(preset_index);
    return p?p->name:0;
}

const km89_palette *km89_palette_get(unsigned char palette_id)
{
    if(palette_id>=km89_palette_count())palette_id=0;
    return &km89_palettes[palette_id];
}

unsigned char km89_palette_count(void)
{
    return (unsigned char)(sizeof(km89_palettes)/sizeof(km89_palettes[0]));
}

long km89_from_millimetres(long mm)
{
    return (mm*KM89_FP_ONE)/10L;
}

long km89_to_millimetres(long fp_value)
{
    return (fp_value*10L)/KM89_FP_ONE;
}
