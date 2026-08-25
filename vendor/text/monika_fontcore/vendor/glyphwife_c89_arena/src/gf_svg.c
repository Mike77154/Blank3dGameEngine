#include "gf_svg.h"
#include <stdio.h>

int gf_svg_write_glyph(const char *path, const GF_Glyph *g, int canvas_w, int canvas_h) {
    FILE *fp; int ci, j, start, end; GF_Point p;
    fp = fopen(path, "wb"); if (!fp) return -1;
    fprintf(fp, "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"%d\" height=\"%d\" viewBox=\"0 0 %d %d\">\n", canvas_w, canvas_h, canvas_w, canvas_h);
    fprintf(fp, "<path fill=\"black\" fill-rule=\"evenodd\" d=\"");
    for (ci = 0; ci < (int)g->contour_count; ++ci) {
        start = g->contours[ci].start; end = start + g->contours[ci].count;
        if (start >= end) continue;
        p = g->points[start]; fprintf(fp, "M %d %d ", GF_INT(p.x), canvas_h - GF_INT(p.y));
        j = start + 1;
        while (j < end) {
            p = g->points[j];
            if (p.tag == GF_TAG_ON) { fprintf(fp, "L %d %d ", GF_INT(p.x), canvas_h - GF_INT(p.y)); j++; }
            else if (p.tag == GF_TAG_QUAD && j + 1 < end) {
                GF_Point q = g->points[j + 1];
                fprintf(fp, "Q %d %d %d %d ", GF_INT(p.x), canvas_h - GF_INT(p.y), GF_INT(q.x), canvas_h - GF_INT(q.y));
                j += 2;
            } else j++;
        }
        fprintf(fp, "Z ");
    }
    fprintf(fp, "\"/>\n</svg>\n"); fclose(fp); return 0;
}
