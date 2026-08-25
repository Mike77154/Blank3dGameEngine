#include "blank3d_bighud.h"
#include "blank3d_gproj_ammo_gbar.h"
#include <stdio.h>
#include <string.h>

static FILE *g_svg;

static void color_hex(unsigned long c, char out[8])
{
    static const char h[] = "0123456789ABCDEF";
    unsigned int r=(unsigned int)((c>>24)&255UL), g=(unsigned int)((c>>16)&255UL), b=(unsigned int)((c>>8)&255UL);
    out[0]='#'; out[1]=h[(r>>4)&15]; out[2]=h[r&15]; out[3]=h[(g>>4)&15]; out[4]=h[g&15]; out[5]=h[(b>>4)&15]; out[6]=h[b&15]; out[7]='\0';
}
static void svg_rect(void *u,int x,int y,int w,int h,unsigned long c){char s[8];(void)u;color_hex(c,s);fprintf(g_svg,"<rect x='%d' y='%d' width='%d' height='%d' fill='%s'/>\n",x,y,w,h,s);} 
static void svg_line(void *u,int x1,int y1,int x2,int y2,unsigned long c){char s[8];(void)u;color_hex(c,s);fprintf(g_svg,"<line x1='%d' y1='%d' x2='%d' y2='%d' stroke='%s' stroke-width='1'/>\n",x1,y1,x2,y2,s);} 
static void svg_sprite(void *u,int si,int sx,int sy,int sw,int sh,int dx,int dy,int dw,int dh,unsigned long c){(void)u;(void)si;(void)sx;(void)sy;(void)sw;(void)sh;svg_rect(0,dx,dy,dw,dh,c);} 
static void svg_tris(void *u,const GBar89_Vertex *v,int vc,const int *idx,int ic,int sid){int i;char s[8];(void)u;(void)vc;(void)sid;for(i=0;i+2<ic;i+=3){color_hex(v[idx[i]].color,s);fprintf(g_svg,"<polygon points='%d,%d %d,%d %d,%d' fill='%s'/>\n",v[idx[i]].x,v[idx[i]].y,v[idx[i+1]].x,v[idx[i+1]].y,v[idx[i+2]].x,v[idx[i+2]].y,s);}}
static void clip_push(void*u,int x,int y,int w,int h){(void)u;(void)x;(void)y;(void)w;(void)h;}
static void clip_pop(void*u){(void)u;}

int main(void)
{
    static Blank3DBigHud hud;
    static Blank3DEcgVitals ecg;
    Blank3DBigHudNode *node=0;
    Blank3DGProjAmmoGBar bridge;
    Blank3DBigHudTelemetry t;
    GBar89_RenderOps ops;
    int i;
    blank3d_ecg_vitals_init(&ecg);
    if(!blank3d_bighud_load(&hud,&ecg,"config/hud/presets/ammo_cartridges_gold.bhud")){fprintf(stderr,"%s\n",blank3d_bighud_error(&hud));return 1;}
    for(i=0;i<hud.node_count;++i) if(strcmp(hud.nodes[i].name,"ammo_units")==0) node=&hud.nodes[i];
    if(!node) return 2;
    if(node->unit_renderer_kind!=B3D_BIGHUD_UNIT_RENDERER_GPROJ_AMMO) return 3;
    if(node->unit_point_count!=0 || node->meter.unit_points!=0) return 4;
    memset(&t,0,sizeof(t)); t.numbar_value=6; t.numbar_min=0; t.numbar_max=8; t.numbar_segments=8; t.numbar_units=8;
    t.numbar_layer_count=1; t.numbar_layer_size=0; t.numbar_phase=430; t.numbar_overlay=250;
    t.numbar_overlay_max=900; t.numbar_mid=520;
    blank3d_bighud_update_bar(node,&t,16u);
    node->meter.rect.x=24; node->meter.rect.y=22; node->meter.rect.w=520; node->meter.rect.h=62;
    blank3d_gproj_ammo_gbar_init(&bridge); blank3d_gproj_ammo_gbar_set_ammo(&bridge,2); blank3d_gproj_ammo_gbar_attach(&bridge,&node->meter);
    g_svg=fopen("tests/gproj_bvhud_visual.svg","wb"); if(!g_svg)return 5;
    fprintf(g_svg,"<svg xmlns='http://www.w3.org/2000/svg' width='600' height='150' viewBox='0 0 600 150'>\n<rect width='600' height='150' fill='#111418'/>\n");
    memset(&ops,0,sizeof(ops)); ops.draw_rect=svg_rect; ops.draw_line=svg_line; ops.draw_sprite=svg_sprite; ops.draw_triangles=svg_tris; ops.push_clip=clip_push; ops.pop_clip=clip_pop;
    gbar89_draw(&node->meter,&ops);
    /* Visual QA fixture for the base HUD active-reload contract.  The live
       HUD uses the same bindings and GProj cursor renderer. */
    {
        int x0=210, x1=544, y=103;
        int ws=x0+(int)((t.numbar_overlay*(long)(x1-x0))/t.numbar_overlay_max);
        int we=x0+(int)((t.numbar_mid*(long)(x1-x0))/t.numbar_overlay_max);
        int cx=x0+(int)((t.numbar_phase*(long)(x1-x0))/t.numbar_overlay_max);
        GBar89_Rect cursor;
        svg_rect(0,x0,y,x1-x0+1,2,0x747C86B0UL);
        svg_rect(0,ws,y-1,we-ws+1,4,0xDDE4EAE0UL);
        cursor.x=cx-5; cursor.y=y-13; cursor.w=10; cursor.h=18;
        blank3d_gproj_ammo_gbar_render(&bridge,&ops,&cursor,0xF4F6F8FFUL,0x11161CE8UL,100);
        fprintf(g_svg,"<text x='544' y='132' text-anchor='end' fill='#EEF1F4' font-family='monospace' font-size='18'>06 / 024</text>\n");
    }
    fprintf(g_svg,"</svg>\n"); fclose(g_svg);
    printf("BVHUD GProj visual OK: renderer=%d points=%d profile=%d value=%ld/%ld\n",node->unit_renderer_kind,node->unit_point_count,bridge.ammo_id,node->meter.value,node->meter.max_value);
    return 0;
}
