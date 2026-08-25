#include "glyphwifettf.h"

/*
   GlyphWifeTTF SVG debug/export unit.
   Kept outside the core file so freestanding/game runtimes can omit it.
   No stdio/sprintf: all text is emitted through the caller-provided writer.
*/

static int gwt_svg_emit(GWT_SvgWriteFn writer, void *user, const char *text){
    if(!writer || !text) return GWT_ERR_BAD_ARG;
    return writer(user, text) ? GWT_ERR_BAD_ARG : GWT_OK;
}

static int gwt_svg_emit_char(GWT_SvgWriteFn writer, void *user, char ch){
    char s[2];
    s[0] = ch;
    s[1] = 0;
    return gwt_svg_emit(writer, user, s);
}

static int gwt_svg_emit_ulong_dec(GWT_SvgWriteFn writer, void *user, unsigned long v){
    char buf[16];
    unsigned short n;
    n = 0;
    if(v == 0UL) return gwt_svg_emit_char(writer, user, '0');
    while(v && n < (unsigned short)(sizeof(buf))){
        buf[n++] = (char)('0' + (char)(v % 10UL));
        v /= 10UL;
    }
    while(n){
        int r;
        r = gwt_svg_emit_char(writer, user, buf[--n]);
        if(r != GWT_OK) return r;
    }
    return GWT_OK;
}

static int gwt_svg_emit_long_dec(GWT_SvgWriteFn writer, void *user, long v){
    if(v < 0){
        int r;
        r = gwt_svg_emit_char(writer, user, '-');
        if(r != GWT_OK) return r;
        return gwt_svg_emit_ulong_dec(writer, user, (unsigned long)(-v));
    }
    return gwt_svg_emit_ulong_dec(writer, user, (unsigned long)v);
}

static int gwt_svg_emit_hex4(GWT_SvgWriteFn writer, void *user, unsigned long v){
    char buf[9];
    unsigned short i, started;
    const char *hex;
    hex = "0123456789ABCDEF";
    for(i=0; i<8; i++){
        unsigned short shift;
        shift = (unsigned short)((7U - i) * 4U);
        buf[i] = hex[(v >> shift) & 15UL];
    }
    buf[8] = 0;
    started = 0;
    for(i=0; i<8; i++){
        if(buf[i] != '0' || i >= 4U) started = 1;
        if(started){
            int r = gwt_svg_emit_char(writer, user, buf[i]);
            if(r != GWT_OK) return r;
        }
    }
    return GWT_OK;
}

static int gwt_svg_emit_attr_escaped(GWT_SvgWriteFn writer, void *user, const char *s){
    int r;
    if(!s) return GWT_OK;
    while(*s){
        if(*s == '&') r = gwt_svg_emit(writer, user, "&amp;");
        else if(*s == '"') r = gwt_svg_emit(writer, user, "&quot;");
        else if(*s == '<') r = gwt_svg_emit(writer, user, "&lt;");
        else if(*s == '>') r = gwt_svg_emit(writer, user, "&gt;");
        else r = gwt_svg_emit_char(writer, user, *s);
        if(r != GWT_OK) return r;
        s++;
    }
    return GWT_OK;
}

static int gwt_svg_emit_coord_pair(GWT_SvgWriteFn writer, void *user, const char *prefix, long x, long y){
    int r;
    r = gwt_svg_emit(writer, user, prefix); if(r != GWT_OK) return r;
    r = gwt_svg_emit_long_dec(writer, user, x); if(r != GWT_OK) return r;
    r = gwt_svg_emit_char(writer, user, ' '); if(r != GWT_OK) return r;
    r = gwt_svg_emit_long_dec(writer, user, y); if(r != GWT_OK) return r;
    return gwt_svg_emit_char(writer, user, ' ');
}

static int gwt_svg_fp_to_int_round(gwt_fixed v){
    if(v >= 0) return (int)((v + GWT_FP_HALF) >> GWT_FP_SHIFT);
    return -(int)(((-v) + GWT_FP_HALF) >> GWT_FP_SHIFT);
}

