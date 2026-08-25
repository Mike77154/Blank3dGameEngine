#ifndef GHANDGRENADE3D89_H
#define GHANDGRENADE3D89_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * ghandgrenade3d89 - low-poly visual hand-grenade mesh catalogue.
 * C89, caller-owned buffers, no heap, fixed point only.
 * Coordinates use signed Q24.8 millimetres. Normals use signed Q1.14.
 */

#define GHG3D_VERSION_MAJOR 1
#define GHG3D_VERSION_MINOR 0
#define GHG3D_VERSION_PATCH 1

#define GHG3D_FX_SHIFT 8
#define GHG3D_FX_ONE 256L
#define GHG3D_FX_MM(v) ((ghg3d_fx)(v) * GHG3D_FX_ONE)
#define GHG3D_FX_FROM_RATIO(n,d) ((ghg3d_fx)(((long)(n) * GHG3D_FX_ONE) / (long)(d)))

typedef long ghg3d_fx;

enum GHG3D_Status {
    GHG3D_OK = 0,
    GHG3D_E_ARGUMENT = -1,
    GHG3D_E_PRESET = -2,
    GHG3D_E_CAPACITY = -3
};

enum GHG3D_Part {
    GHG3D_PART_BODY = 0,
    GHG3D_PART_HOLDER = 1,
    GHG3D_PART_SAFETY = 2,
    GHG3D_PART_COUNT = 3
};

#define GHG3D_PARTMASK_BODY   (1UL << GHG3D_PART_BODY)
#define GHG3D_PARTMASK_HOLDER (1UL << GHG3D_PART_HOLDER)
#define GHG3D_PARTMASK_SAFETY (1UL << GHG3D_PART_SAFETY)
#define GHG3D_PARTMASK_ALL    (GHG3D_PARTMASK_BODY | GHG3D_PARTMASK_HOLDER | GHG3D_PARTMASK_SAFETY)

enum GHG3D_LOD {
    GHG3D_LOD_TINY = 0,
    GHG3D_LOD_GAME = 1,
    GHG3D_LOD_CLOSE = 2
};

enum GHG3D_Family {
    GHG3D_FAMILY_STICK = 0,
    GHG3D_FAMILY_BALL,
    GHG3D_FAMILY_SEGMENTED,
    GHG3D_FAMILY_OVOID,
    GHG3D_FAMILY_CYLINDER,
    GHG3D_FAMILY_CONE,
    GHG3D_FAMILY_DISC,
    GHG3D_FAMILY_PRISM,
    GHG3D_FAMILY_HYBRID
};

enum GHG3D_Preset {
    GHG3D_PRESET_STICK_CLASSIC = 0,
    GHG3D_PRESET_STICK_LONG,
    GHG3D_PRESET_STICK_SHORT,
    GHG3D_PRESET_STICK_BALL_HEAD,
    GHG3D_PRESET_STICK_BOX_HEAD,
    GHG3D_PRESET_STICK_CONE_HEAD,
    GHG3D_PRESET_BALL_CLASSIC,
    GHG3D_PRESET_BALL_MINI,
    GHG3D_PRESET_BALL_TRUE_SPHERE,
    GHG3D_PRESET_BALL_NECKED,
    GHG3D_PRESET_BALL_HANDLE,
    GHG3D_PRESET_BALL_SEGMENTED,
    GHG3D_PRESET_PINEAPPLE_CLASSIC,
    GHG3D_PRESET_PINEAPPLE_TALL,
    GHG3D_PRESET_PINEAPPLE_SQUAT,
    GHG3D_PRESET_PINEAPPLE_ROUND,
    GHG3D_PRESET_EGG_SEGMENTED,
    GHG3D_PRESET_LEMON_CLASSIC,
    GHG3D_PRESET_ALMOND_TALL,
    GHG3D_PRESET_EGG_SMALL,
    GHG3D_PRESET_EGG_LONG,
    GHG3D_PRESET_OVAL_FLAT,
    GHG3D_PRESET_CAPSULE_OVAL,
    GHG3D_PRESET_PEAR,
    GHG3D_PRESET_GOURD,
    GHG3D_PRESET_FOOTBALL,
    GHG3D_PRESET_OVOID_RIBBED,
    GHG3D_PRESET_CYLINDER_TALL,
    GHG3D_PRESET_CYLINDER_SHORT,
    GHG3D_PRESET_CAN_JAM,
    GHG3D_PRESET_DOUBLE_CYLINDER,
    GHG3D_PRESET_CYLINDER_RIBBED,
    GHG3D_PRESET_TAPERED_CAN,
    GHG3D_PRESET_BARREL,
    GHG3D_PRESET_CONE_SEGMENTED,
    GHG3D_PRESET_BOTTLE,
    GHG3D_PRESET_BELL,
    GHG3D_PRESET_DISC,
    GHG3D_PRESET_BOX,
    GHG3D_PRESET_HEX_PRISM,
    GHG3D_PRESET_DOME,
    GHG3D_PRESET_COUNT
};

typedef struct GHG3D_Vertex {
    ghg3d_fx x;
    ghg3d_fx y;
    ghg3d_fx z;
    long nx;
    long ny;
    long nz;
    ghg3d_fx u;
    ghg3d_fx v;
    unsigned short part;
} GHG3D_Vertex;

typedef struct GHG3D_Triangle {
    unsigned short a;
    unsigned short b;
    unsigned short c;
    unsigned short part;
} GHG3D_Triangle;

typedef struct GHG3D_PartRange {
    unsigned long first_vertex;
    unsigned long vertex_count;
    unsigned long first_triangle;
    unsigned long triangle_count;
    ghg3d_fx pivot_x;
    ghg3d_fx pivot_y;
    ghg3d_fx pivot_z;
} GHG3D_PartRange;

typedef struct GHG3D_Desc {
    int preset;
    int lod;
    unsigned long parts_mask;
    ghg3d_fx uniform_scale;
} GHG3D_Desc;

typedef struct GHG3D_Result {
    unsigned long vertex_count;
    unsigned long triangle_count;
    GHG3D_PartRange parts[GHG3D_PART_COUNT];
    ghg3d_fx bounds_min_x;
    ghg3d_fx bounds_min_y;
    ghg3d_fx bounds_min_z;
    ghg3d_fx bounds_max_x;
    ghg3d_fx bounds_max_y;
    ghg3d_fx bounds_max_z;
} GHG3D_Result;

typedef struct GHG3D_PresetInfo {
    int preset;
    int family;
    const char *name;
    const char *visual_reference;
    long nominal_height_mm;
    long nominal_width_mm;
} GHG3D_PresetInfo;

void ghg3d_desc_default(GHG3D_Desc *desc, int preset);
int ghg3d_get_preset_info(int preset, GHG3D_PresetInfo *info);
const char *ghg3d_preset_name(int preset);

/* Passing NULL buffers with zero capacity performs a count-only build. */
int ghg3d_build(const GHG3D_Desc *desc,
                GHG3D_Vertex *vertices,
                unsigned long vertex_capacity,
                GHG3D_Triangle *triangles,
                unsigned long triangle_capacity,
                GHG3D_Result *result);

#ifdef __cplusplus
}
#endif

#endif
