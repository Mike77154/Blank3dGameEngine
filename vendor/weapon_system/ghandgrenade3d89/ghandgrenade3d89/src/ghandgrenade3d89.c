#include "ghandgrenade3d89.h"

#define GHG_PI_INDEX 128
#define GHG_MAX_PROFILE 32
#define GHG_NORM_ONE 16384L

#define GHG_SHAPE_STICK_CYL 0
#define GHG_SHAPE_STICK_BALL 1
#define GHG_SHAPE_STICK_BOX 2
#define GHG_SHAPE_STICK_CONE 3
#define GHG_SHAPE_BALL 4
#define GHG_SHAPE_TRUE_BALL 5
#define GHG_SHAPE_SEG_BALL 6
#define GHG_SHAPE_SEG_OVOID 7
#define GHG_SHAPE_LEMON 8
#define GHG_SHAPE_ALMOND 9
#define GHG_SHAPE_EGG 10
#define GHG_SHAPE_FLAT_OVAL 11
#define GHG_SHAPE_CAPSULE 12
#define GHG_SHAPE_PEAR 13
#define GHG_SHAPE_GOURD 14
#define GHG_SHAPE_FOOTBALL 15
#define GHG_SHAPE_RIBBED_OVOID 16
#define GHG_SHAPE_CYLINDER 17
#define GHG_SHAPE_DOUBLE_CYL 18
#define GHG_SHAPE_RIBBED_CYL 19
#define GHG_SHAPE_TAPERED_CYL 20
#define GHG_SHAPE_BARREL 21
#define GHG_SHAPE_CONE_SEG 22
#define GHG_SHAPE_BOTTLE 23
#define GHG_SHAPE_BELL 24
#define GHG_SHAPE_DISC 25
#define GHG_SHAPE_BOX 26
#define GHG_SHAPE_HEX 27
#define GHG_SHAPE_DOME 28

#define GHG_HOLDER_SPOON_LONG 0
#define GHG_HOLDER_SPOON_SHORT 1
#define GHG_HOLDER_TOP_BRIDGE 2
#define GHG_HOLDER_KNOB 3
#define GHG_HOLDER_STICK_COLLAR 4
#define GHG_HOLDER_STRAP 5

#define GHG_SAFE_PIN_RING 0
#define GHG_SAFE_SMALL_RING 1
#define GHG_SAFE_BOTTOM_RING 2
#define GHG_SAFE_WIRE_LOOP 3
#define GHG_SAFE_NONE 4

typedef struct GHG_ProfilePoint {
    ghg3d_fx y;
    ghg3d_fx r;
} GHG_ProfilePoint;

typedef struct GHG_PresetSpec {
    const char *name;
    const char *reference;
    int family;
    int shape;
    short height_mm;
    short width_mm;
    unsigned char holder;
    unsigned char safety;
    unsigned char game_segments;
    unsigned char rows;
    unsigned char cols;
} GHG_PresetSpec;

typedef struct GHG_Ctx {
    GHG3D_Vertex *v;
    GHG3D_Triangle *t;
    unsigned long vcap;
    unsigned long tcap;
    unsigned long vc;
    unsigned long tc;
    int overflow;
    ghg3d_fx scale;
    GHG3D_Result *result;
} GHG_Ctx;

static const long ghg_sin_q14[256] = {
    0L, 402L, 804L, 1205L, 1606L, 2006L, 2404L, 2801L,
    3196L, 3590L, 3981L, 4370L, 4756L, 5139L, 5520L, 5897L,
    6270L, 6639L, 7005L, 7366L, 7723L, 8076L, 8423L, 8765L,
    9102L, 9434L, 9760L, 10080L, 10394L, 10702L, 11003L, 11297L,
    11585L, 11866L, 12140L, 12406L, 12665L, 12916L, 13160L, 13395L,
    13623L, 13842L, 14053L, 14256L, 14449L, 14635L, 14811L, 14978L,
    15137L, 15286L, 15426L, 15557L, 15679L, 15791L, 15893L, 15986L,
    16069L, 16143L, 16207L, 16261L, 16305L, 16340L, 16364L, 16379L,
    16384L, 16379L, 16364L, 16340L, 16305L, 16261L, 16207L, 16143L,
    16069L, 15986L, 15893L, 15791L, 15679L, 15557L, 15426L, 15286L,
    15137L, 14978L, 14811L, 14635L, 14449L, 14256L, 14053L, 13842L,
    13623L, 13395L, 13160L, 12916L, 12665L, 12406L, 12140L, 11866L,
    11585L, 11297L, 11003L, 10702L, 10394L, 10080L, 9760L, 9434L,
    9102L, 8765L, 8423L, 8076L, 7723L, 7366L, 7005L, 6639L,
    6270L, 5897L, 5520L, 5139L, 4756L, 4370L, 3981L, 3590L,
    3196L, 2801L, 2404L, 2006L, 1606L, 1205L, 804L, 402L,
    0L, -402L, -804L, -1205L, -1606L, -2006L, -2404L, -2801L,
    -3196L, -3590L, -3981L, -4370L, -4756L, -5139L, -5520L, -5897L,
    -6270L, -6639L, -7005L, -7366L, -7723L, -8076L, -8423L, -8765L,
    -9102L, -9434L, -9760L, -10080L, -10394L, -10702L, -11003L, -11297L,
    -11585L, -11866L, -12140L, -12406L, -12665L, -12916L, -13160L, -13395L,
    -13623L, -13842L, -14053L, -14256L, -14449L, -14635L, -14811L, -14978L,
    -15137L, -15286L, -15426L, -15557L, -15679L, -15791L, -15893L, -15986L,
    -16069L, -16143L, -16207L, -16261L, -16305L, -16340L, -16364L, -16379L,
    -16384L, -16379L, -16364L, -16340L, -16305L, -16261L, -16207L, -16143L,
    -16069L, -15986L, -15893L, -15791L, -15679L, -15557L, -15426L, -15286L,
    -15137L, -14978L, -14811L, -14635L, -14449L, -14256L, -14053L, -13842L,
    -13623L, -13395L, -13160L, -12916L, -12665L, -12406L, -12140L, -11866L,
    -11585L, -11297L, -11003L, -10702L, -10394L, -10080L, -9760L, -9434L,
    -9102L, -8765L, -8423L, -8076L, -7723L, -7366L, -7005L, -6639L,
    -6270L, -5897L, -5520L, -5139L, -4756L, -4370L, -3981L, -3590L,
    -3196L, -2801L, -2404L, -2006L, -1606L, -1205L, -804L, -402L
};

