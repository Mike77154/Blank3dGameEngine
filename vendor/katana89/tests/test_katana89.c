#include "katana89.h"
#include <stdio.h>

static km89_vertex vertices[KM89_MAX_VERTICES];
static km89_triangle triangles[KM89_MAX_TRIANGLES];

int main(void)
{
    km89_mesh mesh;
    km89_build_info info;
    unsigned short i;
    unsigned long checksum=2166136261UL;
    km89_mesh_init(&mesh,vertices,KM89_MAX_VERTICES,triangles,KM89_MAX_TRIANGLES);
    if(km89_preset_count()<24)return 1;
    for(i=0;i<km89_preset_count();i++) {
        unsigned short j;
        int r=km89_build_preset(i,&mesh,&info);
        if(r!=KM89_OK)return 2;
        if(mesh.vertex_count==0||mesh.triangle_count==0)return 3;
        if(info.blade.triangle_count==0||info.handle.triangle_count==0)return 4;
        for(j=0;j<mesh.triangle_count;j++) {
            const km89_triangle *t=&mesh.triangles[j];
            if(t->a>=mesh.vertex_count||t->b>=mesh.vertex_count||t->c>=mesh.vertex_count)return 5;
            if(t->material>=KM89_MAT_COUNT)return 6;
            checksum^=(unsigned long)t->a;checksum*=16777619UL;
            checksum^=(unsigned long)t->b;checksum*=16777619UL;
            checksum^=(unsigned long)t->c;checksum*=16777619UL;
        }
    }
    printf("katana89: %u presets OK, checksum=%lu\n",(unsigned)km89_preset_count(),checksum);
    return 0;
}
