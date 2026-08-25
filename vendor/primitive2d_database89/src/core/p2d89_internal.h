#ifndef P2D89_INTERNAL_H
#define P2D89_INTERNAL_H
#include "primitive2d_database89.h"
#define P2D89_KAPPA 9048

typedef int (*P2D89_SubmoduleEmitFn)(P2D89_ShapeId id, const P2D89_Provider *provider);
typedef struct P2D89_Submodule {
    P2D89_ModuleInfo info;
    const P2D89_ShapeInfo *shapes;
    P2D89_SubmoduleEmitFn emit;
} P2D89_Submodule;

int p2d89_i_point(const P2D89_Provider *p,p2d89_q14 x,p2d89_q14 y);
int p2d89_i_move(const P2D89_Provider *p,p2d89_q14 x,p2d89_q14 y);
int p2d89_i_line(const P2D89_Provider *p,p2d89_q14 x,p2d89_q14 y);
int p2d89_i_quad(const P2D89_Provider *p,p2d89_q14 cx,p2d89_q14 cy,p2d89_q14 x,p2d89_q14 y);
int p2d89_i_cubic(const P2D89_Provider *p,p2d89_q14 c1x,p2d89_q14 c1y,p2d89_q14 c2x,p2d89_q14 c2y,p2d89_q14 x,p2d89_q14 y);
int p2d89_i_close(const P2D89_Provider *p);
void p2d89_i_rotate(p2d89_q14 x,p2d89_q14 y,p2d89_u16 turn,p2d89_q14 *ox,p2d89_q14 *oy);
int p2d89_i_poly(const p2d89_q14 *xy,unsigned int count,int close_path,const P2D89_Provider *p);
int p2d89_i_rot_poly(const p2d89_q14 *xy,unsigned int count,p2d89_u16 turn,int close_path,const P2D89_Provider *p);
int p2d89_i_rounded_rect(p2d89_q14 hx,p2d89_q14 hy,p2d89_q14 radius,const P2D89_Provider *p);
int p2d89_i_frame(p2d89_q14 hx,p2d89_q14 hy,p2d89_q14 inset,const P2D89_Provider *p);
int p2d89_i_semicircle(int filled,const P2D89_Provider *p);
int p2d89_i_quarter_circle(int filled,const P2D89_Provider *p);
int p2d89_i_lens(p2d89_q14 bend,const P2D89_Provider *p);
int p2d89_i_arc_3q(const P2D89_Provider *p);

extern const P2D89_Submodule p2d89_submodule_pixels_basic;
extern const P2D89_Submodule p2d89_submodule_lines_basic;
extern const P2D89_Submodule p2d89_submodule_lines_curves;
extern const P2D89_Submodule p2d89_submodule_quadrilaterals_squares;
extern const P2D89_Submodule p2d89_submodule_quadrilaterals_rectangles;
extern const P2D89_Submodule p2d89_submodule_triangles_variants;
extern const P2D89_Submodule p2d89_submodule_triangles_incomplete;
extern const P2D89_Submodule p2d89_submodule_quadrilaterals_rhombi_and_slanted;
extern const P2D89_Submodule p2d89_submodule_polygons_regular_05_16;
extern const P2D89_Submodule p2d89_submodule_curves_round;
extern const P2D89_Submodule p2d89_submodule_curves_organic;
extern const P2D89_Submodule p2d89_submodule_stars_regular_04_16;
extern const P2D89_Submodule p2d89_submodule_stars_sparks;
extern const P2D89_Submodule p2d89_submodule_chevrons_basic;
extern const P2D89_Submodule p2d89_submodule_brackets_basic;
extern const P2D89_Submodule p2d89_submodule_symbols_basic;
extern const P2D89_Submodule p2d89_submodule_bars_basic;
extern const P2D89_Submodule p2d89_submodule_arrows_basic;
extern const P2D89_Submodule p2d89_submodule_controls_media;
extern const P2D89_Submodule p2d89_submodule_lines_incomplete_figures;
extern const P2D89_Submodule p2d89_submodule_polygons_irregular;

extern const P2D89_Submodule p2d89_submodule_spirals_basic;
extern const P2D89_Submodule p2d89_submodule_spirals_angular;
extern const P2D89_Submodule p2d89_submodule_ornaments_floral;
extern const P2D89_Submodule p2d89_submodule_ornaments_geometric;
extern const P2D89_Submodule p2d89_submodule_technical_drafting;
extern const P2D89_Submodule p2d89_submodule_technical_electrical;
extern const P2D89_Submodule p2d89_submodule_map_navigation;
extern const P2D89_Submodule p2d89_submodule_map_terrain;
extern const P2D89_Submodule p2d89_submodule_map_infrastructure;
extern const P2D89_Submodule p2d89_submodule_gizmos_transform;
extern const P2D89_Submodule p2d89_submodule_gizmos_selection;
extern const P2D89_Submodule p2d89_submodule_gizmos_nodes;
extern const P2D89_Submodule p2d89_submodule_polygons_weird_concave;
extern const P2D89_Submodule p2d89_submodule_polygons_weird_star;

extern const P2D89_Submodule p2d89_submodule_curves_mathematical;
extern const P2D89_Submodule p2d89_submodule_spirals_extended;
extern const P2D89_Submodule p2d89_submodule_ornaments_floral_extra;
extern const P2D89_Submodule p2d89_submodule_ornaments_geometric_extra;
extern const P2D89_Submodule p2d89_submodule_technical_symbols_mechanical;
extern const P2D89_Submodule p2d89_submodule_technical_symbols_logic;
extern const P2D89_Submodule p2d89_submodule_map_symbols_services;
extern const P2D89_Submodule p2d89_submodule_gizmos_advanced;
extern const P2D89_Submodule p2d89_submodule_polygons_weird_extra;
extern const P2D89_Submodule p2d89_submodule_diagrams_flowchart;
extern const P2D89_Submodule p2d89_submodule_diagrams_process;
extern const P2D89_Submodule p2d89_submodule_connectors_basic;
extern const P2D89_Submodule p2d89_submodule_connectors_markers;
extern const P2D89_Submodule p2d89_submodule_patterns_hatch;
extern const P2D89_Submodule p2d89_submodule_patterns_tiles;
extern const P2D89_Submodule p2d89_submodule_graph_symbols_nodes;
extern const P2D89_Submodule p2d89_submodule_graph_symbols_edges;
extern const P2D89_Submodule p2d89_submodule_annotations_callouts;
extern const P2D89_Submodule p2d89_submodule_weather_symbols_basic;
extern const P2D89_Submodule p2d89_submodule_weather_symbols_fronts;
extern const P2D89_Submodule p2d89_submodule_architecture_basic;
extern const P2D89_Submodule p2d89_submodule_controls_widgets;

#endif