static const GHG_PresetSpec ghg_specs[GHG3D_PRESET_COUNT] = {
    {"stick_classic", "Stielhandgranate-style cylindrical head", GHG3D_FAMILY_STICK, GHG_SHAPE_STICK_CYL, 350, 65, GHG_HOLDER_STICK_COLLAR, GHG_SAFE_BOTTOM_RING, 12, 0, 0},
    {"stick_long", "long stick silhouette", GHG3D_FAMILY_STICK, GHG_SHAPE_STICK_CYL, 390, 70, GHG_HOLDER_STICK_COLLAR, GHG_SAFE_BOTTOM_RING, 12, 0, 0},
    {"stick_short", "compact stick silhouette", GHG3D_FAMILY_STICK, GHG_SHAPE_STICK_CYL, 270, 72, GHG_HOLDER_STICK_COLLAR, GHG_SAFE_BOTTOM_RING, 12, 0, 0},
    {"stick_ball_head", "historic ball-headed stick silhouette", GHG3D_FAMILY_STICK, GHG_SHAPE_STICK_BALL, 320, 78, GHG_HOLDER_STICK_COLLAR, GHG_SAFE_BOTTOM_RING, 12, 0, 0},
    {"stick_box_head", "historic box-headed stick silhouette", GHG3D_FAMILY_STICK, GHG_SHAPE_STICK_BOX, 330, 82, GHG_HOLDER_STICK_COLLAR, GHG_SAFE_BOTTOM_RING, 8, 0, 0},
    {"stick_cone_head", "conical stick silhouette", GHG3D_FAMILY_STICK, GHG_SHAPE_STICK_CONE, 325, 70, GHG_HOLDER_STICK_COLLAR, GHG_SAFE_BOTTOM_RING, 12, 0, 0},
    {"ball_classic", "smooth spherical hand-grenade silhouette", GHG3D_FAMILY_BALL, GHG_SHAPE_BALL, 92, 70, GHG_HOLDER_SPOON_LONG, GHG_SAFE_PIN_RING, 16, 0, 0},
    {"ball_mini", "V-40-scale mini sphere silhouette", GHG3D_FAMILY_BALL, GHG_SHAPE_BALL, 54, 38, GHG_HOLDER_SPOON_SHORT, GHG_SAFE_SMALL_RING, 12, 0, 0},
    {"ball_true_sphere", "extra-round near-spherical ball silhouette", GHG3D_FAMILY_BALL, GHG_SHAPE_TRUE_BALL, 78, 76, GHG_HOLDER_SPOON_SHORT, GHG_SAFE_PIN_RING, 18, 0, 0},
    {"ball_necked", "spherical body with pronounced neck", GHG3D_FAMILY_BALL, GHG_SHAPE_BALL, 105, 76, GHG_HOLDER_TOP_BRIDGE, GHG_SAFE_PIN_RING, 16, 0, 0},
    {"ball_handle", "large ball with short handle", GHG3D_FAMILY_HYBRID, GHG_SHAPE_STICK_BALL, 240, 115, GHG_HOLDER_STRAP, GHG_SAFE_PIN_RING, 16, 0, 0},
    {"ball_segmented", "segmented spherical shell", GHG3D_FAMILY_SEGMENTED, GHG_SHAPE_SEG_BALL, 95, 76, GHG_HOLDER_SPOON_LONG, GHG_SAFE_PIN_RING, 16, 5, 8},
    {"pineapple_classic", "Mills/Mk-II family silhouette", GHG3D_FAMILY_SEGMENTED, GHG_SHAPE_SEG_OVOID, 102, 64, GHG_HOLDER_SPOON_LONG, GHG_SAFE_PIN_RING, 16, 6, 8},
    {"pineapple_tall", "tall segmented ovoid", GHG3D_FAMILY_SEGMENTED, GHG_SHAPE_SEG_OVOID, 125, 60, GHG_HOLDER_SPOON_LONG, GHG_SAFE_PIN_RING, 16, 7, 8},
    {"pineapple_squat", "short wide segmented ovoid", GHG3D_FAMILY_SEGMENTED, GHG_SHAPE_SEG_OVOID, 88, 75, GHG_HOLDER_SPOON_SHORT, GHG_SAFE_PIN_RING, 16, 5, 8},
    {"pineapple_round", "rounded segmented body", GHG3D_FAMILY_SEGMENTED, GHG_SHAPE_SEG_BALL, 105, 82, GHG_HOLDER_SPOON_LONG, GHG_SAFE_PIN_RING, 16, 6, 10},
    {"egg_segmented", "slender segmented egg", GHG3D_FAMILY_SEGMENTED, GHG_SHAPE_SEG_OVOID, 118, 55, GHG_HOLDER_TOP_BRIDGE, GHG_SAFE_SMALL_RING, 16, 7, 8},
    {"lemon_classic", "M26-like smooth lemon silhouette", GHG3D_FAMILY_OVOID, GHG_SHAPE_LEMON, 105, 62, GHG_HOLDER_SPOON_LONG, GHG_SAFE_PIN_RING, 16, 0, 0},
    {"almond_tall", "almond/teardrop body", GHG3D_FAMILY_OVOID, GHG_SHAPE_ALMOND, 125, 58, GHG_HOLDER_SPOON_LONG, GHG_SAFE_PIN_RING, 16, 0, 0},
    {"egg_small", "No.34-style compact egg silhouette", GHG3D_FAMILY_OVOID, GHG_SHAPE_EGG, 96, 40, GHG_HOLDER_KNOB, GHG_SAFE_SMALL_RING, 14, 0, 0},
    {"egg_long", "Eierhandgranate-style long egg silhouette", GHG3D_FAMILY_OVOID, GHG_SHAPE_EGG, 135, 50, GHG_HOLDER_KNOB, GHG_SAFE_WIRE_LOOP, 16, 0, 0},
    {"oval_flat", "flattened ball / oblate ovoid", GHG3D_FAMILY_OVOID, GHG_SHAPE_FLAT_OVAL, 112, 74, GHG_HOLDER_SPOON_LONG, GHG_SAFE_PIN_RING, 16, 0, 0},
    {"capsule_oval", "smooth capsule body", GHG3D_FAMILY_OVOID, GHG_SHAPE_CAPSULE, 120, 58, GHG_HOLDER_SPOON_LONG, GHG_SAFE_PIN_RING, 16, 0, 0},
    {"pear", "pear-shaped body", GHG3D_FAMILY_OVOID, GHG_SHAPE_PEAR, 130, 70, GHG_HOLDER_SPOON_LONG, GHG_SAFE_PIN_RING, 16, 0, 0},
    {"gourd", "waisted gourd body", GHG3D_FAMILY_HYBRID, GHG_SHAPE_GOURD, 138, 74, GHG_HOLDER_TOP_BRIDGE, GHG_SAFE_PIN_RING, 16, 0, 0},
    {"football", "pointed football/torpedo body", GHG3D_FAMILY_OVOID, GHG_SHAPE_FOOTBALL, 145, 64, GHG_HOLDER_TOP_BRIDGE, GHG_SAFE_PIN_RING, 16, 0, 0},
    {"ovoid_ribbed", "longitudinally ribbed ovoid", GHG3D_FAMILY_SEGMENTED, GHG_SHAPE_RIBBED_OVOID, 118, 65, GHG_HOLDER_SPOON_LONG, GHG_SAFE_PIN_RING, 16, 0, 10},
    {"cylinder_tall", "tall smoke/incendiary can silhouette", GHG3D_FAMILY_CYLINDER, GHG_SHAPE_CYLINDER, 150, 64, GHG_HOLDER_SPOON_LONG, GHG_SAFE_PIN_RING, 16, 0, 0},
    {"cylinder_short", "short cylindrical body", GHG3D_FAMILY_CYLINDER, GHG_SHAPE_CYLINDER, 95, 68, GHG_HOLDER_SPOON_SHORT, GHG_SAFE_PIN_RING, 16, 0, 0},
    {"can_jam", "No.8 jam-tin / double-cylinder silhouette", GHG3D_FAMILY_CYLINDER, GHG_SHAPE_CYLINDER, 83, 77, GHG_HOLDER_KNOB, GHG_SAFE_WIRE_LOOP, 14, 0, 0},
    {"double_cylinder", "stepped double-cylinder body", GHG3D_FAMILY_CYLINDER, GHG_SHAPE_DOUBLE_CYL, 105, 74, GHG_HOLDER_TOP_BRIDGE, GHG_SAFE_PIN_RING, 16, 0, 0},
    {"cylinder_ribbed", "cylindrical body with circumferential ribs", GHG3D_FAMILY_CYLINDER, GHG_SHAPE_RIBBED_CYL, 135, 66, GHG_HOLDER_SPOON_LONG, GHG_SAFE_PIN_RING, 16, 6, 0},
    {"tapered_can", "slightly tapered can body", GHG3D_FAMILY_CYLINDER, GHG_SHAPE_TAPERED_CYL, 140, 67, GHG_HOLDER_SPOON_LONG, GHG_SAFE_PIN_RING, 16, 0, 0},
    {"barrel", "barrel-shaped ribbed body", GHG3D_FAMILY_CYLINDER, GHG_SHAPE_BARREL, 120, 76, GHG_HOLDER_TOP_BRIDGE, GHG_SAFE_PIN_RING, 16, 4, 0},
    {"cone_segmented", "Newton-Pippin-style segmented cone", GHG3D_FAMILY_CONE, GHG_SHAPE_CONE_SEG, 125, 45, GHG_HOLDER_KNOB, GHG_SAFE_WIRE_LOOP, 14, 7, 8},
    {"bottle", "bottle/flask body", GHG3D_FAMILY_HYBRID, GHG_SHAPE_BOTTLE, 165, 70, GHG_HOLDER_KNOB, GHG_SAFE_SMALL_RING, 16, 0, 0},
    {"bell", "bell/skirt body", GHG3D_FAMILY_HYBRID, GHG_SHAPE_BELL, 125, 78, GHG_HOLDER_TOP_BRIDGE, GHG_SAFE_PIN_RING, 16, 0, 0},
    {"disc", "discoid/puck hand-grenade silhouette", GHG3D_FAMILY_DISC, GHG_SHAPE_DISC, 58, 105, GHG_HOLDER_TOP_BRIDGE, GHG_SAFE_PIN_RING, 18, 0, 0},
    {"box", "rectangular box body", GHG3D_FAMILY_PRISM, GHG_SHAPE_BOX, 110, 72, GHG_HOLDER_STRAP, GHG_SAFE_PIN_RING, 8, 0, 0},
    {"hex_prism", "hexagonal prism body", GHG3D_FAMILY_PRISM, GHG_SHAPE_HEX, 120, 68, GHG_HOLDER_TOP_BRIDGE, GHG_SAFE_PIN_RING, 6, 0, 0},
    {"dome", "domed flattened body", GHG3D_FAMILY_DISC, GHG_SHAPE_DOME, 72, 92, GHG_HOLDER_TOP_BRIDGE, GHG_SAFE_PIN_RING, 18, 0, 0}
};

