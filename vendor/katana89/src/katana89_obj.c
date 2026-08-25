#include "katana89_obj.h"
#include <stdio.h>

static void km89_print_fp(FILE *f,long v)
{
    long a=v<0?-v:v;
    long whole=a/KM89_FP_ONE;
    long frac=((a%KM89_FP_ONE)*10000L)/KM89_FP_ONE;
    if(v<0)fputc('-',f);
    fprintf(f,"%ld.%04ld",whole,frac);
}

int km89_write_obj(FILE *obj, FILE *mtl, const char *mtl_name, const km89_mesh *mesh, const km89_palette *palette)
{
    unsigned short i;
    int current=-1;
    if(!obj||!mesh||!palette)return KM89_ERR_ARGUMENT;
    fprintf(obj,"# katana89 C89 fixed-point generated mesh\n");
    if(mtl_name)fprintf(obj,"mtllib %s\n",mtl_name);
    for(i=0;i<mesh->vertex_count;i++) {
        fputs("v ",obj);km89_print_fp(obj,mesh->vertices[i].x);fputc(' ',obj);
        km89_print_fp(obj,mesh->vertices[i].y);fputc(' ',obj);
        km89_print_fp(obj,mesh->vertices[i].z);fputc('\n',obj);
    }
    for(i=0;i<mesh->triangle_count;i++) {
        const km89_triangle *t=&mesh->triangles[i];
        if((int)t->material!=current){current=t->material;fprintf(obj,"usemtl mat_%d\n",current);}
        fprintf(obj,"f %u %u %u\n",(unsigned)t->a+1u,(unsigned)t->b+1u,(unsigned)t->c+1u);
    }
    if(mtl) {
        static const char *names[KM89_MAT_COUNT]={"steel","edge","hamon","guard","wrap","same","fitting","lacquer","groove"};
        for(i=0;i<KM89_MAT_COUNT;i++) {
            fprintf(mtl,"newmtl mat_%u\n# %s\nKd %u.%03u %u.%03u %u.%03u\nKa 0.050 0.050 0.050\nKs 0.350 0.350 0.350\nNs 48\n\n",
                (unsigned)i,names[i],
                (unsigned)(palette->rgb[i][0]/255u),(unsigned)((palette->rgb[i][0]%255u)*1000u/255u),
                (unsigned)(palette->rgb[i][1]/255u),(unsigned)((palette->rgb[i][1]%255u)*1000u/255u),
                (unsigned)(palette->rgb[i][2]/255u),(unsigned)((palette->rgb[i][2]%255u)*1000u/255u));
        }
    }
    return KM89_OK;
}
