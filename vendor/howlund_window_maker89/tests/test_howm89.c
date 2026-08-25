#include "howm89.h"
#include <stdio.h>
#include <string.h>

typedef struct MockImpl {
    int active;
    int w;
    int h;
    int x;
    int y;
    int visible;
    char title[HOWM89_TITLE_CAPACITY];
} MockImpl;

static MockImpl g_impls[HOWM89_MAX_WINDOWS];
static int g_inited = 0;

static int mock_init(void *u) { (void)u; g_inited = 1; return 0; }
static void mock_shutdown(void *u) { (void)u; g_inited = 0; }
static void *mock_create(void *u, const char *title, int w, int h, int *ow, int *oh)
{
    int i; (void)u;
    for (i = 0; i < HOWM89_MAX_WINDOWS; ++i) {
        if (!g_impls[i].active) {
            memset(&g_impls[i], 0, sizeof(g_impls[i]));
            g_impls[i].active = 1; g_impls[i].w = w; g_impls[i].h = h; g_impls[i].visible = 1;
            strncpy(g_impls[i].title, title ? title : "", HOWM89_TITLE_CAPACITY - 1);
            g_impls[i].title[HOWM89_TITLE_CAPACITY - 1] = '\0';
            if (ow) *ow = w;
            if (oh) *oh = h;
            return &g_impls[i];
        }
    }
    return 0;
}
static void mock_destroy(void *u, void *p) { MockImpl *m=(MockImpl*)p; (void)u; if(m) memset(m,0,sizeof(*m)); }
static int mock_set_size(void *u, void *p, int w, int h) { MockImpl *m=(MockImpl*)p; (void)u; m->w=w;m->h=h;return 0; }
static int mock_get_size(void *u, void *p, int *w, int *h) { MockImpl *m=(MockImpl*)p; (void)u; *w=m->w;*h=m->h;return 0; }
static int mock_show(void *u, void *p, int show) { MockImpl *m=(MockImpl*)p; (void)u;m->visible=show?1:0;return 0; }
static int mock_visible(void *u, void *p, int *v) { MockImpl *m=(MockImpl*)p; (void)u;*v=m->visible;return 0; }
static int mock_title(void *u, void *p, const char *s) { MockImpl *m=(MockImpl*)p; (void)u;strncpy(m->title,s?s:"",HOWM89_TITLE_CAPACITY-1);m->title[HOWM89_TITLE_CAPACITY-1]='\0';return 0; }
static int mock_position(void *u, void *p, int x, int y) { MockImpl *m=(MockImpl*)p; (void)u;m->x=x;m->y=y;return 0; }
static int mock_get_position(void *u, void *p, int *x, int *y) { MockImpl *m=(MockImpl*)p;(void)u;*x=m->x;*y=m->y;return 0; }
static int mock_center(void *u, void *p, int monitor) { MockImpl *m=(MockImpl*)p;(void)u;m->x=100+monitor;m->y=200+monitor;return 0; }
static int mock_bool(void *u, void *p, int v) { (void)u;(void)p;(void)v;return 0; }
static int mock_action(void *u, void *p) { (void)u;(void)p;return 0; }
static int mock_query(void *u, void *p, int *v) { (void)u;(void)p;*v=0;return 0; }
static int mock_opacity(void *u, void *p, HOWM89_Fix q) { (void)u;(void)p;return (q>=0 && q<=HOWM89_Q16_ONE)?0:-1; }
static int mock_component(void *u, void *p, unsigned int m, HOWM89_Fix q) { (void)u;(void)p;(void)m;return (q>=0 && q<=HOWM89_Q16_ONE)?0:-1; }
static int mock_limits(void *u, void *p, int a,int b,int c,int d){(void)u;(void)p;(void)a;(void)b;(void)c;(void)d;return 0;}
static int mock_scale(void *u, void *p, HOWM89_Fix *q){(void)u;(void)p;*q=HOWM89_Q16_ONE+32768;return 0;}
static void *mock_native(void *u, void *p){(void)u;return p;}

int main(void)
{
    HOWM89_BackendVTable vt;
    HOWM89_WindowDesc d;
    HOWM89_Window *w;
    int x,y;
    HOWM89_Bool b;
    HOWM89_Fix q;
    memset(&vt,0,sizeof(vt));
    vt.init=mock_init;vt.shutdown=mock_shutdown;vt.create_window=mock_create;vt.destroy_window=mock_destroy;
    vt.set_window_size=mock_set_size;vt.get_window_size=mock_get_size;vt.set_fullscreen=mock_bool;
    vt.minimize=mock_action;vt.restore=mock_action;vt.maximize=mock_action;vt.set_opacity_q16=mock_opacity;
    vt.set_component_opacity_q16=mock_component;vt.get_native_handle=mock_native;vt.show_window=mock_show;
    vt.is_window_visible=mock_visible;vt.set_title=mock_title;vt.set_position=mock_position;vt.get_position=mock_get_position;vt.center_on_monitor=mock_center;
    vt.set_resizable=mock_bool;vt.set_decorated=mock_bool;vt.set_topmost=mock_bool;vt.is_minimized=mock_query;
    vt.is_maximized=mock_query;vt.focus=mock_action;vt.request_attention=mock_action;vt.request_close=mock_action;
    vt.set_size_limits=mock_limits;vt.get_scale_factor_q16=mock_scale;
    if (howm89_register_backend(&vt,0)!=HOWM89_OK) return 1;
    howm89_window_desc_defaults(&d); d.title="test";d.width=320;d.height=180;d.flags|=HOWM89_WINDOW_TOPMOST;
    w=howm89_window_create_ex(&d); if(!w||!g_inited) return 2;
    if(howm89_window_set_size(w,640,360)!=HOWM89_OK) return 3;
    if(howm89_window_get_size(w,&x,&y)!=HOWM89_OK||x!=640||y!=360) return 4;
    if(howm89_window_set_position(w,12,34)!=HOWM89_OK) return 5;
    if(howm89_window_get_position(w,&x,&y)!=HOWM89_OK||x!=12||y!=34) return 6;
    if(howm89_window_center_on_monitor(w,2)!=HOWM89_OK) return 7;
    if(howm89_window_get_position(w,&x,&y)!=HOWM89_OK||x!=102||y!=202) return 8;
    if(howm89_window_hide(w)!=HOWM89_OK) return 9;
    if(howm89_window_is_visible(w,&b)!=HOWM89_OK||b) return 10;
    if(howm89_window_set_opacity_q16(w,32768)!=HOWM89_OK) return 11;
    if(howm89_window_get_scale_factor_q16(w,&q)!=HOWM89_OK||q!=(HOWM89_Q16_ONE+32768)) return 12;
    if(!howm89_window_get_native_handle(w)) return 13;
    howm89_window_destroy(w);
    if(howm89_window_active_count()!=0) return 14;
    howm89_library_shutdown();
    if(g_inited) return 15;
    if(howm89_unregister_backend()!=HOWM89_OK) return 16;
    puts("Howlund Window Maker 89 core: PASS");
    return 0;
}
