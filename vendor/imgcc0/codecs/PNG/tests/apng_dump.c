#include <stdio.h>
#include <string.h>
#include "png_decoder.h"
static void u32le(FILE* f, png_u32 v){ unsigned char b[4]; b[0]=(unsigned char)v; b[1]=(unsigned char)(v>>8); b[2]=(unsigned char)(v>>16); b[3]=(unsigned char)(v>>24); fwrite(b,1,4,f); }
int main(int argc,char**argv){ png_apng a; png_decode_options o; FILE*f; png_u32 i,total; int e; if(argc!=3)return 2; memset(&a,0,sizeof(a)); png_decode_options_init(&o); o.output_format=PNG_OUTPUT_RGBA8; o.transform_flags=PNG_DEC_TRANSFORM_NONE; e=png_load_apng_file_ex_zlib(argv[1],&o,&a); if(e!=PNG_DEC_OK){fprintf(stderr,"ERR=%d\n",e);return 10-e;} f=fopen(argv[2],"wb"); if(!f){png_free_apng(&a);return 3;} u32le(f,a.width);u32le(f,a.height);u32le(f,a.frame_count);u32le(f,a.num_plays); for(i=0;i<a.frame_count;++i){u32le(f,a.frames[i].control.width);u32le(f,a.frames[i].control.height);u32le(f,a.frames[i].pixel_rowbytes);total=a.frames[i].pixel_rowbytes*a.height;fwrite(a.frames[i].pixels,1,total,f);} fclose(f);png_free_apng(&a);return 0;}
