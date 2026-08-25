#include "p2d89_internal.h"

static const P2D89_ShapeInfo shapes[] = {
    { P2D89_SHAPE_SPARK_4, "spark_4", "stars", "sparks", P2D89_MODULE_STARS, P2D89_FLAG_CLOSED|P2D89_FLAG_CONCAVE|P2D89_FLAG_SYMMETRIC_X|P2D89_FLAG_SYMMETRIC_Y, P2D89_FILL_NONZERO, P2D89_CAP_BUTT, P2D89_JOIN_MITER },
    { P2D89_SHAPE_SPARK_8, "spark_8", "stars", "sparks", P2D89_MODULE_STARS, P2D89_FLAG_CLOSED|P2D89_FLAG_CONCAVE|P2D89_FLAG_SYMMETRIC_X|P2D89_FLAG_SYMMETRIC_Y, P2D89_FILL_NONZERO, P2D89_CAP_BUTT, P2D89_JOIN_MITER },
    { P2D89_SHAPE_ASTERISK, "asterisk", "stars", "sparks", P2D89_MODULE_STARS, P2D89_FLAG_STROKE_ONLY|P2D89_FLAG_MULTI_SUBPATH, P2D89_FILL_NONZERO, P2D89_CAP_ROUND, P2D89_JOIN_MITER },
    { P2D89_SHAPE_STARBURST_12, "starburst_12", "stars", "sparks", P2D89_MODULE_STARS, P2D89_FLAG_CLOSED|P2D89_FLAG_CONCAVE|P2D89_FLAG_PARAMETRIC, P2D89_FILL_NONZERO, P2D89_CAP_BUTT, P2D89_JOIN_MITER },
    { P2D89_SHAPE_STARBURST_16, "starburst_16", "stars", "sparks", P2D89_MODULE_STARS, P2D89_FLAG_CLOSED|P2D89_FLAG_CONCAVE|P2D89_FLAG_PARAMETRIC, P2D89_FILL_NONZERO, P2D89_CAP_BUTT, P2D89_JOIN_MITER },
};

static int p2d89_emit_stars_sparks(P2D89_ShapeId id,const P2D89_Provider *p){if(id==P2D89_SHAPE_SPARK_4)return p2d89_emit_star(4U,3072,49152U,p);if(id==P2D89_SHAPE_SPARK_8)return p2d89_emit_star(8U,4096,49152U,p);if(id==P2D89_SHAPE_STARBURST_12)return p2d89_emit_star(12U,2048,49152U,p);if(id==P2D89_SHAPE_STARBURST_16)return p2d89_emit_star(16U,2048,49152U,p);if(id==P2D89_SHAPE_ASTERISK){if(!p2d89_i_move(p,0,-14336))return 0;if(!p2d89_i_line(p,0,14336))return 0;if(!p2d89_i_move(p,-12288,-7168))return 0;if(!p2d89_i_line(p,12288,7168))return 0;if(!p2d89_i_move(p,12288,-7168))return 0;return p2d89_i_line(p,-12288,7168);}return 0;}
const P2D89_Submodule p2d89_submodule_stars_sparks = { { P2D89_MODULE_STARS, "stars", "sparks", (unsigned int)(sizeof(shapes)/sizeof(shapes[0])) }, shapes, p2d89_emit_stars_sparks };
