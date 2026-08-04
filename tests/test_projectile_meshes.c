#include <stdio.h>
#include "blank3d_projectile_mesh.h"

static g3d_mesh meshes[B3D_PROJECTILE_MESH_COUNT];
static g3d_vertex vertices[B3D_PROJECTILE_MESH_COUNT]
                           [B3D_PROJECTILE_VERTEX_CAPACITY];
static g3d_index indices[B3D_PROJECTILE_MESH_COUNT]
                         [B3D_PROJECTILE_INDEX_CAPACITY];
static g3d_mesh casing_meshes[B3D_CASING_MESH_COUNT];
static g3d_vertex casing_vertices[B3D_CASING_MESH_COUNT]
                                 [B3D_CASING_VERTEX_CAPACITY];
static g3d_index casing_indices[B3D_CASING_MESH_COUNT]
                               [B3D_CASING_INDEX_CAPACITY];

static int mesh_has_forward_axis(const g3d_mesh *mesh)
{
    unsigned short i;
    g3d_fx min_x;
    g3d_fx max_x;
    g3d_fx min_y;
    g3d_fx max_y;
    g3d_fx min_z;
    g3d_fx max_z;
    g3d_fx span_x;
    g3d_fx span_y;
    g3d_fx span_z;
    if (!mesh || mesh->vertex_count == 0U) return 0;
    min_x = max_x = mesh->vertices[0].position.x;
    min_y = max_y = mesh->vertices[0].position.y;
    min_z = max_z = mesh->vertices[0].position.z;
    for (i = 1U; i < mesh->vertex_count; ++i) {
        if (mesh->vertices[i].position.x < min_x) min_x = mesh->vertices[i].position.x;
        if (mesh->vertices[i].position.x > max_x) max_x = mesh->vertices[i].position.x;
        if (mesh->vertices[i].position.y < min_y) min_y = mesh->vertices[i].position.y;
        if (mesh->vertices[i].position.y > max_y) max_y = mesh->vertices[i].position.y;
        if (mesh->vertices[i].position.z < min_z) min_z = mesh->vertices[i].position.z;
        if (mesh->vertices[i].position.z > max_z) max_z = mesh->vertices[i].position.z;
    }
    span_x = max_x - min_x;
    span_y = max_y - min_y;
    span_z = max_z - min_z;
    return span_z >= span_x && span_z >= span_y;
}

int main(void)
{
    int i;
    for (i = 0; i < B3D_PROJECTILE_MESH_COUNT; ++i) {
        if (!blank3d_projectile_mesh_build(
                i + 1, &meshes[i], vertices[i],
                B3D_PROJECTILE_VERTEX_CAPACITY, indices[i],
                B3D_PROJECTILE_INDEX_CAPACITY)) {
            fprintf(stderr, "build failed for mesh %d\n", i + 1);
            return 1;
        }
        if (meshes[i].vertex_count == 0U || meshes[i].index_count == 0U)
            return 2;
        /* Directional projectiles use +Z as their nose. The slingshot stone
           is intentionally spherical and therefore has no preferred axis. */
        if (i != 8 && !mesh_has_forward_axis(&meshes[i])) {
            fprintf(stderr, "mesh %d is not authored along +Z: %s\n",
                    i + 1, blank3d_projectile_mesh_name(i + 1));
            return 3;
        }
    }
    if (meshes[2].vertex_count >= 30U) return 4; /* exactly one pellet */
    if (meshes[7].vertex_count != meshes[1].vertex_count) return 5;
    if (meshes[5].vertex_count < 40U) return 6;  /* real 40 mm mesh */
    if (meshes[6].vertex_count < 100U) return 7; /* real RPG mesh */
    if (meshes[8].vertex_count < 80U) return 11; /* primitive stone */
    for (i = 0; i < B3D_CASING_MESH_COUNT; ++i) {
        if (!blank3d_casing_mesh_build(
                i + 1, &casing_meshes[i], casing_vertices[i],
                B3D_CASING_VERTEX_CAPACITY, casing_indices[i],
                B3D_CASING_INDEX_CAPACITY)) {
            fprintf(stderr, "casing build failed for mesh %d\n", i + 1);
            return 8;
        }
        if (casing_meshes[i].vertex_count == 0U ||
            casing_meshes[i].index_count == 0U) return 9;
        if (!mesh_has_forward_axis(&casing_meshes[i])) {
            fprintf(stderr, "casing %d has wrong local axis: %s\n",
                    i + 1, blank3d_casing_mesh_name(i + 1));
            return 10;
        }
    }
    puts("Blank3D projectile/casing mesh provider test: OK");
    return 0;
}