static ghg3d_fx ghg_scale_fx(ghg3d_fx v, ghg3d_fx s)
{
    long a;
    long b;
    long q;
    long r;
    int neg;
    neg = 0;
    a = v;
    b = s;
    if (a < 0L) { a = -a; neg = !neg; }
    if (b < 0L) { b = -b; neg = !neg; }
    q = a / GHG3D_FX_ONE;
    r = a % GHG3D_FX_ONE;
    a = q * b + (r * b) / GHG3D_FX_ONE;
    return neg ? -a : a;
}

static ghg3d_fx ghg_ratio(ghg3d_fx v, long n, long d)
{
    return (v / d) * n + ((v % d) * n) / d;
}

static ghg3d_fx ghg_mul_trig(ghg3d_fx v, long trig)
{
    long av;
    long at;
    long q;
    long r;
    int neg;
    neg = 0;
    av = v;
    at = trig;
    if (av < 0L) { av = -av; neg = !neg; }
    if (at < 0L) { at = -at; neg = !neg; }
    q = av >> 8;
    r = av & 255L;
    av = (q * at >> 6) + ((r * at) >> 14);
    return neg ? -av : av;
}

static long ghg_cos_q14(int idx)
{
    return ghg_sin_q14[(idx + 64) & 255];
}

static void ghg_begin_part(GHG_Ctx *c, int part)
{
    GHG3D_PartRange *r;
    r = &c->result->parts[part];
    r->first_vertex = c->vc;
    r->first_triangle = c->tc;
}

static void ghg_end_part(GHG_Ctx *c, int part)
{
    GHG3D_PartRange *r;
    r = &c->result->parts[part];
    r->vertex_count = c->vc - r->first_vertex;
    r->triangle_count = c->tc - r->first_triangle;
}

