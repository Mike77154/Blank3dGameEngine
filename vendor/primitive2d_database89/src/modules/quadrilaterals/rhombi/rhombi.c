#include "p2d89_internal.h"

static const P2D89_ShapeInfo shapes[] = {
    { P2D89_SHAPE_DIAMOND, "diamond", "quadrilaterals", "rhombi_and_slanted", P2D89_MODULE_QUADRILATERALS, P2D89_FLAG_CLOSED|P2D89_FLAG_SYMMETRIC_X|P2D89_FLAG_SYMMETRIC_Y, P2D89_FILL_NONZERO, P2D89_CAP_BUTT, P2D89_JOIN_MITER },
    { P2D89_SHAPE_RHOMBUS, "rhombus", "quadrilaterals", "rhombi_and_slanted", P2D89_MODULE_QUADRILATERALS, P2D89_FLAG_CLOSED, P2D89_FILL_NONZERO, P2D89_CAP_BUTT, P2D89_JOIN_MITER },
    { P2D89_SHAPE_KITE, "kite", "quadrilaterals", "rhombi_and_slanted", P2D89_MODULE_QUADRILATERALS, P2D89_FLAG_CLOSED, P2D89_FILL_NONZERO, P2D89_CAP_BUTT, P2D89_JOIN_MITER },
    { P2D89_SHAPE_PARALLELOGRAM, "parallelogram", "quadrilaterals", "rhombi_and_slanted", P2D89_MODULE_QUADRILATERALS, P2D89_FLAG_CLOSED, P2D89_FILL_NONZERO, P2D89_CAP_BUTT, P2D89_JOIN_MITER },
    { P2D89_SHAPE_TRAPEZOID, "trapezoid", "quadrilaterals", "rhombi_and_slanted", P2D89_MODULE_QUADRILATERALS, P2D89_FLAG_CLOSED, P2D89_FILL_NONZERO, P2D89_CAP_BUTT, P2D89_JOIN_MITER },
};

static int p2d89_emit_quadrilaterals_rhombi_and_slanted(P2D89_ShapeId id,const P2D89_Provider *p){static const p2d89_q14 diamond[]={0,-16384,16384,0,0,16384,-16384,0};static const p2d89_q14 rhombus[]={-12288,-8192,16384,-8192,12288,8192,-16384,8192};static const p2d89_q14 kite[]={0,-16384,12288,2048,0,16384,-8192,2048};static const p2d89_q14 para[]={-8192,-12288,16384,-12288,8192,12288,-16384,12288};static const p2d89_q14 trap[]={-8192,-12288,8192,-12288,16384,12288,-16384,12288};if(id==P2D89_SHAPE_DIAMOND)return p2d89_i_poly(diamond,4U,1,p);if(id==P2D89_SHAPE_RHOMBUS)return p2d89_i_poly(rhombus,4U,1,p);if(id==P2D89_SHAPE_KITE)return p2d89_i_poly(kite,4U,1,p);if(id==P2D89_SHAPE_PARALLELOGRAM)return p2d89_i_poly(para,4U,1,p);if(id==P2D89_SHAPE_TRAPEZOID)return p2d89_i_poly(trap,4U,1,p);return 0;}
const P2D89_Submodule p2d89_submodule_quadrilaterals_rhombi_and_slanted = { { P2D89_MODULE_QUADRILATERALS, "quadrilaterals", "rhombi_and_slanted", (unsigned int)(sizeof(shapes)/sizeof(shapes[0])) }, shapes, p2d89_emit_quadrilaterals_rhombi_and_slanted };
