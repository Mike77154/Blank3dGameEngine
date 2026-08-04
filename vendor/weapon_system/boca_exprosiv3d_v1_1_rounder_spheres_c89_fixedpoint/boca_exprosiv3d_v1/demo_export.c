#include <stdio.h>
#include "boca_exprosiv3d.h"

#define DEMO_VERTEX_CAPACITY 4096UL
#define DEMO_INDEX_CAPACITY  32768UL

static BEX3D_Vertex demo_vertices[DEMO_VERTEX_CAPACITY];
static unsigned short demo_indices[DEMO_INDEX_CAPACITY];

static const char *profile_names[BEX3D_PROFILE_COUNT] =
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

static void write_fixed(FILE *file, BEX3D_Fixed value)
{
    BEX3D_Fixed absolute_value;
    BEX3D_Fixed whole;
    BEX3D_Fixed fraction;

    if (value < 0)
    {
        fputc('-', file);
        absolute_value = -value;
    }
    else
    {
        absolute_value = value;
    }

    whole = absolute_value >> BEX3D_FP_SHIFT;
    fraction = ((absolute_value & (BEX3D_FP_ONE - 1L)) * 1000L) /
               BEX3D_FP_ONE;
    fprintf(file, "%ld.%03ld", whole, fraction);
}

static int write_obj(const char *filename, const BEX3D_MeshBuffer *mesh)
{
    FILE *file;
    unsigned long i;

    file = fopen(filename, "w");
    if (file == (FILE *)0)
    {
        return 0;
    }

    fprintf(file, "# boca_exprosiv3d generated mesh\n");
    fprintf(file, "# coordinates are centimeters\n");
    fprintf(file, "# vertices: %lu indices: %lu\n",
            mesh->vertex_count, mesh->index_count);

    for (i = 0UL; i < mesh->vertex_count; ++i)
    {
        fputs("v ", file);
        write_fixed(file, mesh->vertices[i].x);
        fputc(' ', file);
        write_fixed(file, mesh->vertices[i].y);
        fputc(' ', file);
        write_fixed(file, mesh->vertices[i].z);
        fputc('\n', file);
    }

    for (i = 0UL; i + 2UL < mesh->index_count; i += 3UL)
    {
        fprintf(file, "f %u %u %u\n",
                (unsigned int)mesh->indices[i] + 1U,
                (unsigned int)mesh->indices[i + 1UL] + 1U,
                (unsigned int)mesh->indices[i + 2UL] + 1U);
    }

    fclose(file);
    return 1;
}

int main(void)
{
    BEX3D_State state;
    BEX3D_Profile profile;
    BEX3D_MeshBuffer mesh;
    char filename[128];
    int profile_id;
    int result;

    mesh.vertices = demo_vertices;
    mesh.indices = demo_indices;
    mesh.vertex_capacity = DEMO_VERTEX_CAPACITY;
    mesh.index_capacity = DEMO_INDEX_CAPACITY;
    mesh.vertex_count = 0UL;
    mesh.index_count = 0UL;
    mesh.truncated = 0;

    for (profile_id = 0; profile_id < BEX3D_PROFILE_COUNT; ++profile_id)
    {
        bex3d_default_profile(&profile, profile_id);
        bex3d_init(&state, 0xB0CA3D00UL + (unsigned long)profile_id);
        bex3d_set_profile(&state, &profile);
        bex3d_fire(&state);
        bex3d_update_us(&state, 900UL);

        result = bex3d_build_mesh(&state, &mesh);
        if (result != BEX3D_BUILD_OK)
        {
            fprintf(stderr, "mesh build failed for profile %s\n",
                    profile_names[profile_id]);
            return 1;
        }

        sprintf(filename, "samples/%s_0900us.obj",
                profile_names[profile_id]);
        if (!write_obj(filename, &mesh))
        {
            fprintf(stderr, "could not write %s\n", filename);
            return 1;
        }

        printf("%-18s primitives=%2d vertices=%4lu triangles=%4lu\n",
               profile_names[profile_id],
               bex3d_visible_primitive_count(&state),
               mesh.vertex_count,
               mesh.index_count / 3UL);
    }

    return 0;
}
