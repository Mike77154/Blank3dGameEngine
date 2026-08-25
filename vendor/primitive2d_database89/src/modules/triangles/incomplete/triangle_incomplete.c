#include "p2d89_internal.h"

static const P2D89_ShapeInfo shapes[] = {
    { P2D89_SHAPE_HOLLOW_TRIANGLE, "hollow_triangle", "triangles", "incomplete", P2D89_MODULE_TRIANGLES, P2D89_FLAG_CLOSED|P2D89_FLAG_STROKE_ONLY, P2D89_FILL_NONZERO, P2D89_CAP_ROUND, P2D89_JOIN_MITER },
};

static int p2d89_emit_triangles_incomplete(P2D89_ShapeId id,const P2D89_Provider *p){static const p2d89_q14 eq[]={0,-16384,14189,8192,-14189,8192};if(id==P2D89_SHAPE_HOLLOW_TRIANGLE)return p2d89_i_poly(eq,3U,1,p);return 0;}
const P2D89_Submodule p2d89_submodule_triangles_incomplete = { { P2D89_MODULE_TRIANGLES, "triangles", "incomplete", (unsigned int)(sizeof(shapes)/sizeof(shapes[0])) }, shapes, p2d89_emit_triangles_incomplete };
