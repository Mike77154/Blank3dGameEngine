#include <stdio.h>
#include <string.h>
#include "png_decoder.h"
#include "png_mem89.h"
int main(void)
{
    static png_u8 src[32u*24u*4u];
    png_u8* enc;
    png_u32 enc_size;
    png_encode_options eo;
    png_decode_options dopt;
    png_image img;
    png_u32 i;
    int err;
    for(i=0u;i<(png_u32)sizeof(src);++i) src[i]=(png_u8)((i*37u+i/7u)&255u);
    png_mem89_reset();
    png_encode_options_init(&eo);
    eo.color_type=PNG_COLOR_TRUECOLOR_ALPHA;
    eo.bit_depth=8u;
    eo.interlace_method=1u;
    eo.write_gAMA=1;
    eo.image_gamma=png_fixed89_from_ratio(45455,100000);
    enc=0; enc_size=0u;
    err=png_encode_memory_ex_zlib(src,32u,24u,&eo,&enc,&enc_size);
    if(err!=PNG_DEC_OK){printf("ENC_FAIL %d\n",err);return 2;}
    png_decode_options_init(&dopt);
    dopt.output_format=PNG_OUTPUT_RGBA8;
    dopt.transform_flags=PNG_DEC_TRANSFORM_NONE;
    err=png_decode_memory_ex_zlib(enc,enc_size,&dopt,&img);
    if(err!=PNG_DEC_OK){printf("DEC_FAIL %d\n",err);png_free_file(enc);return 3;}
    if(img.pixel_rowbytes!=32u*4u || img.height!=24u || memcmp(src,img.pixels,sizeof(src))!=0){printf("PIXEL_MISMATCH\n");png_free_image(&img);png_free_file(enc);return 4;}
    png_free_image(&img);
    png_free_file(enc);
    if(png_mem89_bytes_used()!=0u){printf("LEAK %u\n",png_mem89_bytes_used());return 5;}
    printf("ROUNDTRIP_OK encoded=%u peak=%u\n",enc_size,png_mem89_peak_used());
    return 0;
}
