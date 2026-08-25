#include "p2d89_internal.h"
#include <string.h>
typedef struct P2D89_Alias { const char *alias; P2D89_ShapeId id; } P2D89_Alias;
extern const P2D89_Alias p2d89_aliases[];
extern const unsigned int p2d89_alias_count;

static const P2D89_Submodule *const units[] = {
    &p2d89_submodule_pixels_basic,
    &p2d89_submodule_lines_basic,
    &p2d89_submodule_lines_curves,
    &p2d89_submodule_quadrilaterals_squares,
    &p2d89_submodule_quadrilaterals_rectangles,
    &p2d89_submodule_triangles_variants,
    &p2d89_submodule_triangles_incomplete,
    &p2d89_submodule_quadrilaterals_rhombi_and_slanted,
    &p2d89_submodule_polygons_regular_05_16,
    &p2d89_submodule_curves_round,
    &p2d89_submodule_curves_organic,
    &p2d89_submodule_stars_regular_04_16,
    &p2d89_submodule_stars_sparks,
    &p2d89_submodule_chevrons_basic,
    &p2d89_submodule_brackets_basic,
    &p2d89_submodule_symbols_basic,
    &p2d89_submodule_bars_basic,
    &p2d89_submodule_arrows_basic,
    &p2d89_submodule_controls_media,
    &p2d89_submodule_lines_incomplete_figures,
    &p2d89_submodule_polygons_irregular,
    &p2d89_submodule_spirals_basic,
    &p2d89_submodule_spirals_angular,
    &p2d89_submodule_ornaments_floral,
    &p2d89_submodule_ornaments_geometric,
    &p2d89_submodule_technical_drafting,
    &p2d89_submodule_technical_electrical,
    &p2d89_submodule_map_navigation,
    &p2d89_submodule_map_terrain,
    &p2d89_submodule_map_infrastructure,
    &p2d89_submodule_gizmos_transform,
    &p2d89_submodule_gizmos_selection,
    &p2d89_submodule_gizmos_nodes,
    &p2d89_submodule_polygons_weird_concave,
    &p2d89_submodule_polygons_weird_star,
    &p2d89_submodule_curves_mathematical,
    &p2d89_submodule_spirals_extended,
    &p2d89_submodule_ornaments_floral_extra,
    &p2d89_submodule_ornaments_geometric_extra,
    &p2d89_submodule_technical_symbols_mechanical,
    &p2d89_submodule_technical_symbols_logic,
    &p2d89_submodule_map_symbols_services,
    &p2d89_submodule_gizmos_advanced,
    &p2d89_submodule_polygons_weird_extra,
    &p2d89_submodule_diagrams_flowchart,
    &p2d89_submodule_diagrams_process,
    &p2d89_submodule_connectors_basic,
    &p2d89_submodule_connectors_markers,
    &p2d89_submodule_patterns_hatch,
    &p2d89_submodule_patterns_tiles,
    &p2d89_submodule_graph_symbols_nodes,
    &p2d89_submodule_graph_symbols_edges,
    &p2d89_submodule_annotations_callouts,
    &p2d89_submodule_weather_symbols_basic,
    &p2d89_submodule_weather_symbols_fronts,
    &p2d89_submodule_architecture_basic,
    &p2d89_submodule_controls_widgets,
};
static int ascii_equal(const char *a,const char *b){unsigned char ca,cb;if(a==0||b==0)return 0;while(*a!='\0'&&*b!='\0'){ca=(unsigned char)*a;cb=(unsigned char)*b;if(ca>='A'&&ca<='Z')ca=(unsigned char)(ca+('a'-'A'));if(cb>='A'&&cb<='Z')cb=(unsigned char)(cb+('a'-'A'));if(ca!=cb)return 0;++a;++b;}return(*a=='\0'&&*b=='\0')?1:0;}
unsigned int p2d89_submodule_count(void){return(unsigned int)(sizeof(units)/sizeof(units[0]));}
const P2D89_ModuleInfo *p2d89_submodule_at(unsigned int index){if(index>=p2d89_submodule_count())return 0;return &units[index]->info;}
unsigned int p2d89_shape_count(void){return (unsigned int)P2D89_SHAPE_COUNT;}
const P2D89_ShapeInfo *p2d89_shape_at(unsigned int index){unsigned int u,k,base;base=0U;for(u=0U;u<p2d89_submodule_count();++u){if(index<base+units[u]->info.shape_count){k=index-base;return &units[u]->shapes[k];}base+=units[u]->info.shape_count;}return 0;}
const P2D89_ShapeInfo *p2d89_shape_info(P2D89_ShapeId id){unsigned int u,k;for(u=0U;u<p2d89_submodule_count();++u)for(k=0U;k<units[u]->info.shape_count;++k)if(units[u]->shapes[k].id==(p2d89_u16)id)return &units[u]->shapes[k];return 0;}
const char *p2d89_shape_name(P2D89_ShapeId id){const P2D89_ShapeInfo *i;i=p2d89_shape_info(id);return i?i->name:0;}
P2D89_ShapeId p2d89_find_shape(const char *s){unsigned int u,k;if(s==0)return P2D89_SHAPE_COUNT;for(u=0U;u<p2d89_submodule_count();++u)for(k=0U;k<units[u]->info.shape_count;++k)if(ascii_equal(s,units[u]->shapes[k].name))return(P2D89_ShapeId)units[u]->shapes[k].id;for(k=0U;k<p2d89_alias_count;++k)if(ascii_equal(s,p2d89_aliases[k].alias))return p2d89_aliases[k].id;return P2D89_SHAPE_COUNT;}
int p2d89_emit_shape(P2D89_ShapeId id,const P2D89_Provider *p){unsigned int u,k;if(p==0||p->emit==0)return 0;for(u=0U;u<p2d89_submodule_count();++u){for(k=0U;k<units[u]->info.shape_count;++k){if(units[u]->shapes[k].id==(p2d89_u16)id)return units[u]->emit(id,p);}}return 0;}
unsigned int p2d89_module_shape_count(P2D89_ModuleId id){unsigned int u,n;n=0U;for(u=0U;u<p2d89_submodule_count();++u)if(units[u]->info.module_id==(unsigned char)id)n+=units[u]->info.shape_count;return n;}
