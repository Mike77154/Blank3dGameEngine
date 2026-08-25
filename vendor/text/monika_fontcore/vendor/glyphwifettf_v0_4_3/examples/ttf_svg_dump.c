#include "glyphwifettf.h"
#include "common_io.h"

static GWT_Point points[8192];
static GWT_Contour contours[512];
static GWT_Segment segs[16384];

static int fp_int(gwt_fixed v){ return (int)(v >> GWT_FP_SHIFT); }

int main(int argc, char **argv){
    GWT_Font font; GWT_Outline out; GWT_SegmentBuffer sb; GWT_GlyphMetrics m; unsigned long size, cp; unsigned short gid, i; int r, w, h;
    if(argc < 3){ fprintf(stderr,"usage: %s font.ttf codepoint_decimal > glyph.svg\n", argv[0]); return 1; }
    size=read_file_static(argv[1],g_font_buf,FONT_BUF_MAX); if(!size)return 1;
    r=gwt_font_init(&font,g_font_buf,size); if(r)return 1; cp=(unsigned long)strtoul(argv[2],0,10); r=gwt_codepoint_to_glyph(&font,cp,&gid); if(r)return 1;
    out.points=points; out.max_points=8192; out.contours=contours; out.max_contours=512; gwt_outline_reset(&out); gwt_get_glyph_metrics(&font,gid,&m); r=gwt_decode_glyph_outline(&font,gid,&out); if(r)return 1;
    sb.segments=segs; sb.max_segments=16384; sb.count=0; r=gwt_outline_to_segments(&out,&sb); if(r)return 1;
    w=(m.x_max-m.x_min)+40; h=(m.y_max-m.y_min)+40;
    printf("<svg xmlns='http://www.w3.org/2000/svg' viewBox='%d %d %d %d'>\n", m.x_min-20, -m.y_max-20, w, h);
    printf("<path d='");
    for(i=0;i<sb.count;i++){
        GWT_Segment *s=&sb.segments[i];
        if(s->type==0) printf("M %d %d ", fp_int(s->x0), -fp_int(s->y0));
        else if(s->type==1) printf("L %d %d ", fp_int(s->x1), -fp_int(s->y1));
        else if(s->type==2) printf("Q %d %d %d %d ", fp_int(s->x1), -fp_int(s->y1), fp_int(s->x2), -fp_int(s->y2));
        else printf("Z ");
    }
    printf("' fill='black'/>\n</svg>\n");
    return 0;
}
