#include "p2d89_internal.h"
static const P2D89_ShapeInfo shapes[]={
 {P2D89_SHAPE_MARKER_ARROW_OPEN,"marker_arrow_open","connectors","markers",P2D89_MODULE_CONNECTORS,P2D89_FLAG_STROKE_ONLY|P2D89_FLAG_MARKER,P2D89_FILL_NONZERO,P2D89_CAP_ROUND,P2D89_JOIN_ROUND},
 {P2D89_SHAPE_MARKER_ARROW_CLOSED,"marker_arrow_closed","connectors","markers",P2D89_MODULE_CONNECTORS,P2D89_FLAG_STROKE_ONLY|P2D89_FLAG_MARKER,P2D89_FILL_NONZERO,P2D89_CAP_ROUND,P2D89_JOIN_ROUND},
 {P2D89_SHAPE_MARKER_ARROW_STEALTH,"marker_arrow_stealth","connectors","markers",P2D89_MODULE_CONNECTORS,P2D89_FLAG_STROKE_ONLY|P2D89_FLAG_MARKER,P2D89_FILL_NONZERO,P2D89_CAP_ROUND,P2D89_JOIN_ROUND},
 {P2D89_SHAPE_MARKER_TRIANGLE,"marker_triangle","connectors","markers",P2D89_MODULE_CONNECTORS,P2D89_FLAG_STROKE_ONLY|P2D89_FLAG_MARKER,P2D89_FILL_NONZERO,P2D89_CAP_ROUND,P2D89_JOIN_ROUND},
 {P2D89_SHAPE_MARKER_DIAMOND,"marker_diamond","connectors","markers",P2D89_MODULE_CONNECTORS,P2D89_FLAG_STROKE_ONLY|P2D89_FLAG_MARKER,P2D89_FILL_NONZERO,P2D89_CAP_ROUND,P2D89_JOIN_ROUND},
 {P2D89_SHAPE_MARKER_CIRCLE,"marker_circle","connectors","markers",P2D89_MODULE_CONNECTORS,P2D89_FLAG_STROKE_ONLY|P2D89_FLAG_MARKER,P2D89_FILL_NONZERO,P2D89_CAP_ROUND,P2D89_JOIN_ROUND},
 {P2D89_SHAPE_MARKER_ENDPOINT_BAR,"marker_endpoint_bar","connectors","markers",P2D89_MODULE_CONNECTORS,P2D89_FLAG_STROKE_ONLY|P2D89_FLAG_MARKER,P2D89_FILL_NONZERO,P2D89_CAP_ROUND,P2D89_JOIN_ROUND},
 {P2D89_SHAPE_MARKER_ENDPOINT_DOUBLE_BAR,"marker_endpoint_double_bar","connectors","markers",P2D89_MODULE_CONNECTORS,P2D89_FLAG_STROKE_ONLY|P2D89_FLAG_MULTI_SUBPATH|P2D89_FLAG_MARKER,P2D89_FILL_NONZERO,P2D89_CAP_ROUND,P2D89_JOIN_ROUND},
 {P2D89_SHAPE_MARKER_ENDPOINT_SQUARE,"marker_endpoint_square","connectors","markers",P2D89_MODULE_CONNECTORS,P2D89_FLAG_STROKE_ONLY|P2D89_FLAG_MARKER,P2D89_FILL_NONZERO,P2D89_CAP_ROUND,P2D89_JOIN_ROUND},
 {P2D89_SHAPE_MARKER_ENDPOINT_CIRCLE,"marker_endpoint_circle","connectors","markers",P2D89_MODULE_CONNECTORS,P2D89_FLAG_STROKE_ONLY|P2D89_FLAG_MARKER,P2D89_FILL_NONZERO,P2D89_CAP_ROUND,P2D89_JOIN_ROUND},
 {P2D89_SHAPE_MARKER_ENDPOINT_DIAMOND,"marker_endpoint_diamond","connectors","markers",P2D89_MODULE_CONNECTORS,P2D89_FLAG_STROKE_ONLY|P2D89_FLAG_MARKER,P2D89_FILL_NONZERO,P2D89_CAP_ROUND,P2D89_JOIN_ROUND},
 {P2D89_SHAPE_MARKER_ENDPOINT_DOT,"marker_endpoint_dot","connectors","markers",P2D89_MODULE_CONNECTORS,P2D89_FLAG_STROKE_ONLY|P2D89_FLAG_MARKER,P2D89_FILL_NONZERO,P2D89_CAP_ROUND,P2D89_JOIN_ROUND},
 {P2D89_SHAPE_MARKER_CROW_ONE,"marker_crow_one","connectors","markers",P2D89_MODULE_CONNECTORS,P2D89_FLAG_STROKE_ONLY|P2D89_FLAG_MULTI_SUBPATH|P2D89_FLAG_MARKER,P2D89_FILL_NONZERO,P2D89_CAP_ROUND,P2D89_JOIN_ROUND},
 {P2D89_SHAPE_MARKER_CROW_MANY,"marker_crow_many","connectors","markers",P2D89_MODULE_CONNECTORS,P2D89_FLAG_STROKE_ONLY|P2D89_FLAG_MULTI_SUBPATH|P2D89_FLAG_MARKER,P2D89_FILL_NONZERO,P2D89_CAP_ROUND,P2D89_JOIN_ROUND},
 {P2D89_SHAPE_MARKER_CROW_ZERO,"marker_crow_zero","connectors","markers",P2D89_MODULE_CONNECTORS,P2D89_FLAG_STROKE_ONLY|P2D89_FLAG_MULTI_SUBPATH|P2D89_FLAG_MARKER,P2D89_FILL_NONZERO,P2D89_CAP_ROUND,P2D89_JOIN_ROUND},
 {P2D89_SHAPE_MARKER_CROW_ZERO_MANY,"marker_crow_zero_many","connectors","markers",P2D89_MODULE_CONNECTORS,P2D89_FLAG_STROKE_ONLY|P2D89_FLAG_MULTI_SUBPATH|P2D89_FLAG_MARKER,P2D89_FILL_NONZERO,P2D89_CAP_ROUND,P2D89_JOIN_ROUND},
 {P2D89_SHAPE_MARKER_CROW_ONE_MANY,"marker_crow_one_many","connectors","markers",P2D89_MODULE_CONNECTORS,P2D89_FLAG_STROKE_ONLY|P2D89_FLAG_MULTI_SUBPATH|P2D89_FLAG_MARKER,P2D89_FILL_NONZERO,P2D89_CAP_ROUND,P2D89_JOIN_ROUND},
 {P2D89_SHAPE_MARKER_TEE,"marker_tee","connectors","markers",P2D89_MODULE_CONNECTORS,P2D89_FLAG_STROKE_ONLY|P2D89_FLAG_MULTI_SUBPATH|P2D89_FLAG_MARKER,P2D89_FILL_NONZERO,P2D89_CAP_ROUND,P2D89_JOIN_ROUND},
};
static const P2D89_Command r0[]={
 {2,-9830,-7373,0,0,0,0}, {3,0,0,0,0,0,0}, {3,-9830,7373,0,0,0,0},
};
static const P2D89_Command r1[]={
 {2,-11469,-8192,0,0,0,0}, {3,0,0,0,0,0,0}, {3,-11469,8192,0,0,0,0}, {6,0,0,0,0,0,0},
};
static const P2D89_Command r2[]={
 {2,-12288,-7864,0,0,0,0}, {3,0,0,0,0,0,0}, {3,-12288,7864,0,0,0,0}, {3,-7373,0,0,0,0,0},
 {6,0,0,0,0,0,0},
};
static const P2D89_Command r3[]={
 {2,10650,0,0,0,0,0}, {3,-5325,9223,0,0,0,0}, {3,-5325,-9223,0,0,0,0}, {6,0,0,0,0,0,0},
};
static const P2D89_Command r4[]={
 {2,0,-9830,0,0,0,0}, {3,9830,0,0,0,0,0}, {3,0,9830,0,0,0,0}, {3,-9830,0,0,0,0,0},
 {6,0,0,0,0,0,0},
};
static const P2D89_Command r5[]={
 {2,8520,0,0,0,0,0}, {3,8103,2633,0,0,0,0}, {3,6893,5008,0,0,0,0}, {3,5008,6893,0,0,0,0},
 {3,2633,8103,0,0,0,0}, {3,0,8520,0,0,0,0}, {3,-2633,8103,0,0,0,0}, {3,-5008,6893,0,0,0,0},
 {3,-6893,5008,0,0,0,0}, {3,-8103,2633,0,0,0,0}, {3,-8520,0,0,0,0,0}, {3,-8103,-2633,0,0,0,0},
 {3,-6893,-5008,0,0,0,0}, {3,-5008,-6893,0,0,0,0}, {3,-2633,-8103,0,0,0,0}, {3,0,-8520,0,0,0,0},
 {3,2633,-8103,0,0,0,0}, {3,5008,-6893,0,0,0,0}, {3,6893,-5008,0,0,0,0}, {3,8103,-2633,0,0,0,0},
 {6,0,0,0,0,0,0},
};
static const P2D89_Command r6[]={
 {2,0,-10650,0,0,0,0}, {3,0,10650,0,0,0,0},
};
static const P2D89_Command r7[]={
 {2,-2621,-10650,0,0,0,0}, {3,-2621,10650,0,0,0,0}, {2,2621,-10650,0,0,0,0}, {3,2621,10650,0,0,0,0},
};
static const P2D89_Command r8[]={
 {2,-7864,-7864,0,0,0,0}, {3,7864,-7864,0,0,0,0}, {3,7864,7864,0,0,0,0}, {3,-7864,7864,0,0,0,0},
 {6,0,0,0,0,0,0},
};
static const P2D89_Command r9[]={
 {2,7864,0,0,0,0,0}, {3,7479,2430,0,0,0,0}, {3,6362,4623,0,0,0,0}, {3,4623,6362,0,0,0,0},
 {3,2430,7479,0,0,0,0}, {3,0,7864,0,0,0,0}, {3,-2430,7479,0,0,0,0}, {3,-4623,6362,0,0,0,0},
 {3,-6362,4623,0,0,0,0}, {3,-7479,2430,0,0,0,0}, {3,-7864,0,0,0,0,0}, {3,-7479,-2430,0,0,0,0},
 {3,-6362,-4623,0,0,0,0}, {3,-4623,-6362,0,0,0,0}, {3,-2430,-7479,0,0,0,0}, {3,0,-7864,0,0,0,0},
 {3,2430,-7479,0,0,0,0}, {3,4623,-6362,0,0,0,0}, {3,6362,-4623,0,0,0,0}, {3,7479,-2430,0,0,0,0},
 {6,0,0,0,0,0,0},
};
static const P2D89_Command r10[]={
 {2,0,-8192,0,0,0,0}, {3,8192,0,0,0,0,0}, {3,0,8192,0,0,0,0}, {3,-8192,0,0,0,0,0},
 {6,0,0,0,0,0,0},
};
static const P2D89_Command r11[]={
 {2,3277,0,0,0,0,0}, {3,2952,1422,0,0,0,0}, {3,2043,2562,0,0,0,0}, {3,729,3195,0,0,0,0},
 {3,-729,3195,0,0,0,0}, {3,-2043,2562,0,0,0,0}, {3,-2952,1422,0,0,0,0}, {3,-3277,0,0,0,0,0},
 {3,-2952,-1422,0,0,0,0}, {3,-2043,-2562,0,0,0,0}, {3,-729,-3195,0,0,0,0}, {3,729,-3195,0,0,0,0},
 {3,2043,-2562,0,0,0,0}, {3,2952,-1422,0,0,0,0}, {6,0,0,0,0,0,0},
};
static const P2D89_Command r12[]={
 {2,0,-9830,0,0,0,0}, {3,0,9830,0,0,0,0}, {2,0,0,0,0,0,0}, {3,11469,0,0,0,0,0},
};
static const P2D89_Command r13[]={
 {2,0,0,0,0,0,0}, {3,11469,-9011,0,0,0,0}, {2,0,0,0,0,0,0}, {3,11469,0,0,0,0,0},
 {2,0,0,0,0,0,0}, {3,11469,9011,0,0,0,0},
};
static const P2D89_Command r14[]={
 {2,983,0,0,0,0,0}, {3,594,1706,0,0,0,0}, {3,-497,3074,0,0,0,0}, {3,-2074,3834,0,0,0,0},
 {3,-3824,3834,0,0,0,0}, {3,-5401,3074,0,0,0,0}, {3,-6492,1706,0,0,0,0}, {3,-6881,0,0,0,0,0},
 {3,-6492,-1706,0,0,0,0}, {3,-5401,-3074,0,0,0,0}, {3,-3824,-3834,0,0,0,0}, {3,-2074,-3834,0,0,0,0},
 {3,-497,-3074,0,0,0,0}, {3,594,-1706,0,0,0,0}, {6,0,0,0,0,0,0}, {2,1311,0,0,0,0,0},
 {3,11469,0,0,0,0,0},
};
static const P2D89_Command r15[]={
 {2,983,0,0,0,0,0}, {3,594,1706,0,0,0,0}, {3,-497,3074,0,0,0,0}, {3,-2074,3834,0,0,0,0},
 {3,-3824,3834,0,0,0,0}, {3,-5401,3074,0,0,0,0}, {3,-6492,1706,0,0,0,0}, {3,-6881,0,0,0,0,0},
 {3,-6492,-1706,0,0,0,0}, {3,-5401,-3074,0,0,0,0}, {3,-3824,-3834,0,0,0,0}, {3,-2074,-3834,0,0,0,0},
 {3,-497,-3074,0,0,0,0}, {3,594,-1706,0,0,0,0}, {6,0,0,0,0,0,0}, {2,1311,0,0,0,0,0},
 {3,11469,-9011,0,0,0,0}, {2,1311,0,0,0,0,0}, {3,11469,9011,0,0,0,0},
};
static const P2D89_Command r16[]={
 {2,-1311,-9830,0,0,0,0}, {3,-1311,9830,0,0,0,0}, {2,1311,0,0,0,0,0}, {3,11469,-9011,0,0,0,0},
 {2,1311,0,0,0,0,0}, {3,11469,9011,0,0,0,0},
};
static const P2D89_Command r17[]={
 {2,0,-10650,0,0,0,0}, {3,0,10650,0,0,0,0}, {2,0,0,0,0,0,0}, {3,11469,0,0,0,0,0},
};
static int replay(const P2D89_Command *c,unsigned int n,const P2D89_Provider *p){unsigned int i;if(c==0||p==0||p->emit==0)return 0;for(i=0U;i<n;++i)if(!p->emit(p->user,&c[i]))return 0;return 1;}
static int emit(P2D89_ShapeId id,const P2D89_Provider *p){switch(id){
case P2D89_SHAPE_MARKER_ARROW_OPEN: return replay(r0,(unsigned int)(sizeof(r0)/sizeof(r0[0])),p);
case P2D89_SHAPE_MARKER_ARROW_CLOSED: return replay(r1,(unsigned int)(sizeof(r1)/sizeof(r1[0])),p);
case P2D89_SHAPE_MARKER_ARROW_STEALTH: return replay(r2,(unsigned int)(sizeof(r2)/sizeof(r2[0])),p);
case P2D89_SHAPE_MARKER_TRIANGLE: return replay(r3,(unsigned int)(sizeof(r3)/sizeof(r3[0])),p);
case P2D89_SHAPE_MARKER_DIAMOND: return replay(r4,(unsigned int)(sizeof(r4)/sizeof(r4[0])),p);
case P2D89_SHAPE_MARKER_CIRCLE: return replay(r5,(unsigned int)(sizeof(r5)/sizeof(r5[0])),p);
case P2D89_SHAPE_MARKER_ENDPOINT_BAR: return replay(r6,(unsigned int)(sizeof(r6)/sizeof(r6[0])),p);
case P2D89_SHAPE_MARKER_ENDPOINT_DOUBLE_BAR: return replay(r7,(unsigned int)(sizeof(r7)/sizeof(r7[0])),p);
case P2D89_SHAPE_MARKER_ENDPOINT_SQUARE: return replay(r8,(unsigned int)(sizeof(r8)/sizeof(r8[0])),p);
case P2D89_SHAPE_MARKER_ENDPOINT_CIRCLE: return replay(r9,(unsigned int)(sizeof(r9)/sizeof(r9[0])),p);
case P2D89_SHAPE_MARKER_ENDPOINT_DIAMOND: return replay(r10,(unsigned int)(sizeof(r10)/sizeof(r10[0])),p);
case P2D89_SHAPE_MARKER_ENDPOINT_DOT: return replay(r11,(unsigned int)(sizeof(r11)/sizeof(r11[0])),p);
case P2D89_SHAPE_MARKER_CROW_ONE: return replay(r12,(unsigned int)(sizeof(r12)/sizeof(r12[0])),p);
case P2D89_SHAPE_MARKER_CROW_MANY: return replay(r13,(unsigned int)(sizeof(r13)/sizeof(r13[0])),p);
case P2D89_SHAPE_MARKER_CROW_ZERO: return replay(r14,(unsigned int)(sizeof(r14)/sizeof(r14[0])),p);
case P2D89_SHAPE_MARKER_CROW_ZERO_MANY: return replay(r15,(unsigned int)(sizeof(r15)/sizeof(r15[0])),p);
case P2D89_SHAPE_MARKER_CROW_ONE_MANY: return replay(r16,(unsigned int)(sizeof(r16)/sizeof(r16[0])),p);
case P2D89_SHAPE_MARKER_TEE: return replay(r17,(unsigned int)(sizeof(r17)/sizeof(r17[0])),p);
default: return 0;}}
const P2D89_Submodule p2d89_submodule_connectors_markers={{P2D89_MODULE_CONNECTORS,"connectors","markers",(unsigned int)(sizeof(shapes)/sizeof(shapes[0]))},shapes,emit};