static unsigned long ghg_vertex(GHG_Ctx *c, ghg3d_fx x, ghg3d_fx y, ghg3d_fx z,
                                long nx, long ny, long nz, ghg3d_fx u, ghg3d_fx v,
                                int part)
{
    unsigned long idx;
    GHG3D_Vertex *out;
    idx = c->vc++;
    if (c->v != 0) {
        if (idx >= c->vcap) {
            c->overflow = 1;
        } else {
            out = &c->v[idx];
            out->x = ghg_scale_fx(x, c->scale);
            out->y = ghg_scale_fx(y, c->scale);
            out->z = ghg_scale_fx(z, c->scale);
            out->nx = nx;
            out->ny = ny;
            out->nz = nz;
            out->u = u;
            out->v = v;
            out->part = (unsigned short)part;
        }
    }
    return idx;
}

static void ghg_triangle(GHG_Ctx *c, unsigned long a, unsigned long b, unsigned long d, int part)
{
    unsigned long idx;
    GHG3D_Triangle *out;
    idx = c->tc++;
    if (c->t != 0) {
        if (idx >= c->tcap || a > 65535UL || b > 65535UL || d > 65535UL) {
            c->overflow = 1;
        } else {
            out = &c->t[idx];
            out->a = (unsigned short)a;
            out->b = (unsigned short)b;
            out->c = (unsigned short)d;
            out->part = (unsigned short)part;
        }
    }
}

static void ghg_add_lathe(GHG_Ctx *c, const GHG_ProfilePoint *p, int pn, int segs, int part,
                          ghg3d_fx ox, ghg3d_fx oy, ghg3d_fx oz)
{
    int j;
    int i;
    int idx;
    long sn;
    long cs;
    ghg3d_fx x;
    ghg3d_fx z;
    unsigned long base;
    unsigned long a;
    unsigned long b;
    unsigned long d;
    unsigned long e;
    base = c->vc;
    for (j = 0; j < pn; ++j) {
        for (i = 0; i < segs; ++i) {
            idx = (i * 256) / segs;
            sn = ghg_sin_q14[idx & 255];
            cs = ghg_cos_q14(idx);
            x = ox + ghg_mul_trig(p[j].r, cs);
            z = oz + ghg_mul_trig(p[j].r, sn);
            ghg_vertex(c, x, oy + p[j].y, z, cs, 0L, sn,
                       (ghg3d_fx)((i * GHG3D_FX_ONE) / segs),
                       (ghg3d_fx)((j * GHG3D_FX_ONE) / (pn - 1)), part);
        }
    }
    for (j = 0; j < pn - 1; ++j) {
        for (i = 0; i < segs; ++i) {
            a = base + (unsigned long)(j * segs + i);
            b = base + (unsigned long)(j * segs + ((i + 1) % segs));
            d = base + (unsigned long)((j + 1) * segs + i);
            e = base + (unsigned long)((j + 1) * segs + ((i + 1) % segs));
            ghg_triangle(c, a, d, b, part);
            ghg_triangle(c, b, d, e, part);
        }
    }
}

static void ghg_add_segmented_lathe(GHG_Ctx *c, const GHG_ProfilePoint *p, int pn,
                                    int cols, int part, ghg3d_fx groove,
                                    ghg3d_fx ox, ghg3d_fx oy, ghg3d_fx oz)
{
    int segs;
    int j;
    int i;
    int idx;
    long sn;
    long cs;
    ghg3d_fx rr;
    ghg3d_fx x;
    ghg3d_fx z;
    unsigned long base;
    unsigned long a;
    unsigned long b;
    unsigned long d;
    unsigned long e;
    segs = cols * 2;
    base = c->vc;
    for (j = 0; j < pn; ++j) {
        for (i = 0; i < segs; ++i) {
            rr = p[j].r;
            if ((i & 1) == 0 && rr > groove) rr -= groove;
            if ((j & 1) == 0 && rr > groove) rr -= groove;
            idx = (i * 256) / segs;
            sn = ghg_sin_q14[idx & 255];
            cs = ghg_cos_q14(idx);
            x = ox + ghg_mul_trig(rr, cs);
            z = oz + ghg_mul_trig(rr, sn);
            ghg_vertex(c, x, oy + p[j].y, z, cs, 0L, sn,
                       (ghg3d_fx)((i * GHG3D_FX_ONE) / segs),
                       (ghg3d_fx)((j * GHG3D_FX_ONE) / (pn - 1)), part);
        }
    }
    for (j = 0; j < pn - 1; ++j) {
        for (i = 0; i < segs; ++i) {
            a = base + (unsigned long)(j * segs + i);
            b = base + (unsigned long)(j * segs + ((i + 1) % segs));
            d = base + (unsigned long)((j + 1) * segs + i);
            e = base + (unsigned long)((j + 1) * segs + ((i + 1) % segs));
            ghg_triangle(c, a, d, b, part);
            ghg_triangle(c, b, d, e, part);
        }
    }
}

static void ghg_add_box(GHG_Ctx *c, ghg3d_fx x0, ghg3d_fx y0, ghg3d_fx z0,
                        ghg3d_fx x1, ghg3d_fx y1, ghg3d_fx z1, int part)
{
    unsigned long b;
    b = c->vc;
    ghg_vertex(c, x0,y0,z0,-GHG_NORM_ONE,0,0,0,0,part);
    ghg_vertex(c, x1,y0,z0,0,0,-GHG_NORM_ONE,GHG3D_FX_ONE,0,part);
    ghg_vertex(c, x1,y1,z0,0,GHG_NORM_ONE,0,GHG3D_FX_ONE,GHG3D_FX_ONE,part);
    ghg_vertex(c, x0,y1,z0,-GHG_NORM_ONE,0,0,0,GHG3D_FX_ONE,part);
    ghg_vertex(c, x0,y0,z1,0,0,GHG_NORM_ONE,0,0,part);
    ghg_vertex(c, x1,y0,z1,GHG_NORM_ONE,0,0,GHG3D_FX_ONE,0,part);
    ghg_vertex(c, x1,y1,z1,0,GHG_NORM_ONE,0,GHG3D_FX_ONE,GHG3D_FX_ONE,part);
    ghg_vertex(c, x0,y1,z1,0,0,GHG_NORM_ONE,0,GHG3D_FX_ONE,part);
    ghg_triangle(c,b+0,b+1,b+2,part); ghg_triangle(c,b+0,b+2,b+3,part);
    ghg_triangle(c,b+4,b+6,b+5,part); ghg_triangle(c,b+4,b+7,b+6,part);
    ghg_triangle(c,b+0,b+4,b+5,part); ghg_triangle(c,b+0,b+5,b+1,part);
    ghg_triangle(c,b+3,b+2,b+6,part); ghg_triangle(c,b+3,b+6,b+7,part);
    ghg_triangle(c,b+1,b+5,b+6,part); ghg_triangle(c,b+1,b+6,b+2,part);
    ghg_triangle(c,b+0,b+3,b+7,part); ghg_triangle(c,b+0,b+7,b+4,part);
}

