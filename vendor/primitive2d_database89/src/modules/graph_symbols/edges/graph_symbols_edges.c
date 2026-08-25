#include "p2d89_internal.h"
static const P2D89_ShapeInfo shapes[]={
 {P2D89_SHAPE_GRAPH_EDGE_DIRECTED,"graph_edge_directed","graph_symbols","edges",P2D89_MODULE_GRAPH_SYMBOLS,P2D89_FLAG_STROKE_ONLY|P2D89_FLAG_MULTI_SUBPATH|P2D89_FLAG_DIRECTIONAL,P2D89_FILL_NONZERO,P2D89_CAP_ROUND,P2D89_JOIN_ROUND},
 {P2D89_SHAPE_GRAPH_EDGE_BIDIRECTED,"graph_edge_bidirected","graph_symbols","edges",P2D89_MODULE_GRAPH_SYMBOLS,P2D89_FLAG_STROKE_ONLY|P2D89_FLAG_MULTI_SUBPATH|P2D89_FLAG_DIRECTIONAL,P2D89_FILL_NONZERO,P2D89_CAP_ROUND,P2D89_JOIN_ROUND},
 {P2D89_SHAPE_GRAPH_EDGE_UNDIRECTED,"graph_edge_undirected","graph_symbols","edges",P2D89_MODULE_GRAPH_SYMBOLS,P2D89_FLAG_STROKE_ONLY|P2D89_FLAG_MULTI_SUBPATH|P2D89_FLAG_DIRECTIONAL,P2D89_FILL_NONZERO,P2D89_CAP_ROUND,P2D89_JOIN_ROUND},
 {P2D89_SHAPE_GRAPH_EDGE_DASHED,"graph_edge_dashed","graph_symbols","edges",P2D89_MODULE_GRAPH_SYMBOLS,P2D89_FLAG_STROKE_ONLY|P2D89_FLAG_MULTI_SUBPATH|P2D89_FLAG_DIRECTIONAL,P2D89_FILL_NONZERO,P2D89_CAP_ROUND,P2D89_JOIN_ROUND},
 {P2D89_SHAPE_GRAPH_EDGE_DOTTED,"graph_edge_dotted","graph_symbols","edges",P2D89_MODULE_GRAPH_SYMBOLS,P2D89_FLAG_STROKE_ONLY|P2D89_FLAG_MULTI_SUBPATH|P2D89_FLAG_DIRECTIONAL,P2D89_FILL_NONZERO,P2D89_CAP_ROUND,P2D89_JOIN_ROUND},
 {P2D89_SHAPE_GRAPH_EDGE_LOOP,"graph_edge_loop","graph_symbols","edges",P2D89_MODULE_GRAPH_SYMBOLS,P2D89_FLAG_STROKE_ONLY|P2D89_FLAG_MULTI_SUBPATH|P2D89_FLAG_DIRECTIONAL,P2D89_FILL_NONZERO,P2D89_CAP_ROUND,P2D89_JOIN_ROUND},
 {P2D89_SHAPE_GRAPH_EDGE_PARALLEL_PAIR,"graph_edge_parallel_pair","graph_symbols","edges",P2D89_MODULE_GRAPH_SYMBOLS,P2D89_FLAG_STROKE_ONLY|P2D89_FLAG_MULTI_SUBPATH|P2D89_FLAG_DIRECTIONAL,P2D89_FILL_NONZERO,P2D89_CAP_ROUND,P2D89_JOIN_ROUND},
 {P2D89_SHAPE_GRAPH_EDGE_BUS,"graph_edge_bus","graph_symbols","edges",P2D89_MODULE_GRAPH_SYMBOLS,P2D89_FLAG_STROKE_ONLY|P2D89_FLAG_MULTI_SUBPATH|P2D89_FLAG_DIRECTIONAL,P2D89_FILL_NONZERO,P2D89_CAP_ROUND,P2D89_JOIN_ROUND},
 {P2D89_SHAPE_GRAPH_EDGE_BRANCH,"graph_edge_branch","graph_symbols","edges",P2D89_MODULE_GRAPH_SYMBOLS,P2D89_FLAG_STROKE_ONLY|P2D89_FLAG_MULTI_SUBPATH|P2D89_FLAG_DIRECTIONAL,P2D89_FILL_NONZERO,P2D89_CAP_ROUND,P2D89_JOIN_ROUND},
 {P2D89_SHAPE_GRAPH_EDGE_MERGE,"graph_edge_merge","graph_symbols","edges",P2D89_MODULE_GRAPH_SYMBOLS,P2D89_FLAG_STROKE_ONLY|P2D89_FLAG_MULTI_SUBPATH|P2D89_FLAG_DIRECTIONAL,P2D89_FILL_NONZERO,P2D89_CAP_ROUND,P2D89_JOIN_ROUND},
 {P2D89_SHAPE_GRAPH_EDGE_PORT,"graph_edge_port","graph_symbols","edges",P2D89_MODULE_GRAPH_SYMBOLS,P2D89_FLAG_STROKE_ONLY|P2D89_FLAG_MULTI_SUBPATH|P2D89_FLAG_DIRECTIONAL,P2D89_FILL_NONZERO,P2D89_CAP_ROUND,P2D89_JOIN_ROUND},
 {P2D89_SHAPE_GRAPH_EDGE_DEPENDENCY,"graph_edge_dependency","graph_symbols","edges",P2D89_MODULE_GRAPH_SYMBOLS,P2D89_FLAG_STROKE_ONLY|P2D89_FLAG_MULTI_SUBPATH|P2D89_FLAG_DIRECTIONAL,P2D89_FILL_NONZERO,P2D89_CAP_ROUND,P2D89_JOIN_ROUND},
};
static const P2D89_Command r0[]={
 {2,-16384,0,0,0,0,0}, {3,10650,0,0,0,0,0}, {2,5734,-4096,0,0,0,0}, {3,12288,0,0,0,0,0},
 {3,5734,4096,0,0,0,0}, {6,0,0,0,0,0,0},
};
static const P2D89_Command r1[]={
 {2,-10650,0,0,0,0,0}, {3,10650,0,0,0,0,0}, {2,-5734,-4096,0,0,0,0}, {3,-12288,0,0,0,0,0},
 {3,-5734,4096,0,0,0,0}, {6,0,0,0,0,0,0}, {2,5734,-4096,0,0,0,0}, {3,12288,0,0,0,0,0},
 {3,5734,4096,0,0,0,0}, {6,0,0,0,0,0,0},
};
static const P2D89_Command r2[]={
 {2,-16384,0,0,0,0,0}, {3,16384,0,0,0,0,0},
};
static const P2D89_Command r3[]={
 {2,-15565,0,0,0,0,0}, {3,-11960,0,0,0,0,0}, {2,-9011,0,0,0,0,0}, {3,-5407,0,0,0,0,0},
 {2,-2458,0,0,0,0,0}, {3,1147,0,0,0,0,0}, {2,4096,0,0,0,0,0}, {3,7700,0,0,0,0,0},
 {2,10650,0,0,0,0,0}, {3,14254,0,0,0,0,0},
};
static const P2D89_Command r4[]={
 {2,-11960,0,0,0,0,0}, {3,-12296,811,0,0,0,0}, {3,-13107,1147,0,0,0,0}, {3,-13918,811,0,0,0,0},
 {3,-14254,0,0,0,0,0}, {3,-13918,-811,0,0,0,0}, {3,-13107,-1147,0,0,0,0}, {3,-12296,-811,0,0,0,0},
 {6,0,0,0,0,0,0}, {2,-5407,0,0,0,0,0}, {3,-5743,811,0,0,0,0}, {3,-6554,1147,0,0,0,0},
 {3,-7365,811,0,0,0,0}, {3,-7700,0,0,0,0,0}, {3,-7365,-811,0,0,0,0}, {3,-6554,-1147,0,0,0,0},
 {3,-5743,-811,0,0,0,0}, {6,0,0,0,0,0,0}, {2,1147,0,0,0,0,0}, {3,811,811,0,0,0,0},
 {3,0,1147,0,0,0,0}, {3,-811,811,0,0,0,0}, {3,-1147,0,0,0,0,0}, {3,-811,-811,0,0,0,0},
 {3,0,-1147,0,0,0,0}, {3,811,-811,0,0,0,0}, {6,0,0,0,0,0,0}, {2,7700,0,0,0,0,0},
 {3,7365,811,0,0,0,0}, {3,6554,1147,0,0,0,0}, {3,5743,811,0,0,0,0}, {3,5407,0,0,0,0,0},
 {3,5743,-811,0,0,0,0}, {3,6554,-1147,0,0,0,0}, {3,7365,-811,0,0,0,0}, {6,0,0,0,0,0,0},
 {2,14254,0,0,0,0,0}, {3,13918,811,0,0,0,0}, {3,13107,1147,0,0,0,0}, {3,12296,811,0,0,0,0},
 {3,11960,0,0,0,0,0}, {3,12296,-811,0,0,0,0}, {3,13107,-1147,0,0,0,0}, {3,13918,-811,0,0,0,0},
 {6,0,0,0,0,0,0},
};
static const P2D89_Command r5[]={
 {2,9011,0,0,0,0,0}, {3,8704,2332,0,0,0,0}, {3,7804,4506,0,0,0,0}, {3,6372,6372,0,0,0,0},
 {3,4506,7804,0,0,0,0}, {3,2332,8704,0,0,0,0}, {3,0,9011,0,0,0,0}, {3,-2332,8704,0,0,0,0},
 {3,-4506,7804,0,0,0,0}, {3,-6372,6372,0,0,0,0}, {3,-7804,4506,0,0,0,0}, {3,-8704,2332,0,0,0,0},
 {3,-9011,0,0,0,0,0}, {3,-8704,-2332,0,0,0,0}, {3,-7804,-4506,0,0,0,0}, {3,-6372,-6372,0,0,0,0},
 {3,-4506,-7804,0,0,0,0}, {3,-2332,-8704,0,0,0,0}, {3,0,-9011,0,0,0,0}, {3,2332,-8704,0,0,0,0},
 {3,4506,-7804,0,0,0,0}, {3,6372,-6372,0,0,0,0}, {3,7804,-4506,0,0,0,0}, {3,8704,-2332,0,0,0,0},
 {6,0,0,0,0,0,0}, {2,5734,-7864,0,0,0,0}, {3,11796,-6554,0,0,0,0}, {3,8520,-1966,0,0,0,0},
 {6,0,0,0,0,0,0},
};
static const P2D89_Command r6[]={
 {2,-16384,-2949,0,0,0,0}, {3,16384,-2949,0,0,0,0}, {2,-16384,2949,0,0,0,0}, {3,16384,2949,0,0,0,0},
};
static const P2D89_Command r7[]={
 {2,-16384,0,0,0,0,0}, {3,16384,0,0,0,0,0}, {2,-9830,0,0,0,0,0}, {3,-9830,9011,0,0,0,0},
 {2,-3277,0,0,0,0,0}, {3,-3277,9011,0,0,0,0}, {2,3277,0,0,0,0,0}, {3,3277,9011,0,0,0,0},
 {2,9830,0,0,0,0,0}, {3,9830,9011,0,0,0,0},
};
static const P2D89_Command r8[]={
 {2,0,-13107,0,0,0,0}, {3,0,0,0,0,0,0}, {2,0,0,0,0,0,0}, {3,-13107,11469,0,0,0,0},
 {2,0,0,0,0,0,0}, {3,13107,11469,0,0,0,0},
};
static const P2D89_Command r9[]={
 {2,-16384,-9011,0,0,0,0}, {3,0,0,0,0,0,0}, {2,-16384,9011,0,0,0,0}, {3,0,0,0,0,0,0},
 {2,0,0,0,0,0,0}, {3,16384,0,0,0,0,0},
};
static const P2D89_Command r10[]={
 {2,-16384,0,0,0,0,0}, {3,11469,0,0,0,0,0}, {2,15401,0,0,0,0,0}, {3,15025,1156,0,0,0,0},
 {3,14042,1870,0,0,0,0}, {3,12827,1870,0,0,0,0}, {3,11844,1156,0,0,0,0}, {3,11469,0,0,0,0,0},
 {3,11844,-1156,0,0,0,0}, {3,12827,-1870,0,0,0,0}, {3,14042,-1870,0,0,0,0}, {3,15025,-1156,0,0,0,0},
 {6,0,0,0,0,0,0},
};
static const P2D89_Command r11[]={
 {2,-14746,0,0,0,0,0}, {3,-11796,0,0,0,0,0}, {2,-9011,0,0,0,0,0}, {3,-6062,0,0,0,0,0},
 {2,-3277,0,0,0,0,0}, {3,-328,0,0,0,0,0}, {2,2458,0,0,0,0,0}, {3,5407,0,0,0,0,0},
 {2,8192,0,0,0,0,0}, {3,11141,0,0,0,0,0}, {2,9011,-3604,0,0,0,0}, {3,14746,0,0,0,0,0},
 {3,9011,3604,0,0,0,0},
};
static int replay(const P2D89_Command *c,unsigned int n,const P2D89_Provider *p){unsigned int i;if(c==0||p==0||p->emit==0)return 0;for(i=0U;i<n;++i)if(!p->emit(p->user,&c[i]))return 0;return 1;}
static int emit(P2D89_ShapeId id,const P2D89_Provider *p){switch(id){
case P2D89_SHAPE_GRAPH_EDGE_DIRECTED: return replay(r0,(unsigned int)(sizeof(r0)/sizeof(r0[0])),p);
case P2D89_SHAPE_GRAPH_EDGE_BIDIRECTED: return replay(r1,(unsigned int)(sizeof(r1)/sizeof(r1[0])),p);
case P2D89_SHAPE_GRAPH_EDGE_UNDIRECTED: return replay(r2,(unsigned int)(sizeof(r2)/sizeof(r2[0])),p);
case P2D89_SHAPE_GRAPH_EDGE_DASHED: return replay(r3,(unsigned int)(sizeof(r3)/sizeof(r3[0])),p);
case P2D89_SHAPE_GRAPH_EDGE_DOTTED: return replay(r4,(unsigned int)(sizeof(r4)/sizeof(r4[0])),p);
case P2D89_SHAPE_GRAPH_EDGE_LOOP: return replay(r5,(unsigned int)(sizeof(r5)/sizeof(r5[0])),p);
case P2D89_SHAPE_GRAPH_EDGE_PARALLEL_PAIR: return replay(r6,(unsigned int)(sizeof(r6)/sizeof(r6[0])),p);
case P2D89_SHAPE_GRAPH_EDGE_BUS: return replay(r7,(unsigned int)(sizeof(r7)/sizeof(r7[0])),p);
case P2D89_SHAPE_GRAPH_EDGE_BRANCH: return replay(r8,(unsigned int)(sizeof(r8)/sizeof(r8[0])),p);
case P2D89_SHAPE_GRAPH_EDGE_MERGE: return replay(r9,(unsigned int)(sizeof(r9)/sizeof(r9[0])),p);
case P2D89_SHAPE_GRAPH_EDGE_PORT: return replay(r10,(unsigned int)(sizeof(r10)/sizeof(r10[0])),p);
case P2D89_SHAPE_GRAPH_EDGE_DEPENDENCY: return replay(r11,(unsigned int)(sizeof(r11)/sizeof(r11[0])),p);
default: return 0;}}
const P2D89_Submodule p2d89_submodule_graph_symbols_edges={{P2D89_MODULE_GRAPH_SYMBOLS,"graph_symbols","edges",(unsigned int)(sizeof(shapes)/sizeof(shapes[0]))},shapes,emit};
