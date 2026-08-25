#include "p2d89_internal.h"

static const P2D89_ShapeInfo shapes[] = {
    { P2D89_SHAPE_CHEVRON_LEFT, "chevron_left", "chevrons", "basic", P2D89_MODULE_CHEVRONS, P2D89_FLAG_STROKE_ONLY, P2D89_FILL_NONZERO, P2D89_CAP_ROUND, P2D89_JOIN_MITER },
    { P2D89_SHAPE_CHEVRON_RIGHT, "chevron_right", "chevrons", "basic", P2D89_MODULE_CHEVRONS, P2D89_FLAG_STROKE_ONLY, P2D89_FILL_NONZERO, P2D89_CAP_ROUND, P2D89_JOIN_MITER },
    { P2D89_SHAPE_CHEVRON_UP, "chevron_up", "chevrons", "basic", P2D89_MODULE_CHEVRONS, P2D89_FLAG_STROKE_ONLY, P2D89_FILL_NONZERO, P2D89_CAP_ROUND, P2D89_JOIN_MITER },
    { P2D89_SHAPE_CHEVRON_DOWN, "chevron_down", "chevrons", "basic", P2D89_MODULE_CHEVRONS, P2D89_FLAG_STROKE_ONLY, P2D89_FILL_NONZERO, P2D89_CAP_ROUND, P2D89_JOIN_MITER },
    { P2D89_SHAPE_DOUBLE_CHEVRON_LEFT, "double_chevron_left", "chevrons", "basic", P2D89_MODULE_CHEVRONS, P2D89_FLAG_STROKE_ONLY|P2D89_FLAG_MULTI_SUBPATH, P2D89_FILL_NONZERO, P2D89_CAP_ROUND, P2D89_JOIN_MITER },
    { P2D89_SHAPE_DOUBLE_CHEVRON_RIGHT, "double_chevron_right", "chevrons", "basic", P2D89_MODULE_CHEVRONS, P2D89_FLAG_STROKE_ONLY|P2D89_FLAG_MULTI_SUBPATH, P2D89_FILL_NONZERO, P2D89_CAP_ROUND, P2D89_JOIN_MITER },
    { P2D89_SHAPE_CARET_UP, "caret_up", "chevrons", "basic", P2D89_MODULE_CHEVRONS, P2D89_FLAG_STROKE_ONLY, P2D89_FILL_NONZERO, P2D89_CAP_ROUND, P2D89_JOIN_MITER },
    { P2D89_SHAPE_CARET_DOWN, "caret_down", "chevrons", "basic", P2D89_MODULE_CHEVRONS, P2D89_FLAG_STROKE_ONLY, P2D89_FILL_NONZERO, P2D89_CAP_ROUND, P2D89_JOIN_MITER },
};

static int p2d89_emit_chevrons_basic(P2D89_ShapeId id,const P2D89_Provider *p){static const p2d89_q14 pts[]={-8192,-12288,8192,0,-8192,12288};static const p2d89_q14 angle_l[]={8192,-12288,-8192,0,8192,12288};static const p2d89_q14 a[]={-12288,-12288,0,0,-12288,12288};static const p2d89_q14 b[]={-2048,-12288,10240,0,-2048,12288};if(id==P2D89_SHAPE_CHEVRON_RIGHT)return p2d89_i_rot_poly(pts,3U,0U,0,p);if(id==P2D89_SHAPE_CHEVRON_LEFT)return p2d89_i_rot_poly(pts,3U,32768U,0,p);if(id==P2D89_SHAPE_CHEVRON_UP||id==P2D89_SHAPE_CARET_UP)return p2d89_i_rot_poly(angle_l,3U,49152U,0,p);if(id==P2D89_SHAPE_CHEVRON_DOWN||id==P2D89_SHAPE_CARET_DOWN)return p2d89_i_rot_poly(angle_l,3U,16384U,0,p);if(id==P2D89_SHAPE_DOUBLE_CHEVRON_RIGHT){if(!p2d89_i_rot_poly(a,3U,0U,0,p))return 0;return p2d89_i_rot_poly(b,3U,0U,0,p);}if(id==P2D89_SHAPE_DOUBLE_CHEVRON_LEFT){if(!p2d89_i_rot_poly(a,3U,32768U,0,p))return 0;return p2d89_i_rot_poly(b,3U,32768U,0,p);}return 0;}
const P2D89_Submodule p2d89_submodule_chevrons_basic = { { P2D89_MODULE_CHEVRONS, "chevrons", "basic", (unsigned int)(sizeof(shapes)/sizeof(shapes[0])) }, shapes, p2d89_emit_chevrons_basic };