static void ghg_add_cylinder_y(GHG_Ctx *c, ghg3d_fx y0, ghg3d_fx y1,
                               ghg3d_fx radius, int segs, int part,
                               ghg3d_fx ox, ghg3d_fx oz)
{
    GHG_ProfilePoint p[4];
    p[0].y=y0; p[0].r=0;
    p[1].y=y0; p[1].r=radius;
    p[2].y=y1; p[2].r=radius;
    p[3].y=y1; p[3].r=0;
    ghg_add_lathe(c,p,4,segs,part,ox,0,oz);
}

static void ghg_add_cylinder_x(GHG_Ctx *c, ghg3d_fx x0, ghg3d_fx x1,
                               ghg3d_fx cy, ghg3d_fx cz, ghg3d_fx r,
                               int segs, int part)
{
    int i;
    int idx;
    long sn;
    long cs;
    unsigned long base;
    unsigned long a;
    unsigned long b;
    unsigned long d;
    unsigned long e;
    base = c->vc;
    for (i=0;i<segs;++i) {
        idx=(i*256)/segs; sn=ghg_sin_q14[idx]; cs=ghg_cos_q14(idx);
        ghg_vertex(c,x0,cy+ghg_mul_trig(r,cs),cz+ghg_mul_trig(r,sn),0,cs,sn,0,0,part);
    }
    for (i=0;i<segs;++i) {
        idx=(i*256)/segs; sn=ghg_sin_q14[idx]; cs=ghg_cos_q14(idx);
        ghg_vertex(c,x1,cy+ghg_mul_trig(r,cs),cz+ghg_mul_trig(r,sn),0,cs,sn,GHG3D_FX_ONE,0,part);
    }
    for(i=0;i<segs;++i) {
        a=base+(unsigned long)i; b=base+(unsigned long)((i+1)%segs);
        d=base+(unsigned long)(segs+i); e=base+(unsigned long)(segs+((i+1)%segs));
        ghg_triangle(c,a,d,b,part); ghg_triangle(c,b,d,e,part);
    }
}

static void ghg_add_torus_xy(GHG_Ctx *c, ghg3d_fx cx, ghg3d_fx cy, ghg3d_fx cz,
                             ghg3d_fx major_r, ghg3d_fx tube_r,
                             int major_seg, int minor_seg, int part)
{
    int i;
    int j;
    int ia;
    int ib;
    long sa;
    long ca;
    long sb;
    long cb;
    ghg3d_fx rr;
    unsigned long base;
    unsigned long a;
    unsigned long b;
    unsigned long d;
    unsigned long e;
    base=c->vc;
    for(j=0;j<major_seg;++j) {
        ia=(j*256)/major_seg; sa=ghg_sin_q14[ia]; ca=ghg_cos_q14(ia);
        for(i=0;i<minor_seg;++i) {
            ib=(i*256)/minor_seg; sb=ghg_sin_q14[ib]; cb=ghg_cos_q14(ib);
            rr=major_r+ghg_mul_trig(tube_r,cb);
            ghg_vertex(c,cx+ghg_mul_trig(rr,ca),cy+ghg_mul_trig(rr,sa),cz+ghg_mul_trig(tube_r,sb),
                       ghg_mul_trig(GHG_NORM_ONE,ca),ghg_mul_trig(GHG_NORM_ONE,sa),sb,0,0,part);
        }
    }
    for(j=0;j<major_seg;++j) {
        for(i=0;i<minor_seg;++i) {
            a=base+(unsigned long)(j*minor_seg+i);
            b=base+(unsigned long)(j*minor_seg+((i+1)%minor_seg));
            d=base+(unsigned long)(((j+1)%major_seg)*minor_seg+i);
            e=base+(unsigned long)(((j+1)%major_seg)*minor_seg+((i+1)%minor_seg));
            ghg_triangle(c,a,d,b,part); ghg_triangle(c,b,d,e,part);
        }
    }
}

static int ghg_profile_fraction(GHG_ProfilePoint *p, const int *rf, int count,
                                ghg3d_fx h, ghg3d_fx r)
{
    int i;
    for(i=0;i<count;++i) {
        p[i].y=ghg_ratio(h,i,count-1);
        p[i].r=ghg_ratio(r,rf[i],100);
    }
    return count;
}

static int ghg_make_profile(int shape, ghg3d_fx h, ghg3d_fx r,
                            GHG_ProfilePoint *p)
{
    static const int ball[9]={0,38,71,92,100,92,71,38,0};
    static const int true_ball[11]={0,44,68,86,97,100,97,86,68,44,0};
    static const int lemon[11]={0,28,60,83,97,100,95,82,61,34,0};
    static const int almond[11]={0,20,48,74,93,100,96,82,60,34,0};
    static const int egg[11]={0,38,70,90,100,98,90,76,56,30,0};
    static const int flat[9]={0,48,82,97,100,97,82,48,0};
    static const int capsule[11]={0,55,88,100,100,100,100,100,88,55,0};
    static const int pear[11]={0,25,53,78,96,100,94,78,57,35,0};
    static const int gourd[13]={0,35,72,98,100,83,68,73,82,74,53,28,0};
    static const int football[11]={0,28,58,82,97,100,97,82,58,28,0};
    static const int bottle[13]={0,42,82,98,100,100,98,88,52,35,34,20,0};
    static const int bell[11]={0,68,96,100,96,84,70,58,44,28,0};
    static const int dome[9]={0,72,98,100,96,82,62,34,0};
    int n;
    n=0;
    if(shape==GHG_SHAPE_BALL) n=ghg_profile_fraction(p,ball,9,h,r);
    else if(shape==GHG_SHAPE_TRUE_BALL) n=ghg_profile_fraction(p,true_ball,11,h,r);
    else if(shape==GHG_SHAPE_LEMON) n=ghg_profile_fraction(p,lemon,11,h,r);
    else if(shape==GHG_SHAPE_ALMOND) n=ghg_profile_fraction(p,almond,11,h,r);
    else if(shape==GHG_SHAPE_EGG) n=ghg_profile_fraction(p,egg,11,h,r);
    else if(shape==GHG_SHAPE_FLAT_OVAL) n=ghg_profile_fraction(p,flat,9,h,r);
    else if(shape==GHG_SHAPE_CAPSULE) n=ghg_profile_fraction(p,capsule,11,h,r);
    else if(shape==GHG_SHAPE_PEAR) n=ghg_profile_fraction(p,pear,11,h,r);
    else if(shape==GHG_SHAPE_GOURD) n=ghg_profile_fraction(p,gourd,13,h,r);
    else if(shape==GHG_SHAPE_FOOTBALL) n=ghg_profile_fraction(p,football,11,h,r);
    else if(shape==GHG_SHAPE_BOTTLE) n=ghg_profile_fraction(p,bottle,13,h,r);
    else if(shape==GHG_SHAPE_BELL) n=ghg_profile_fraction(p,bell,11,h,r);
    else if(shape==GHG_SHAPE_DISC || shape==GHG_SHAPE_DOME) n=ghg_profile_fraction(p,dome,9,h,r);
    return n;
}

