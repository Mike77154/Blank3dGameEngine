#include "glyphwifettf.h"
#include "common_io.h"

static GWT_LayoutGlyph layout[1024];

int main(int argc, char **argv){
    GWT_Font font; unsigned long size; unsigned short count, i; int r, px;
    if(argc < 4){ fprintf(stderr,"usage: %s font.ttf text pixel_size\n", argv[0]); return 1; }
    size=read_file_static(argv[1],g_font_buf,FONT_BUF_MAX); if(!size)return 1; r=gwt_font_init(&font,g_font_buf,size); if(r){ fprintf(stderr,"init err %d\n",r); return 1; }
    px=atoi(argv[3]); r=gwt_layout_utf8(&font,argv[2],px,0,0,layout,1024,&count); if(r && r!=GWT_ERR_BUFFER_FULL){ fprintf(stderr,"layout err %d\n",r); return 1; }
    for(i=0;i<count;i++) printf("U+%04lX gid=%u x=%d y=%d adv=%d\n", layout[i].codepoint, layout[i].glyph_id, layout[i].x, layout[i].y, layout[i].advance_px);
    return 0;
}
