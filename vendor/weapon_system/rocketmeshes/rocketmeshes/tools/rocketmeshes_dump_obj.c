/* rocketmeshes_dump_obj.c - test exporter. C89; no heap. */
#include <stdio.h>
#include <stdlib.h>
#include "rocketmeshes.h"

static void print_q8(FILE *fp, int v)
{
    int neg;
    int whole;
    int rem;
    neg = 0;
    if (v < 0) {
        neg = 1;
        v = -v;
    }
    whole = v >> RMESH_Q;
    rem = ((v & (RMESH_ONE - 1)) * 1000) >> RMESH_Q;
    if (neg) {
        fputc('-', fp);
    }
    fprintf(fp, "%d.%03d", whole, rem);
}

static void write_obj(const RM_Mesh *m, FILE *fp)
{
    unsigned int i;
    unsigned int last_mat;
    fprintf(fp, "# rocketmeshes OBJ dump: %s\n", m->name);
    fprintf(fp, "# vertices=%u triangles=%u scale=Q%d\n", (unsigned int)m->vcount, (unsigned int)m->tcount, RMESH_Q);
    fprintf(fp, "o mesh_%s\n", m->name);
    for (i = 0; i < (unsigned int)m->vcount; ++i) {
        fputs("v ", fp);
        print_q8(fp, m->v[i].x); fputc(' ', fp);
        print_q8(fp, m->v[i].y); fputc(' ', fp);
        print_q8(fp, m->v[i].z); fputc('\n', fp);
    }
    last_mat = 999u;
    for (i = 0; i < (unsigned int)m->tcount; ++i) {
        if ((unsigned int)m->t[i].mat != last_mat) {
            last_mat = (unsigned int)m->t[i].mat;
            fprintf(fp, "usemtl rm_mat_%u\n", last_mat);
        }
        fprintf(fp, "f %u %u %u\n", (unsigned int)m->t[i].a + 1u, (unsigned int)m->t[i].b + 1u, (unsigned int)m->t[i].c + 1u);
    }
}

int main(int argc, char **argv)
{
    int id;
    const RM_Mesh *m;
    FILE *fp;

    if (argc < 3) {
        fprintf(stderr, "usage: rocketmeshes_dump_obj <mesh_id> <out.obj>\n");
        fprintf(stderr, "mesh ids: 0..%d\n", RMESH_COUNT - 1);
        return 2;
    }
    id = atoi(argv[1]);
    m = rm_get_mesh(id);
    if (!m) {
        fprintf(stderr, "bad mesh id\n");
        return 2;
    }
    fp = fopen(argv[2], "w");
    if (!fp) {
        fprintf(stderr, "could not write obj\n");
        return 1;
    }
    write_obj(m, fp);
    fclose(fp);
    return 0;
}