static void ghg_make_segment_profile(GHG_ProfilePoint *p, int rows,
                                     ghg3d_fx h, ghg3d_fx r, int ballish)
{
    int i;
    int n;
    int pct;
    int x;
    n=rows*2+1;
    for(i=0;i<n;++i) {
        p[i].y=ghg_ratio(h,i,n-1);
        x=(i*200)/(n-1)-100;
        if(x<0) x=-x;
        if(ballish) pct=100-(x*x)/100;
        else pct=100-(x*x*70)/10000;
        if(pct<0) pct=0;
        p[i].r=ghg_ratio(r,pct,100);
    }
    p[0].r=0;
    p[n-1].r=0;
}

static int ghg_segments_for_lod(const GHG_PresetSpec *s, int lod)
{
    int seg;
    seg=(int)s->game_segments;
    if(lod==GHG3D_LOD_TINY) seg=(seg*2)/3;
    if(lod==GHG3D_LOD_CLOSE) seg=(seg*3)/2;
    if(seg<6) seg=6;
    if(seg>24) seg=24;
    return seg;
}

static void ghg_build_stick_body(GHG_Ctx *c, const GHG_PresetSpec *s, int segs)
{
    ghg3d_fx h;
    ghg3d_fx w;
    ghg3d_fx handle_h;
    ghg3d_fx head_h;
    ghg3d_fx head_r;
    ghg3d_fx handle_r;
    GHG_ProfilePoint p[11];
    int pn;
    h=GHG3D_FX_MM(s->height_mm);
    w=GHG3D_FX_MM(s->width_mm);
    handle_h=ghg_ratio(h,64,100);
    head_h=h-handle_h;
    head_r=w/2;
    handle_r=ghg_ratio(head_r,30,100);
    ghg_add_cylinder_y(c,0,handle_h,handle_r,segs,GHG3D_PART_BODY,0,0);
    ghg_add_cylinder_y(c,0,ghg_ratio(h,4,100),ghg_ratio(handle_r,120,100),segs,GHG3D_PART_BODY,0,0);
    if(s->shape==GHG_SHAPE_STICK_BOX) {
        ghg_add_box(c,-head_r,handle_h,-head_r,head_r,h,head_r,GHG3D_PART_BODY);
    } else if(s->shape==GHG_SHAPE_STICK_BALL) {
        pn=ghg_make_profile(GHG_SHAPE_BALL,head_h,head_r,p);
        ghg_add_lathe(c,p,pn,segs,GHG3D_PART_BODY,0,handle_h,0);
    } else if(s->shape==GHG_SHAPE_STICK_CONE) {
        p[0].y=0; p[0].r=head_r;
        p[1].y=ghg_ratio(head_h,70,100); p[1].r=head_r;
        p[2].y=head_h; p[2].r=ghg_ratio(head_r,35,100);
        p[3].y=head_h; p[3].r=0;
        ghg_add_lathe(c,p,4,segs,GHG3D_PART_BODY,0,handle_h,0);
    } else {
        ghg_add_cylinder_y(c,handle_h,h,head_r,segs,GHG3D_PART_BODY,0,0);
    }
}

