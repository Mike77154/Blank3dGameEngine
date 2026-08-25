#include "glyphwifettf.h"
#include "common_io.h"

static GWT_Point points[8192];
static GWT_Contour contours[512];
static GWT_Segment segs[16384];

static int file_writer(void *user, const char *text){
    FILE *fp;
    fp = (FILE*)user;
    fputs(text, fp);
    return ferror(fp) ? 1 : 0;
}

int main(int argc, char **argv){
    GWT_Font font;
    GWT_Outline out;
    GWT_SegmentBuffer sb;
    GWT_SvgOptions opt;
    unsigned long size, cp;
    int r;
    if(argc < 3){
        fprintf(stderr,"usage: %s font.ttf codepoint_decimal [points]\n", argv[0]);
        return 1;
    }
    size = read_file_static(argv[1], g_font_buf, FONT_BUF_MAX);
    if(!size){ fprintf(stderr,"could not read font\n"); return 1; }
    r = gwt_font_init(&font, g_font_buf, size);
    if(r != GWT_OK){ fprintf(stderr,"font init: %s\n", gwt_error_string(r)); return 1; }
    cp = (unsigned long)strtoul(argv[2], 0, 10);
    out.points = points;
    out.max_points = 8192;
    out.contours = contours;
    out.max_contours = 512;
    gwt_outline_reset(&out);
    sb.segments = segs;
    sb.max_segments = 16384;
    sb.count = 0;
    opt.padding_units = 80;
    opt.flip_y = 1;
    opt.include_metrics = 1;
    opt.include_control_points = (argc >= 4) ? 1 : 0;
    opt.fill = "black";
    opt.stroke = "none";
    r = gwt_svg_write_glyph(&font, cp, &out, &sb, file_writer, stdout, &opt);
    if(r != GWT_OK){ fprintf(stderr,"svg: %s\n", gwt_error_string(r)); return 1; }
    return 0;
}
