#include "blank3d_katana_mesh.h"

#include <string.h>
#include "katana89.h"

static km89_vertex b3d_katana_source_vertices[KM89_MAX_VERTICES];
static km89_triangle b3d_katana_source_triangles[KM89_MAX_TRIANGLES];

static g3d_fx b3d_katana_cm_q8_to_m_q16(long value, int percent)
{
    long scaled;
    if (percent < 1) percent = 1;
    scaled = ((value * (long)percent) * 256L) / 10000L;
    if (scaled > 2147483647L) return (g3d_fx)2147483647L;
    if (scaled < (-2147483647L - 1L))
        return (g3d_fx)(-2147483647L - 1L);
    return (g3d_fx)scaled;
}

static void b3d_katana_zero_vertex(g3d_vertex *vertex)
{
    if (!vertex) return;
    memset(vertex, 0, sizeof(*vertex));
    vertex->normal.y = G3D_FX_ONE;
    vertex->color = g3d_color_rgba(255U, 255U, 255U, 255U);
}

int blank3d_katana_mesh_build(g3d_mesh *destination,
                              g3d_vertex *vertices,
                              unsigned short vertex_capacity,
                              g3d_index *indices,
                              unsigned short index_capacity,
                              unsigned short preset_index,
                              int thickness_percent,
                              int width_percent,
                              int length_percent)
{
    km89_mesh source;
    km89_build_info info;
    const km89_desc *desc;
    const km89_palette *palette;
    unsigned long required;
    unsigned short i;
    int result;

    if (!destination || !vertices || !indices) return 0;
    if (preset_index >= km89_preset_count()) preset_index = 0U;
    desc = km89_preset(preset_index);
    if (!desc) return 0;
    palette = km89_palette_get(desc->palette_id);
    if (!palette) return 0;

    km89_mesh_init(&source,
                   b3d_katana_source_vertices, KM89_MAX_VERTICES,
                   b3d_katana_source_triangles, KM89_MAX_TRIANGLES);
    result = km89_build_preset(preset_index, &source, &info);
    if (result != KM89_OK) return 0;
    (void)info;

    required = (unsigned long)source.triangle_count * 3UL;
    if (required > (unsigned long)vertex_capacity ||
        required > (unsigned long)index_capacity ||
        required > 65535UL) return 0;
    if (g3d_mesh_init(destination, vertices, vertex_capacity,
                      indices, index_capacity) != G3D_OK) return 0;

    for (i = 0U; i < source.triangle_count; ++i) {
        const km89_triangle *triangle;
        const km89_vertex *sv[3];
        g3d_vec3 p[3];
        g3d_vec3 edge_a;
        g3d_vec3 edge_b;
        g3d_vec3 normal;
        g3d_color color;
        unsigned short base;
        unsigned char material;
        int j;

        triangle = &source.triangles[i];
        if (triangle->a >= source.vertex_count ||
            triangle->b >= source.vertex_count ||
            triangle->c >= source.vertex_count) return 0;
        sv[0] = &source.vertices[triangle->a];
        sv[1] = &source.vertices[triangle->b];
        sv[2] = &source.vertices[triangle->c];
        for (j = 0; j < 3; ++j) {
            p[j].x = b3d_katana_cm_q8_to_m_q16(
                sv[j]->x, thickness_percent);
            p[j].y = b3d_katana_cm_q8_to_m_q16(
                sv[j]->y, width_percent);
            p[j].z = b3d_katana_cm_q8_to_m_q16(
                sv[j]->z, length_percent);
        }
        edge_a = g3d_vec3_sub(p[1], p[0]);
        edge_b = g3d_vec3_sub(p[2], p[0]);
        normal = g3d_vec3_normalize(g3d_vec3_cross(edge_a, edge_b));
        if (g3d_vec3_length_sq(normal) <= G3D_FX_EPSILON)
            normal = g3d_vec3_make(0, G3D_FX_ONE, 0);
        material = triangle->material;
        if (material >= KM89_MAT_COUNT) material = KM89_MAT_STEEL;
        color = g3d_color_rgba(palette->rgb[material][0],
                               palette->rgb[material][1],
                               palette->rgb[material][2], 255U);
        base = (unsigned short)((unsigned long)i * 3UL);
        for (j = 0; j < 3; ++j) {
            b3d_katana_zero_vertex(&vertices[base + (unsigned short)j]);
            vertices[base + (unsigned short)j].position = p[j];
            vertices[base + (unsigned short)j].normal = normal;
            vertices[base + (unsigned short)j].color = color;
            indices[base + (unsigned short)j] =
                (g3d_index)(base + (unsigned short)j);
        }
    }
    destination->vertex_count = (unsigned short)required;
    destination->index_count = (unsigned short)required;
    destination->primitive = G3D_PRIMITIVE_TRIANGLES;
    destination->error = G3D_OK;
    return 1;
}
