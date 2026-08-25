#include <stdio.h>
#include "imgcc0.h"
#define OUT_CAP (16U * 1024U * 1024U)
#define TMP_CAP (16U * 1024U * 1024U)
#define FILE_CAP (8U * 1024U * 1024U)
IMGCC0_DECLARE_BUFFER(g_out, OUT_CAP);
IMGCC0_DECLARE_BUFFER(g_tmp, TMP_CAP);
IMGCC0_DECLARE_BUFFER(g_file, FILE_CAP);
static void put_u32(FILE *f, unsigned int v){ unsigned char b[4]; b[0]=(unsigned char)v; b[1]=(unsigned char)(v>>8); b[2]=(unsigned char)(v>>16); b[3]=(unsigned char)(v>>24); fwrite(b,1U,4U,f); }
int main(int argc,char**argv){imgcc0_image im;imgcc0_open_options o;FILE*f;unsigned int i,bytes;int rc;if(argc!=3)return 2;imgcc0_image_init(&im);imgcc0_open_options_init(&o);o.output_buffer=IMGCC0_BUFFER_DATA(g_out);o.output_buffer_size=IMGCC0_BUFFER_SIZE(g_out);o.temp_buffer=IMGCC0_BUFFER_DATA(g_tmp);o.temp_buffer_size=IMGCC0_BUFFER_SIZE(g_tmp);o.file_buffer=IMGCC0_BUFFER_DATA(g_file);o.file_buffer_size=IMGCC0_BUFFER_SIZE(g_file);rc=imgcc0_open_file(argv[1],&o,&im);if(rc!=IMGCC0_OK){fprintf(stderr,"%d %s\n",rc,im.error_message);return 10;}f=fopen(argv[2],"wb");if(!f)return 11;fwrite("IC0TEST1",1U,8U,f);put_u32(f,im.width);put_u32(f,im.height);put_u32(f,im.frame_count);put_u32(f,(unsigned int)im.loop_count);for(i=0U;i<im.frame_count;++i){put_u32(f,im.frames[i].delay_ms);put_u32(f,im.frames[i].stride);bytes=im.frames[i].stride*im.frames[i].height;if(fwrite(im.frames[i].pixels,1U,bytes,f)!=bytes){fclose(f);return 12;}}fclose(f);return 0;}
