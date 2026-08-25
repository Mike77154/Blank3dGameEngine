#ifndef GF_SVG_PATH_H
#define GF_SVG_PATH_H
#include "gf_outline.h"

/* Minimal no-heap SVG path subset parser: M/m, L/l, Q/q, Z/z with integer coordinates. */
int gf_svg_path_to_glyph(GF_Glyph *g, const char *path, unsigned int codepoint, long advance_x, int canvas_h);

#endif