static int gwt_svg_coord_y(gwt_fixed y, unsigned char flip_y){
    int iy;
    iy = gwt_svg_fp_to_int_round(y);
    return flip_y ? -iy : iy;
}

int gwt_svg_write_path_d(const GWT_Outline *outline, GWT_SegmentBuffer *segments, GWT_SvgWriteFn writer, void *user, unsigned char flip_y){
    unsigned short i;
    int r;
    if(!outline || !segments || !segments->segments || !writer) return GWT_ERR_BAD_ARG;
    r = gwt_outline_to_segments(outline, segments);
    if(r != GWT_OK) return r;
    for(i=0; i<segments->count; i++){
        GWT_Segment *s;
        s = &segments->segments[i];
        if(s->type == 0){
            r = gwt_svg_emit_coord_pair(writer, user, "M ", (long)gwt_svg_fp_to_int_round(s->x0), (long)gwt_svg_coord_y(s->y0, flip_y));
        } else if(s->type == 1){
            r = gwt_svg_emit_coord_pair(writer, user, "L ", (long)gwt_svg_fp_to_int_round(s->x1), (long)gwt_svg_coord_y(s->y1, flip_y));
        } else if(s->type == 2){
            r = gwt_svg_emit_coord_pair(writer, user, "Q ", (long)gwt_svg_fp_to_int_round(s->x1), (long)gwt_svg_coord_y(s->y1, flip_y));
            if(r == GWT_OK) r = gwt_svg_emit_coord_pair(writer, user, "", (long)gwt_svg_fp_to_int_round(s->x2), (long)gwt_svg_coord_y(s->y2, flip_y));
        } else {
            r = gwt_svg_emit(writer, user, "Z ");
        }
        if(r != GWT_OK) return r;
    }
    return GWT_OK;
}

static void gwt_svg_default_options(GWT_SvgOptions *o){
    o->padding_units = 64;
    o->flip_y = 1;
    o->include_metrics = 1;
    o->include_control_points = 0;
    o->fill = "black";
    o->stroke = "none";
}

static int gwt_svg_emit_open_svg(GWT_SvgWriteFn writer, void *user, int vx, int vy, int vw, int vh){
    int r;
    r = gwt_svg_emit(writer, user, "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\""); if(r != GWT_OK) return r;
    r = gwt_svg_emit_long_dec(writer, user, (long)vx); if(r != GWT_OK) return r;
    r = gwt_svg_emit_char(writer, user, ' '); if(r != GWT_OK) return r;
    r = gwt_svg_emit_long_dec(writer, user, (long)vy); if(r != GWT_OK) return r;
    r = gwt_svg_emit_char(writer, user, ' '); if(r != GWT_OK) return r;
    r = gwt_svg_emit_long_dec(writer, user, (long)vw); if(r != GWT_OK) return r;
    r = gwt_svg_emit_char(writer, user, ' '); if(r != GWT_OK) return r;
    r = gwt_svg_emit_long_dec(writer, user, (long)vh); if(r != GWT_OK) return r;
    return gwt_svg_emit(writer, user, "\">\n");
}