static void ghg_build_body(GHG_Ctx *c, const GHG_PresetSpec *s, int segs)
{
    ghg3d_fx h;
    ghg3d_fx r;
    ghg3d_fx groove;
    GHG_ProfilePoint p[GHG_MAX_PROFILE];
    int pn;
    int rows;
    int cols;
    int i;
    h=GHG3D_FX_MM(s->height_mm);
    r=GHG3D_FX_MM(s->width_mm)/2;
    if(s->shape<=GHG_SHAPE_STICK_CONE) {
        ghg_build_stick_body(c,s,segs);
        return;
    }
    if(s->shape==GHG_SHAPE_SEG_BALL || s->shape==GHG_SHAPE_SEG_OVOID || s->shape==GHG_SHAPE_CONE_SEG) {
        rows=(int)s->rows; cols=(int)s->cols;
        if(rows<3) rows=5;
        if(cols<4) cols=8;
        ghg_make_segment_profile(p,rows,h,r,s->shape==GHG_SHAPE_SEG_BALL);
        if(s->shape==GHG_SHAPE_CONE_SEG) {
            pn=rows*2+1;
            for(i=0;i<pn;++i) p[i].r=ghg_ratio(r,(pn-1-i)*100/(pn-1),100);
            p[0].r=ghg_ratio(r,85,100);
            p[pn-1].r=0;
        }
        groove=GHG3D_FX_MM(2);
        ghg_add_segmented_lathe(c,p,rows*2+1,cols,GHG3D_PART_BODY,groove,0,0,0);
        return;
    }
    if(s->shape==GHG_SHAPE_RIBBED_OVOID) {
        pn=ghg_make_profile(GHG_SHAPE_EGG,h,r,p);
        ghg_add_segmented_lathe(c,p,pn,(int)s->cols,GHG3D_PART_BODY,GHG3D_FX_MM(1),0,0,0);
        return;
    }
    if(s->shape==GHG_SHAPE_CYLINDER || s->shape==GHG_SHAPE_TAPERED_CYL || s->shape==GHG_SHAPE_RIBBED_CYL || s->shape==GHG_SHAPE_BARREL || s->shape==GHG_SHAPE_DOUBLE_CYL) {
        if(s->shape==GHG_SHAPE_CYLINDER) {
            ghg_add_cylinder_y(c,0,h,r,segs,GHG3D_PART_BODY,0,0);
        } else if(s->shape==GHG_SHAPE_TAPERED_CYL) {
            p[0].y=0;p[0].r=0; p[1].y=0;p[1].r=ghg_ratio(r,92,100);
            p[2].y=h;p[2].r=r; p[3].y=h;p[3].r=0;
            ghg_add_lathe(c,p,4,segs,GHG3D_PART_BODY,0,0,0);
        } else if(s->shape==GHG_SHAPE_DOUBLE_CYL) {
            ghg_add_cylinder_y(c,0,ghg_ratio(h,62,100),r,segs,GHG3D_PART_BODY,0,0);
            ghg_add_cylinder_y(c,ghg_ratio(h,62,100),h,ghg_ratio(r,78,100),segs,GHG3D_PART_BODY,0,0);
        } else {
            pn=13;
            for(i=0;i<pn;++i) {
                p[i].y=ghg_ratio(h,i,pn-1);
                p[i].r=r;
                if(i==0 || i==pn-1) p[i].r=0;
                else if(s->shape==GHG_SHAPE_BARREL) {
                    if(i<3 || i>9) p[i].r=ghg_ratio(r,85,100);
                } else if((i&1)==0) p[i].r=ghg_ratio(r,92,100);
            }
            ghg_add_lathe(c,p,pn,segs,GHG3D_PART_BODY,0,0,0);
        }
        return;
    }
    if(s->shape==GHG_SHAPE_BOX) {
        ghg_add_box(c,-r,0,-ghg_ratio(r,70,100),r,h,ghg_ratio(r,70,100),GHG3D_PART_BODY);
        return;
    }
    if(s->shape==GHG_SHAPE_HEX) {
        ghg_add_cylinder_y(c,0,h,r,6,GHG3D_PART_BODY,0,0);
        return;
    }
    pn=ghg_make_profile(s->shape,h,r,p);
    if(pn==0) pn=ghg_make_profile(GHG_SHAPE_BALL,h,r,p);
    ghg_add_lathe(c,p,pn,segs,GHG3D_PART_BODY,0,0,0);
}

static void ghg_build_holder(GHG_Ctx *c, const GHG_PresetSpec *s, int segs)
{
    ghg3d_fx h;
    ghg3d_fx r;
    ghg3d_fx top;
    ghg3d_fx cap_h;
    ghg3d_fx cap_r;
    h=GHG3D_FX_MM(s->height_mm);
    r=GHG3D_FX_MM(s->width_mm)/2;
    top=h;
    cap_h=GHG3D_FX_MM(14);
    cap_r=ghg_ratio(r,40,100);
    if(s->shape<=GHG_SHAPE_STICK_CONE) {
        top=h;
        cap_h=GHG3D_FX_MM(10);
        cap_r=ghg_ratio(r,55,100);
    }
    ghg_add_cylinder_y(c,top,top+cap_h,cap_r,segs<12?segs:12,GHG3D_PART_HOLDER,0,0);
    if(s->holder==GHG_HOLDER_SPOON_LONG) {
        ghg_add_box(c,cap_r/2,top+cap_h/2,-GHG3D_FX_MM(5),r+GHG3D_FX_MM(8),top+GHG3D_FX_MM(25),GHG3D_FX_MM(5),GHG3D_PART_HOLDER);
        ghg_add_box(c,r-GHG3D_FX_MM(2),top-ghg_ratio(h,32,100),-GHG3D_FX_MM(5),r+GHG3D_FX_MM(8),top+GHG3D_FX_MM(8),GHG3D_FX_MM(5),GHG3D_PART_HOLDER);
    } else if(s->holder==GHG_HOLDER_SPOON_SHORT) {
        ghg_add_box(c,cap_r/2,top+cap_h/2,-GHG3D_FX_MM(4),r+GHG3D_FX_MM(4),top+GHG3D_FX_MM(18),GHG3D_FX_MM(4),GHG3D_PART_HOLDER);
    } else if(s->holder==GHG_HOLDER_TOP_BRIDGE) {
        ghg_add_box(c,-cap_r,top+cap_h,-GHG3D_FX_MM(4),cap_r,top+cap_h+GHG3D_FX_MM(7),GHG3D_FX_MM(4),GHG3D_PART_HOLDER);
    } else if(s->holder==GHG_HOLDER_KNOB) {
        ghg_add_cylinder_y(c,top+cap_h,top+cap_h+GHG3D_FX_MM(12),ghg_ratio(cap_r,55,100),10,GHG3D_PART_HOLDER,0,0);
    } else if(s->holder==GHG_HOLDER_STICK_COLLAR) {
        ghg_add_cylinder_y(c,ghg_ratio(h,62,100)-GHG3D_FX_MM(4),ghg_ratio(h,62,100)+GHG3D_FX_MM(5),ghg_ratio(r,48,100),12,GHG3D_PART_HOLDER,0,0);
    } else {
        ghg_add_box(c,-cap_r,top+cap_h/2,-GHG3D_FX_MM(5),r+GHG3D_FX_MM(4),top+cap_h+GHG3D_FX_MM(8),GHG3D_FX_MM(5),GHG3D_PART_HOLDER);
    }
    c->result->parts[GHG3D_PART_HOLDER].pivot_x=ghg_scale_fx(0,c->scale);
    c->result->parts[GHG3D_PART_HOLDER].pivot_y=ghg_scale_fx(top+cap_h,c->scale);
    c->result->parts[GHG3D_PART_HOLDER].pivot_z=0;
}

