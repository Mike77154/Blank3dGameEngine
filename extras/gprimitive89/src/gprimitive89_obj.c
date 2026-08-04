#include "gprimitive89_obj.h"

/* Optional standard-C OBJ exporter kept outside the dependency-free core. */

typedef unsigned int gp89_obj_u32;

static gp89_obj_u32 gp89_obj_abs_u32(gp89_fx value)
{
    if (value < 0) {
        return (gp89_obj_u32)(-(value + 1)) + 1U;
    }
    return (gp89_obj_u32)value;
}


static void gp89_write_fixed(FILE *file, gp89_fx value)
{
    gp89_obj_u32 magnitude;
    gp89_obj_u32 whole;
    gp89_obj_u32 remainder;
    gp89_obj_u32 digit;
    int i;

    if (value < 0) {
        fputc('-', file);
    }
    magnitude = gp89_obj_abs_u32(value);
    whole = magnitude >> 16;
    remainder = magnitude & 0xffffU;
    fprintf(file, "%u.", whole);
    for (i = 0; i < 6; ++i) {
        remainder *= 10U;
        digit = remainder >> 16;
        remainder &= 0xffffU;
        fputc((int)('0' + digit), file);
    }
}

int gp89_write_obj(FILE *file, const gp89_mesh *mesh, const char *object_name)
{
    unsigned int i;
    const gp89_vertex *vertex;
    const gp89_triangle *triangle;

    if (file == (FILE *)0 || mesh == (const gp89_mesh *)0) {
        return GP89_ERR_ARGUMENT;
    }
    fprintf(file, "# gprimitive89 generated mesh\n");
    fprintf(file, "o %s\n", object_name != (const char *)0 ? object_name : "primitive");
    for (i = 0U; i < mesh->vertex_count; ++i) {
        vertex = &mesh->vertices[i];
        fputs("v ", file);
        gp89_write_fixed(file, vertex->x);
        fputc(' ', file);
        gp89_write_fixed(file, vertex->y);
        fputc(' ', file);
        gp89_write_fixed(file, vertex->z);
        fputc('\n', file);
    }
    for (i = 0U; i < mesh->vertex_count; ++i) {
        vertex = &mesh->vertices[i];
        fputs("vt ", file);
        gp89_write_fixed(file, vertex->u);
        fputc(' ', file);
        gp89_write_fixed(file, vertex->v);
        fputc('\n', file);
    }
    for (i = 0U; i < mesh->vertex_count; ++i) {
        vertex = &mesh->vertices[i];
        fputs("vn ", file);
        gp89_write_fixed(file, vertex->nx);
        fputc(' ', file);
        gp89_write_fixed(file, vertex->ny);
        fputc(' ', file);
        gp89_write_fixed(file, vertex->nz);
        fputc('\n', file);
    }
    for (i = 0U; i < mesh->triangle_count; ++i) {
        triangle = &mesh->triangles[i];
        fprintf(file,
                "f %u/%u/%u %u/%u/%u %u/%u/%u\n",
                (unsigned int)triangle->a + 1U,
                (unsigned int)triangle->a + 1U,
                (unsigned int)triangle->a + 1U,
                (unsigned int)triangle->b + 1U,
                (unsigned int)triangle->b + 1U,
                (unsigned int)triangle->b + 1U,
                (unsigned int)triangle->c + 1U,
                (unsigned int)triangle->c + 1U,
                (unsigned int)triangle->c + 1U);
    }
    return ferror(file) ? GP89_ERR_ARGUMENT : GP89_OK;
}