static int gwt_svg_emit_metadata(GWT_SvgWriteFn writer, void *user, const GWT_Font *font, const GWT_GlyphMetrics *m){
    int r;
    r = gwt_svg_emit(writer, user, "  <metadata>advance="); if(r != GWT_OK) return r;
    r = gwt_svg_emit_long_dec(writer, user, (long)m->advance_width); if(r != GWT_OK) return r;
    r = gwt_svg_emit(writer, user, " lsb="); if(r != GWT_OK) return r;
    r = gwt_svg_emit_long_dec(writer, user, (long)m->left_side_bearing); if(r != GWT_OK) return r;
    r = gwt_svg_emit(writer, user, " bbox="); if(r != GWT_OK) return r;
    r = gwt_svg_emit_long_dec(writer, user, (long)m->x_min); if(r != GWT_OK) return r;
    r = gwt_svg_emit_char(writer, user, ','); if(r != GWT_OK) return r;
    r = gwt_svg_emit_long_dec(writer, user, (long)m->y_min); if(r != GWT_OK) return r;
    r = gwt_svg_emit_char(writer, user, ','); if(r != GWT_OK) return r;
    r = gwt_svg_emit_long_dec(writer, user, (long)m->x_max); if(r != GWT_OK) return r;
    r = gwt_svg_emit_char(writer, user, ','); if(r != GWT_OK) return r;
    r = gwt_svg_emit_long_dec(writer, user, (long)m->y_max); if(r != GWT_OK) return r;
    r = gwt_svg_emit(writer, user, " unitsPerEm="); if(r != GWT_OK) return r;
    r = gwt_svg_emit_ulong_dec(writer, user, (unsigned long)font->units_per_em); if(r != GWT_OK) return r;
    return gwt_svg_emit(writer, user, "</metadata>\n");
}