static void ghg_build_safety(GHG_Ctx *c, const GHG_PresetSpec *s, int segs)
{
    ghg3d_fx h;
    ghg3d_fx r;
    ghg3d_fx cy;
    ghg3d_fx cx;
    ghg3d_fx ring_r;
    h=GHG3D_FX_MM(s->height_mm);
    r=GHG3D_FX_MM(s->width_mm)/2;
    if(s->safety==GHG_SAFE_NONE) return;
    if(s->safety==GHG_SAFE_BOTTOM_RING) {
        cy=GHG3D_FX_MM(6);
        cx=0;
        ring_r=GHG3D_FX_MM(11);
        ghg_add_torus_xy(c,cx,cy,-GHG3D_FX_MM(2),ring_r,GHG3D_FX_MM(2),12,4,GHG3D_PART_SAFETY);
        c->result->parts[GHG3D_PART_SAFETY].pivot_y=ghg_scale_fx(cy,c->scale);
        return;
    }
    cy=h+GHG3D_FX_MM(13);
    cx=ghg_ratio(r,40,100);
    ring_r=(s->safety==GHG_SAFE_SMALL_RING)?GHG3D_FX_MM(8):GHG3D_FX_MM(14);
    ghg_add_cylinder_x(c,-GHG3D_FX_MM(8),GHG3D_FX_MM(8),cy,0,GHG3D_FX_MM(2),segs<10?segs:10,GHG3D_PART_SAFETY);
    if(s->safety==GHG_SAFE_WIRE_LOOP) ring_r=GHG3D_FX_MM(10);
    ghg_add_torus_xy(c,cx+ring_r,cy,0,ring_r,GHG3D_FX_MM(2),12,4,GHG3D_PART_SAFETY);
    c->result->parts[GHG3D_PART_SAFETY].pivot_x=ghg_scale_fx(0,c->scale);
    c->result->parts[GHG3D_PART_SAFETY].pivot_y=ghg_scale_fx(cy,c->scale);
    c->result->parts[GHG3D_PART_SAFETY].pivot_z=0;
}

static void ghg_compute_bounds(GHG_Ctx *c)
{
    unsigned long i;
    GHG3D_Vertex *v;
    if(c->v==0 || c->vc==0 || c->overflow) {
        c->result->bounds_min_x=0; c->result->bounds_min_y=0; c->result->bounds_min_z=0;
        c->result->bounds_max_x=0; c->result->bounds_max_y=0; c->result->bounds_max_z=0;
        return;
    }
    v=&c->v[0];
    c->result->bounds_min_x=v->x; c->result->bounds_max_x=v->x;
    c->result->bounds_min_y=v->y; c->result->bounds_max_y=v->y;
    c->result->bounds_min_z=v->z; c->result->bounds_max_z=v->z;
    for(i=1;i<c->vc;++i) {
        v=&c->v[i];
        if(v->x<c->result->bounds_min_x)c->result->bounds_min_x=v->x;
        if(v->y<c->result->bounds_min_y)c->result->bounds_min_y=v->y;
        if(v->z<c->result->bounds_min_z)c->result->bounds_min_z=v->z;
        if(v->x>c->result->bounds_max_x)c->result->bounds_max_x=v->x;
        if(v->y>c->result->bounds_max_y)c->result->bounds_max_y=v->y;
        if(v->z>c->result->bounds_max_z)c->result->bounds_max_z=v->z;
    }
}

void ghg3d_desc_default(GHG3D_Desc *desc, int preset)
{
    if(desc==0) return;
    desc->preset=preset;
    desc->lod=GHG3D_LOD_GAME;
    desc->parts_mask=GHG3D_PARTMASK_ALL;
    desc->uniform_scale=GHG3D_FX_ONE;
}

int ghg3d_get_preset_info(int preset, GHG3D_PresetInfo *info)
{
    const GHG_PresetSpec *s;
    if(info==0) return GHG3D_E_ARGUMENT;
    if(preset<0 || preset>=GHG3D_PRESET_COUNT) return GHG3D_E_PRESET;
    s=&ghg_specs[preset];
    info->preset=preset;
    info->family=s->family;
    info->name=s->name;
    info->visual_reference=s->reference;
    info->nominal_height_mm=s->height_mm;
    info->nominal_width_mm=s->width_mm;
    return GHG3D_OK;
}

const char *ghg3d_preset_name(int preset)
{
    if(preset<0 || preset>=GHG3D_PRESET_COUNT) return "invalid";
    return ghg_specs[preset].name;
}

int ghg3d_build(const GHG3D_Desc *desc,
                GHG3D_Vertex *vertices,
                unsigned long vertex_capacity,
                GHG3D_Triangle *triangles,
                unsigned long triangle_capacity,
                GHG3D_Result *result)
{
    GHG_Ctx c;
    const GHG_PresetSpec *s;
    int segs;
    int i;
    if(desc==0 || result==0) return GHG3D_E_ARGUMENT;
    if(desc->preset<0 || desc->preset>=GHG3D_PRESET_COUNT) return GHG3D_E_PRESET;
    if(desc->lod<GHG3D_LOD_TINY || desc->lod>GHG3D_LOD_CLOSE) return GHG3D_E_ARGUMENT;
    if(desc->uniform_scale<=0) return GHG3D_E_ARGUMENT;
    c.v=vertices; c.t=triangles; c.vcap=vertex_capacity; c.tcap=triangle_capacity;
    c.vc=0; c.tc=0; c.overflow=0; c.scale=desc->uniform_scale; c.result=result;
    result->vertex_count=0; result->triangle_count=0;
    for(i=0;i<GHG3D_PART_COUNT;++i) {
        result->parts[i].first_vertex=0; result->parts[i].vertex_count=0;
        result->parts[i].first_triangle=0; result->parts[i].triangle_count=0;
        result->parts[i].pivot_x=0; result->parts[i].pivot_y=0; result->parts[i].pivot_z=0;
    }
    s=&ghg_specs[desc->preset];
    segs=ghg_segments_for_lod(s,desc->lod);
    if(desc->parts_mask & GHG3D_PARTMASK_BODY) {
        ghg_begin_part(&c,GHG3D_PART_BODY);
        ghg_build_body(&c,s,segs);
        ghg_end_part(&c,GHG3D_PART_BODY);
    }
    if(desc->parts_mask & GHG3D_PARTMASK_HOLDER) {
        ghg_begin_part(&c,GHG3D_PART_HOLDER);
        ghg_build_holder(&c,s,segs);
        ghg_end_part(&c,GHG3D_PART_HOLDER);
    }
    if(desc->parts_mask & GHG3D_PARTMASK_SAFETY) {
        ghg_begin_part(&c,GHG3D_PART_SAFETY);
        ghg_build_safety(&c,s,segs);
        ghg_end_part(&c,GHG3D_PART_SAFETY);
    }
    result->vertex_count=c.vc;
    result->triangle_count=c.tc;
    if(c.overflow) return GHG3D_E_CAPACITY;
    if((vertices!=0 && c.vc>vertex_capacity) || (triangles!=0 && c.tc>triangle_capacity)) return GHG3D_E_CAPACITY;
    ghg_compute_bounds(&c);
    return GHG3D_OK;
}
