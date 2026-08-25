#ifndef KATANA89_H
#define KATANA89_H

/*
    katana89 v1.1 - realistic low-poly Japanese sword mesh generator
    C89, fixed point, no malloc/realloc/free, no float/double.

    Coordinate convention:
      X = thickness axis
      Y = blade profile / curvature axis
      Z = sword length axis

    Fixed point:
      KM89_FP_ONE units = 1 centimetre.
      Default is Q24.8 (1/256 cm ~= 0.039 mm).
*/

#ifdef __cplusplus
extern "C" {
#endif

#define KM89_FP_SHIFT 8
#define KM89_FP_ONE   256L
#define KM89_CM(v)    ((long)((v) * KM89_FP_ONE))

#define KM89_MAX_VERTICES 4096
#define KM89_MAX_TRIANGLES 8192
#define KM89_NAME_MAX 48

#define KM89_OK                 0
#define KM89_ERR_ARGUMENT      -1
#define KM89_ERR_VERTEX_CAP    -2
#define KM89_ERR_TRIANGLE_CAP  -3
#define KM89_ERR_PRESET        -4

/* Blade cross-sections. */
#define KM89_BLADE_SHINOGI  0
#define KM89_BLADE_HIRA     1
#define KM89_BLADE_SHOBU    2
#define KM89_BLADE_UNOKUBI  3
#define KM89_BLADE_KIRIHA   4
#define KM89_BLADE_KATASHINOGI 5

/* Kissaki silhouettes. */
#define KM89_KISSAKI_KO      0
#define KM89_KISSAKI_CHU     1
#define KM89_KISSAKI_O       2
#define KM89_KISSAKI_SHOBU   3

/* Tsuba silhouettes. */
#define KM89_TSUBA_MARU       0
#define KM89_TSUBA_NADEKAKU   1
#define KM89_TSUBA_MOKKO      2
#define KM89_TSUBA_AOI        3
#define KM89_TSUBA_HACHI      4
#define KM89_TSUBA_CROSS      5
#define KM89_TSUBA_NONE       6

/* Tsuka profiles. */
#define KM89_TSUKA_STRAIGHT   0
#define KM89_TSUKA_RIKKO      1
#define KM89_TSUKA_HA_AGARI   2
#define KM89_TSUKA_TACHI      3

/* Materials are intentionally renderer-agnostic IDs. */
#define KM89_MAT_STEEL       0
#define KM89_MAT_EDGE        1
#define KM89_MAT_HAMON       2
#define KM89_MAT_GUARD       3
#define KM89_MAT_WRAP        4
#define KM89_MAT_SAME        5
#define KM89_MAT_FITTING     6
#define KM89_MAT_LACQUER     7
#define KM89_MAT_GROOVE      8
#define KM89_MAT_COUNT       9

#define KM89_FLAG_HAMON       1u
#define KM89_FLAG_BOHI        2u
#define KM89_FLAG_SAYA        4u
#define KM89_FLAG_MENUKI      8u
#define KM89_FLAG_SUKASHI    16u
#define KM89_FLAG_DOUBLE_WRAP 32u

typedef struct km89_vertex_s {
    long x;
    long y;
    long z;
} km89_vertex;

typedef struct km89_triangle_s {
    unsigned short a;
    unsigned short b;
    unsigned short c;
    unsigned char material;
} km89_triangle;

typedef struct km89_mesh_s {
    km89_vertex *vertices;
    km89_triangle *triangles;
    unsigned short vertex_capacity;
    unsigned short triangle_capacity;
    unsigned short vertex_count;
    unsigned short triangle_count;
} km89_mesh;

typedef struct km89_palette_s {
    unsigned char rgb[KM89_MAT_COUNT][3];
} km89_palette;

typedef struct km89_desc_s {
    const char *name;
    long blade_length;
    long handle_length;
    long sori;
    long blade_width_base;
    long blade_width_tip;
    long blade_thickness;
    long guard_radius;
    long guard_thickness;
    unsigned char blade_style;
    unsigned char kissaki_style;
    unsigned char tsuba_style;
    unsigned char tsuka_style;
    unsigned char palette_id;
    unsigned int flags;
} km89_desc;

typedef struct km89_part_range_s {
    unsigned short first_vertex;
    unsigned short vertex_count;
    unsigned short first_triangle;
    unsigned short triangle_count;
} km89_part_range;

typedef struct km89_build_info_s {
    km89_part_range blade;
    km89_part_range guard;
    km89_part_range handle;
    km89_part_range fittings;
    km89_part_range saya;
} km89_build_info;

void km89_mesh_init(km89_mesh *mesh,
                    km89_vertex *vertices,
                    unsigned short vertex_capacity,
                    km89_triangle *triangles,
                    unsigned short triangle_capacity);

void km89_mesh_reset(km89_mesh *mesh);

int km89_build(const km89_desc *desc,
               km89_mesh *mesh,
               km89_build_info *info);

int km89_build_preset(unsigned short preset_index,
                      km89_mesh *mesh,
                      km89_build_info *info);

unsigned short km89_preset_count(void);
const km89_desc *km89_preset(unsigned short preset_index);
const char *km89_preset_name(unsigned short preset_index);

const km89_palette *km89_palette_get(unsigned char palette_id);
unsigned char km89_palette_count(void);

/* Convenience conversion without floating point. */
long km89_from_millimetres(long mm);
long km89_to_millimetres(long fp_value);

#ifdef __cplusplus
}
#endif

#endif
