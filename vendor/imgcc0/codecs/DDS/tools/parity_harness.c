#include <stdio.h>
#include <string.h>
#include "giffany_dds/gdds.h"

static void fill(unsigned char* p, unsigned int w, unsigned int h) {
    unsigned int x,y;
    for(y=0;y<h;++y) for(x=0;x<w;++x) {
        unsigned int i=(y*w+x)*4u;
        p[i+0]=(unsigned char)((x*17u+y*13u+3u)&255u);
        p[i+1]=(unsigned char)((x*5u+y*29u+91u)&255u);
        p[i+2]=(unsigned char)((x*31u+y*7u+47u)&255u);
        p[i+3]=(unsigned char)((x*19u+y*11u+127u)&255u);
    }
}
static int dump(const char* dir,const char* name,const void* data,unsigned int n){
    char path[512]; FILE* f; sprintf(path,"%s/%s",dir,name); f=fopen(path,"wb"); if(!f)return 0;
    if(n && fwrite(data,1,n,f)!=n){fclose(f);return 0;} fclose(f); return 1;
}
static int enc_one(const char* dir, const char* tag, gdds_format fmt, const unsigned char* rgba, unsigned int w,unsigned int h){
    gdds_encode_options eo=gdds_encode_options_default(fmt); gdds_buffer b; gdds_image im; char n[128]; gdds_result rc;
    memset(&b,0,sizeof(b)); memset(&im,0,sizeof(im));
    rc=gdds_encode_memory_rgba8(rgba,w,h,&eo,&b); if(rc!=GDDS_RESULT_OK){printf("enc fail %s %d\n",tag,(int)rc);return 0;}
    sprintf(n,"%s.dds",tag); if(!dump(dir,n,b.data,(unsigned int)b.size))return 0;
    rc=gdds_decode_memory(b.data,b.size,&im); if(rc!=GDDS_RESULT_OK){printf("dec fail %s %d\n",tag,(int)rc);return 0;}
    sprintf(n,"%s.rgba",tag); if(!dump(dir,n,im.pixels,(unsigned int)im.size))return 0;
    gdds_image_release(&im); gdds_buffer_release(&b); return 1;
}
int main(int argc,char**argv){
    unsigned char rgba[13u*11u*4u]; unsigned char tmp[13u*11u*4u]; unsigned int i;
    struct F { gdds_format f; const char* n; } fs[] = {
      {GDDS_FORMAT_RGBA8,"rgba8"},{GDDS_FORMAT_BGRA8,"bgra8"},{GDDS_FORMAT_DXT1,"dxt1"},{GDDS_FORMAT_DXT3,"dxt3"},{GDDS_FORMAT_DXT5,"dxt5"},
      {GDDS_FORMAT_RGBA8_SRGB,"rgba8s"},{GDDS_FORMAT_BGRA8_SRGB,"bgra8s"},{GDDS_FORMAT_DXT1_SRGB,"dxt1s"},{GDDS_FORMAT_DXT3_SRGB,"dxt3s"},{GDDS_FORMAT_DXT5_SRGB,"dxt5s"},
      {GDDS_FORMAT_BC4_UNORM,"bc4"},{GDDS_FORMAT_BC5_UNORM,"bc5"},{GDDS_FORMAT_BC4_SNORM,"bc4s"},{GDDS_FORMAT_BC5_SNORM,"bc5s"}
    };
    gdds_buffer b; gdds_encode_options eo; gdds_mipmap_options mo; gdds_normal_map_encode_options no; char n[128]; gdds_result rc;
    if(argc!=2)return 2; fill(rgba,13,11);
    for(i=0;i<sizeof(fs)/sizeof(fs[0]);++i) if(!enc_one(argv[1],fs[i].n,fs[i].f,rgba,13,11))return 3;
    memset(&b,0,sizeof(b)); eo=gdds_encode_options_default(GDDS_FORMAT_DXT5); mo=gdds_mipmap_options_default(); mo.mip_count=4u;
    rc=gdds_encode_memory_rgba8_auto_mips(rgba,13,11,&eo,&mo,&b); if(rc)return 4; dump(argv[1],"auto_dxt5.dds",b.data,(unsigned int)b.size); gdds_buffer_release(&b);
    memset(&b,0,sizeof(b)); eo=gdds_encode_options_default(GDDS_FORMAT_RGBA8_SRGB); mo.color_space=GDDS_MIP_COLOR_SPACE_SRGB;
    rc=gdds_encode_memory_rgba8_auto_mips(rgba,13,11,&eo,&mo,&b); if(rc)return 5; dump(argv[1],"auto_rgba8s.dds",b.data,(unsigned int)b.size); gdds_buffer_release(&b);
    no=gdds_normal_map_encode_options_default(); no.input_layout=GDDS_NORMAL_MAP_LAYOUT_XYZ_RGB; no.output_format=GDDS_FORMAT_BC5_UNORM;
    memset(&b,0,sizeof(b)); rc=gdds_encode_bc5_normal_map_rgba8(rgba,13,11,&no,&b); if(rc)return 6; dump(argv[1],"normal_bc5.dds",b.data,(unsigned int)b.size); gdds_buffer_release(&b);
    no.output_format=GDDS_FORMAT_BC5_SNORM; memset(&b,0,sizeof(b)); rc=gdds_encode_bc5_normal_map_rgba8(rgba,13,11,&no,&b); if(rc)return 7; dump(argv[1],"normal_bc5s.dds",b.data,(unsigned int)b.size); gdds_buffer_release(&b);
    no.input_layout=GDDS_NORMAL_MAP_LAYOUT_XY_RG; no.output_format=GDDS_FORMAT_BC5_UNORM; mo=gdds_mipmap_options_default(); mo.mip_count=4u; memset(&b,0,sizeof(b)); rc=gdds_encode_bc5_normal_map_rgba8_auto_mips(rgba,13,11,&no,&mo,&b); if(rc)return 8; dump(argv[1],"normal_auto.dds",b.data,(unsigned int)b.size); gdds_buffer_release(&b);
    memcpy(tmp,rgba,sizeof(tmp)); rc=gdds_normal_map_reconstruct_z_rgba8(tmp,13,11); if(rc)return 9; dump(argv[1],"reconstruct.rgba",tmp,sizeof(tmp));
    memcpy(tmp,rgba,sizeof(tmp)); rc=gdds_normal_map_normalize_rgba8(tmp,13,11); if(rc)return 10; dump(argv[1],"normalize.rgba",tmp,sizeof(tmp));
    memcpy(tmp,rgba,sizeof(tmp)); rc=gdds_normal_map_invert_y_rgba8(tmp,13,11); if(rc)return 11; dump(argv[1],"invert.rgba",tmp,sizeof(tmp));
    return 0;
}
