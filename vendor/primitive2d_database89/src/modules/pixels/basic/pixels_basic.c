#include "p2d89_internal.h"

static const P2D89_ShapeInfo shapes[] = {
    { P2D89_SHAPE_POINT, "point", "pixels", "basic", P2D89_MODULE_PIXELS, P2D89_FLAG_STROKE_ONLY, P2D89_FILL_NONZERO, P2D89_CAP_ROUND, P2D89_JOIN_MITER },
    { P2D89_SHAPE_PIXEL, "pixel", "pixels", "basic", P2D89_MODULE_PIXELS, P2D89_FLAG_CLOSED|P2D89_FLAG_SYMMETRIC_X|P2D89_FLAG_SYMMETRIC_Y, P2D89_FILL_NONZERO, P2D89_CAP_BUTT, P2D89_JOIN_MITER },
    { P2D89_SHAPE_PIXEL_HOLLOW, "pixel_hollow", "pixels", "basic", P2D89_MODULE_PIXELS, P2D89_FLAG_CLOSED|P2D89_FLAG_STROKE_ONLY|P2D89_FLAG_SYMMETRIC_X|P2D89_FLAG_SYMMETRIC_Y, P2D89_FILL_NONZERO, P2D89_CAP_BUTT, P2D89_JOIN_MITER },
};

static int p2d89_emit_pixels_basic(P2D89_ShapeId id,const P2D89_Provider *p){static const p2d89_q14 px[]={-1024,-1024,1024,-1024,1024,1024,-1024,1024};if(id==P2D89_SHAPE_POINT)return p2d89_i_point(p,0,0);if(id==P2D89_SHAPE_PIXEL||id==P2D89_SHAPE_PIXEL_HOLLOW)return p2d89_i_poly(px,4U,1,p);return 0;}
const P2D89_Submodule p2d89_submodule_pixels_basic = { { P2D89_MODULE_PIXELS, "pixels", "basic", (unsigned int)(sizeof(shapes)/sizeof(shapes[0])) }, shapes, p2d89_emit_pixels_basic };
