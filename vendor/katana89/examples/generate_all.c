#include "katana89.h"
#include "katana89_obj.h"
#include <stdio.h>
#include <string.h>

static km89_vertex g_vertices[KM89_MAX_VERTICES];
static km89_triangle g_triangles[KM89_MAX_TRIANGLES];

static void safe_name(char *out,const char *in)
{
    int i=0;
    while(*in&&i<63){char c=*in++;if(c>='A'&&c<='Z')c=(char)(c-'A'+'a');if((c>='a'&&c<='z')||(c>='0'&&c<='9'))out[i++]=c;else if(i>0&&out[i-1]!='_')out[i++]='_';}
    if(i>0&&out[i-1]=='_')i--;
    out[i]='\0';
}

int main(void)
{
    km89_mesh mesh;
    km89_build_info info;
    unsigned short i;
    km89_mesh_init(&mesh,g_vertices,KM89_MAX_VERTICES,g_triangles,KM89_MAX_TRIANGLES);
    for(i=0;i<km89_preset_count();i++) {
        const km89_desc *d=km89_preset(i);
        char base[64],objname[128],mtlname[128],mtlref[80];
        FILE *obj,*mtl;
        int r;
        safe_name(base,d->name);
        sprintf(objname,"generated/%02u_%s.obj",(unsigned)i,base);
        sprintf(mtlname,"generated/%02u_%s.mtl",(unsigned)i,base);
        sprintf(mtlref,"%02u_%s.mtl",(unsigned)i,base);
        r=km89_build(d,&mesh,&info);
        if(r){fprintf(stderr,"build %u failed: %d\n",(unsigned)i,r);return 1;}
        obj=fopen(objname,"w");mtl=fopen(mtlname,"w");
        if(!obj||!mtl){fprintf(stderr,"cannot open output\n");return 2;}
        km89_write_obj(obj,mtl,mtlref,&mesh,km89_palette_get(d->palette_id));
        fclose(obj);fclose(mtl);
        printf("%02u %-28s v=%u t=%u\n",(unsigned)i,d->name,(unsigned)mesh.vertex_count,(unsigned)mesh.triangle_count);
    }
    return 0;
}
