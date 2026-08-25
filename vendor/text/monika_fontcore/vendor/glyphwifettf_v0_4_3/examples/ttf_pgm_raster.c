#include "glyphwifettf.h"
#include "common_io.h"

static GWT_Point points[8192];
static GWT_Contour contours[512];
static unsigned char bmp_pixels[512*512];

int main(int argc, char **argv){
    GWT_Font font; GWT_Outline out; GWT_GlyphMetrics m; GWT_Bitmap bmp; unsigned long size, cp; unsigned short gid; int r, px, base, ox, i; gwt_fixed sc;
    if(argc < 4){ fprintf(stderr,"usage: %s font.ttf codepoint_decimal pixel_size > glyph.pgm\n", argv[0]); return 1; }
    size=read_file_static(argv[1],g_font_buf,FONT_BUF_MAX); if(!size)return 1; r=gwt_font_init(&font,g_font_buf,size); if(r)return 1;
    cp=(unsigned long)strtoul(argv[2],0,10); px=atoi(argv[3]); if(px<1 || px>256)px=64;
    r=gwt_codepoint_to_glyph(&font,cp,&gid); if(r)return 1; gwt_get_glyph_metrics(&font,gid,&m);
    out.points=points; out.max_points=8192; out.contours=contours; out.max_contours=512; gwt_outline_reset(&out); r=gwt_decode_glyph_outline(&font,gid,&out); if(r)return 1;
    for(i=0;i<512*512;i++) bmp_pixels[i]=0;
    bmp.pixels=bmp_pixels; bmp.width=512; bmp.height=512; bmp.stride=512;
    sc=gwt_make_scale(&font,px); ox=64; base=300; r=gwt_rasterize_outline_aa(&out,sc,&bmp,ox,base,255,1); if(r)return 1;
    printf("P2\n512 512\n255\n");
    for(i=0;i<512*512;i++){ printf("%u ",(unsigned)bmp_pixels[i]); if((i%512)==511)printf("\n"); }
    return 0;
}
