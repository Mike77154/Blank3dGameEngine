#include "glyphwifettf.h"
#include "common_io.h"

static GWT_Point points[8192];
static GWT_Contour contours[512];

int main(int argc, char **argv){
    GWT_Font font; GWT_Outline out; GWT_GlyphMetrics m; unsigned long size, cp; unsigned short gid; int r;
    if(argc < 3){ fprintf(stderr,"usage: %s font.ttf codepoint_decimal\n", argv[0]); return 1; }
    size = read_file_static(argv[1], g_font_buf, FONT_BUF_MAX); if(!size){ fprintf(stderr,"cannot read font\n"); return 1; }
    r = gwt_font_init(&font, g_font_buf, size); if(r){ fprintf(stderr,"gwt_font_init err %d\n", r); return 1; }
    cp = (unsigned long)strtoul(argv[2],0,10);
    r = gwt_codepoint_to_glyph(&font, cp, &gid); if(r){ fprintf(stderr,"lookup err %d\n", r); return 1; }
    out.points=points; out.max_points=8192; out.contours=contours; out.max_contours=512; gwt_outline_reset(&out);
    r = gwt_get_glyph_metrics(&font,gid,&m); if(r){ fprintf(stderr,"metrics err %d\n", r); return 1; }
    r = gwt_decode_glyph_outline(&font,gid,&out); if(r){ fprintf(stderr,"decode err %d\n", r); return 1; }
    printf("GlyphWifeTTF dump\n");
    printf("unitsPerEm=%u glyphs=%u ascent=%d descent=%d\n", font.units_per_em, font.num_glyphs, font.ascent, font.descent);
    printf("codepoint=U+%04lX glyph_id=%u\n", cp, gid);
    printf("advance=%d lsb=%d bbox=(%d,%d)-(%d,%d)\n", m.advance_width, m.left_side_bearing, m.x_min, m.y_min, m.x_max, m.y_max);
    printf("outline points=%u contours=%u\n", out.point_count, out.contour_count);
    return 0;
}
