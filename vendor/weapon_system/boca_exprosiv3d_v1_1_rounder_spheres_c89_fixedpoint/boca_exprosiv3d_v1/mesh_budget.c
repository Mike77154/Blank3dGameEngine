#include <stdio.h>
#include "boca_exprosiv3d.h"

#define PROBE_VERTEX_CAPACITY 4096UL
#define PROBE_INDEX_CAPACITY  32768UL

static BEX3D_Vertex probe_vertices[PROBE_VERTEX_CAPACITY];
static unsigned short probe_indices[PROBE_INDEX_CAPACITY];

static const char *probe_names[BEX3D_PROFILE_COUNT] =
{
    "pistol",
    "machine_gun",
    "shotgun",
    "magnum_auto",
    "magnum_revolver",
    "precision_rifle",
    "precision_brake",
    "suppressed"
};

int main(void)
{
    BEX3D_State state;
    BEX3D_Profile profile;
    BEX3D_MeshBuffer mesh;
    unsigned long max_vertices;
    unsigned long max_indices;
    unsigned long max_age;
    unsigned long age;
    int profile_id;
    int result;

    mesh.vertices = probe_vertices;
    mesh.indices = probe_indices;
    mesh.vertex_capacity = PROBE_VERTEX_CAPACITY;
    mesh.index_capacity = PROBE_INDEX_CAPACITY;
    mesh.vertex_count = 0UL;
    mesh.index_count = 0UL;
    mesh.truncated = 0;

    for (profile_id = 0; profile_id < BEX3D_PROFILE_COUNT; ++profile_id)
    {
        bex3d_default_profile(&profile, profile_id);
        bex3d_init(&state, 0xB0CA3D00UL + (unsigned long)profile_id);
        bex3d_set_profile(&state, &profile);
        bex3d_fire(&state);

        max_vertices = 0UL;
        max_indices = 0UL;
        max_age = 0UL;

        for (age = 0UL; age < state.duration_us; age += 100UL)
        {
            state.age_us = age;
            state.active = 1;
            result = bex3d_build_mesh(&state, &mesh);
            if (result != BEX3D_BUILD_OK)
            {
                fprintf(stderr, "probe buffer too small\n");
                return 1;
            }
            if (mesh.vertex_count > max_vertices)
            {
                max_vertices = mesh.vertex_count;
                max_indices = mesh.index_count;
                max_age = age;
            }
        }

        printf("%-18s max_vertices=%4lu max_triangles=%4lu at=%4lu us\n",
               probe_names[profile_id],
               max_vertices,
               max_indices / 3UL,
               max_age);
    }

    return 0;
}
