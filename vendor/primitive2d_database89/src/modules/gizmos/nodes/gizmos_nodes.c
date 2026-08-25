#include "p2d89_internal.h"
static const P2D89_ShapeInfo shapes[]={
 {P2D89_SHAPE_NODE_CORNER,"node_corner","gizmos","nodes",P2D89_MODULE_GIZMOS,P2D89_FLAG_CLOSED,P2D89_FILL_NONZERO,P2D89_CAP_BUTT,P2D89_JOIN_MITER},
 {P2D89_SHAPE_NODE_SMOOTH,"node_smooth","gizmos","nodes",P2D89_MODULE_GIZMOS,P2D89_FLAG_CLOSED|P2D89_FLAG_CURVED,P2D89_FILL_NONZERO,P2D89_CAP_ROUND,P2D89_JOIN_ROUND},
 {P2D89_SHAPE_NODE_SYMMETRIC,"node_symmetric","gizmos","nodes",P2D89_MODULE_GIZMOS,P2D89_FLAG_CLOSED,P2D89_FILL_NONZERO,P2D89_CAP_BUTT,P2D89_JOIN_MITER},
 {P2D89_SHAPE_BEZIER_HANDLES,"bezier_handles","gizmos","nodes",P2D89_MODULE_GIZMOS,P2D89_FLAG_MULTI_SUBPATH|P2D89_FLAG_COMPOUND_SYMBOL,P2D89_FILL_NONZERO,P2D89_CAP_ROUND,P2D89_JOIN_ROUND},
 {P2D89_SHAPE_ANCHOR_POINT,"anchor_point","gizmos","nodes",P2D89_MODULE_GIZMOS,P2D89_FLAG_CLOSED|P2D89_FLAG_CURVED,P2D89_FILL_NONZERO,P2D89_CAP_ROUND,P2D89_JOIN_ROUND},
 {P2D89_SHAPE_TANGENT_PAIR,"tangent_pair","gizmos","nodes",P2D89_MODULE_GIZMOS,P2D89_FLAG_MULTI_SUBPATH|P2D89_FLAG_SYMMETRIC_X,P2D89_FILL_NONZERO,P2D89_CAP_ROUND,P2D89_JOIN_ROUND}
};
static int l(p2d89_q14 a,p2d89_q14 b,p2d89_q14 c,p2d89_q14 d,const P2D89_Provider *p){if(!p2d89_i_move(p,a,b))return 0;return p2d89_i_line(p,c,d);}
static int emit(P2D89_ShapeId id,const P2D89_Provider *p){static const p2d89_q14 sq[]={-4096,-4096,4096,-4096,4096,4096,-4096,4096};static const p2d89_q14 dia[]={0,-5120,5120,0,0,5120,-5120,0};if(id==P2D89_SHAPE_NODE_CORNER)return p2d89_i_poly(sq,4U,1,p);if(id==P2D89_SHAPE_NODE_SMOOTH)return p2d89_emit_ellipse(5120,5120,1,p);if(id==P2D89_SHAPE_NODE_SYMMETRIC)return p2d89_i_poly(dia,4U,1,p);if(id==P2D89_SHAPE_BEZIER_HANDLES){if(!l(-14336,0,14336,0,p))return 0;if(!p2d89_emit_ellipse(3072,3072,1,p))return 0;if(!p2d89_i_move(p,-11264,0))return 0;if(!p2d89_i_line(p,-16384,0))return 0;return l(11264,0,16384,0,p);}if(id==P2D89_SHAPE_ANCHOR_POINT)return p2d89_emit_ellipse(4096,4096,1,p);if(id==P2D89_SHAPE_TANGENT_PAIR){if(!l(-14336,0,14336,0,p))return 0;if(!p2d89_emit_ellipse(3072,3072,1,p))return 0;return 1;}return 0;}
const P2D89_Submodule p2d89_submodule_gizmos_nodes={{P2D89_MODULE_GIZMOS,"gizmos","nodes",(unsigned int)(sizeof(shapes)/sizeof(shapes[0]))},shapes,emit};