int gwt_svg_write_glyph(const GWT_Font *font, unsigned long codepoint, GWT_Outline *scratch_outline, GWT_SegmentBuffer *scratch_segments, GWT_SvgWriteFn writer, void *user, const GWT_SvgOptions *options){
    GWT_SvgOptions opt;
    GWT_GlyphMetrics m;
    unsigned short gid;
    int r;
    int vx, vy, vw, vh;
    unsigned short ci, pi;
    if(!font || !scratch_outline || !scratch_segments || !writer) return GWT_ERR_BAD_ARG;
    if(options) opt = *options; else gwt_svg_default_options(&opt);
    if(!opt.fill) opt.fill = "black";
    if(!opt.stroke) opt.stroke = "none";
    r = gwt_codepoint_to_glyph(font, codepoint, &gid);
    if(r != GWT_OK) return r;
    r = gwt_get_glyph_metrics(font, gid, &m);
    if(r != GWT_OK) return r;
    r = gwt_decode_glyph_outline(font, gid, scratch_outline);
    if(r != GWT_OK) return r;
    vx = (int)m.x_min - (int)opt.padding_units;
    vw = (int)(m.x_max - m.x_min) + ((int)opt.padding_units * 2);
    if(opt.flip_y) vy = -((int)m.y_max + (int)opt.padding_units);
    else vy = (int)m.y_min - (int)opt.padding_units;
    vh = (int)(m.y_max - m.y_min) + ((int)opt.padding_units * 2);

    r = gwt_svg_emit_open_svg(writer, user, vx, vy, vw, vh); if(r != GWT_OK) return r;
    r = gwt_svg_emit(writer, user, "  <title>U+"); if(r != GWT_OK) return r;
    r = gwt_svg_emit_hex4(writer, user, codepoint); if(r != GWT_OK) return r;
    r = gwt_svg_emit(writer, user, " glyph "); if(r != GWT_OK) return r;
    r = gwt_svg_emit_ulong_dec(writer, user, (unsigned long)gid); if(r != GWT_OK) return r;
    r = gwt_svg_emit(writer, user, "</title>\n"); if(r != GWT_OK) return r;
    if(opt.include_metrics){
        r = gwt_svg_emit_metadata(writer, user, font, &m); if(r != GWT_OK) return r;
    }
    r = gwt_svg_emit(writer, user, "  <path fill=\""); if(r != GWT_OK) return r;
    r = gwt_svg_emit_attr_escaped(writer, user, opt.fill); if(r != GWT_OK) return r;
    r = gwt_svg_emit(writer, user, "\" stroke=\""); if(r != GWT_OK) return r;
    r = gwt_svg_emit_attr_escaped(writer, user, opt.stroke); if(r != GWT_OK) return r;
    r = gwt_svg_emit(writer, user, "\" d=\""); if(r != GWT_OK) return r;
    r = gwt_svg_write_path_d(scratch_outline, scratch_segments, writer, user, opt.flip_y);
    if(r != GWT_OK) return r;
    r = gwt_svg_emit(writer, user, "\"/>\n"); if(r != GWT_OK) return r;
    if(opt.include_metrics){
        int base_y;
        base_y = 0;
        r = gwt_svg_emit(writer, user, "  <line x1=\""); if(r != GWT_OK) return r;
        r = gwt_svg_emit_long_dec(writer, user, (long)vx); if(r != GWT_OK) return r;
        r = gwt_svg_emit(writer, user, "\" y1=\""); if(r != GWT_OK) return r;
        r = gwt_svg_emit_long_dec(writer, user, (long)base_y); if(r != GWT_OK) return r;
        r = gwt_svg_emit(writer, user, "\" x2=\""); if(r != GWT_OK) return r;
        r = gwt_svg_emit_long_dec(writer, user, (long)(vx + vw)); if(r != GWT_OK) return r;
        r = gwt_svg_emit(writer, user, "\" y2=\""); if(r != GWT_OK) return r;
        r = gwt_svg_emit_long_dec(writer, user, (long)base_y); if(r != GWT_OK) return r;
        r = gwt_svg_emit(writer, user, "\" stroke=\"#00a\" stroke-width=\"8\" opacity=\"0.35\"/>\n"); if(r != GWT_OK) return r;

        r = gwt_svg_emit(writer, user, "  <rect x=\""); if(r != GWT_OK) return r;
        r = gwt_svg_emit_long_dec(writer, user, (long)m.x_min); if(r != GWT_OK) return r;
        r = gwt_svg_emit(writer, user, "\" y=\""); if(r != GWT_OK) return r;
        r = gwt_svg_emit_long_dec(writer, user, (long)(opt.flip_y ? -(int)m.y_max : (int)m.y_min)); if(r != GWT_OK) return r;
        r = gwt_svg_emit(writer, user, "\" width=\""); if(r != GWT_OK) return r;
        r = gwt_svg_emit_long_dec(writer, user, (long)(m.x_max - m.x_min)); if(r != GWT_OK) return r;
        r = gwt_svg_emit(writer, user, "\" height=\""); if(r != GWT_OK) return r;
        r = gwt_svg_emit_long_dec(writer, user, (long)(m.y_max - m.y_min)); if(r != GWT_OK) return r;
        r = gwt_svg_emit(writer, user, "\" fill=\"none\" stroke=\"#f0f\" stroke-width=\"8\" opacity=\"0.35\"/>\n"); if(r != GWT_OK) return r;
    }
    if(opt.include_control_points){
        r = gwt_svg_emit(writer, user, "  <g id=\"control-points\">\n"); if(r != GWT_OK) return r;
        for(ci=0; ci<scratch_outline->contour_count; ci++){
            GWT_Contour c;
            c = scratch_outline->contours[ci];
            for(pi=0; pi<c.point_count; pi++){
                GWT_Point pt;
                int cx, cy;
                pt = scratch_outline->points[c.first_point + pi];
                cx = gwt_svg_fp_to_int_round(pt.x);
                cy = gwt_svg_coord_y(pt.y, opt.flip_y);
                r = gwt_svg_emit(writer, user, "    <circle cx=\""); if(r != GWT_OK) return r;
                r = gwt_svg_emit_long_dec(writer, user, (long)cx); if(r != GWT_OK) return r;
                r = gwt_svg_emit(writer, user, "\" cy=\""); if(r != GWT_OK) return r;
                r = gwt_svg_emit_long_dec(writer, user, (long)cy); if(r != GWT_OK) return r;
                if(pt.flags & GWT_POINT_ON_CURVE) r = gwt_svg_emit(writer, user, "\" r=\"18\" fill=\"#00aa00\"/>\n");
                else r = gwt_svg_emit(writer, user, "\" r=\"18\" fill=\"#cc0000\"/>\n");
                if(r != GWT_OK) return r;
            }
        }
        r = gwt_svg_emit(writer, user, "  </g>\n"); if(r != GWT_OK) return r;
    }
    return gwt_svg_emit(writer, user, "</svg>\n");
}
